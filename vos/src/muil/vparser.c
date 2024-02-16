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

static char *strndup(const char *s, size_t n);

char *generate_json_for_type(const Type *type, int indentLevel);

char *generate_json_for_property(const Property *property, int indentLevel);

char *generate_json_for_component(Component *node, int indentLevel);

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

ASTNode *parser_parse_expression(ParserState *state);

ASTNode *parser_parse_property_or_assignment(ParserState *state);

ASTNode *parser_parse_literal(ParserState *state) {
    Token token = consumeToken(state, peekToken(state).type); // Consume the literal token
    ASTNode *literalNode = platform_allocate(sizeof(ASTNode), false);
    if (!literalNode) return NULL; // Allocation failure
    
    literalNode->nodeType = AST_LITERAL;
    
    switch (token.type) {
        case TOKEN_NUMBER:
            literalNode->data.literal.type = LITERAL_NUMBER;
            // Conversion to number is simplified here; actual implementation may vary
            literalNode->data.literal.value.numberValue = atof(token.start);
            break;
        case TOKEN_STRING:
            literalNode->data.literal.type = LITERAL_STRING;
            literalNode->data.literal.value.stringValue = platform_allocate(token.length + 1, false);
            platform_copy_memory(literalNode->data.literal.value.stringValue, token.start, token.length);
            literalNode->data.literal.value.stringValue[token.length] = '\0'; // Null-terminate
            break;
            // Add cases for other literal types
        default:
            platform_free(literalNode, false);
            verror("Invalid literal token type: %s", get_token_type_name(token.type));
            return NULL;
    }
    
    return literalNode;
}

ASTNode *parser_parse_component_instance(ParserState *state) {
    // Skip delimiters before checking the next token
    while (peekToken(state).type == TOKEN_DELIMITER) {
        consumeToken(state, TOKEN_DELIMITER);
    }
    Token lookahead = peekToken(state);
    if (lookahead.type != TOKEN_LBRACE) {
        verror("Expected '{' at the start of a component instance, but got %s at line %u, column %u",
               get_token_type_name(lookahead.type), lookahead.line, lookahead.column);
        return NULL;
    }
    // Consume the left brace now that we've verified it's correct
    consumeToken(state, TOKEN_LBRACE);
    
    ASTNode *instanceNode = platform_allocate(sizeof(ASTNode), false);
    if (!instanceNode) {
        // Memory allocation failure
        return NULL;
    }
    
    instanceNode->nodeType = AST_COMPONENT_INSTANCE;
    
    // Initialize the component instance properties list
    instanceNode->data.componentInstance.properties = NULL;
    ASTNode **currentProperty = &instanceNode->data.componentInstance.properties;
    
    while (peekToken(state).type != TOKEN_RBRACE && peekToken(state).type != TOKEN_EOF) {
        // Parse the next property or assignment within the component instance
        ASTNode *propertyOrAssignment = parser_parse_property_or_assignment(state);
        if (!propertyOrAssignment) {
            // Error handling: Failed to parse a property or assignment within the component instance
            // Remember to perform cleanup of previously allocated nodes
            return NULL;
        }
        
        // Link the parsed property or assignment into the component instance's properties list
        *currentProperty = propertyOrAssignment;
        currentProperty = &propertyOrAssignment->next;
        
        // Optionally, handle delimiters (e.g., commas) between properties or assignments
        matchToken(state, TOKEN_COMMA);
    }
    
    // Ensure to consume the right brace marking the end of the component instance
    consumeToken(state, TOKEN_RBRACE);
    
    return instanceNode;
}

ASTNode *parser_parse_component_instance_with_identifier(ParserState *state) {
    // Consume the identifier token which is expected to be the component's name or type
    Token identifierToken = consumeToken(state, TOKEN_IDENTIFIER);
    // Log or process the identifierToken.name as needed here
    
    // Now expect and consume the LBRACE, signaling the start of the component's properties
    Token nextToken = peekToken(state);
    if (nextToken.type != TOKEN_LBRACE) {
        verror("Expected '{' after component identifier, but got %s at line %u, column %u",
               get_token_type_name(nextToken.type), nextToken.line, nextToken.column);
        return NULL;
    }
    
    // Proceed with parsing the component instance as usual
    ASTNode *instance = parser_parse_component_instance(state);
    if (instance) {
        // Link the identifier into the component instance node
        instance->data.componentInstance.type = platform_allocate(identifierToken.length + 1, false);
        instance->data.componentInstance.type->name = platform_allocate(identifierToken.length + 1, false);
        platform_copy_memory(instance->data.componentInstance.type->name, identifierToken.start,
                             identifierToken.length);
        instance->data.componentInstance.type->name[identifierToken.length] = '\0'; // Null-terminate
    }
    return instance;
}

