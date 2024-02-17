/**
 * Created by jraynor on 2/15/2024.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "muil_parser.h"
#include "core/vlogger.h"
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

// Parses a primary expression (variable, literal, array, scope, etc.), with precedence
ASTNode *parser_parse_primary(ParserState *state);

// Parses an assignment (variable assignment)
ASTNode *parser_parse_assignment(ParserState *state);

// Parses an array literal
ASTNode *parser_parse_array(ParserState *state);

//Parse a binary operation
ASTNode *parser_parse_binary_op(ParserState *state);

// Parses an expression
ASTNode *parser_parse_expression(ParserState *state);

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
        ASTNode *childNode = parser_parse_expression(state);
        if (!childNode) {
            // Handle parsing error, including potential cleanup
            break; // Exit the loop on error
        }
        
        *current = childNode; // Link the parsed property or nested component
        current = &((*current)->next); // Prepare for the next property/component
        
    }
    
    return scopeNode;
}


// Parses a literal value (number, string, boolean)
ASTNode *parser_parse_literal(ParserState *state) {
    Token token = consume(state, peek(state).type); // Adjusted to directly consume the next token
    ASTNode *node = (ASTNode *) kgimme(sizeof(ASTNode));
    if (!node) {
        verror("Allocation failure for ASTNode");
        return null; // Allocation failure
    }
    
    node->nodeType = AST_LITERAL;
    switch (token.type) {
        case TOKEN_NUMBER:node->data.literal.type = LITERAL_NUMBER;
            node->data.literal.value.numberValue = atof(token.start);
            break;
        case TOKEN_STRING:node->data.literal.type = LITERAL_STRING;
            node->data.literal.value.stringValue = string_ndup(token.start, token.length);
            break;
        case TOKEN_TRUE:
        case TOKEN_FALSE:node->data.literal.type = LITERAL_BOOLEAN;
            node->data.literal.value.booleanValue = (strcmp(token.start, "true") == 0);
            break;
        default:kbye(node, sizeof(ASTNode)); // Cleanup the allocated node
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


static int get_operator_precedence(TokenType type) {
    switch (type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:return 1;
        case TOKEN_STAR:
        case TOKEN_SLASH:return 2;
            // Extend with more operators and precedence levels as needed
        default:return -1; // Non-operator or unsupported operator
    }
}

ASTNode *parser_parse_expression_with_precedence(ParserState *state, int precedence) {
    // Parse the left-hand side as a primary expression
    ASTNode *left = parser_parse_primary(state);
    if (!left) return NULL;
    
    while (true) {
        int operatorPrecedence = get_operator_precedence(peek(state).type);
        if (operatorPrecedence < precedence) break;
        
        // Consume the operator
        TokenType operatorType = consume(state, peek(state).type).type;
        
        // Parse the right-hand side with higher precedence
        ASTNode *right = parser_parse_expression_with_precedence(state, operatorPrecedence + 1);
        if (!right) {
            parser_free_node(left); // Cleanup left node on failure
            return NULL;
        }
        
        // Create a new binary operation node
        ASTNode *binaryOpNode = create_node(AST_BINARY_OP);
        binaryOpNode->data.binaryOp.left = left;
        binaryOpNode->data.binaryOp.operator = operatorType;
        binaryOpNode->data.binaryOp.right = right;
        
        left = binaryOpNode; // The binary operation node becomes the new left-hand side
    }
    
    return left;
}


ASTNode *parser_parse_expression(ParserState *state) {
    // This function now acts as a wrapper around parser_parse_expression_with_precedence
    // with the lowest precedence level, allowing it to be the entry point for parsing any expression.
    return parser_parse_expression_with_precedence(state, 0);
}

ASTNode *parser_parse_primary(ParserState *state) {
    Token token = peek(state);
    switch (token.type) {
        case TOKEN_NUMBER:
        case TOKEN_STRING:
        case TOKEN_TRUE:
        case TOKEN_FALSE:return parser_parse_literal(state);
        case TOKEN_IDENTIFIER: {
            // Lookahead to check for assignment or function call
            Token nextToken = peek_distance(state, 1); // Ensure peek_distance correctly skips delimiters
            consume(state, TOKEN_IDENTIFIER);
            if (nextToken.type == TOKEN_EQUALS) {
                ASTNode *assignmentNode = create_node(AST_PROPERTY);
                if (!assignmentNode) return null; // Error handling within create_node
                assignmentNode->data.variable.name = string_ndup(token.start, token.length);
                consume(state, TOKEN_EQUALS);
                assignmentNode->data.variable.value = parser_parse_expression(state);
                return assignmentNode;
            } else if (nextToken.type == TOKEN_LBRACE) {
                // It's a nested, named component. Lets create an assignment node and a scope node
                ASTNode *assignmentNode = create_node(AST_PROPERTY);
                if (!assignmentNode) return null; // Error handling within create_node
                assignmentNode->data.variable.name = string_ndup(token.start, token.length);
                consume(state, TOKEN_LBRACE);
                ASTNode *scope = parser_parse_scope(state);
                assignmentNode->data.variable.value = scope;
                return assignmentNode;
            } else if (nextToken.type == TOKEN_COLON) {
                ASTNode *propertyNode = create_node(AST_PROPERTY);
                if (!propertyNode) return null; // Error handling within create_node
                propertyNode->data.variable.name = string_ndup(token.start, token.length);
                consume(state, TOKEN_COLON);
                propertyNode->data.variable.type = parser_parse_type(state);
                if (match(state, TOKEN_EQUALS) || peek(state).type == TOKEN_LBRACE) {
                    propertyNode->data.variable.value = parser_parse_expression(state);
                }
                return propertyNode;
            } else {
                // It's a variable
                ASTNode *variableNode = create_node(AST_PROPERTY);
                if (!variableNode) return null; // Error handling within create_node
                variableNode->data.variable.name = string_ndup(token.start, token.length);
                if (match(state, TOKEN_EQUALS)) {
                    variableNode->data.variable.value = parser_parse_expression(state);
                }
                return variableNode;
            }
        }
        case TOKEN_LPAREN: {
            consume(state, TOKEN_LPAREN);
            ASTNode *subExpr = parser_parse_expression(state);
            consume(state, TOKEN_RPAREN);
            return subExpr;
        }
        
        case TOKEN_LBRACE:consume(state, TOKEN_LBRACE);
            return parser_parse_scope(state);
        
        default:
            // Error handling for unexpected token
            verror("Unexpected token '%s' in expression", get_token_type_name(token.type));
            return NULL;
//            return parser_parse_scope(state);
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


/// =============================================================================
// Memory cleanup functions
// =============================================================================

void parser_free_type(Type *type) {
    while (type != NULL) {
        Type *next = type->next;
        if (type->name) {
            kbye(type->name, strlen(type->name) + 1); // +1 for null terminator
        }
        kbye(type, sizeof(Type));
        type = next;
    }
}

void parser_free_component_node(ComponentNode *component) {
    if (component->name) {
        kbye(component->name, strlen(component->name) + 1);
    }
    if (component->extends) {
        parser_free_type(component->extends);
    }
    parser_free_node(component->body);
}

void parser_free_property_node(VariableNode *variable) {
    if (variable->name) {
        kbye(variable->name, strlen(variable->name) + 1);
    }
    if (variable->type) {
        parser_free_type(variable->type);
    }
    parser_free_node(variable->value);
}

void parser_free_literal_node(LiteralNode *literal) {
    if (literal->type == LITERAL_STRING && literal->value.stringValue) {
        kbye(literal->value.stringValue, strlen(literal->value.stringValue) + 1);
    }
}

void parser_free_assignment_node(AssignmentNode *assignment) {
    if (assignment->variableName) {
        kbye(assignment->variableName, strlen(assignment->variableName) + 1);
    }
    parser_free_node(assignment->value);
}

void parser_free_array_node(ArrayNode *array) {
    ASTNode *current = array->elements;
    while (current != NULL) {
        ASTNode *next = current->next;
        parser_free_node(current);
        current = next;
    }
}

void parser_free_binary_op_node(BinaryOpNode *binaryOp) {
    parser_free_node(binaryOp->left);
    parser_free_node(binaryOp->right);
}

void parser_free_scope_node(ScopeNode *scope) {
    ASTNode *current = scope->nodes;
    while (current != NULL) {
        ASTNode *next = current->next;
        parser_free_node(current);
        current = next;
    }
}

b8 parser_free_node(ASTNode *node) {
    if (node == NULL) return false;
    
    switch (node->nodeType) {
        case AST_COMPONENT:parser_free_component_node(&node->data.component);
            break;
        case AST_PROPERTY:parser_free_property_node(&node->data.variable);
            break;
        case AST_LITERAL:parser_free_literal_node(&node->data.literal);
            break;
        case AST_ASSIGNMENT:parser_free_assignment_node(&node->data.assignment);
            break;
        case AST_ARRAY:parser_free_array_node(&node->data.array);
            break;
        case AST_SCOPE:parser_free_scope_node(&node->data.scope);
            break;
        case AST_BINARY_OP:parser_free_binary_op_node(&node->data.binaryOp);
            break;
        default:kbye(node, sizeof(ASTNode));
            vwarn("Unhandled node type in parser_free_node");
            return false;
    }
    kbye(node, sizeof(ASTNode));
    return true;
}

b8 parser_free_program(ProgramAST *program) {
    if (!program) return false;
    parser_free_node(program->root);
    return true;
}