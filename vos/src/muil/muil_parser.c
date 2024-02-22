/**
 * Created by jraynor on 2/15/2024.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include "muil_parser.h"
#include "core/vlogger.h"
#include "core/vstring.h"
#include "lib/vmem.h"
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

// Parses an identifier
ASTNode *parser_parse_identifier(ParserState *state);

ASTNode *parser_parse_reference(ParserState *state, Token identifier);

ASTNode *parser_parse_func_call(ParserState *state, Token identifier);

ASTNode *parser_parse_assignment(ParserState *state, Token identifier);



// =============================================================================
// Type parsing
// =============================================================================

//Parses a type. A type can be a single identifier a compound type or an array type.
//I.e. "string", "int", "string | int", "string[]", "int[]", "string | int[]"
TypeSymbol *parser_parse_type(ParserState *state);

TypeSymbol *parser_parse_tuple_element(ParserState *state);

TypeSymbol *parser_parse_tuple_type(ParserState *state);

TypeSymbol *parser_parse_single_type(ParserState *state);


// =============================================================================
// Parser API
// =============================================================================

ProgramAST parser_parse(ProgramSource *source) {
    ParserState state = {source, 0, stack_new()};
    ProgramAST result = {null};
    result.root = parser_parse_scope(&state);
//    ASTNode **current = &(root_scope->data.scope.body);
//
//    while (state.current_token_index < source->count && peek(&state).type != TOKEN_EOF) {
//        ASTNode *componentNode = parser_parse_component(&state);
//        if (!componentNode) {
//            verror("Failed to parse top-level component");
//            // Perform any necessary cleanup
//            break;
//        }
//
//        *current = componentNode;
//        current = &((*current)->next);
//
//    }
    
    return result;
}

// =============================================================================
// Parser API
// =============================================================================


ASTNode *parser_parse_component(ParserState *state) {
    u32 index = state->current_token_index;
    Token nameToken = peek(state);
    if (nameToken.type != TOKEN_IDENTIFIER) {
        return null;
    }
    consume(state, TOKEN_IDENTIFIER);
    ASTNode *componentNode = create_node(AST_COMPONENT_DECLARE);
    if (!componentNode) return null; // Error handling within create_node
    
    componentNode->data.compound.name = string_ndup(nameToken.start, nameToken.length);
    
    // Potentially parse the super type if applicable
    if (match(state, TOKEN_COLON)) {
        componentNode->data.compound.super = parser_parse_type(state);
        if (!componentNode->data.compound.super) {
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
    componentNode->data.compound.body = parser_parse_scope(state);
    return componentNode;
}

ASTNode *parser_parse_scope(ParserState *state) {
    ASTNode *scopeNode = create_node(AST_SCOPE);
    if (!scopeNode) return null; // Error handling within create_node
    
    ASTNode **current = &(scopeNode->data.scope.body);
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

//
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
        case TOKEN_IDENTIFIER:/*return parser_parse_identifier(state);*/
