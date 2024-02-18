/**
 * Created by jraynor on 2/15/2024.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "muil_parser.h"
#include "core/vlogger.h"
#include "core/vstring.h"
#include "core/vmem.h"
#include "platform/platform.h"
#include "containers/stack.h"

#ifndef MAX_ERROR_MESSAGE
#define MAX_ERROR_MESSAGE 256
#endif
typedef struct ParserState {
    ProgramSource *source;
    u32 current_token_index;
    Stack *error_stack;
} ParserState;

// =============================================================================//
// Internal helper functions                                                    //
// =============================================================================//
static Token peek(ParserState *state);

static Token peek_distance(ParserState *state, u32 distance);

static Token consume(ParserState *state, TokenType expectedType);


static Token _expect(ParserState *state, TokenType expectedType, const char *message, const char *file_location);

// Replaces _expectf with direct usage of _expect and snprintf for message formatting
#define expect(state, expectedType, fmt, ...) do { \
    char _message[256]; /* Adjust size as needed */ \
    snprintf(_message, sizeof(_message), (fmt), ##__VA_ARGS__); \
    _expect((state), (expectedType), _message, LOG_CALL_LOCATION); \
} while (0)


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

// Parses an array literal
ASTNode *parser_parse_array(ParserState *state);

// Parses an expression
ASTNode *parser_parse_expression(ParserState *state);

//Parses a type. A type can be a single identifier a compound type or an array type.
//I.e. "string", "int", "string | int", "string[]", "int[]", "string | int[]"
Type *parser_parse_type(ParserState *state);

Type *parser_parse_tuple_element(ParserState *state);

Type *parser_parse_tuple_type(ParserState *state);

Type *parser_parse_single_type(ParserState *state);

// =============================================================================
// Parser API
// =============================================================================

