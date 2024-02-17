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
#include "core/vmem.h"

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
        ASTNode *componentNode = parser_parse_component(&state);
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
            kbye(componentNode, sizeof(ASTNode));
            state->current_token_index = index;
            return null;
        }
    }

    // If there's not a LBRACE, at this point, we return null because it's not a valid component
    if (!match(state, TOKEN_LBRACE)) {
        state->current_token_index = index;
        kbye(componentNode, sizeof(ASTNode));
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
            kbye(name, identifierToken.length);
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
            kbye(name, identifierToken.length);
            return null;
        }
        // Fill in the property node details
        propertyNode->data.variable.name = name;
        propertyNode->data.variable.type = parser_parse_type(state);
        if (!propertyNode->data.variable.type) {
            verror("Failed to parse property type");
            kbye(name, identifierToken.length);
            kbye(propertyNode, sizeof(ASTNode));
            return null;
        }
        // Optionally parse the assignment
        if (match(state, TOKEN_EQUALS)) {
            propertyNode->data.variable.value = parser_parse_expression(state);
            if (!propertyNode->data.variable.value) {
                verror("Failed to parse property value");
                kbye(name, identifierToken.length);
                kbye(propertyNode, sizeof(ASTNode));
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
            kbye(componentNode, sizeof(ASTNode));
            return null;
        }
        // Optionally parse the super type
        if (match(state, TOKEN_COLON)) {
            componentNode->data.component.extends = parser_parse_type(state);
            if (!componentNode->data.component.extends) {
                verror("Failed to parse component super type");
                kbye(componentNode->data.component.name, identifierToken.length);
                kbye(componentNode, sizeof(ASTNode));
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
    ASTNode *node = (ASTNode *) kgimme(sizeof(ASTNode));
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
            kbye(node, sizeof(ASTNode)); // Cleanup the allocated node
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

    ASTNode *arrayNode = (ASTNode *) kgimme(sizeof(ASTNode));
    arrayNode->nodeType = AST_ARRAY;
    ASTNode **currentElement = &(arrayNode->data.array.elements);

    while (!match(state, TOKEN_RBRACKET)) {
        skip_delimiters(state); // Skip delimiters between elements

        ASTNode *elementNode = parser_parse_expression(state); // Assuming this function is implemented
        if (!elementNode) {
            // Handle parsing error, clean up
            verror("Failed to parse array element");
            kbye(arrayNode, sizeof(ASTNode));
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
                kbye(temp->name, strlen(temp->name));
                kbye(temp, sizeof(Type));
            }
            return NULL;
        }

        Type *type = (Type *) kgimme(sizeof(Type));
        if (!type) {
            verror("Allocation failure for Type");
            // Cleanup any allocated types before returning
            kbye(type, sizeof(Type));
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
    ASTNode *node = (ASTNode *) kgimme(sizeof(ASTNode));
    if (!node) {
        verror("Failed to allocate memory for AST node");
        return null;
    }
    node->nodeType = type;
    node->next = null; // Ensure the next pointer is initialized to null
    return node;
}




// =============================================================================
// Dump Functions for Each AST Node Type
// =============================================================================
// Forward declarations for recursive dumping
void dumpASTNode(ASTNode* node, StringBuilder* sb, int indentLevel, int isLast);

// Utility function to append indentation
void appendIndent(StringBuilder* sb, int indentLevel, int isLast) {
    for (int i = 0; i < indentLevel; ++i) {
        sb_appendf(sb, "│   ");
    }
    if (indentLevel > 0) {
        sb_appendf(sb, "%s── ", isLast ? "└" : "├");
    }
}

// Utility function for type dumping (handles composite types)
void dumpType(Type* type, StringBuilder* sb) {
    for (Type* t = type; t != NULL; t = t->next) {
        sb_appendf(sb, "%s", t->name);
        if (t->next) {
            sb_appendf(sb, " | ");
        }
    }
}

// Specific dumping functions for each node type
void dumpLiteral(ASTNode* node, StringBuilder* sb, int indentLevel, int isLast) {
    appendIndent(sb, indentLevel, isLast);
    sb_appendf(sb, "Literal: ");
    switch (node->data.literal.type) {
        case LITERAL_NUMBER:
            sb_appendf(sb, "%f\n", node->data.literal.value.numberValue);
            break;
        case LITERAL_STRING:
            sb_appendf(sb, "%s\n", node->data.literal.value.stringValue);
            break;
        case LITERAL_BOOLEAN:
            sb_appendf(sb, "%s\n", node->data.literal.value.booleanValue ? "true" : "false");
            break;
    }
}

void dumpProperty(ASTNode* node, StringBuilder* sb, int indentLevel, int isLast) {
    appendIndent(sb, indentLevel, isLast);
    sb_appendf(sb, "Property: %s, Type: ", node->data.variable.name);
    dumpType(node->data.variable.type, sb);
    sb_appendf(sb, "\n");
    if (node->data.variable.value) {
        dumpASTNode(node->data.variable.value, sb, indentLevel + 1, 1); // Property value is always last
    }
}

void dumpComponent(ASTNode* node, StringBuilder* sb, int indentLevel, int isLast) {
    appendIndent(sb, indentLevel, isLast);
    sb_appendf(sb, "Component: %s\n", node->data.component.name);
    if (node->data.component.extends) {
        appendIndent(sb, indentLevel + 1, 0);
        sb_appendf(sb, "Extends: ");
        dumpType(node->data.component.extends, sb);
        sb_appendf(sb, "\n");
    }
    if (node->data.component.body) {
        dumpASTNode(node->data.component.body, sb, indentLevel + 1, 1); // Component body is always last
    }
}

void dumpScope(ASTNode* node, StringBuilder* sb, int indentLevel, int isLast) {
    appendIndent(sb, indentLevel, isLast);
    sb_appendf(sb, "Scope:\n");
    for (ASTNode* stmt = node->data.scope.nodes; stmt != NULL; stmt = stmt->next) {
        dumpASTNode(stmt, sb, indentLevel + 1, stmt->next == NULL);
    }
}

void dumpAssignment(ASTNode* node, StringBuilder* sb, int indentLevel, int isLast) {
    appendIndent(sb, indentLevel, isLast);
    sb_appendf(sb, "Assignment: %s = \n", node->data.assignment.variableName);
    dumpASTNode(node->data.assignment.value, sb, indentLevel + 1, 1); // Assignment value is always last
}

void dumpArray(ASTNode* node, StringBuilder* sb, int indentLevel, int isLast) {
    appendIndent(sb, indentLevel, isLast);
    sb_appendf(sb, "Array:\n");
    for (ASTNode* elem = node->data.array.elements; elem != NULL; elem = elem->next) {
        dumpASTNode(elem, sb, indentLevel + 1, elem->next == NULL);
    }
}

// Main recursive function to dump AST nodes
void dumpASTNode(ASTNode* node, StringBuilder* sb, int indentLevel, int isLast) {
    if (!node) return;

    switch (node->nodeType) {
        case AST_LITERAL:
            dumpLiteral(node, sb, indentLevel, isLast);
            break;
        case AST_PROPERTY:
            dumpProperty(node, sb, indentLevel, isLast);
            break;
        case AST_COMPONENT:
            dumpComponent(node, sb, indentLevel, isLast);
            break;
        case AST_SCOPE:
            dumpScope(node, sb, indentLevel, isLast);
            break;
        case AST_ASSIGNMENT:
            dumpAssignment(node, sb, indentLevel, isLast);
            break;
        case AST_ARRAY:
            dumpArray(node, sb, indentLevel, isLast);
            break;
            // Add other cases as necessary
    }

//     Uncomment if siblings are handled outside of child nodes
     if (node->next && node->nodeType == AST_COMPONENT) {
         dumpASTNode(node->next, sb, indentLevel, !node->next->next);
     }
}

// Entry point for dumping the AST to a string
char* dumpASTToString(ASTNode* root) {
    StringBuilder* sb = sb_new();
    dumpASTNode(root, sb, 0, 1);
    char* result = sb_build(sb);
    sb_free(sb);
    return result;
}
// =============================================================================
// Entry Point for Dumping the Program AST
// =============================================================================
char *parser_dump_program(ProgramAST *result) {
    if (!result || !result->root) {
        return string_ndup("Empty AST", 9);
    }
    return dumpASTToString(result->root);
}

b8 parser_free_program(ProgramAST *program) {
    if (!program) return false;
    ASTNode *current = program->root;
    while (current) {
        ASTNode *next = current->next;
        //free the type

        if (current->nodeType == AST_PROPERTY) {
            kbye(current->data.variable.name, strlen(current->data.variable.name));
            kbye(current->data.variable.type, sizeof(Type));
        }
        if (current->nodeType == AST_LITERAL && current->data.literal.type == LITERAL_STRING) {
            kbye(current->data.literal.value.stringValue, strlen(current->data.literal.value.stringValue));
        }
        if (current->nodeType == AST_COMPONENT) {
            kbye(current->data.component.name, strlen(current->data.component.name));
            if (current->data.component.extends) {
                kbye(current->data.component.extends, sizeof(Type));
            }
            kbye(current->data.component.body, sizeof(ASTNode));
        }
        if (current->nodeType == AST_ARRAY) {
            ASTNode *element = current->data.array.elements;
            while (element) {
                ASTNode *nextElement = element->next;
                kbye(element, sizeof(ASTNode));
                element = nextElement;
            }
        }

        if (current->nodeType == AST_SCOPE) {
            ASTNode *child = current->data.scope.nodes;
            while (child) {
                ASTNode *nextChild = child->next;
                kbye(child, sizeof(ASTNode));
                child = nextChild;
            }
        }
        kbye(current, sizeof(ASTNode));
        current = next;
    }
    return true;
}
