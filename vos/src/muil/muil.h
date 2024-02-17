/**
 * Created by jraynor on 2/15/2024.
 */
#pragma once

#include "muil_parser.h"
#include "muil_dump.h"

//
///**
// * @struct Visitor
// *
// * @brief The Visitor struct represents a visitor object that can visit different types of AST nodes.
// *
// * The Visitor struct contains function pointers for visiting different types of AST nodes. Each function
// * pointer takes an ASTNode pointer and a visitor pointer as parameters.
// */
//typedef struct Visitor {
//    // Function pointer to
//    void (*visit_component)(ASTNode *node, struct Visitor *visitor);
//
//    void (*visit_property)(ASTNode *node, struct Visitor *visitor);
//
//    void (*visit_type)(ASTNode *node, struct Visitor *visitor);
//} Visitor;
//
///**
// * @brief Traverses the Abstract Syntax Tree (AST) starting from the given node and applies
// *        the specified visitor object on each visited node.
// *
// * @param node The root node of the AST.
// * @param visitor The visitor object that will be applied on each visited node.
// */
//VAPI void muil_traverse_ast(ASTNode *node, Visitor *visitor);
//
//
///**
// * @file
// *
// * @brief This file contains the definition and documentation for the function `muil_traverse`.
// *
// * The `muil_traverse` function is used to traverse the specified `ProgramAST` object and call the appropriate
// * visit functions on the provided `Visitor` object.
// */
//VAPI void muil_traverse(ProgramAST *program, Visitor *visitor);
//
///**
// * @brief Creates a new visitor object with the specified function pointers for visiting different types of AST nodes.
// *
// * @param visitComponent Function pointer for visiting component nodes.
// * @param visitProperty Function pointer for visiting property nodes.
// * @param visitType Function pointer for visiting type nodes.
// * @return A new visitor object.
// */
//VAPI Visitor muil_create_visitor(void (*visitComponent)(ASTNode *node, Visitor *visitor),
//                            void (*visitProperty)(ASTNode *node, Visitor *visitor),
//                            void (*visitType)(ASTNode *node, Visitor *visitor));
//
//

