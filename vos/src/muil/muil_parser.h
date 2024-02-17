//
// Created by jwraynor on 2/15/2024.
//
#pragma once

#include "defines.h"

// Includes necessary standard libraries
#include "muil_lexer.h"

// Forward declarations to resolve circular dependencies
typedef struct ASTNode ASTNode;
typedef struct ProgramSource ProgramSource;


// Enum to represent different types of AST nodes
typedef enum {
    AST_COMPONENT,
    AST_PROPERTY, // Represents a property within a component
    AST_LITERAL,
    AST_ASSIGNMENT,
    AST_EXPRESSION, // General expressions
    AST_ARRAY, // Represents an array of types or values
    AST_SCOPE, // Represents a scope of nodes
    AST_BINARY_OP
} ASTNodeType;

// Enum for different types of literals
typedef enum {
    LITERAL_STRING,
    LITERAL_NUMBER,
    LITERAL_BOOLEAN,
} ASTLiteralType;

typedef struct ASTNode ASTNode; // Forward declaration for recursive structure


typedef struct Type {
    char *name; // Component name, NULL for instances
    u8 isComposite; // Indicates if the component is a composite component
    u8 isArray;
    struct Type *next; // If this is a composite component, this points to the next component in the list
} Type;

// A scope is a collection of nodes.
typedef struct ScopeNode {
    ASTNode *nodes; // Nodes in the scope
    struct ScopeNode *parent; // Parent scope
} ScopeNode;

// Separate data structures for each node type
typedef struct {
    char *name; // Component name, null for instances
    Type *extends; // The super component types that this component extends. Can be null.
    ASTNode *body; // A body will typically be a scope node
    b8 topLevel; // Indicates if this is a top-level component
} ComponentNode;

typedef struct {
    char *name; // Property name
    Type *type; // Type of the component
    ASTNode *value; // Can be a type declaration, expression, or array
} VariableNode;

typedef struct {
    ASTLiteralType type;
    union {
        char *stringValue;
        double numberValue;
        int booleanValue;
    } value;
} LiteralNode;

typedef struct {
    char *variableName;
    ASTNode *value; // Assigned value, can be any expression including arrays
} AssignmentNode;

typedef struct {
    ASTNode *elements; // Elements of the array, can be types or values
} ArrayNode;

typedef struct {
    ASTNode *left;
    ASTNode *right;
    TokenType operator;
} BinaryOpNode;

struct ASTNode {
    ASTNodeType nodeType;
    union {
        ComponentNode component;
        VariableNode variable;
        LiteralNode literal;
        AssignmentNode assignment;
        ArrayNode array;
        ScopeNode scope;
        BinaryOpNode binaryOp;
    } data;
    ASTNode *next; // Next sibling in the AST
};


// Root of the AST for a parsed program
typedef struct ProgramAST {
    ASTNode *root; // Root node of the AST
} ProgramAST;


/**
 * @brief Parse the program AST from the given source code.
 *
 * This function takes a ProgramSource object and parses it to generate the ProgramAST (Abstract Syntax Tree).
 * The ProgramAST represents the structure and semantics of the source code in a hierarchical form.
 *
 * @param source The ProgramSource object containing the source code to parse.
 *
 * @return The parsed ProgramAST.
 */
ProgramAST parser_parse(ProgramSource *source);


/**
 * @brief Frees memory allocated for an ASTNode object.
 *
 * This function frees the memory allocated for an ASTNode object and its associated data.
 *
 * @param node The ASTNode object to be freed.
 * @return true if the memory was successfully freed, false otherwise.
 */
b8 parser_free_node(ASTNode *node);

/**
 * @brief Frees the memory allocated for a ProgramAST structure.
 *
 * This function frees the memory allocated for a ProgramAST structure and
 * all of its associated AST nodes. If the program argument is NULL, no
 * action is taken.
 *
 * @param program The ProgramAST structure to free
 * @return Returns true if the memory was successfully freed, or false
 *         if the program argument is NULL
 */
b8 parser_free_program(ProgramAST *program);