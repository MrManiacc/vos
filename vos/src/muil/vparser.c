/**
 * Created by jraynor on 2/15/2024.
 */
#include "vparser.h"
#include "filesystem/vfs.h"
#include "core/vlogger.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Function prototypes for readability
static char *appendString(char **dest, const char *src, size_t *cursor, size_t *bufferSize);

char *generate_json_for_type(const Type *type, int indentLevel);

char *generate_json_for_property(const Property *property, int indentLevel);

char *generate_json_for_component(const ASTNode *node, int indentLevel);

char *create_indent(int indentLevel);

typedef struct ParserState {
    ProgramSource *source;
    u32 current_token_index;
} ParserState;


static Token peekToken(const ParserState *state) {
    if (state->current_token_index < state->source->count) {
        return state->source->tokens[state->current_token_index];
    }
    // Return an EOF token if beyond the token stream
    return (Token) {TOKEN_EOF, "", 0, 0, 0};
}

static Token consumeToken(ParserState *state, TokenType expectedType) {
    Token token = peekToken(state);
    if (token.type == expectedType) {
        state->current_token_index++;
        return token;
    }
    
    // Find token information from the lexer to provide more context
    // Handle unexpected token type, potentially logging an error
    u32 line_number = token.line;
    u32 column_number = token.column;
    const char *expectedTypeStr = get_token_type_name(expectedType);
    const char *tokenTypeStr = get_token_type_name(token.type);
    verror("Expected token type %s but got %s at line %d, column %d", expectedTypeStr, tokenTypeStr, line_number,
           column_number);
    return token; // Consider returning an error token instead
}

static b8 matchToken(ParserState *state, TokenType type) {
    // Automatically skip over all delimiter tokens
    while (peekToken(state).type == TOKEN_DELIMITER) {
        state->current_token_index++; // Skip the delimiter
    }
    
    // Now match the expected token type
    Token token = peekToken(state);
    if (token.type == type) {
        state->current_token_index++; // Consume the matched token
        return true;
    }
    return false;
}

Type *parser_parse_simple_or_composite_type(ParserState *state) {
    Type *head = null, *current = null;
    
    do {
        Token typeNameToken = consumeToken(state, TOKEN_IDENTIFIER);
        if (typeNameToken.type != TOKEN_IDENTIFIER) {
            // Error handling: Expected identifier but not found
            // Clean up any allocated types
            // Note: You might need a function to recursively free types
            return null;
        }
        
        Type *type = platform_allocate(sizeof(Type), false);
        if (!type) {
            // Error handling: Memory allocation failed
            // Clean up any allocated types
            // Note: You might need a function to recursively free types
            return null;
        }
        
        // Initialize the type
        type->name = platform_allocate(typeNameToken.length + 1, false);
        platform_copy_memory(type->name, typeNameToken.start, typeNameToken.length);
        type->name[typeNameToken.length] = '\0'; // Ensure null-termination
        type->isArray = false; // This will be set to true later if it's an array
        type->isComposite = false; // This might be updated in the loop
        type->next = null;
        
        // Link the type into the list
        if (current) {
            current->next = type;
            current->isComposite = true;
        } else {
            head = type;
        }
        current = type;
    } while (matchToken(state, TOKEN_PIPE)); // Continue if there's a composite type
    
    return head;
}

Type *parser_parse_array_type(ParserState *state) {
    if (!matchToken(state, TOKEN_LBRACKET)) {
        // Error handling: Expected '[' but not found
        return null;
    }
    
    Type *arrayType = parser_parse_simple_or_composite_type(state); // Parse the array's element type(s)
    
    if (!matchToken(state, TOKEN_RBRACKET)) {
        // Error handling: Expected ']' but not found
        // Clean up any allocated types
        // Note: You might need a function to recursively free types
        return null;
    }
    
    // Mark the parsed type(s) as array type(s)
    Type *current = arrayType;
    while (current) {
        current->isArray = true;
        current = current->next;
    }
    
    return arrayType;
}

Type *parser_parse_type(ParserState *state) {
    if (peekToken(state).type == TOKEN_LBRACKET) {
        return parser_parse_array_type(state);
    } else {
        return parser_parse_simple_or_composite_type(state);
    }
}

/**
 * @brief Parses a property from the input token stream.
 *
 * This function expects a property name (identifier) followed by a colon, and then
 * parses the type of the property. The property can also be marked as optional by
 * appending a question mark.
 *
 * @param state The parser state.
 * @return A pointer to the parsed Property structure, or null if an error occurred.
 *   If null is returned, memory allocated for the Property structure and its fields
 *   should be freed by the caller.
 */
