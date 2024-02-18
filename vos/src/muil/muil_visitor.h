/**
 * Created by jraynor on 2/17/2024.
 */
#pragma once

#include "muil/muil_parser.h"

typedef enum SemanticsPassMask {
    SEMANTICS_MASK_NONE = 0,
    SEMANTICS_MASK_PROGRAM = 1 << 0,
    SEMANTICS_MASK_COMPONENT = 1 << 1,
    SEMANTICS_MASK_PROPERTY = 1 << 2,
    SEMANTICS_MASK_LITERAL = 1 << 3,
    SEMANTICS_MASK_ASSIGNMENT = 1 << 4,
    SEMANTICS_MASK_ARRAY = 1 << 5,
    SEMANTICS_MASK_SCOPE = 1 << 6,
    SEMANTICS_MASK_BINARY_OP = 1 << 7,
    SEMANTICS_MASK_REFERENCE = 1 << 8,
    SEMANTICS_MASK_FUNCTION_CALL = 1 << 9,
    SEMANTICS_MASK_TYPE = 1 << 10,
    SEMANTICS_MASK_ALL = 0x7FF
} SemanticsPassMask;

/**
 * @brief The A symbol table
 */
typedef struct SemanticsPass {
    
    
    void (*enterProgramNode)(struct SemanticsPass *self, ProgramAST *node);
    
    void (*exitProgramNode)(struct SemanticsPass *self, ProgramAST *node);
    
    void (*enterComponentNode)(struct SemanticsPass *self, ComponentNode *node);
    
    void (*exitComponentNode)(struct SemanticsPass *self, ComponentNode *node);
    
    void (*enterPropertyNode)(struct SemanticsPass *self, PropertyNode *node);
    
    void (*exitPropertyNode)(struct SemanticsPass *self, PropertyNode *node);
    
    void (*enterLiteralNode)(struct SemanticsPass *self, LiteralNode *node);
    
    void (*exitLiteralNode)(struct SemanticsPass *self, LiteralNode *node);
    
    void (*enterAssignmentNode)(struct SemanticsPass *self, AssignmentNode *node);
    
    void (*exitAssignmentNode)(struct SemanticsPass *self, AssignmentNode *node);
    
    void (*enterArrayNode)(struct SemanticsPass *self, ArrayNode *node);
    
    void (*exitArrayNode)(struct SemanticsPass *self, ArrayNode *node);
    
    void (*enterScopeNode)(struct SemanticsPass *self, ScopeNode *node);
    
    void (*exitScopeNode)(struct SemanticsPass *self, ScopeNode *node);
    
    void (*enterBinaryOpNode)(struct SemanticsPass *self, BinaryOpNode *node);
    
    void (*exitBinaryOpNode)(struct SemanticsPass *self, BinaryOpNode *node);
    
    void (*enterReferenceNode)(struct SemanticsPass *self, ReferenceNode *node);
    
    void (*exitReferenceNode)(struct SemanticsPass *self, ReferenceNode *node);
    
    void (*enterFunctionCallNode)(struct SemanticsPass *self, FunctionCallNode *node);
    
    void (*exitFunctionCallNode)(struct SemanticsPass *self, FunctionCallNode *node);
    
    void (*enterType)(struct SemanticsPass *self, TypeAST *type);
    
    void (*exitType)(struct SemanticsPass *self, TypeAST *type);
    
    //because msvc sets it to 0xcccccccccc when it's copied into the darray
    i32 type_mask;
} SemanticsPass;

/**
 * @brief Visits the nodes of the given ProgramAST using the provided ASTVisitor.
 *
 * This function traverses the AST nodes in a depth-first manner, calling the appropriate
 * enter and exit functions on the ASTVisitor for each node type.
 *
 * @param program The ProgramAST to visit.
 * @param visitor The ASTVisitor to use for visiting the nodes.
 */
VAPI void muil_visit(SemanticsPass *visitor, ProgramAST *program);

VAPI void muil_set_visitor(SemanticsPass *visitor, SemanticsPassMask mask, void *enter, void *exit);

VAPI void *muil_get_visitor_enter(SemanticsPass *visitor, SemanticsPassMask mask);

VAPI void *muil_get_visitor_exit(SemanticsPass *visitor, SemanticsPassMask mask);

VAPI b8 muil_has_visitor(SemanticsPass *visitor, SemanticsPassMask mask);


#define define_pass(pass_name)\
    typedef struct pass_name pass_name;\
    VAPI pass_name *new_##pass_name(); \
    VAPI void delete_##pass_name(pass_name *pass)

//Creates a new pass with the given name, and sets the type mask
// struct TypePass {
//     // Required to be first, so we can cast to it
//    SemanticsPass base;
//}
#define implement_pass(pass_name, fields, body) \
     struct pass_name { \
        SemanticsPass base; \
        fields; \
    } ; \
    VAPI pass_name *new_##pass_name() { \
        pass_name *pass = vnew(pass_name); \
        const b8 free = false;                                         \
        if (pass) body \
        return pass; \
    }                                        \
                                             \
                                             \
    VAPI void delete_##pass_name(pass_name *pass) { \
        const b8 free = true;                                     \
        if (pass) body                       \
vdelete(pass);                                                \
    }                                           \

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

VAPI void muil_visit_node(SemanticsPass *visitor, ASTNode *node);

// A special visitor for the type, because it's not an ASTNode
VAPI void muil_visit_type(SemanticsPass *visitor, TypeAST *type);