b8 isNextTokenLBrace(ParserState *state) {
    ParserState tempState = *state; // Create a copy to peek ahead without affecting the original state
    consumeToken(&tempState, TOKEN_IDENTIFIER); // Consume the identifier in the temp state
    // Skip delimiters that might be between the identifier and the next significant token
    while (peekToken(&tempState).type == TOKEN_DELIMITER) {
        consumeToken(&tempState, TOKEN_DELIMITER);
    }
    return peekToken(&tempState).type == TOKEN_LBRACE;
}


ASTNode *parser_parse_expression_or_component_instance(ParserState *state) {
    // Skip delimiters before checking the next token
    while (peekToken(state).type == TOKEN_DELIMITER) {
        consumeToken(state, TOKEN_DELIMITER);
    }
    
    Token lookahead = peekToken(state);
    
    if (lookahead.type == TOKEN_IDENTIFIER && isNextTokenLBrace(state)) {
        // Component instance prefixed by an identifier
        return parser_parse_component_instance_with_identifier(state);
    } else if (lookahead.type == TOKEN_LBRACE) {
        // Component instance starting directly with '{'
        return parser_parse_component_instance(state);
    } else {
        // General expression
        return parser_parse_expression(state);
    }
}


ASTNode *parser_parse_array_expression(ParserState *state) {
    // Check for the start of an array expression
    if (!matchToken(state, TOKEN_LBRACKET)) {
        verror("Expected '[' at the start of an array expression");
        return NULL;
    }
    
    ASTNode *arrayNode = platform_allocate(sizeof(ASTNode), false);
    arrayNode->nodeType = AST_ARRAY_LITERAL;
    arrayNode->data.arrayLiteral.elements = NULL;
    
    ASTNode **currentElement = &arrayNode->data.arrayLiteral.elements;
    
    while (peekToken(state).type != TOKEN_RBRACKET && peekToken(state).type != TOKEN_EOF) {
        ASTNode *element = parser_parse_expression_or_component_instance(state);
        if (!element) {
            // Error handling and cleanup
            return NULL;
        }
        
        *currentElement = element;
        currentElement = &element->next;
        
        // Optionally skip a comma between elements
        matchToken(state, TOKEN_COMMA);
    }
    
    consumeToken(state, TOKEN_RBRACKET); // Consume the closing bracket
    return arrayNode;
}

ASTNode *parser_parse_expression(ParserState *state) {
    // Skip any leading delimiters.
    while (peekToken(state).type == TOKEN_DELIMITER) {
        consumeToken(state, TOKEN_DELIMITER);
    }
    
    Token token = peekToken(state);
    ASTNode *exprNode = NULL;
    
    switch (token.type) {
        case TOKEN_NUMBER:
        case TOKEN_STRING:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            exprNode = parser_parse_literal(state);
            break;
        case TOKEN_IDENTIFIER:
            // For now, treat identifiers as variable references
            exprNode = platform_allocate(sizeof(ASTNode), false);
            if (!exprNode) return NULL; // Allocation failure
            exprNode->nodeType = AST_LITERAL;
            exprNode->data.literal.type = LITERAL_STRING; // Adjust based on actual type
            exprNode->data.literal.value.stringValue = platform_allocate(token.length + 1, false);
            platform_copy_memory(exprNode->data.literal.value.stringValue, token.start, token.length);
            exprNode->data.literal.value.stringValue[token.length] = '\0'; // Null-terminate
            consumeToken(state, TOKEN_IDENTIFIER); // Move past the identifier
            break;
            // Add cases for other expression types
        default:
            //TODO: we might need to ignore delimiters here
            verror("Unexpected token in expression: %s", get_token_type_name(token.type));
            return NULL;
    }
    
    return exprNode;
}

