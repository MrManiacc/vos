/**
 * Created by jraynor on 2/15/2024.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "vparser.h"
#include "filesystem/vfs.h"
#include "core/vlogger.h"
#include "platform/platform.h"
#include "core/vstring.h"


typedef struct ParserState {
    ProgramSource *source;
    u32 current_token_index;
} ParserState;

// =============================================================================//
// Internal helper functions                                                    //
// =============================================================================//
static Token peek(ParserState *state);

static Token peek_distance(ParserState *state, u32 distance);

static Token consume(ParserState *state, TokenType expectedType);

static b8 match(ParserState *state, TokenType type);

static void skip_delimiters(ParserState *state);

static ASTNode *create_node(ASTNodeType type);

// =============================================================================
// Parser API
// =============================================================================

// Parses the full program AST from the given source code
ASTNode *parser_parse_component(ParserState *state);

// Parses a scope (a block of code)
ASTNode *parser_parse_scope(ParserState *state);


// Parses a literal value (number, string, boolean)
ASTNode *parser_parse_literal(ParserState *state);


// Parses an array literal
ASTNode *parser_parse_array(ParserState *state);

// Parses an expression
ASTNode *parser_parse_expression(ParserState *state);

// Helper to parse either a property or a nested component
ASTNode *parser_parse_body(ParserState *state);

//Parses a type. A type can be a single identifier a compound type or an array type.
//I.e. "string", "int", "string | int", "string[]", "int[]", "string | int[]"
Type *parser_parse_type(ParserState *state);

// =============================================================================
// Parser API
// =============================================================================

ProgramAST parser_parse(ProgramSource *source) {
    ParserState state = {source, 0};
    ProgramAST result = {null};
    ASTNode **current = &(result.root);
    
    while (state.current_token_index < source->count && peek(&state).type != TOKEN_EOF) {
        ASTNode *componentNode = parser_parse_expression(&state);
        if (!componentNode) {
            verror("Failed to parse top-level component");
            // Perform any necessary cleanup
            break;
        }
        
        *current = componentNode;
        current = &((*current)->next);
        
    }
    
    return result;
}


ASTNode *parser_parse_component(ParserState *state) {
    skip_delimiters(state);
    u32 index = state->current_token_index;
    Token nameToken = peek(state);
    if (nameToken.type != TOKEN_IDENTIFIER) {
        return null;
    }
    consume(state, TOKEN_IDENTIFIER);
    ASTNode *componentNode = create_node(AST_COMPONENT);
    if (!componentNode) return null; // Error handling within create_node
    
    componentNode->data.component.name = string_ndup(nameToken.start, nameToken.length);
    
    // Potentially parse the super type if applicable
    if (match(state, TOKEN_COLON)) {
        componentNode->data.component.extends = parser_parse_type(state);
        if (!componentNode->data.component.extends) {
            verror("Failed to parse component super type");
            platform_free(componentNode, false);
            state->current_token_index = index;
            return null;
        }
    }
    
    // If there's not a LBRACE, at this point, we return null because it's not a valid component
    if (!match(state, TOKEN_LBRACE)) {
        state->current_token_index = index;
        platform_free(componentNode, false);
        return null;
    }
    componentNode->data.component.body = parser_parse_scope(state);
    return componentNode;
}

ASTNode *parser_parse_scope(ParserState *state) {
    ASTNode *scopeNode = create_node(AST_SCOPE);
    if (!scopeNode) return null; // Error handling within create_node
    
    ASTNode **current = &(scopeNode->data.scope.nodes);
    if (match(state, TOKEN_RBRACE)) {
        return scopeNode;
    }
    while (!match(state, TOKEN_RBRACE) && peek(state).type != TOKEN_EOF) {
        skip_delimiters(state);
        Token now = peek(state);
//        ASTNode *childNode = parser_parse_body(state);
        ASTNode *childNode = parser_parse_expression(state);
        if (!childNode) {
            // Handle parsing error, including potential cleanup
            break; // Exit the loop on error
        }
        
        *current = childNode; // Link the parsed property or nested component
        current = &((*current)->next); // Prepare for the next property/component
        
        skip_delimiters(state); // Ready for the next property/component or the end of the component
    }
    
    return scopeNode;
}


ASTNode *parser_parse_body(ParserState *state) {
    
    // Expect an identifier for a property or component
    Token identifierToken = consume(state, TOKEN_IDENTIFIER);
    // Look ahead to determine if this is a property assignment or a nested component
    Token lookaheadToken = peek(state); // We need to look ahead to decide the next step
    if (lookaheadToken.type == TOKEN_EQUALS) { // It's a property assignment
        //consume the equals token
        consume(state, TOKEN_EQUALS);
        // It's a property assignment, save the name and parse the value
        char *name = string_ndup(identifierToken.start, identifierToken.length);
        if (!name) {
            verror("Failed to allocate memory for property name");
            return null;
        }
        // Create an assignment node
        ASTNode *assignmentNode = create_node(AST_ASSIGNMENT);
        if (!assignmentNode) {
            platform_free(name, false);
            return null;
        }
        // Fill in the assignment node details
        assignmentNode->data.assignment.variableName = name;
        assignmentNode->data.assignment.value = parser_parse_expression(state);
        return assignmentNode;
    } else if (lookaheadToken.type == TOKEN_COLON) {
        //consume the colon token
        consume(state, TOKEN_COLON);
        // It's a property declaration
        // It's property declaration, save the name and parse the type and value
        char *name = string_ndup(identifierToken.start, identifierToken.length);
        if (!name) {
            verror("Failed to allocate memory for property name");
            return null;
        }
        // Create a property node
        ASTNode *propertyNode = create_node(AST_PROPERTY);
        if (!propertyNode) {
            platform_free(name, false);
            return null;
        }
        // Fill in the property node details
        propertyNode->data.variable.name = name;
        propertyNode->data.variable.type = parser_parse_type(state);
        if (!propertyNode->data.variable.type) {
            verror("Failed to parse property type");
            platform_free(name, false);
            platform_free(propertyNode, false);
            return null;
        }
        // Optionally parse the assignment
        if (match(state, TOKEN_EQUALS)) {
            propertyNode->data.variable.value = parser_parse_expression(state);
            if (!propertyNode->data.variable.value) {
                verror("Failed to parse property value");
                platform_free(name, false);
                platform_free(propertyNode, false);
                return null;
            }
        }
        return propertyNode;
    } else {
        // It's a nested component
        // Create a component node
        ASTNode *componentNode = create_node(AST_COMPONENT);
        if (!componentNode) {
            return null;
        }
        // Fill in the component node details
        componentNode->data.component.name = string_ndup(identifierToken.start, identifierToken.length);
        if (!componentNode->data.component.name) {
            verror("Failed to allocate memory for component name");
            platform_free(componentNode, false);
            return null;
        }
        // Optionally parse the super type
        if (match(state, TOKEN_COLON)) {
            componentNode->data.component.extends = parser_parse_type(state);
            if (!componentNode->data.component.extends) {
                verror("Failed to parse component super type");
                platform_free(componentNode->data.component.name, false);
                platform_free(componentNode, false);
                return null;
            }
        }
        // Optionally parse the component body
        Token next = peek(state);
        if (next.type == TOKEN_LBRACE) {
            consume(state, TOKEN_LBRACE);
            componentNode->data.component.body = parser_parse_scope(state);
        } else {
            componentNode->data.component.body = parser_parse_expression(state);
        }
        return componentNode;
    }
}


// Parses a literal value (number, string, boolean)
ASTNode *parser_parse_literal(ParserState *state) {
    Token token = consume(state, peek(state).type); // Adjusted to directly consume the next token
    ASTNode *node = (ASTNode *) platform_allocate(sizeof(ASTNode), false);
    if (!node) return null; // Allocation failure
    
    node->nodeType = AST_LITERAL;
    switch (token.type) {
        case TOKEN_NUMBER:
            node->data.literal.type = LITERAL_NUMBER;
            node->data.literal.value.numberValue = atof(token.start);
            break;
        case TOKEN_STRING:
            node->data.literal.type = LITERAL_STRING;
            node->data.literal.value.stringValue = string_ndup(token.start, token.length);
            break;
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            node->data.literal.type = LITERAL_BOOLEAN;
            node->data.literal.value.booleanValue = (strcmp(token.start, "true") == 0);
            break;
        default:
            platform_free(node, false);
            verror("Invalid literal token type");
            return null;
    }
    return node;
}


ASTNode *parser_parse_array(ParserState *state) {
    skip_delimiters(state); // Skip leading delimiters
    
    if (!match(state, TOKEN_LBRACKET)) {
        verror("Expected '[' at start of array");
        return null;
    }
    
    ASTNode *arrayNode = (ASTNode *) platform_allocate(sizeof(ASTNode), false);
    arrayNode->nodeType = AST_ARRAY;
    ASTNode **currentElement = &(arrayNode->data.array.elements);
    
    while (!match(state, TOKEN_RBRACKET)) {
        skip_delimiters(state); // Skip delimiters between elements
        
        ASTNode *elementNode = parser_parse_expression(state); // Assuming this function is implemented
        if (!elementNode) {
            // Handle parsing error, clean up
            verror("Failed to parse array element");
            platform_free(arrayNode, false); // Example cleanup, adapt as necessary
            return null;
        }
        //expect a comma or a right bracket
        if (peek(state).type == TOKEN_COMMA) {
            //consume the comma
            consume(state, TOKEN_COMMA);
        }
        
        *currentElement = elementNode;
        currentElement = &((*currentElement)->next);
        skip_delimiters(state); // Skip delimiters after elements
    }
    
    return arrayNode;
}

ASTNode *parser_parse_expression(ParserState *state) {
    skip_delimiters(state);
    Token nextToken = peek(state);
    //parse assignment
    
    
    if (nextToken.type == TOKEN_IDENTIFIER) {
        ASTNode *component = parser_parse_component(state);
        if (component) {
            return component;
        }
        
        Token identifierToken = consume(state, TOKEN_IDENTIFIER);
        Token lookahead = peek(state);
        if (lookahead.type == TOKEN_EQUALS) {
            //parse an assignment
            ASTNode *assignmentNode = create_node(AST_ASSIGNMENT);
            if (!assignmentNode) return null; // Error handling within create_node
            assignmentNode->data.assignment.variableName = string_ndup(identifierToken.start, identifierToken.length);
            consume(state, TOKEN_EQUALS);
            assignmentNode->data.assignment.value = parser_parse_expression(state);
            return assignmentNode;
        } else if (lookahead.type == TOKEN_COLON) {
            // parse the type
            ASTNode *propertyNode = create_node(AST_PROPERTY);
            if (!propertyNode) return null; // Error handling within create_node
            propertyNode->data.variable.name = string_ndup(identifierToken.start, identifierToken.length);
            consume(state, TOKEN_COLON);
            propertyNode->data.variable.type = parser_parse_type(state);
            if (match(state, TOKEN_EQUALS)) {
                propertyNode->data.variable.value = parser_parse_expression(state);
            }
            return propertyNode;
        } else {
            //parse a variable
            ASTNode *variableNode = create_node(AST_PROPERTY);
            if (!variableNode) return null; // Error handling within create_node
            variableNode->data.variable.name = string_ndup(identifierToken.start, identifierToken.length);
            if (match(state, TOKEN_EQUALS)) {
                variableNode->data.variable.value = parser_parse_expression(state);
            }
            return variableNode;
        }
    } else if (nextToken.type == TOKEN_NUMBER || nextToken.type == TOKEN_STRING || nextToken.type == TOKEN_TRUE ||
               nextToken.type == TOKEN_FALSE) {
        return parser_parse_literal(state);
    } else if (nextToken.type == TOKEN_LBRACKET) {
        //Parse an array
        return parser_parse_array(state);
    } else if (nextToken.type == TOKEN_LBRACE) {
        consume(state, TOKEN_LBRACE);
        return parser_parse_scope(state);
    } else {
        //First we try to parse a component
        ASTNode *component = parser_parse_component(state);
        if (component) {
            return component;
        }
        
        return null;
    }
}


//Parses a type. A type can be a single identifier a compound type or an array type.
//I.e. "string", "int", "string | int", "string[]", "int[]", "string | int[]"
Type *parser_parse_type(ParserState *state) {
    skip_delimiters(state);
    
    Type *head = NULL, *current = NULL;
    
    while (true) {
        Token nextToken = consume(state, TOKEN_IDENTIFIER);
        if (nextToken.type != TOKEN_IDENTIFIER) {
            verror("Expected identifier for type");
            // Cleanup any allocated types before returning
            Type *temp;
            while (head != NULL) {
                temp = head;
                head = head->next;
                platform_free(temp->name, false);
                platform_free(temp, false);
            }
            return NULL;
        }
        
        Type *type = (Type *) platform_allocate(sizeof(Type), false);
        if (!type) {
            verror("Allocation failure for Type");
            return NULL; // Ensure to handle cleanup of previously allocated types before returning
        }
        
        type->name = string_ndup(nextToken.start, nextToken.length);
        type->isComposite = false;
        type->isArray = false;
        type->next = NULL;
        
        if (!head) {
            head = type;
        } else {
            current->next = type;
            // If we've encountered more than one type, it's a composite type
            head->isComposite = true;
        }
        current = type;
        
        // Look ahead to determine if this is an array or part of a compound type
        Token lookahead = peek(state);
        if (lookahead.type == TOKEN_LBRACKET) {
            consume(state, TOKEN_LBRACKET); // Consume '['
            Token closeBracket = consume(state, TOKEN_RBRACKET); // Expecting ']'
            if (closeBracket.type != TOKEN_RBRACKET) {
                verror("Expected ']' after '[' for array type");
                return NULL; // Ensure to handle cleanup before returning
            }
            type->isArray = true;
        }
        
        // Check if there's a '|' indicating a compound type
        lookahead = peek(state);
        if (lookahead.type != TOKEN_PIPE) break; // If not a compound type, exit the loop
        consume(state, TOKEN_PIPE); // Consume the '|' for the next type in the compound
    }
    
    return head;
}


// =============================================================================
// Internal helper functions
// =============================================================================
static Token peek(ParserState *state) {
    //make sure to skip delimiters
    while (state->source->tokens[state->current_token_index].type == TOKEN_DELIMITER) {
        state->current_token_index++;
    }
    // Return the current token if within the token stream
    if (state->current_token_index < state->source->count) {
        return state->source->tokens[state->current_token_index];
    }
    // Return an EOF token if beyond the token stream
    return (Token) {TOKEN_EOF, "", 0, 0, 0};
}

static Token peek_distance(ParserState *state, u32 distance) {
    u32 index = state->current_token_index + distance;
    //iterate over the tokens until we find a non delimiter
    while (state->source->tokens[index].type == TOKEN_DELIMITER) {
        index++;
    }
    
    // Return an EOF token if beyond the token stream
    return index < state->source->count ? state->source->tokens[index] : (Token) {TOKEN_EOF, "", 0, 0, 0};
}

static Token consume(ParserState *state, TokenType expectedType) {
    Token token = peek(state);
    if (token.type != expectedType) {
        verror("Expected token type %s but got %s at line %d, column %d", get_token_type_name(expectedType),
               get_token_type_name(token.type), token.line, token.column);
        return (Token) {TOKEN_ERROR, null, 0, token.line, token.column}; // Assuming TOKEN_ERROR is defined
    }
    state->current_token_index++;
    return token;
}

static b8 match(ParserState *state, TokenType type) {
    // Automatically skip over all delimiter tokens
    while (peek(state).type == TOKEN_DELIMITER) {
        state->current_token_index++; // Skip the delimiter
    }
    
    // Now match the expected token type
    Token token = peek(state);
    if (token.type == type) {
        state->current_token_index++; // Consume the matched token
        return true;
    }
    return false;
}

static void skip_delimiters(ParserState *state) {
    while (peek(state).type == TOKEN_DELIMITER) {
        consume(state, TOKEN_DELIMITER);
    }
}

// Utility function to create a new AST node.
static ASTNode *create_node(ASTNodeType type) {
    ASTNode *node = (ASTNode *) platform_allocate(sizeof(ASTNode), false);
    if (!node) {
        verror("Failed to allocate memory for AST node");
        return null;
    }
    node->nodeType = type;
    node->next = null; // Ensure the next pointer is initialized to null
    return node;
}

// Forward declarations for recursive dumping
static char *dumpASTNode(ASTNode *node, int indentLevel, b8 onlyPassIndent);

// Helper function to create indentation

static char *createIndent(int level) {
    if (level == 0) {
        //Allocate a single space for the root level
        char *indent = (char *) platform_allocate(1, false);
        indent[0] = '\0';
        return indent;
    }
    
    char *indent = (char *) platform_allocate(2 * level + 1, false);
    memset(indent, ' ', level * 2); // Fill with spaces
    indent[level] = '\0';
    return indent;
}

// =============================================================================
// Dumping the AST for debugging
// =============================================================================
// Main function to dump the AST
char *parser_dump_program(ProgramAST *result) {
    if (!result || !result->root) {
        return string_ndup("Empty AST", 9);
    }
    ASTNode *node = result->root;
    StringBuilder *builder = sb_new();
    while (node) {
        char *dump = dumpASTNode(node, 0, false);
        sb_appendf(builder, "%s", dump);
        platform_free(dump, false);
        node = node->next;
        if (node) {
            sb_appendf(builder, ",\n");
        }
    }
    sb_appendf(builder, "\n");
    return sb_build(builder);
}

static char *dumpType(Type *type) {
    if (!type) return NULL;
    
    char *buffer = (char *) platform_allocate(2048, false); // Initial buffer size
    size_t bufferSize = 1024;
    size_t cursor = 0;
    
    while (type) {
        cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s", type->name);
        if (type->isArray) {
            cursor += snprintf(buffer + cursor, bufferSize - cursor, "[]");
        }
        if (type->next) {
            cursor += snprintf(buffer + cursor, bufferSize - cursor, " | ");
        }
        type = type->next;
    }
    
    // Ensure null termination of the buffer
    buffer[cursor] = '\0';
    
    return buffer;
}

// Recursive function to dump an AST node and its children/properties
static char *dumpASTNode(ASTNode *node, int indentLevel, b8 onlyPassIndent) {
    if (!node) return NULL;
    
    char *indent;
    if (!onlyPassIndent) {
        indent = createIndent(indentLevel);
    } else {
        indent = createIndent(0);
    }
    char *buffer = (char *) platform_allocate(1024, false); // Initial buffer size
    size_t bufferSize = 1024;
    size_t cursor = 0;
    
    switch (node->nodeType) {
        case AST_COMPONENT:
            cursor += snprintf(buffer + cursor, bufferSize - cursor, "%sComponent(%s)", indent,
                               node->data.component.name);
            if (node->data.component.extends) {
                char *extendsDump = dumpType(node->data.component.extends);
                cursor += snprintf(buffer + cursor, bufferSize - cursor, " : Type(%s)\n", extendsDump);
                platform_free(extendsDump, false);
            } else {
                cursor += snprintf(buffer + cursor, bufferSize - cursor, "\n");
            }
            if (node->data.component.body) {
                // Increase indent level for the body
                char *bodyDump = dumpASTNode(node->data.component.body, indentLevel + 1, false);
                cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s", bodyDump);
                platform_free(bodyDump, false);
                
            }
            break;
        case AST_SCOPE:
            cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s{\n", indent);
            ASTNode *child = node->data.scope.nodes;
            while (child) {
                // Recursively dump child nodes with increased indent level
                char *childDump = dumpASTNode(child, indentLevel + 3, false);
                cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s", childDump);
                platform_free(childDump, false);
                child = child->next;
                
                //if not last, add comma and new line
                if (child) {
                    cursor += snprintf(buffer + cursor, bufferSize - cursor, ",\n");
                } else {
                    // If it's the last child, do not add a comma, just close the scope.
                    cursor += snprintf(buffer + cursor, bufferSize - cursor, "\n%s}",
                                       createIndent(indentLevel)); // Properly close the scope
                }
            }
            break;
        case AST_PROPERTY:
            cursor += snprintf(buffer + cursor, bufferSize - cursor, "%sProperty(%s)", indent,
                               node->data.variable.name);
            //dump the type if it exists
            if (node->data.variable.type) {
                char *typeDump = dumpType(node->data.variable.type);
                cursor += snprintf(buffer + cursor, bufferSize - cursor, " : Type(%s)", typeDump);
                platform_free(typeDump, false);
            }
            //print the value if not null
            if (node->data.variable.value) {
                //if the value is a component, we pass the indent level
                if (node->data.variable.value->nodeType == AST_COMPONENT) {
                    char *valueDump = dumpASTNode(node->data.variable.value, indentLevel, false);
                    cursor += snprintf(buffer + cursor, bufferSize - cursor, " = %s", valueDump);
                    platform_free(valueDump, false);
                } else {
                    char *valueDump = dumpASTNode(node->data.variable.value, 0, false);
                    cursor += snprintf(buffer + cursor, bufferSize - cursor, " = %s", valueDump);
                    platform_free(valueDump, false);
                }
            }
            break;
        case AST_LITERAL:
            cursor += snprintf(buffer + cursor, bufferSize - cursor, "%sLiteral(", indent);
            switch (node->data.literal.type) {
                case LITERAL_NUMBER:
                    cursor += snprintf(buffer + cursor, bufferSize - cursor, "%f",
                                       node->data.literal.value.numberValue);
                    break;
                case LITERAL_STRING:
                    cursor += snprintf(buffer + cursor, bufferSize - cursor, "\"%s\"",
                                       node->data.literal.value.stringValue);
                    break;
                case LITERAL_BOOLEAN:
                    cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s",
                                       node->data.literal.value.booleanValue ? "true" : "false");
                    break;
            }
            cursor += snprintf(buffer + cursor, bufferSize - cursor, ")");
            break;
        case AST_ASSIGNMENT:
            // Prints in format "<indent>Assignment: <variableName> = <value>"
            cursor += snprintf(buffer + cursor, bufferSize - cursor, "%sAssignment(%s) = ", indent,
                               node->data.assignment.variableName);
            char *assignmentValueDump = dumpASTNode(node->data.assignment.value, indentLevel + 1, true);
            cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s", assignmentValueDump);
            break;
        case AST_ARRAY:
            cursor += snprintf(buffer + cursor, bufferSize - cursor, "%sArray[", indent);
            // Iterate over the array's elements
            ASTNode *element = node->data.array.elements;
            while (element) {
                char *elementDump = dumpASTNode(element, 0, false);
                cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s", elementDump);
                platform_free(elementDump, false);
                element = element->next;
                // Add a comma and a new line if there are more elements
                if (element) {
                    cursor += snprintf(buffer + cursor, bufferSize - cursor, ", ");
                }
            }
            cursor += snprintf(buffer + cursor, bufferSize - cursor, "%s]", indent);
            // Handle other cases as necessary
            break;
        case AST_EXPRESSION:
            cursor += snprintf(buffer + cursor, bufferSize - cursor, "%sExpression\n", indent);
            break;
        
    }
    
    
    platform_free(indent, false);
    
    // Ensure null termination of the buffer
    buffer[cursor] = '\0';
    
    return buffer;
}