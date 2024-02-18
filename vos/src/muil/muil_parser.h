//
// Created by jwraynor on 2/15/2024.
//
#pragma once

#include "defines.h"

// Includes necessary standard libraries
#include "muil_lexer.h"

// Forward declarations to resolve circular dependencies
typedef struct ASTNode ASTNode;


// Enum to represent different types of AST nodes
typedef enum {
    AST_COMPONENT,
    AST_PROPERTY, // Represents a property within a component
    AST_LITERAL,
    AST_ASSIGNMENT,
    AST_ARRAY,
    AST_SCOPE,
    AST_BINARY_OP,
    AST_REFERENCE,
    AST_FUNCTION_CALL,
} ASTNodeType;


// Enum for different types of literals
typedef enum {
    LITERAL_STRING,
    LITERAL_NUMBER,
    LITERAL_BOOLEAN,
} ASTLiteralType;

typedef struct ASTNode ASTNode; // Forward declaration for recursive structure


// A type represents a component or a property type
typedef enum {
    TYPE_BASIC, // Identifier, including primitives and user-defined types
    TYPE_ARRAY, // Array type
    TYPE_UNION, // Union type
    TYPE_INTERSECTION, // Intersection type
    TYPE_FUNCTION, // Function type
    TYPE_TUPLE, // Tuple type
} TypeKind;

typedef struct TypeAST {
    TypeKind kind;
    char *alias; // The alias name
    struct TypeAST *next; // For linked lists in unions, intersections, tuples
    union {
        char *name; // For basic types
        struct {
            struct TypeAST *elementType;
        } array;
        struct {
            struct TypeAST *lhs;
            struct TypeAST *rhs;
        } binary; // For union, intersection, function
        struct TypeAST *tupleTypes; // Pointer to the first type in the tuple
    } data;
} TypeAST;

// A scope is a collection of nodes.
typedef struct {
    struct ASTNode *parent; // Parent scope
    ASTNode *body; // Nodes in the scope
} ScopeNode;

// Separate data structures for each node type
typedef struct {
    char *name; // Component name, null for instances
    TypeAST *super; // The super component types that this component extends. Can be null.
    ASTNode *body; // A body will typically be a scope node
    b8 topLevel; // Indicates if this is a top-level component
} ComponentNode;

typedef struct {
    char *name; // Property name
    TypeAST *type; // Type of the component
    ASTNode *value; // Can be a type declaration, expression, or array
} PropertyNode;

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

typedef struct {
    char *name; // The name of the reference
    TypeAST *type; // The type of the reference. Can be null if the type is not known/inferred from reference
    ASTNode *reference; // The reference node
} ReferenceNode;

typedef struct {
    char *name; // The name of the function
    ASTNode *reference; // The reference to the function
    ASTNode *arguments; // The arguments to the function
} FunctionCallNode;

struct ASTNode {
    ASTNodeType nodeType;
    union {
        ComponentNode component;
        PropertyNode property;
        LiteralNode literal;
        AssignmentNode assignment;
        ArrayNode array;
        ScopeNode scope;
        BinaryOpNode binaryOp;
        ReferenceNode reference;
        FunctionCallNode functionCall;
        TypeAST type;
    } data;
    ASTNode *next; // Next sibling in the AST
};

// Type alias specifics
typedef struct {
    char *alias; // The alias name
    TypeAST *actualType; // The actual type definition
} TypeAliasNode;


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


ASTNode *parser_get_node(void *node);

