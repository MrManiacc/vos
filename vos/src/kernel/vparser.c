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

// Utility function for creating indents
static char *create_indent(int indentLevel) {
    char *indent = platform_allocate(indentLevel * 2 + 1, false);
    for (int i = 0; i < indentLevel * 2; ++i) {
        indent[i] = ' ';
    }
    indent[indentLevel * 2] = '\0';
    return indent;
}


static char *appendString(char **dest, const char *src, size_t *cursor, size_t *bufferSize);

static char *dump_type(const Type *type, int indentLevel);

static char *dump_property(const Property *property, int indentLevel);

static char *dump_component(const Component *component, int indentLevel);


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
    Type *head = NULL, *current = NULL;
    
    do {
        Token typeNameToken = consumeToken(state, TOKEN_IDENTIFIER);
        if (typeNameToken.type != TOKEN_IDENTIFIER) {
            // Error handling: Expected identifier but not found
            // Clean up any allocated types
            // Note: You might need a function to recursively free types
            return NULL;
        }
        
        Type *type = platform_allocate(sizeof(Type), false);
        if (!type) {
            // Error handling: Memory allocation failed
            // Clean up any allocated types
            // Note: You might need a function to recursively free types
            return NULL;
        }
        
        // Initialize the type
        type->name = platform_allocate(typeNameToken.length + 1, false);
        memcpy(type->name, typeNameToken.start, typeNameToken.length);
        type->name[typeNameToken.length] = '\0'; // Ensure null-termination
        type->isArray = false; // This will be set to true later if it's an array
        type->isComposite = false; // This might be updated in the loop
        type->next = NULL;
        
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
        return NULL;
    }
    
    Type *arrayType = parser_parse_simple_or_composite_type(state); // Parse the array's element type(s)
    
    if (!matchToken(state, TOKEN_RBRACKET)) {
        // Error handling: Expected ']' but not found
        // Clean up any allocated types
        // Note: You might need a function to recursively free types
        return NULL;
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

//Type *parser_parse_type(ParserState *state) {
//    Type *head = NULL, *current = NULL, *type = NULL;
//
//    do {
//        // Allocate memory for the type structure
//        type = platform_allocate(sizeof(Type), false);
//        if (!type) {
//            // Memory allocation failed, clean up any previously allocated types
//            while (head != NULL) {
//                Type *temp = head;
//                head = head->next;
//                platform_free(temp->name, false);
//                platform_free(temp, false);
//            }
//            vtrace("Memory allocation failed for type.");
//            return NULL; // Return NULL to indicate failure
//        }
//
//        // Initialize the newly allocated type
//        type->isArray = false;
//        type->isComposite = false;
//        type->next = NULL;
//
//        // Consume and copy the type name
//        Token typeNameToken = consumeToken(state, TOKEN_IDENTIFIER);
//        if (typeNameToken.type != TOKEN_IDENTIFIER) {
//            // Expected a type identifier but didn't find one, clean up
//            platform_free(type, false);
//            // If this isn't the first type in a composite, clean up others
//            while (head != NULL) {
//                Type *temp = head;
//                head = head->next;
//                platform_free(temp->name, false);
//                platform_free(temp, false);
//            }
//            return NULL; // Error handling
//        }
//        type->name = platform_allocate(typeNameToken.length + 1, false);
//        platform_copy_memory(type->name, typeNameToken.start, typeNameToken.length + 1);
//        type->name[typeNameToken.length] = '\0'; // Null-terminate the string
//        // Check for array type '[]'
//        if (matchToken(state, TOKEN_LBRACKET)) {
//            if (!matchToken(state, TOKEN_RBRACKET)) {
//                // Expected ']', but it's missing. Clean up.
//                platform_free(type->name, false);
//                platform_free(type, false);
//                return NULL; // Error handling
//            }
//            type->isArray = true;
//        }
//
//        // Link the type into the list
//        if (current != NULL) {
//            current->next = type;
//            current->isComposite = true; // Mark previous node as composite if it's not the first
//        } else {
//            head = type; // First type in the list
//        }
//        current = type;
//
//        // Check if there's a '|' indicating a composite type
//    } while (matchToken(state, TOKEN_PIPE));
//
//    return head;
//}

Property *parser_parse_property(ParserState *state) {
    Property *property = platform_allocate(sizeof(Property), false);
    if (!property) return NULL; // Check for allocation failure
    
    // Expect a property name (identifier)
    Token nameToken = consumeToken(state, TOKEN_IDENTIFIER);
    if (nameToken.type != TOKEN_IDENTIFIER) {
        platform_free(property, false);
        return NULL; // Error handling
    }
    property->name = platform_allocate(nameToken.length + 1, false);
    platform_copy_memory(property->name, nameToken.start, nameToken.length + 1);
    property->name[nameToken.length] = '\0'; // Null-terminate the string
    
    // Expect a colon separating the name and the type
    if (!matchToken(state, TOKEN_COLON)) {
        platform_free(property->name, false);
        platform_free(property, false);
        return NULL; // Error handling
    }
    
    // Parse the type next
    property->type = parser_parse_type(state);
    if (!property->type) {
        platform_free(property->name, false);
        platform_free(property, false);
        return NULL; // Error handling for type parsing
    }
    
    // Check if the property is optional (indicated by '?')
    property->isOptional = matchToken(state, TOKEN_QUESTION);
    
    property->next = NULL; // Initialize next pointer
    return property;
}

ASTNode *parser_parse_component(ParserState *state) {
    // We might have a delimiter before the component definition
    matchToken(state, TOKEN_DELIMITER);
    Token nameToken = consumeToken(state, TOKEN_IDENTIFIER); // Assuming the component name follows immediately
    if (nameToken.type != TOKEN_IDENTIFIER) {
        // Handle error: Expected component name
        return NULL;
    }
    
    // Initialize component
    ASTNode *componentNode = platform_allocate(sizeof(ASTNode), false);
    componentNode->type = AST_COMPONENT;
    componentNode->name = platform_allocate(nameToken.length + 1, false);
    platform_copy_memory(componentNode->name, nameToken.start, nameToken.length + 1);
    componentNode->name[nameToken.length] = '\0'; // Null-terminate the string
    componentNode->data.component.name = componentNode->name;
    componentNode->data.component.properties = NULL;
    componentNode->data.component.extends = NULL;
    componentNode->next = NULL;
    
    //see if we can match a type
    if (matchToken(state, TOKEN_COLON)) {
        // Parse the type next
        componentNode->data.component.extends = parser_parse_type(state);
        if (!componentNode->data.component.extends) {
            platform_free(componentNode->name, false);
            platform_free(componentNode, false);
            verror("Failed to parse component type");
            return NULL; // Error handling for type parsing
        }
    }
    
    
    // Expecting an opening brace '{' to start the component definition
    if (!matchToken(state, TOKEN_LBRACE)) {
        // Free previously allocated resources and return NULL
        platform_free(componentNode->name, false);
        platform_free(componentNode, false);
        return NULL; // Error handling
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


ProgramAST parser_parse_from_memory(const char *source_code, u64 source_code_length) {
    ProgramSource source = lexer_analysis_from_mem(source_code, source_code_length);
    ProgramAST ast = parser_parse(&source);
    char *program_dump = parser_dump_program(&ast);
    vinfo("Program dump: %s", program_dump);
    return ast;
}

ProgramAST parser_parse_from_file(char *file_name) {
    if (!vfs_exists(file_name)) {
        verror("File does not exist: %s", file_name);
        return (ProgramAST) {0};
    }
    fs_node *file = vfs_get(file_name);
    if (file == null) {
        verror("Failed to get file: %s", file_name);
        return (ProgramAST) {0};
    }
    const char *source_code = file->data.file.data;
    u64 source_code_length = file->data.file.size;
    return parser_parse_from_memory(source_code, source_code_length);
}


char *parser_dump_program(ProgramAST *result) {
    size_t bufferSize = 1024; // Initial buffer size
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    size_t cursor = 0; // Tracks the current position in the buffer
    ASTNode *current = result->statements;
    while (current) {
        char *nodeStr = NULL;
        switch (current->type) {
            case AST_COMPONENT:
                nodeStr = dump_component(&current->data.component, 0);
                break;
                // Handle other ASTNode types as needed
            default:
                nodeStr = "Unknown AST node type\n";
        }
        
        if (nodeStr) {
            buffer = appendString(&buffer, nodeStr, &cursor, &bufferSize);
            if (!buffer) {
                platform_free(nodeStr, false);
                return NULL;
            }
            platform_free(nodeStr, false);
        }
        current = current->next;
    }
    
    return buffer;
}

static char *dump_component(const Component *component, int indentLevel) {
    // Similar logic to parser_dump_program for component specifics
    // Example:
    char *indent = create_indent(indentLevel);
    size_t bufferSize = 256; // Adjust based on needs
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    size_t cursor = 0;
    buffer = appendString(&buffer, indent, &cursor, &bufferSize);
    buffer = appendString(&buffer, "Component: ", &cursor, &bufferSize);
    buffer = appendString(&buffer, component->name, &cursor, &bufferSize);
    buffer = appendString(&buffer, "\n", &cursor, &bufferSize);
    
    // Iterate through properties of the component
    Property *prop = component->properties;
    while (prop) {
        char *propStr = dump_property(prop, indentLevel + 1);
        buffer = appendString(&buffer, propStr, &cursor, &bufferSize);
        prop = prop->next;
        platform_free(propStr, false);
    }
    
    platform_free(indent, false);
    return buffer;
}


char *dump_property(const Property *property, int indentLevel) {
    // Calculate the buffer size needed. Adjust the size based on your needs.
    size_t bufferSize = 256;
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    char *indent = create_indent(indentLevel);
    
    snprintf(buffer, bufferSize, "%sProperty: %s, Type: ", indent, property->name);
    char *typeStr = dump_type(property->type, 0); // No additional indent for type
    if (typeStr) {
        // Ensure the buffer can hold the additional type string
        strcat(buffer, typeStr);
        platform_free(typeStr, false);
    }
    
    if (property->isOptional) {
        strcat(buffer, ", Optional: true\n");
    } else {
        strcat(buffer, ", Optional: false\n");
    }
    
    platform_free(indent, false);
    return buffer;
}

static char *dump_type(const Type *type, int indentLevel) {
    size_t bufferSize = 256; // Initial buffer size, adjust as needed
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
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
            return NULL; // Fail on allocation error
        }
        
        // Copy existing content into new buffer
        memcpy(newBuffer, *dest, *cursor);
        
        // Free old buffer if it was previously allocated
        if (*dest) {
            platform_free(*dest, false);
        }
        
        // Update buffer pointer and size
        *dest = newBuffer;
        *bufferSize = newBufferSize;
    }
    
    // Append new string
    memcpy(*dest + *cursor, src, srcLen); // Copy string content
    *cursor += srcLen; // Update cursor position
    (*dest)[*cursor] = '\0'; // Ensure null termination
    
    return *dest;
}