//        case TOKEN_EQUALS:return parser_parse_assignment(state, token);
            consume(state, TOKEN_IDENTIFIER);
            if (nextToken.type == TOKEN_EQUALS) { // Untyped property definition/assignment
                ASTNode *propertyDeclaration = create_node(AST_PROPERTY_DECLARE);
                if (!propertyDeclaration) return null; // Error handling within create_node
                propertyDeclaration->data.property.name = string_ndup(token.start, token.length);
                consume(state, TOKEN_EQUALS);
                propertyDeclaration->data.property.value = parser_parse_expression(state);
                return propertyDeclaration;
            } else if (nextToken.type == TOKEN_LBRACE) {
                // It's a nested, named component. Lets create an assignment node and a scope node
                ASTNode *assignmentNode = create_node(AST_PROPERTY_DECLARE);
                if (!assignmentNode) return null; // Error handling within create_node
                assignmentNode->data.property.name = string_ndup(token.start, token.length);
                consume(state, TOKEN_LBRACE);
                ASTNode *scope = parser_parse_scope(state);
                assignmentNode->data.property.value = scope;
                return assignmentNode;
            } else if (nextToken.type == TOKEN_COLON) {
                consume(state, TOKEN_COLON);
                if (match(state, TOKEN_EQUALS)) {
                    // delete the property node and create an  node
                    ASTNode *propertyNode = create_node(AST_PROPERTY_DECLARE);
                    if (!propertyNode) return null; // Error handling within create_node
                    propertyNode->data.property.name = string_ndup(token.start, token.length);
                    propertyNode->data.property.type = parser_parse_type(state);
                    propertyNode->data.property.value = parser_parse_expression(state);
                    return propertyNode;
                } else {
                    TypeSymbol *type = parser_parse_type(state);
                    if (match(state, TOKEN_LBRACE)) {
                        //This is a component type definition
                        ASTNode *componentNOde = create_node(AST_COMPONENT_DECLARE);
                        if (!componentNOde) return null; // Error handling within create_node
                        componentNOde->data.compound.name = string_ndup(token.start, token.length);
                        componentNOde->data.compound.super = type;
                        ASTNode *body = parser_parse_scope(state);
                        if (body == null) {
                            verror("Failed to parse component body");
                            kbye(componentNOde, sizeof(ASTNode));
                            return null;
                        }
                        componentNOde->data.compound.body = body;
                        // Sets the parent of the scope node
                        body->data.scope.parent = componentNOde;
                        return componentNOde;
                    } // If we're here, it's a sub component's property definition
                    else if (match(state, TOKEN_EQUALS)) {
                        // Deletes the previously allocated component node, after copying the name and super type
                        ASTNode *propertyNode = create_node(AST_PROPERTY_DECLARE);
                        if (!propertyNode) return null; // Error handling within create_node
                        propertyNode->data.property.name = string_ndup(token.start, token.length);
                        // Copys the actual value of type into the property node from the component so the components type pointer can be freed
                        propertyNode->data.property.type = type;
                        // Frees the component node
                        ASTNode *assignmentNode = create_node(AST_ASSIGNMENT);
                        if (!assignmentNode) return null; // Error handling within create_node
                        assignmentNode->data.assignment.assignee = propertyNode;
                        assignmentNode->data.assignment.assignment = parser_parse_expression(state);
                        propertyNode->data.property.value = assignmentNode;
                        return propertyNode;
                    } else {
                        // Just a typed property definition with no initial value
                        ASTNode *propertyNode = create_node(AST_PROPERTY_DECLARE);
                        if (!propertyNode) return null; // Error handling within create_node
                        propertyNode->data.property.name = string_ndup(token.start, token.length);
                        propertyNode->data.property.type = type;
                        return propertyNode;
                    }
                }
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
                // reference node
                ASTNode *referenceNode = create_node(AST_REFERENCE);
                if (!referenceNode) return null; // Error handling within create_node
                referenceNode->data.reference.name = string_ndup(token.start, token.length);
                return referenceNode;
            }
//    }
//        case TOKEN_LPAREN: {
//            consume(state, TOKEN_LPAREN);
//            ASTNode *subExpr = parser_parse_expression(state);
//            consume(state, TOKEN_RPAREN);
//            return subExpr;
//        }
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
            verror("Unexpected token in primary expression at line %d, column %d, with name %s", token.line,
                    token.column,
                    lexer_token_type_name(token.type));
            return NULL;
//            return parser_parse_scope(state);
    }
    
}

TypeSymbol *create_basic_type(Token token) {
    TypeSymbol *type = (TypeSymbol *) kgimme(sizeof(TypeSymbol));
    if (!type) {
        verror("Allocation failure for Type");
        return null;
    }
    type->kind = TYPE_BASIC;
    type->data.name = string_ndup(token.start, token.length);
    type->next = null;
    return type;
}

