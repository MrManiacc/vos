//
// Created by jwraynor on 2/15/2024.
//
#pragma once

#include "defines.h"

// Includes necessary standard libraries
#include "vlexer.h"

typedef struct ProgramAST {
    // The list of statements in the program.
    struct ASTNode *statements;
} ProgramAST;

// Basic AST node types
typedef enum {
    AST_COMPONENT,
    AST_PROPERTY,
    AST_TYPE,
    AST_ARRAY,
    AST_ELEMENT,
    AST_LITERAL,
    AST_FUNCTION,
    AST_ASSIGNMENT
    // Add more as needed
} ASTNodeType;

// A type can be a composite type, i.e., a union of other types.
// For example, a type can be an Int | Float | String
// This is represented as a linked list of types
typedef struct Type {
    char *name; // The type name. This should be a unique identifier
    b8 isArray; // Whether the type is an array
    b8 isComposite; // Whether the type is a composite type
    struct Type *next; // If this is a composite type, the next type in the list. I.e., Complex: Int | Float | String
} Type;

// A property is a field in a component
// For example, a component can have a property called "name" of type "String"
// Properties are always typed and can be optional
typedef struct Property {
    char *name; // The property name
    Type *type; // The type of the property
    b8 isOptional; // Whether the property is optional
    struct Property *next; // The next property in the list
} Property;

// A component is a collection of properties
typedef struct Component {
    char *name; // The component name
    Property *properties; // The list of properties
    Type *extends; // The list of super types. I.e., Complex : Base | Derived | Another
    struct Component *next; // The next component in the list
} Component;

typedef struct Assignment {
    char *identifier;
    struct ASTNode *value; // RHS can be a literal, another component, or a Lua function
} Assignment;

// AST node for a component or a type
typedef struct ASTNode {
    ASTNodeType type;
    char *name; // For components, types, properties
    union {
        Component component;
        Type type;
        Property property;
        Assignment assignment;
        struct ASTNode *children; // For arrays of elements or function bodies
        char *literalValue; // For literals like strings, numbers, or Lua code
        
    } data;
    struct ASTNode *next; // For linking siblings
} ASTNode;


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