ASTNode *parser_parse_assignment(ParserState *state) {
    Token identifierToken = consumeToken(state, TOKEN_IDENTIFIER);
    if (!matchToken(state, TOKEN_EQUALS)) {
        verror("Expected '=' after identifier in assignment");
        return NULL;
    }
    
    // Look ahead to determine if the assignment is to a component instance
    Token nextToken = peekToken(state);
    ASTNode *valueNode = NULL;
    
    if (nextToken.type == TOKEN_LBRACE || nextToken.type == TOKEN_LBRACKET) {
        // Handle inline component instance or array of instances/expressions
        valueNode = (nextToken.type == TOKEN_LBRACE) ?
                    parser_parse_component_instance(state) :
                    parser_parse_array_expression(state);
    } else {
        // Handle simple expressions
        valueNode = parser_parse_expression(state);
    }
    
    if (!valueNode) {
        verror("Failed to parse value in assignment");
        return NULL;
    }
    
    ASTNode *assignmentNode = platform_allocate(sizeof(ASTNode), false);
    assignmentNode->nodeType = AST_ASSIGNMENT_STATEMENT;
    assignmentNode->data.assignment.variableName = platform_allocate(identifierToken.length + 1, false);
    platform_copy_memory(assignmentNode->data.assignment.variableName, identifierToken.start, identifierToken.length);
    assignmentNode->data.assignment.variableName[identifierToken.length] = '\0'; // Null-terminate
    assignmentNode->data.assignment.expression = valueNode;
    assignmentNode->next = NULL;
    
    return assignmentNode;
}

