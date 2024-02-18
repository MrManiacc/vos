/**
 * Created by jraynor on 2/17/2024.
 */
#pragma once

#include "muil/muil_parser.h"

typedef enum VisitorTypeMask {
    VISITOR_TYPE_MASK_NONE = 0,
    VISITOR_TYPE_MASK_PROGRAM = 1 << 0,
    VISITOR_TYPE_MASK_COMPONENT = 1 << 1,
    VISITOR_TYPE_MASK_PROPERTY = 1 << 2,
    VISITOR_TYPE_MASK_LITERAL = 1 << 3,
    VISITOR_TYPE_MASK_ASSIGNMENT = 1 << 4,
    VISITOR_TYPE_MASK_ARRAY = 1 << 5,
    VISITOR_TYPE_MASK_SCOPE = 1 << 6,
    VISITOR_TYPE_MASK_BINARY_OP = 1 << 7,
    VISITOR_TYPE_MASK_REFERENCE = 1 << 8,
    VISITOR_TYPE_MASK_FUNCTION_CALL = 1 << 9,
    VISITOR_TYPE_MASK_TYPE = 1 << 10,
    VISITOR_TYPE_MASK_ALL = 0x7FF
} VisitorTypeMask;

/**
 * @brief The A symbol table
 */
typedef struct ASTVisitor {
    
    
    void (*enterProgramNode)(struct ASTVisitor *self, ProgramAST *node);
    
    void (*exitProgramNode)(struct ASTVisitor *self, ProgramAST *node);
    
    void (*enterComponentNode)(struct ASTVisitor *self, ComponentNode *node);
    
    void (*exitComponentNode)(struct ASTVisitor *self, ComponentNode *node);
    
    void (*enterPropertyNode)(struct ASTVisitor *self, PropertyNode *node);
    
    void (*exitPropertyNode)(struct ASTVisitor *self, PropertyNode *node);
    
    void (*enterLiteralNode)(struct ASTVisitor *self, LiteralNode *node);
    
    void (*exitLiteralNode)(struct ASTVisitor *self, LiteralNode *node);
    
    void (*enterAssignmentNode)(struct ASTVisitor *self, AssignmentNode *node);
    
    void (*exitAssignmentNode)(struct ASTVisitor *self, AssignmentNode *node);
    
    void (*enterArrayNode)(struct ASTVisitor *self, ArrayNode *node);
    
    void (*exitArrayNode)(struct ASTVisitor *self, ArrayNode *node);
    
    void (*enterScopeNode)(struct ASTVisitor *self, ScopeNode *node);
    
    void (*exitScopeNode)(struct ASTVisitor *self, ScopeNode *node);
    
    void (*enterBinaryOpNode)(struct ASTVisitor *self, BinaryOpNode *node);
    
    void (*exitBinaryOpNode)(struct ASTVisitor *self, BinaryOpNode *node);
    
    void (*enterReferenceNode)(struct ASTVisitor *self, ReferenceNode *node);
    
    void (*exitReferenceNode)(struct ASTVisitor *self, ReferenceNode *node);
    
    void (*enterFunctionCallNode)(struct ASTVisitor *self, FunctionCallNode *node);
    
    void (*exitFunctionCallNode)(struct ASTVisitor *self, FunctionCallNode *node);
    
    void (*enterType)(struct ASTVisitor *self, TypeAST *type);
    
    void (*exitType)(struct ASTVisitor *self, TypeAST *type);
    
    //because msvc sets it to 0xcccccccccc when it's copied into the darray
    i32 type_mask;
} ASTVisitor;

/**
 * @brief Visits the nodes of the given ProgramAST using the provided ASTVisitor.
 *
 * This function traverses the AST nodes in a depth-first manner, calling the appropriate
 * enter and exit functions on the ASTVisitor for each node type.
 *
 * @param program The ProgramAST to visit.
 * @param visitor The ASTVisitor to use for visiting the nodes.
 */
VAPI void muil_visit(ASTVisitor *visitor, ProgramAST *program);

VAPI void muil_set_visitor(ASTVisitor *visitor, VisitorTypeMask mask, void *enter, void *exit);

VAPI void *muil_get_visitor_enter(ASTVisitor *visitor, VisitorTypeMask mask);

VAPI void *muil_get_visitor_exit(ASTVisitor *visitor, VisitorTypeMask mask);

VAPI b8 muil_has_visitor(ASTVisitor *visitor, VisitorTypeMask mask);

/**
 * @brief Visits the given ASTNode using the provided ASTVisitor.
 *
 * This function visits the given ASTNode using the provided ASTVisitor.
 * It checks the node's type and calls the corresponding visit function in the visitor.
 * If the node is null, an error is logged and the function returns.
 *
 * @param visitor A pointer to the ASTVisitor instance.
 * @param node A pointer to the ASTNode to be visited.
 */
VAPI void muil_visit_node(ASTVisitor *visitor, ASTNode *node);

// A special visitor for the type, because it's not an ASTNode
VAPI void muil_visit_type(ASTVisitor *visitor, TypeAST *type);
