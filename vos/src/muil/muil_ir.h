/**
 * Created by jraynor on 2/17/2024.
 */
#pragma once

#include "muil_parser.h"

typedef struct IRVisitor {
    /**
     * @brief Function pointer to visit a ComponentNode.
     *
     * This function pointer is used in the IRVisitor struct to specify the method to visit a ComponentNode.
     *
     * @param self A pointer to the IRVisitor instance.
     * @param node A pointer to the ComponentNode to be visited.
     */
    void (*visitComponentNode)(struct IRVisitor *self, ComponentNode *node);
    
    /**
     * @brief Function pointer to visit a VariableNode.
     *
     * This function pointer is used in the IRVisitor struct to define a visit function for VariableNode.
     * The visit function takes an IRVisitor object and a VariableNode object as parameters.
     *
     * @param self Pointer to the IRVisitor object.
     * @param node Pointer to the VariableNode object.
     */
    void (*visitPropertyNode)(struct IRVisitor *self, PropertyNode *node);
    
    /**
     * @brief Function pointer to visit a LiteralNode in the IRVisitor.
     *
     * This function pointer is used to visit a LiteralNode in the IRVisitor.
     * It takes a pointer to the IRVisitor object and a pointer to the LiteralNode object as arguments.
     * The purpose of this function is to allow different behaviors when visiting different types of nodes in the IRVisitor.
     *
     * @param self Pointer to the IRVisitor object.
     * @param node Pointer to the LiteralNode object.
     */
    void (*visitLiteralNode)(struct IRVisitor *self, LiteralNode *node);
    
    /**
     * @brief Function pointer to visitAssignmentNode.
     *
     * This function pointer is a member of the IRVisitor struct. It is used to visit an AssignmentNode in the IR structure.
     *
     * @param self A pointer to the IRVisitor struct.
     * @param node A pointer to the AssignmentNode being visited.
     *
     */
    void (*visitAssignmentNode)(struct IRVisitor *self, AssignmentNode *node);
    
    /**
     * @brief Function pointer to visit an ArrayNode.
     *
     * This function pointer is a member of the IRVisitor struct. It represents a function
     * that visits an ArrayNode object.
     *
     * @param self A pointer to the IRVisitor struct.
     * @param node A pointer to the ArrayNode object to be visited.
     * @return void
     */
    void (*visitArrayNode)(struct IRVisitor *self, ArrayNode *node);
    
    /**
    *
    */
    void (*visitScopeNode)(struct IRVisitor *self, ScopeNode *node);
    
    /**
     * @brief Function pointer to visit a BinaryOpNode in the IRVisitor.
     *
     * This function pointer is used to visit a BinaryOpNode in the IRVisitor.
     * It takes a pointer to the IRVisitor object and a pointer to the BinaryOpNode object as arguments.
     * The purpose of this function is to allow different behaviors when visiting different types of nodes in the IRVisitor.
     *
     * @param self Pointer to the IRVisitor object.
     * @param node Pointer to the BinaryOpNode object.
     */
    void (*visitBinaryOpNode)(struct IRVisitor *self, BinaryOpNode *node);
    
    
    /**
     * @brief Function pointer to visit a Type node in the IRVisitor.
     *
     * This function pointer is used in the IRVisitor struct to define a visit function for Type nodes.
     * The visit function takes a pointer to the IRVisitor object and a pointer to the Type object as parameters.
     * It allows for different behaviors when visiting different types of nodes in the IRVisitor.
     *
     * @param self Pointer to the IRVisitor object.
     * @param type Pointer to the Type object.
     */
    void (*visitType)(struct IRVisitor *self, Type *type);
    
    /**
     * @brief Function pointer to visit the ProgramAST node in the AST.
     *
     * This function pointer is used by the IRVisitor class to visit the ProgramAST node.
     * It takes in a pointer to an IRVisitor object and a pointer to a ProgramAST object as arguments.
     *
     * @param self A pointer to the IRVisitor object.
     * @param node A pointer to the ProgramAST node to be visited.
     */
    void (*visitProgramAST)(struct IRVisitor *self, ProgramAST *node);
    
} IRVisitor;