ASTNode *parser_parse_property_or_assignment(ParserState *state) {
    // First, skip any delimiters that are not meaningful in this context.
    while (peekToken(state).type == TOKEN_DELIMITER) {
        consumeToken(state, TOKEN_DELIMITER);
    }
    
    // Check if the next token is an identifier or the start of a component/array
    Token lookahead = peekToken(state);
    if (lookahead.type != TOKEN_IDENTIFIER && lookahead.type != TOKEN_LBRACE && lookahead.type != TOKEN_LBRACKET) {
        //try to parse an expression
        ASTNode *expr = parser_parse_expression(state);
        if (expr) return expr;
        verror("Expected an identifier, '{', or '[' but got %s, on line %u, column %u",
               get_token_type_name(lookahead.type), lookahead.line, lookahead.column);
        return NULL;
    }
    
    // If the next token is an identifier, proceed with parsing a property or assignment
    if (lookahead.type == TOKEN_IDENTIFIER) {
        Token identifierToken = consumeToken(state, TOKEN_IDENTIFIER);
        
        Token nextToken = peekToken(state);
        if (nextToken.type == TOKEN_EQUALS) {
            // It's an assignment. Consume '=' and parse the assignment.
            consumeToken(state, TOKEN_EQUALS);
            ASTNode *valueNode = parser_parse_expression_or_component_instance(state);
            if (!valueNode) {
                verror("Failed to parse value in assignment");
                return NULL;
            }
            
            ASTNode *assignmentNode = platform_allocate(sizeof(ASTNode), false);
            assignmentNode->nodeType = AST_ASSIGNMENT_STATEMENT;
            assignmentNode->data.assignment.variableName = strndup(identifierToken.start, identifierToken.length);
            assignmentNode->data.assignment.expression = valueNode;
            assignmentNode->next = NULL;
            
            return assignmentNode;
        } else {
            verror("Unexpected token following identifier: %s at line %u, column %u",
                   get_token_type_name(nextToken.type), nextToken.line, nextToken.column);
            return NULL;
        }
    } else {
        // For cases starting with '{' or '[', parse as component instance or array without a preceding identifier
        return parser_parse_expression_or_component_instance(state);
    }
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
    matchToken(state, TOKEN_DELIMITER);
    Token nameToken = consumeToken(state, TOKEN_IDENTIFIER);
    if (nameToken.type != TOKEN_IDENTIFIER) {
        return NULL; // Error handling
    }
    
    ASTNode *componentNode = platform_allocate(sizeof(ASTNode), false);
    if (!componentNode) return NULL; // Allocation failure
    componentNode->nodeType = AST_COMPONENT;
    componentNode->data.component.name = platform_allocate(nameToken.length + 1, false);
    platform_copy_memory(componentNode->data.component.name, nameToken.start, nameToken.length);
    componentNode->data.component.name[nameToken.length] = '\0';
    
    componentNode->data.component.symbolTable = platform_allocate(sizeof(SymbolTable), false);
    if (!componentNode->data.component.symbolTable) {
        // Cleanup and error handling
        return NULL;
    }
    componentNode->data.component.symbolTable->entries = NULL;
    componentNode->data.component.assignments = NULL; // Initialize the assignment list
    componentNode->data.component.extends = NULL;
    componentNode->next = NULL;
    
    if (matchToken(state, TOKEN_COLON)) {
        componentNode->data.component.extends = parser_parse_type(state);
        if (!componentNode->data.component.extends) {
            // Cleanup and error handling
            return NULL;
        }
    }
    
    if (!matchToken(state, TOKEN_LBRACE)) {
        // Cleanup and error handling
        return NULL;
    }
    
    while (!matchToken(state, TOKEN_RBRACE)) {
        matchToken(state, TOKEN_DELIMITER); // Optional
        Token nextToken = peekToken(state);
        
        if (nextToken.type == TOKEN_IDENTIFIER) {
            // Look ahead to distinguish between a property and an assignment
            ParserState tempState = *state; // Copy state to peek ahead without consuming tokens
            consumeToken(&tempState, TOKEN_IDENTIFIER); // Consume identifier
            Token peek = peekToken(&tempState);
            
            if (peek.type == TOKEN_COLON) {
                // It's a property declaration
                Property *property = parser_parse_property(state);
                if (!property) break; // Error handling
                
                // Link property into the component
                // Assume a function or logic to link properties exists
            } else if (peek.type == TOKEN_EQUALS) {
                // It's an assignment
                ASTNode *assignment = parser_parse_assignment(state);
                if (!assignment) break; // Error handling
                
                // Link the assignment into the component's symbol table and assignment list
                SymbolTableEntry *entry = platform_allocate(sizeof(SymbolTableEntry), false);
                if (!entry) {
                    // Cleanup and error handling
                    return NULL;
                }
                entry->identifier = assignment->data.assignment.variableName;
                entry->node = assignment;
                
                // Add to symbol table
                entry->next = componentNode->data.component.symbolTable->entries;
                componentNode->data.component.symbolTable->entries = entry;
                
                // Add to assignments list
                assignment->next = componentNode->data.component.assignments;
                componentNode->data.component.assignments = assignment;
            } else {
                // Error handling for unexpected token
                break;
            }
        } else {
            // Handle unexpected tokens
            break;
        }
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

char *generate_json_for_literal(Literal *literal, int indentLevel) {
    char *buffer = platform_allocate(1024, false); // Ensure this is appropriately sized
    char *indent = create_indent(indentLevel);
    
    const char *valueStr;
    switch (literal->type) {
        case LITERAL_NUMBER:
            snprintf(buffer, 1024, "%s\"type\": \"number\", \"value\": %f", indent, literal->value.numberValue);
            break;
        case LITERAL_STRING:
            snprintf(buffer, 1024, "%s\"type\": \"string\", \"value\": \"%s\"", indent, literal->value.stringValue);
            break;
            // Handle other literal types...
        default:
            snprintf(buffer, 1024, "%s\"type\": \"unknown\"", indent);
            break;
    }
    
    platform_free(indent, false);
    return buffer;
}

char *generate_json_for_component_instance(ComponentInstance *instance, int indentLevel) {
    size_t bufferSize = 1024; // Adjust based on expected content size.
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    size_t cursor = 0;
    char *indent = create_indent(indentLevel);
    char *childIndent = create_indent(indentLevel + 1);
    
    snprintf(buffer + cursor, bufferSize - cursor, "%s\"ComponentInstance\": {\n", indent);
    cursor += strlen(buffer + cursor);
    
    if (instance->type) {
        char *typeStr = generate_json_for_type(instance->type, 0);
        snprintf(buffer + cursor, bufferSize - cursor, "%s\"Type\": %s,\n", childIndent, typeStr);
        cursor += strlen(buffer + cursor);
        platform_free(typeStr, false);
    }
    
    // Assuming ComponentInstance properties are serialized similarly to Component's properties.
    // If ComponentInstance has its unique way to handle properties, adjust accordingly.
    
    snprintf(buffer + cursor, bufferSize - cursor, "%s}\n", indent);
    cursor += strlen(buffer + cursor);
    
    platform_free(indent, false);
    platform_free(childIndent, false);
    
    return buffer;
}

char *generate_json_for_assignment(const Assignment *assignment, int indentLevel) {
    size_t bufferSize = 2048; // Adjust based on needs.
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    char *indent = create_indent(indentLevel);
    size_t cursor = 0;
    
    cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s\"Assignment\": {\n", indent);
    cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s\"VariableName\": \"%s\",\n", indent, assignment->variableName);
    // Assuming expression serialization is handled elsewhere. Placeholder here.
    cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s}\n", indent);
    
    platform_free(indent, false);
    
    return buffer;
}