void add_type_to_tuple(TypeSymbol *tupleType, TypeSymbol *newType) {
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
        TypeSymbol *current = tupleType->data.tupleTypes;
        while (current->next) {
            current = current->next;
        }
        current->next = newType;
    }
}

TypeSymbol *create_tuple_type() {
    // Allocate memory for the tuple type itself
    TypeSymbol *tupleType = (TypeSymbol *) kgimme(sizeof(TypeSymbol));
    if (!tupleType) {
        verror("Allocation failure for Tuple Type");
        return NULL;
    }
    
    // Initialize the tuple type
    tupleType->kind = TYPE_TUPLE;
    tupleType->data.tupleTypes = NULL; // Initially, there are no types in the tuple
    
    return tupleType;
}


TypeSymbol *create_union_type(TypeSymbol *firstType, TypeSymbol *secondType) {
    TypeSymbol *type = (TypeSymbol *) kgimme(sizeof(TypeSymbol));
    if (!type) {
        verror("Allocation failure for Type");
        return null;
    }
    type->kind = TYPE_UNION;
    type->data.binary.lhs = firstType;
    type->data.binary.rhs = secondType;
    return type;
}

TypeSymbol *create_intersection_type(TypeSymbol *firstType, TypeSymbol *secondType) {
    TypeSymbol *type = (TypeSymbol *) kgimme(sizeof(TypeSymbol));
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

TypeSymbol *create_function_type(TypeSymbol *inputType, TypeSymbol *outputType) {
    TypeSymbol *type = (TypeSymbol *) kgimme(sizeof(TypeSymbol));
    if (!type) {
        verror("Allocation failure for Type");
        return null;
    }
    type->kind = TYPE_FUNCTION;
    type->data.binary.lhs = inputType;
    type->data.binary.rhs = outputType;
    return type;
}

TypeSymbol *create_array_type(TypeSymbol *elementType) {
    TypeSymbol *type = (TypeSymbol *) kgimme(sizeof(TypeSymbol));
    if (!type) {
        verror("Allocation failure for Type");
        return null;
    }
    type->kind = TYPE_ARRAY;
    type->data.array.elementType = elementType;
    return type;
}


TypeSymbol *parser_parse_single_type(ParserState *state) {
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
        TypeSymbol *elementType = parser_parse_type(state);
        expect(state, TOKEN_RBRACKET, "Expected ']' after array type");
        return create_array_type(elementType);
    } else {
//        verror("Unexpected token in type at line %d, column %d", peek(state).line, peek(state).column);
        return null;
    }
}

TypeSymbol *parser_parse_tuple_element(ParserState *state) {
    char *alias = null;
    if (peek(state).type == TOKEN_IDENTIFIER && peek_distance(state, 1).type == TOKEN_COLON) {
        Token aliasToken = consume(state, TOKEN_IDENTIFIER);
        consume(state, TOKEN_COLON); // consume the colon
        alias = string_ndup(aliasToken.start, aliasToken.length);
    }
    
    TypeSymbol *type = parser_parse_single_type(state);
    if (alias) {
        type->alias = alias; // Assign the alias to the type
    }
    
    return type;
}