ProgramAST parser_parse(ProgramSource *source) {
    ParserState state = {source, 0, stack_new()};
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

// =============================================================================
// Parser API
// =============================================================================


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
        componentNode->data.component.super = parser_parse_type(state);
        if (!componentNode->data.component.super) {
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

ASTNode *parser_parse_function_call(ParserState *state) {
    ASTNode *functionCallNode = create_node(AST_FUNCTION_CALL);
    if (!functionCallNode) return null; // Error handling within create_node
    
    //If last token was identifier, it's the function name
    Token lastToken = peek_distance(state, -1);
    if (lastToken.type != TOKEN_IDENTIFIER) {
        if (peek(state).type == TOKEN_IDENTIFIER) {
            functionCallNode->data.functionCall.name = string_ndup(lastToken.start, lastToken.length);
            goto skip_name;
        }
        verror("Expected identifier before '(' in function call");
        kbye(functionCallNode, sizeof(ASTNode));
        return null;
    }
    
    // Parse the function name
//    Token nameToken = consume(state, TOKEN_IDENTIFIER);
    functionCallNode->data.functionCall.name = string_ndup(lastToken.start, lastToken.length);
    skip_name:
    // Parse the function arguments
    consume(state, TOKEN_LPAREN);
    ASTNode **currentArgument = &(functionCallNode->data.functionCall.arguments);
    while (!match(state, TOKEN_RPAREN)) {
        ASTNode *argument = parser_parse_expression(state);
        if (!argument) {
            // Handle parsing error, clean up
            // It's a zero-argument function call
            expect(state, TOKEN_RPAREN, "Expected ')' after function call arguments");
            break;
        }
        *currentArgument = argument;
        currentArgument = &((*currentArgument)->next);
        if (match(state, TOKEN_COMMA)) {
            // Consume the comma and continue parsing arguments
        }
    }
    
    return functionCallNode;
}

ASTNode *parser_parse_primary(ParserState *state) {
    Token token = peek(state);
    // Lookahead to check for assignment or function call
    Token nextToken = peek_distance(state, 1); // Ensure peek_distance correctly skips delimiters
    // if the next token is a dot, it's it's a reference
    
    switch (token.type) {
        case TOKEN_NUMBER:
        case TOKEN_STRING:
        case TOKEN_TRUE:
        case TOKEN_FALSE:return parser_parse_literal(state);
        case TOKEN_IDENTIFIER: {
            consume(state, TOKEN_IDENTIFIER);
            if (nextToken.type == TOKEN_EQUALS) {
                ASTNode *assignmentNode = create_node(AST_ASSIGNMENT);
                if (!assignmentNode) return null; // Error handling within create_node
                assignmentNode->data.assignment.variableName = string_ndup(token.start, token.length);
                consume(state, TOKEN_EQUALS);
                assignmentNode->data.assignment.value = parser_parse_expression(state);
                return assignmentNode;
            } else if (nextToken.type == TOKEN_LBRACE) {
                // It's a nested, named component. Lets create an assignment node and a scope node
                ASTNode *assignmentNode = create_node(AST_PROPERTY);
                if (!assignmentNode) return null; // Error handling within create_node
                assignmentNode->data.property.name = string_ndup(token.start, token.length);
                consume(state, TOKEN_LBRACE);
                ASTNode *scope = parser_parse_scope(state);
                assignmentNode->data.property.value = scope;
                return assignmentNode;
            } else if (nextToken.type == TOKEN_COLON) {
                ASTNode *propertyNode = create_node(AST_PROPERTY);
                if (!propertyNode) return null; // Error handling within create_node
                propertyNode->data.property.name = string_ndup(token.start, token.length);
                consume(state, TOKEN_COLON);
                propertyNode->data.property.type = parser_parse_type(state);
                if (match(state, TOKEN_EQUALS)) {
                    // delete the property node and create an  node
                    
                    ASTNode *assignmentNode = create_node(AST_ASSIGNMENT);
                    if (!assignmentNode) return null; // Error handling within create_node
                    assignmentNode->data.assignment.variableName = string_ndup(token.start, token.length);
                    assignmentNode->data.assignment.value = parser_parse_expression(state);
                    propertyNode->data.property.value = assignmentNode;
                    return propertyNode;
                }
                return propertyNode;
            } else if (nextToken.type == TOKEN_LPAREN) {
                return parser_parse_function_call(state);
            } else if (nextToken.type == TOKEN_DOT) {
                ASTNode *referenceNode = create_node(AST_REFERENCE);
                if (!referenceNode) return null; // Error handling within create_node
                referenceNode->data.reference.name = string_ndup(token.start, token.length);
                consume(state, TOKEN_DOT);
                referenceNode->data.reference.reference = parser_parse_primary(state);
                return referenceNode;
            } else {
                // It's a variable
                if (match(state, TOKEN_EQUALS)) {
                    // create assignment node
                    ASTNode *assignmentNode = create_node(AST_ASSIGNMENT);
                    if (!assignmentNode) return null; // Error handling within create_node
                    assignmentNode->data.assignment.variableName = string_ndup(token.start, token.length);
                    assignmentNode->data.assignment.value = parser_parse_expression(state);
                    return assignmentNode;
                }
                ASTNode *variableNode = create_node(AST_PROPERTY);
                if (!variableNode) return null; // Error handling within create_node
                variableNode->data.property.name = string_ndup(token.start, token.length);
                // Maybe parse type here
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
        case TOKEN_LBRACKET:return parser_parse_array(state);
        
        default:
//            if (match(state, TOKEN_DOT)) {
//                ASTNode *lhs = parser_parse_primary(state);
//                consume(state, TOKEN_DOT);
//                ASTNode *referenceNode = create_node(AST_REFERENCE);
//                if (!referenceNode) return null; // Error handling within create_node
//                referenceNode->data.reference.name = string_ndup(token.start, token.length);
//                referenceNode->data.reference.reference = parser_parse_primary(state);
//                return referenceNode;
//            }
            return NULL;
//            return parser_parse_scope(state);
    }
}

Type *create_basic_type(Token token) {
    Type *type = (Type *) kgimme(sizeof(Type));
    if (!type) {
        verror("Allocation failure for Type");
        return null;
    }
    type->kind = TYPE_BASIC;
    type->data.name = string_ndup(token.start, token.length);
    type->next = null;
    return type;
}

void add_type_to_tuple(Type *tupleType, Type *newType) {
    // Ensure the tupleType is actually a tuple
    if (tupleType->kind != TYPE_TUPLE) {
        verror("Trying to add a type to a non-tuple type");
        return;
    }
    
    // If the tuple is empty, directly assign the new type
    if (!tupleType->data.tupleTypes) {
        tupleType->data.tupleTypes = newType;
    } else {
        // Find the end of the list and add the new type there
        Type *current = tupleType->data.tupleTypes;
        while (current->next) {
            current = current->next;
        }
        current->next = newType;
    }
}

Type *create_tuple_type() {
    // Allocate memory for the tuple type itself
    Type *tupleType = (Type *) kgimme(sizeof(Type));
    if (!tupleType) {
        verror("Allocation failure for Tuple Type");
        return NULL;
    }
    
    // Initialize the tuple type
    tupleType->kind = TYPE_TUPLE;
    tupleType->data.tupleTypes = NULL; // Initially, there are no types in the tuple
    
    return tupleType;
}


Type *create_union_type(Type *firstType, Type *secondType) {
    Type *type = (Type *) kgimme(sizeof(Type));
    if (!type) {
        verror("Allocation failure for Type");
        return null;
    }
    type->kind = TYPE_UNION;
    type->data.binary.lhs = firstType;
    type->data.binary.rhs = secondType;
    return type;
}

Type *create_intersection_type(Type *firstType, Type *secondType) {
    Type *type = (Type *) kgimme(sizeof(Type));
    if (!type) {
        verror("Allocation failure for Type");
        return null;
    }
    type->kind = TYPE_INTERSECTION;
    type->data.binary.lhs = firstType;
    type->data.binary.rhs = secondType;
    return type;
    
    return type;
}

Type *create_function_type(Type *inputType, Type *outputType) {
    Type *type = (Type *) kgimme(sizeof(Type));
    if (!type) {
        verror("Allocation failure for Type");
        return null;
    }
    type->kind = TYPE_FUNCTION;
    type->data.binary.lhs = inputType;
    type->data.binary.rhs = outputType;
    return type;
}

Type *create_array_type(Type *elementType) {
    Type *type = (Type *) kgimme(sizeof(Type));
    if (!type) {
        verror("Allocation failure for Type");
        return null;
    }
    type->kind = TYPE_ARRAY;
    type->data.array.elementType = elementType;
    return type;
}


Type *parser_parse_single_type(ParserState *state) {
    if (peek(state).type == TOKEN_LPAREN) {
        return parser_parse_tuple_type(state);
    }
    if (peek(state).type == TOKEN_IDENTIFIER) {
        Token token = consume(state, TOKEN_IDENTIFIER);
        // Handle array types
        if (match(state, TOKEN_LBRACKET)) {
            expect(state, TOKEN_RBRACKET, "Expected ']' after array type");
            return create_array_type(create_basic_type(token));
        }
        // Basic type
        return create_basic_type(token);
    } else if (match(state, TOKEN_LBRACKET)) {
        Type *elementType = parser_parse_type(state);
        expect(state, TOKEN_RBRACKET, "Expected ']' after array type");
        return create_array_type(elementType);
    } else {
//        verror("Unexpected token in type at line %d, column %d", peek(state).line, peek(state).column);
        return null;
    }
}

Type *parser_parse_tuple_element(ParserState *state) {
    char *alias = null;
    if (peek(state).type == TOKEN_IDENTIFIER && peek_distance(state, 1).type == TOKEN_COLON) {
        Token aliasToken = consume(state, TOKEN_IDENTIFIER);
        consume(state, TOKEN_COLON); // consume the colon
        alias = string_ndup(aliasToken.start, aliasToken.length);
    }
    
    Type *type = parser_parse_single_type(state);
    if (alias) {
        type->alias = alias; // Assign the alias to the type
    }
    
    return type;
}

Type *parser_parse_tuple_type(ParserState *state) {
    expect(state, TOKEN_LPAREN, "Expected '(' to start a tuple");
    
    Type *tupleType = create_tuple_type();
    do {
        Type *elementType = parser_parse_tuple_element(state);
        if (!elementType) {
            //Not an error, just a tuple with no elements
            break;
        }
        add_type_to_tuple(tupleType, elementType);
        // Only expect a comma if there's another tuple element
        if (!match(state, TOKEN_COMMA)) {
            break;
        }
    } while (true);
    
    expect(state, TOKEN_RPAREN, "Expected ')' after tuple");
    return tupleType;
}

//Parses a type. A type can be a single identifier a compound type or an array type.
//I.e. "string", "int", "string | int", "string[]", "int[]", "string | int[]"
Type *parser_parse_type(ParserState *state) {
    Type *type = parser_parse_single_type(state); // Parse the initial type part
    
    while (true) {
        if (match(state, TOKEN_PIPE)) {
            type = create_union_type(type, parser_parse_single_type(state));
        } else if (match(state, TOKEN_AMPERSAND)) {
            type = create_intersection_type(type, parser_parse_single_type(state));
        } else if (match(state, TOKEN_ARROW)) {
            type = create_function_type(type,
                                        parser_parse_type(state)); // Right-hand side could be another complex type
        } else {
            break; // No more type operators to parse
        }
    }
    
    return type;
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
        verror("Expected token type %s but got %s at line %d, column %d", lexer_token_type_name(expectedType),
               lexer_token_type_name(token.type), token.line, token.column);
        return (Token) {TOKEN_ERROR, null, 0, token.line, token.column}; // Assuming TOKEN_ERROR is defined
    }
    state->current_token_index++;
    return token;
}

// Modified _expect function to use dynamic string formatting
static Token _expect(ParserState *state, TokenType expectedType, const char *message, const char *file_location) {
    Token token = peek(state);
    if (token.type != expectedType) {
        char formatted_message[256]; // Adjust size as needed
        snprintf(formatted_message, sizeof(formatted_message), "%s at line %d, column %d.", message, token.line,
                 token.length);
        // Log the formatted message (assuming your logging mechanism)
//        error(formatted_message, file_location);
        log_output(LOG_LEVEL_ERROR, file_location, formatted_message);
//        printf("Parsing Error: %s\n", formatted_message);
        return (Token) {TOKEN_ERROR, NULL, 0, token.line, token.column}; // Assuming TOKEN_ERROR is defined
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
    while (type) {
        Type *next = type->next; // Preserve next pointer before freeing
        
        switch (type->kind) {
            case TYPE_BASIC:if (type->data.name) kbye(type->data.name, strlen(type->data.name) + 1);
                break;
            case TYPE_ARRAY:parser_free_type(type->data.array.elementType);
                break;
            case TYPE_UNION:
            case TYPE_INTERSECTION:
            case TYPE_FUNCTION:parser_free_type(type->data.binary.lhs);
                parser_free_type(type->data.binary.rhs);
                break;
            case TYPE_TUPLE:
                // Assuming TYPE_TUPLE uses the same structure as others for elements
                parser_free_type(type->data.tupleTypes);
                break;
        }
        
        kbye(type, sizeof(Type));
        type = next; // Move to next type in the list (if any)
    }
}

void parser_free_component_node(ComponentNode *component) {
    if (component->name) {
        kbye(component->name, strlen(component->name) + 1);
    }
    if (component->super) {
        parser_free_type(component->super);
    }
    parser_free_node(component->body);
}

void parser_free_property_node(PropertyNode *variable) {
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

void parser_free_reference_node(ReferenceNode *reference) {
    if (reference->name) {
        kbye(reference->name, strlen(reference->name) + 1);
    }
    parser_free_node(reference->reference);
}

void parser_free_function_call_node(FunctionCallNode *functionCall) {
    if (functionCall->name) {
        kbye(functionCall->name, strlen(functionCall->name) + 1);
    }
    parser_free_node(functionCall->reference);
    parser_free_node(functionCall->arguments);
}

b8 parser_free_node(ASTNode *node) {
    if (node == NULL) return false;
    ASTNode *current = node;
    while (current != NULL) {
        ASTNode *next = current->next;
        switch (current->nodeType) {
            case AST_COMPONENT:parser_free_component_node(&current->data.component);
                break;
            case AST_PROPERTY:parser_free_property_node(&current->data.property);
                break;
            case AST_LITERAL:parser_free_literal_node(&current->data.literal);
                break;
            case AST_ASSIGNMENT:parser_free_assignment_node(&current->data.assignment);
                break;
            case AST_ARRAY:parser_free_array_node(&current->data.array);
                break;
            case AST_SCOPE:parser_free_scope_node(&current->data.scope);
                break;
            case AST_BINARY_OP:parser_free_binary_op_node(&current->data.binaryOp);
                break;
            case AST_REFERENCE:parser_free_reference_node(&current->data.reference);
                break;
            case AST_FUNCTION_CALL:parser_free_function_call_node(&current->data.functionCall);
                break;
            default:vwarn("Unhandled node type in parser_free_node");
                return false;
        }
        kbye(current, sizeof(ASTNode));
        current = next;
    }
    kbye(node, sizeof(ASTNode));
    return true;
}

b8 parser_free_program(ProgramAST *program) {
    if (!program) return false;
    parser_free_node(program->root);
    return true;
}

ASTNode *parser_get_node(void *node) {
    // This is a pointer to one of the union members of the ASTNode struct
    // We can use this to get the actual type of the node
    size_t offset = offsetof(ASTNode, data);
    ASTNode *astNode = (ASTNode *) ((char *) node - offset);
    return astNode;
}