Property *parser_parse_property(ParserState *state) {
    Property *property = platform_allocate(sizeof(Property), false);
    if (!property) return null; // Check for allocation failure
    
    // Expect a property name (identifier)
    Token nameToken = consumeToken(state, TOKEN_IDENTIFIER);
    if (nameToken.type != TOKEN_IDENTIFIER) {
        platform_free(property, false);
        return null; // Error handling
    }
    property->name = platform_allocate(nameToken.length + 1, false);
    platform_copy_memory(property->name, nameToken.start, nameToken.length + 1);
    property->name[nameToken.length] = '\0'; // null-terminate the string
    
    // Expect a colon separating the name and the type
    if (!matchToken(state, TOKEN_COLON)) {
        platform_free(property->name, false);
        platform_free(property, false);
        return null; // Error handling
    }
    
    // Parse the type next
    property->type = parser_parse_type(state);
    if (!property->type) {
        platform_free(property->name, false);
        platform_free(property, false);
        return null; // Error handling for type parsing
    }
    
    // Check if the property is optional (indicated by '?')
    property->isOptional = matchToken(state, TOKEN_QUESTION);
    
    property->next = null; // Initialize next pointer
    return property;
}

ASTNode *parser_parse_component(ParserState *state) {
    // We might have a delimiter before the component definition
    matchToken(state, TOKEN_DELIMITER);
    Token nameToken = consumeToken(state, TOKEN_IDENTIFIER); // Assuming the component name follows immediately
    if (nameToken.type != TOKEN_IDENTIFIER) {
        // Handle error: Expected component name
        return null;
    }
    
    // Initialize component
    ASTNode *componentNode = platform_allocate(sizeof(ASTNode), false);
    componentNode->type = AST_COMPONENT;
    componentNode->name = platform_allocate(nameToken.length + 1, false);
    platform_copy_memory(componentNode->name, nameToken.start, nameToken.length + 1);
    componentNode->name[nameToken.length] = '\0'; // null-terminate the string
    componentNode->data.component.name = componentNode->name;
    componentNode->data.component.properties = null;
    componentNode->data.component.extends = null;
    componentNode->next = null;
    
    //see if we can match a type
    if (matchToken(state, TOKEN_COLON)) {
        // Parse the type next
        componentNode->data.component.extends = parser_parse_type(state);
        if (!componentNode->data.component.extends) {
            platform_free(componentNode->name, false);
            platform_free(componentNode, false);
            verror("Failed to parse component type");
            return null; // Error handling for type parsing
        }
    }
    
    
    // Expecting an opening brace '{' to start the component definition
    if (!matchToken(state, TOKEN_LBRACE)) {
        // Free previously allocated resources and return null
        platform_free(componentNode->name, false);
        platform_free(componentNode, false);
        return null; // Error handling
    }
    
    // Parse properties until a closing brace '}'
    Property **currentProperty = &componentNode->data.component.properties;
    while (!matchToken(state, TOKEN_RBRACE) && state->current_token_index < state->source->count) {
        matchToken(state, TOKEN_DELIMITER); // Optional delimiter (new line or semicolon)
        Property *property = parser_parse_property(state);
        if (!property) break; // Error handling
        
        *currentProperty = property; // Link the property
        currentProperty = &property->next; // Move to the next
        
        // Optionally, handle commas between properties
        matchToken(state, TOKEN_COMMA);
    }
    
    return componentNode;
}


ProgramAST parser_parse(ProgramSource *source) {
    ProgramAST result = {0};
    ParserState state = {source, 0};
    
    // Dynamically allocate an initial node pointer to start the linked list
    ASTNode **currentNode = &result.statements;
    
    // Continue parsing components until we reach the end of the source
    while (state.current_token_index < source->count) {
        // Skip any delimiters before attempting to parse the next component
        while (peekToken(&state).type == TOKEN_DELIMITER) {
            state.current_token_index++; // Skip delimiters
        }
        
        // If we've reached the EOF, break out of the loop
        if (peekToken(&state).type == TOKEN_EOF) {
            break;
        }
        
        ASTNode *component = parser_parse_component(&state);
        if (component) {
            // Link the new component into the list
            *currentNode = component;
            // Advance the pointer for the next component
            currentNode = &component->next;
        } else {
            // Handle parsing error, potentially breaking out of the loop or skipping to the next component
//            verror("Error parsing component. Skipping to next component.");
            state.current_token_index++; // Attempt to skip ahead and find the next component
        }
    }
    
    //dump the program to console
    char *program_dump = parser_dump_program(&result);
    vinfo("Program dump: %s", program_dump);
    
    return result;
}