char *generate_json_for_node(ASTNode *node, int indentLevel) {
    switch (node->nodeType) {
        case AST_COMPONENT:
            return generate_json_for_component(&node->data.component, indentLevel);
        case AST_COMPONENT_INSTANCE:
            return generate_json_for_component_instance(&node->data.componentInstance, indentLevel);
        case AST_LITERAL:
            return generate_json_for_literal(&node->data.literal, indentLevel);
        case AST_ASSIGNMENT_STATEMENT:
            return generate_json_for_assignment(&node->data.assignment, indentLevel);
            // Add cases for other ASTNodeTypes...
        default:
            return strndup("Unsupported node type", 20); // Placeholder for unsupported types
    }
}

char *parser_dump_program(ProgramAST *result) {
    size_t bufferSize = 4096; // Start with a buffer large enough for most ASTs.
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    size_t cursor = 0;
    appendString(&buffer, "{\n\"AST\": [\n", &cursor, &bufferSize);
    
    for (ASTNode *current = result->statements; current != NULL; current = current->next) {
        char *nodeStr = generate_json_for_node(current, 1); // Generate JSON for each node
        appendString(&buffer, nodeStr, &cursor, &bufferSize);
        platform_free(nodeStr, false);
        
        if (current->next) {
            appendString(&buffer, ",\n", &cursor, &bufferSize); // Separate nodes with commas
        }
    }
    
    appendString(&buffer, "\n]\n}", &cursor, &bufferSize); // Close JSON structure
    return buffer;
}

char *generate_json_for_component(Component *component, int indentLevel) {
    // Buffer size may need adjustment based on the complexity of your components.
    size_t bufferSize = 4096;
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    size_t cursor = 0;
    char *indent = create_indent(indentLevel);
    char *propIndent = create_indent(indentLevel + 1);
    
    cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s\"Component\": \"%s\",\n", indent, component->name);
    if (component->extends) {
        char *extendsType = generate_json_for_type(component->extends, 0);
        cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s\"Extends\": %s,\n", indent, extendsType);
        platform_free(extendsType, false);
    }
    
    cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s\"Properties\": {\n", indent);
    Property *prop = component->properties;
    while (prop) {
        char *propJson = generate_json_for_property(prop, indentLevel + 2);
        cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s", propJson);
        platform_free(propJson, false);
        prop = prop->next;
        if (prop) {
            cursor += snprintf(buffer + cursor, bufferSize - cursor, ",\n");
        }
    }
    cursor += snprintf(buffer + cursor, bufferSize - cursor, "\n%s}\n", indent);
    
    platform_free(indent, false);
    platform_free(propIndent, false);
    
    return buffer;
}

char *generate_json_for_property(const Property *property, int indentLevel) {
    size_t bufferSize = 1024; // Adjust based on the complexity of type serialization.
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    char *indent = create_indent(indentLevel);
    char *typeJSON = generate_json_for_type(property->type, 0); // Serialize property type.
    
    snprintf(buffer, bufferSize, "%s\"%s\": {\"type\": %s, \"optional\": %s}", indent, property->name, typeJSON,
             property->isOptional ? "true" : "false");
    
    platform_free(indent, false);
    platform_free(typeJSON, false);
    
    return buffer;
}


char *generate_json_for_type(const Type *type, int indentLevel) {
    // This is a placeholder. You should adjust this based on the specifics of your Type structure.
    size_t bufferSize = 512;
    char *buffer = platform_allocate(bufferSize, false);
    if (!buffer) return NULL;
    
    if (type->isArray) {
        // Example for array type serialization.
        snprintf(buffer, bufferSize, "[\"%s\"]", type->name);
    } else if (type->isComposite) {
        // Start composite type serialization.
        snprintf(buffer, bufferSize, "\"%s | ...\"", type->name); // Placeholder for composite type handling.
    } else {
        // Simple type serialization.
        snprintf(buffer, bufferSize, "\"%s\"", type->name);
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

static char *strndup(const char *s, size_t n) {
    char *copy = platform_allocate(n + 1, false);
    if (copy) {
        platform_copy_memory(copy, s, n);
        copy[n] = '\0';
    }
    return copy;
}