/**
 * @brief Visits a component node in the intermediate representation (IR).
 *
 * This function is used to visit a component node in the IR structure.
 *
 * @param visitor A pointer to the IRVisitor object.
 * @param node    A pointer to the ComponentNode to be visited.
 */
VAPI void ir_visit_component_node(IRVisitor *visitor, ComponentNode *node);

/**
 * @brief Visit the ScopeNode in the Intermediate Representation (IR) using the given visitor.
 *
 * This function is responsible for visiting a ScopeNode in the Intermediate Representation (IR)
 * using the provided visitor. The visitor can perform different actions on the visited ScopeNode,
 * such as analyzing, transforming, or gathering information.
 *
 * @param visitor A pointer to the IRVisitor object that will visit the ScopeNode.
 * @param node A pointer to the ScopeNode that needs to be visited.
 */
VAPI void ir_visit_scope_node(IRVisitor *visitor, ScopeNode *node);

/**
 * @brief Visits a VariableNode in the IR tree.
 *
 * This function is called by the IRVisitor to visit a VariableNode in the IR tree.
 * It receives a pointer to the IRVisitor object and a pointer to the VariableNode to be visited.
 *
 * @param visitor A pointer to the IRVisitor object.
 * @param node A pointer to the VariableNode to be visited.
 */
VAPI void ir_visit_property_node(IRVisitor *visitor, PropertyNode *node);

/**
 * @brief Visits a literal node in an intermediate representation (IR) and performs an operation.
 *
 * @param visitor The IR visitor object.
 * @param node The literal node to be visited.
 */
VAPI void ir_visit_literal_node(IRVisitor *visitor, LiteralNode *node);

/**
 * @brief Visits an assignment node.
 *
 * This function is used by the IRVisitor to visit an assignment node.
 *
 * @param visitor The IRVisitor instance.
 * @param node The AssignmentNode to be visited.
 */
VAPI void ir_visit_assignment_node(IRVisitor *visitor, AssignmentNode *node);

/**
 * @brief Visits an array node in an intermediate representation (IR).
 *
 * This function is used by an IRVisitor to visit an ArrayNode in the IR. It allows
 * the visitor to perform operations and/or extract information from the array node.
 *
 * @param visitor A pointer to the IRVisitor object visiting the array node.
 * @param node A pointer to the ArrayNode being visited.
 *
 * @see IRVisitor
 * @see ArrayNode
 */
VAPI void ir_visit_array_node(IRVisitor *visitor, ArrayNode *node);

/**
 * @brief Visits a binary operation node in the intermediate representation (IR).
 *
 * This function is responsible for visiting a binary operation node in the IR.
 * It allows an IR visitor to perform specific operations when encountering a binary operation node.
 *
 * @param visitor The IR visitor object responsible for visiting the node.
 * @param node The binary operation node to visit.
 */
VAPI void ir_visit_binary_op_node(IRVisitor *visitor, BinaryOpNode *node);

/**
 * @brief Visits a given type.
 *
 * This function is responsible for visiting a given type using a provided IRVisitor object.
 *
 * @param visitor A pointer to the IRVisitor object to use for visiting the type.
 * @param type A pointer to the Type object to be visited.
 */
VAPI void ir_visit_type(IRVisitor *visitor, Type *type);

/**
 * \brief Visits the ProgramAST node.
 *
 * This function is responsible for visiting the ProgramAST node in the
 * Intermediate Representation (IR). It takes an IRVisitor object and
 * a ProgramAST object as parameters and performs the necessary actions
 * needed to visit this node.
 *
 * \param visitor The IRVisitor object used for visiting the AST nodes.
 * \param node The ProgramAST node to be visited.
 *
 * \sa IRVisitor
 * \sa ProgramAST
 */
VAPI void ir_visit_tree(IRVisitor *visitor, ProgramAST *node);

