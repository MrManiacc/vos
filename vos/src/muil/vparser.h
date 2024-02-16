//
// Created by jwraynor on 2/15/2024.
//
#pragma once

#include "defines.h"

// Includes necessary standard libraries
#include "vlexer.h"

// Forward declarations to resolve circular dependencies
typedef struct ASTNode ASTNode;

// Basic AST node types
typedef enum {
    AST_COMPONENT,
    AST_EXPRESSION_STATEMENT,
    AST_ASSIGNMENT_STATEMENT,
    AST_FUNCTION_DECLARATION,
    AST_ARRAY_LITERAL,
    AST_LITERAL,
    AST_PROPERTY_DECLARATION,
    AST_TYPE_DECLARATION,
} ASTNodeType;


// Expression and Statement types for finer control
typedef enum {
    EXP_LITERAL,
    EXP_FUNCTION_CALL,
    EXP_VARIABLE_REFERENCE,
    EXP_ARRAY_ACCESS,
    EXP_BINARY_OPERATION, // e.g., addition, subtraction
    // Add more expression types as needed
} ASTExpressionType;

// Literal types for expressions
typedef enum {
    LITERAL_STRING,
    LITERAL_NUMBER,
    LITERAL_BOOLEAN,
    LITERAL_LUA, // Embedding Lua code directly
} ASTLiteralType;

// Type representation
typedef struct Type {
    char *name; // The type name
    b8 isArray; // Indicates if the type is an array
    b8 isComposite; // Indicates if the type is a composite type (union)
    struct Type *next; // Next type in a composite type chain
} Type;


// Property in a component or a type
typedef struct Property {
    char *name;
    Type *type;
    b8 isOptional;
    struct Property *next;
} Property;

typedef struct SymbolTableEntry {
    char *identifier;
    ASTNode *node; // Points to the variable declaration, function, or assignment
    struct SymbolTableEntry *next;
} SymbolTableEntry;

typedef struct SymbolTable {
    SymbolTableEntry *entries; // Linked list of symbol table entries
    struct SymbolTable *parent; // Optional: For nested scopes
} SymbolTable;

// Component or type declaration
typedef struct Component {
    char *name;
    Property *properties; // Existing property list
    SymbolTable *symbolTable; // Each component has its own symbol table
    ASTNode *assignments; // Linked list of assignments
    Type *extends;
    struct Component *next;
} Component;

// Assignment statement
typedef struct Assignment {
    char *variableName;
    ASTNode *expression; // The value or expression assigned
} Assignment;

// Function declaration
typedef struct FunctionDeclaration {
    char *name;
    ASTNode *body; // Function body (could be a block of statements)
} FunctionDeclaration;

typedef struct ComponentInstance {
    Type *type; // Type of the component instance, if needed
    ASTNode *properties; // Linked list of properties or assignments
    // Additional fields as needed for instantiation specifics
} ComponentInstance;

// Literal value expression
typedef struct Literal {
    ASTLiteralType type;
    union {
        char *stringValue;
        double numberValue;
        b8 booleanValue;
        char *luaCode; // For embedding Lua code
    } value;
} Literal;

// Array literal expression
typedef struct ArrayLiteral {
    ASTNode *elements; // Linked list of elements in the array
} ArrayLiteral;

// Generic AST node
struct ASTNode {
    ASTNodeType nodeType;
    union {
        Component component;
        ComponentInstance componentInstance; // New: For component instances
        Property property;
        Type type;
        Assignment assignment;
        FunctionDeclaration functionDeclaration;
        Literal literal;
        ArrayLiteral arrayLiteral; // Added for array literals
        struct {
            ASTNode *left;
            ASTNode *right;
        } binaryOperation; // For binary operations
        // Add more as needed
    } data;
    ASTNode *next; // For linking siblings or statements in a block
};

// Program representation
typedef struct ProgramAST {
    ASTNode *statements; // Root of the AST
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
 * @brief Generates a string representation of a ProgramAST.
 *
 * This function takes a ProgramAST pointer as input and returns a dynamically allocated
 * string representing the structure and content of the ProgramAST. The string is generated
 * in a format suitable for debugging or display purposes.
 *
 * @param result    A pointer to the ProgramAST structure to be converted to a string.
 * @return          A pointer to a dynamically allocated string representation of the ProgramAST.
 *                  The caller is responsible for freeing the memory when no longer needed.
 */
char *parser_dump_program(ProgramAST *result);