TypeSymbol *parser_parse_tuple_type(ParserState *state) {
    expect(state, TOKEN_LPAREN, "Expected '(' to start a tuple");
    
    TypeSymbol *tupleType = create_tuple_type();
    do {
        TypeSymbol *elementType = parser_parse_tuple_element(state);
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
TypeSymbol *parser_parse_type(ParserState *state) {
    
    TypeSymbol *type = parser_parse_single_type(state); // Parse the initial type part
    
    while (true) {
        if (match(state, TOKEN_PIPE)) {
            type = create_union_type(type, parser_parse_single_type(state));
        } else if (match(state, TOKEN_AMPERSAND)) {
            type = create_intersection_type(type, parser_parse_single_type(state));
        } else if (match(state, TOKEN_ARROW)) {
            type = create_function_type(type,
                    parser_parse_single_type(state));
        } else {
            break; // No more type operators to parse
        }
    }
    
    return type;
}


ASTNode *parser_parse_reference(ParserState *state, Token identifier) {
    ASTNode *referenceNode = create_node(AST_REFERENCE);
    if (!referenceNode) return null; // Error handling within create_node
    referenceNode->data.reference.name = string_ndup(identifier.start, identifier.length);
    if (match(state, TOKEN_DOT)) {
        referenceNode->data.reference.reference = parser_parse_primary(state);
    }
    return referenceNode;
}

ASTNode *parser_parse_assignment(ParserState *state, Token identifier) {
    ASTNode *assignmentNode = create_node(AST_ASSIGNMENT);
    if (!assignmentNode) return null;
    ASTNode *lhs = parser_parse_reference(state, identifier);
    // Ensure we consume '='
    consume(state, TOKEN_EQUALS);
    // Parse the RHS as an expression
    ASTNode *rhs = parser_parse_expression(state);
    if (!rhs) {
        // Handle parsing error, clean up
        kbye(assignmentNode, sizeof(ASTNode));
        return null;
    }
    assignmentNode->data.assignment.assignee = lhs;
    assignmentNode->data.assignment.assignment = rhs;
    return assignmentNode;
}

ASTNode *parser_parse_binary_op(ParserState *state) {

}

ASTNode *parser_parse_identifier(ParserState *state) {
    Token token = consume(state, TOKEN_IDENTIFIER);
    if (match(state, TOKEN_DOT)) {
        // Parse as a reference
        return parser_parse_reference(state, token);
    } else if (match(state, TOKEN_EQUALS)) {
        // Parse as an assignment
        return parser_parse_assignment(state, token);
    } else if (match(state, TOKEN_LPAREN)) {
        // Parse as a function call
        return parser_parse_func_call(state, token);
    } else if (match(state, TOKEN_COLON)) {
        // It's a type definition
        char *name = string_ndup(token.start, token.length);
        TypeSymbol *type = parser_parse_type(state);
        // Could be an assignment or a component declaration
        ASTNode *reference = create_node(AST_REFERENCE);
        reference->data.reference.name = name;
        reference->data.reference.type = type;
        return reference;
    }
    // Default to treating the identifier as a variable reference
    return parser_parse_expression(state);
}


ASTNode *parser_parse_func_call(ParserState *state, Token identifier) {
    ASTNode *functionCallNode = create_node(AST_FUNCTION_CALL);
    if (!functionCallNode) return null; // Error handling within create_node
    
    functionCallNode->data.functionCall.name = string_ndup(identifier.start, identifier.length);
    
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


/// =============================================================================
// Memory cleanup functions
// =============================================================================

void parser_free_type(TypeSymbol *type) {
    while (type) {
        TypeSymbol *next = type->next; // Preserve next pointer before freeing
        
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
        
        kbye(type, sizeof(TypeSymbol));
        type = next; // Move to next type in the list (if any)
    }
}

void parser_free_component_node(CompoundDeclaration *component) {
    if (component->name) {
        kbye(component->name, strlen(component->name) + 1);
    }
    if (component->super) {
        parser_free_type(component->super);
    }
    parser_free_node(component->body);
}

void parser_free_property_node(PropertyDeclaration *variable) {
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

void parser_free_assignment_node(PropertyAssignmentNode *assignment) {
    parser_free_node(assignment->assignment);
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
    ASTNode *current = scope->body;
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
            case AST_COMPONENT_DECLARE:parser_free_component_node(&current->data.compound);
                break;
            case AST_PROPERTY_DECLARE:parser_free_property_node(&current->data.property);
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
    node->userData = null; // Ensure the user data pointer is initialized to null
    return node;
}