char *parser_dump_program(ProgramAST *result) {
    size_t bufferSize = 2048; // Adjust buffer size as needed
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    size_t cursor = 0;
    appendString(&buffer, "{\n", &cursor, &bufferSize);
    
    ASTNode *current = result->statements;
    while (current) {
        char *componentStr = generate_json_for_component(current, 1); // Indent components with 1 tab
        appendString(&buffer, componentStr, &cursor, &bufferSize);
        platform_free(componentStr, false);
        
        current = current->next;
        if (current) {
            appendString(&buffer, ",\n", &cursor, &bufferSize); // Separate components with commas
        }
    }
    
    appendString(&buffer, "\n}", &cursor, &bufferSize); // Close JSON object
    return buffer;
}

char *generate_json_for_component(const ASTNode *node, int indentLevel) {
    size_t bufferSize = 512; // Adjust based on needs
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    size_t cursor = 0;
    char *indent = create_indent(indentLevel);
    char *componentIndent = create_indent(indentLevel - 1);
    
    appendString(&buffer, componentIndent, &cursor, &bufferSize);
    appendString(&buffer, "\"", &cursor, &bufferSize);
    appendString(&buffer, node->data.component.name, &cursor, &bufferSize);
    appendString(&buffer, "\": {\n", &cursor, &bufferSize);
    
    Property *prop = node->data.component.properties;
    while (prop) {
        char *propStr = generate_json_for_property(prop, indentLevel + 1); // Indent properties
        appendString(&buffer, propStr, &cursor, &bufferSize);
        platform_free(propStr, false);
        
        prop = prop->next;
        if (prop) {
            appendString(&buffer, ",\n", &cursor, &bufferSize);
        } else {
            appendString(&buffer, "\n", &cursor, &bufferSize);
        }
    }
    
    appendString(&buffer, indent, &cursor, &bufferSize);
    appendString(&buffer, "}", &cursor, &bufferSize);
    
    platform_free(indent, false);
    platform_free(componentIndent, false);
    return buffer;
}

char *generate_json_for_property(const Property *property, int indentLevel) {
    size_t bufferSize = 256; // Adjust based on your needs
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    char *indent = create_indent(indentLevel);
    snprintf(buffer, bufferSize, "%s\"%s\": {\"type\": \"%s\", \"optional\": %s}",
             indent, property->name, generate_json_for_type(property->type, 0),
             property->isOptional ? "true" : "false");
    
    platform_free(indent, false);
    return buffer;
}

char *generate_json_for_type(const Type *type, int indentLevel) {
    size_t bufferSize = 256; // Initial buffer size, adjust as needed
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return null;
    
    char *currentPos = buffer;
    size_t remainingSize = bufferSize;
    size_t written = 0;
    
    if (type && type->isArray) {
        // For array types, start with '['
        written = snprintf(currentPos, remainingSize, "[");
        currentPos += written;
        remainingSize -= written;
    }
    
    while (type) {
        // Concatenate type names, handle composite types within arrays
        written = snprintf(currentPos, remainingSize, "%s", type->name);
        currentPos += written;
        remainingSize -= written;
        
        if (type->next) {
            written = snprintf(currentPos, remainingSize, " | ");
            currentPos += written;
            remainingSize -= written;
        }
        
        type = type->next;
    }
    
    if (buffer[0] == '[') {
        // For array types, close with ']'
        snprintf(currentPos, remainingSize, "]");
    }
    return buffer;
}

char *create_indent(int indentLevel) {
    char *indent = platform_allocate(indentLevel * 4 + 1, false); // 4 spaces per tab
    for (int i = 0; i < indentLevel * 4; i++) {
        indent[i] = ' '; // Using spaces for indentation, adjust as needed
    }
    indent[indentLevel * 4] = '\0';
    return indent;
}


static char *appendString(char **dest, const char *src, size_t *cursor, size_t *bufferSize) {
    size_t srcLen = strlen(src);
    
    // Ensure there is enough space for the new string and the null terminator
    while (*cursor + srcLen + 1 > *bufferSize) {
        // Calculate new buffer size
        size_t newBufferSize = *bufferSize + 1024; // Increase buffer size
        
        // Allocate new buffer
        char *newBuffer = platform_allocate(newBufferSize, false);
        if (!newBuffer) {
            verror("Failed to allocate memory for AST dump.");
            return null; // Fail on allocation error
        }
        
        // Copy existing content into new buffer
        platform_copy_memory(newBuffer, *dest, *cursor);
        
        // Free old buffer if it was previously allocated
        if (*dest) {
            platform_free(*dest, false);
        }
        
        // Update buffer pointer and size
        *dest = newBuffer;
        *bufferSize = newBufferSize;
    }
    
    // Append new string
    platform_copy_memory(*dest + *cursor, src, srcLen); // Copy string content
    *cursor += srcLen; // Update cursor position
    (*dest)[*cursor] = '\0'; // Ensure null termination
    
    return *dest;
}