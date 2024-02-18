/**
 * Created by jraynor on 2/17/2024.
 */
#include "muil_visitor.h"
#include "core/vlogger.h"
#include "muil/muil_dump.h"


/**
 * @brief Visits a component node in the intermediate representation (IR).
 *
 * This function is used to visit a component node in the IR structure.
 *
 * @param visitor A pointer to the IRVisitor object.
 * @param node    A pointer to the ComponentNode to be visited.
 */
void muil_visit_component_node(SemanticsPass *visitor, ComponentNode *node);

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
void muil_visit_scope_node(SemanticsPass *visitor, ScopeNode *node);

/**
 * @brief Visits a VariableNode in the IR tree.
 *
 * This function is called by the IRVisitor to visit a VariableNode in the IR tree.
 * It receives a pointer to the IRVisitor object and a pointer to the VariableNode to be visited.
 *
 * @param visitor A pointer to the IRVisitor object.
 * @param node A pointer to the VariableNode to be visited.
 */
void muil_visit_property_node(SemanticsPass *visitor, PropertyNode *node);

/**
 * @brief Visits a literal node in an intermediate representation (IR) and performs an operation.
 *
 * @param visitor The IR visitor object.
 * @param node The literal node to be visited.
 */
void muil_visit_literal_node(SemanticsPass *visitor, LiteralNode *node);

/**
 * @brief Visits an assignment node.
 *
 * This function is used by the IRVisitor to visit an assignment node.
 *
 * @param visitor The IRVisitor instance.
 * @param node The AssignmentNode to be visited.
 */
void muil_visit_assignment_node(SemanticsPass *visitor, AssignmentNode *node);

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
void muil_visit_array_node(SemanticsPass *visitor, ArrayNode *node);

/**
 * @brief Visits a binary operation node in the intermediate representation (IR).
 *
 * This function is responsible for visiting a binary operation node in the IR.
 * It allows an IR visitor to perform specific operations when encountering a binary operation node.
 *
 * @param visitor The IR visitor object responsible for visiting the node.
 * @param node The binary operation node to visit.
 */
void muil_visit_binary_op_node(SemanticsPass *visitor, BinaryOpNode *node);

/**
 * @brief Visit a ReferenceNode and process it using the given visitor.
 *
 * This function is responsible for visiting a ReferenceNode and processing
 * it using the provided visitor. The visitor is an instance of the ASTVisitor
 * class and is used to perform actions on the node.
 *
 * @param visitor The ASTVisitor instance responsible for processing the node.
 * @param node The ReferenceNode to be visited and processed.
 */
void muil_visit_reference_node(SemanticsPass *visitor, ReferenceNode *node);

/**
 * @brief Visits a FunctionCallNode in the AST.
 *
 * This function is used to visit a FunctionCallNode in the AST. It takes an ASTVisitor object and a FunctionCallNode object
 * as parameters. It is responsible for performing any necessary operations when visiting a FunctionCallNode.
 *
 * @param visitor A pointer to the ASTVisitor object.
 * @param node A pointer to the FunctionCallNode object to be visited.
 * @return void
 */
void muil_visit_function_call_node(SemanticsPass *visitor, FunctionCallNode *node);


/**
 * @brief Visits a given type.
 *
 * This function is responsible for visiting a given type using a provided IRVisitor object.
 *
 * @param visitor A pointer to the IRVisitor object to use for visiting the type.
 * @param type A pointer to the Type object to be visited.
 */
void muil_visit_type(SemanticsPass *visitor, TypeAST *type);

//Declared here because it assumes the visitor has been set
#define has(mask) muil_has_visitor(visitor, mask)


void muil_visit_component_node(SemanticsPass *visitor, ComponentNode *node) {
//    vinfo("Visiting component: %s", node->name)
    if (!node) {
        verror("ComponentNode is null")
        return;
    }
    if (node->super) {
        muil_visit_type(visitor, node->super);
    }
    if (has(SEMANTICS_MASK_COMPONENT)) {
        visitor->enterComponentNode(visitor, node);
    }
    
    if (node->body) {
        muil_visit_node(visitor, node->body);
    }
    
    if (has(SEMANTICS_MASK_COMPONENT)) {
        visitor->exitComponentNode(visitor, node);
    }
}

void muil_visit_scope_node(SemanticsPass *visitor, ScopeNode *node) {
//    vinfo("Visiting scope")
    if (!node) {
        verror("ScopeNode is null")
        return;
    }
    if (has(SEMANTICS_MASK_SCOPE)) {
        visitor->enterScopeNode(visitor, node);
    }
    for (ASTNode *stmt = node->body; stmt != null; stmt = stmt->next) {
        muil_visit_node(visitor, stmt);
    }
    if (has(SEMANTICS_MASK_SCOPE)) {
        visitor->exitScopeNode(visitor, node);
    }
}

void muil_visit_property_node(SemanticsPass *visitor, PropertyNode *node) {
//    vinfo("Visiting property: %s", node->name)
    if (!node) {
        verror("PropertyNode is null")
        return;
    }
    
    if (has(SEMANTICS_MASK_PROPERTY)) {
        visitor->enterPropertyNode(visitor, node);
    }
    if (node->type) {
        muil_visit_type(visitor, node->type);
    }
    if (node->value) {
        muil_visit_node(visitor, node->value);
    }
    if (has(SEMANTICS_MASK_PROPERTY)) {
        visitor->exitPropertyNode(visitor, node);
    }
}

void muil_visit_literal_node(SemanticsPass *visitor, LiteralNode *node) {
//    switch (node->type) {
//        case LITERAL_STRING:vinfo("Literal: %s", node->value.stringValue);
//            break;
//        case LITERAL_NUMBER:vinfo("Literal: %f", node->value.numberValue);
//            break;
//        case LITERAL_BOOLEAN:vinfo("Literal: %s", node->value.booleanValue ? "true" : "false");
//            break;
//    }
    if (!node) {
        verror("LiteralNode is null")
        return;
    }
    if (has(SEMANTICS_MASK_LITERAL)) {
        visitor->enterLiteralNode(visitor, node);
    }
    //TODO: Do we need to visit the type of the literal?
    if (has(SEMANTICS_MASK_LITERAL)) {
        visitor->exitLiteralNode(visitor, node);
    }
}

void muil_visit_assignment_node(SemanticsPass *visitor, AssignmentNode *node) {
//    vinfo("Visiting assignment: %s", node->variableName)
    if (!node) {
        verror("AssignmentNode is null")
        return;
    }
    if (has(SEMANTICS_MASK_ASSIGNMENT)) {
        visitor->enterAssignmentNode(visitor, node);
    }
    muil_visit_node(visitor, node->value);
    if (has(SEMANTICS_MASK_ASSIGNMENT)) {
        visitor->exitAssignmentNode(visitor, node);
        
    }
}

void muil_visit_array_node(SemanticsPass *visitor, ArrayNode *node) {
//    vinfo("Visiting array")
    if (!node) {
        verror("ArrayNode is null")
        return;
    }
    if (has(SEMANTICS_MASK_ARRAY)) {
        visitor->enterArrayNode(visitor, node);
    }
    for (ASTNode *elem = node->elements; elem != null; elem = elem->next) {
        muil_visit_node(visitor, elem);
    }
    if (has(SEMANTICS_MASK_ARRAY)) {
        visitor->exitArrayNode(visitor, node);
    }
}

void muil_visit_binary_op_node(SemanticsPass *visitor, BinaryOpNode *node) {
    if (!node) {
        verror("BinaryOpNode is null")
        return;
    }
    if (has(SEMANTICS_MASK_BINARY_OP)) {
        visitor->enterBinaryOpNode(visitor, node);
    }
    muil_visit_node(visitor, node->left);
    muil_visit_node(visitor, node->right);
    if (has(SEMANTICS_MASK_BINARY_OP)) {
        visitor->exitBinaryOpNode(visitor, node);
    }
}

void muil_visit_type(SemanticsPass *visitor, TypeAST *type) {
    if (type == null) {
        verror("Type is null");
        return;
    }
    if (has(SEMANTICS_MASK_TYPE)) {
        visitor->enterType(visitor, type);
    }
    // Depending on type kind, we perform different visitations or actions
    switch (type->kind) {
        case TYPE_BASIC:break;
        case TYPE_ARRAY:
            // Recursively visit the element type of the array
            muil_visit_type(visitor, type->data.array.elementType);
            break;
        case TYPE_UNION:
            // Recursively visit both sides of the union
            muil_visit_type(visitor, type->data.binary.lhs);
            muil_visit_type(visitor, type->data.binary.rhs);
            break;
        case TYPE_INTERSECTION:
            // Recursively visit both sides of the intersection
            muil_visit_type(visitor, type->data.binary.lhs);
            muil_visit_type(visitor, type->data.binary.rhs);
            break;
        case TYPE_FUNCTION:
            // Recursively visit the input and output types
            muil_visit_type(visitor, type->data.binary.lhs); // Input type
            muil_visit_type(visitor, type->data.binary.rhs); // Output type
            break;
        case TYPE_TUPLE:
            // Recursively visit each type in the tuple
            for (TypeAST *elem = type->data.tupleTypes; elem != null; elem = elem->next) {
                muil_visit_type(visitor, elem);
            }
            break;
        default:verror("Unknown Type Kind: %d", type->kind);
            break;
    }
    if (has(SEMANTICS_MASK_TYPE)) {
        visitor->exitType(visitor, type);
    }
}

void muil_visit_reference_node(SemanticsPass *visitor, ReferenceNode *node) {
    if (node == null) {
        verror("ReferenceNode is null");
        return;
    }
    if (has(SEMANTICS_MASK_REFERENCE)) {
        visitor->enterReferenceNode(visitor, node);
    }
    if (node->type) {
        muil_visit_type(visitor, node->type);
    }
    if (node->reference) {
        muil_visit_node(visitor, node->reference);
    }
    if (has(SEMANTICS_MASK_REFERENCE)) {
        visitor->exitReferenceNode(visitor, node);
    }
}


void muil_visit_function_call_node(SemanticsPass *visitor, FunctionCallNode *node) {
    if (node == null) {
        verror("FunctionCallNode is null");
        return;
    }
    if (has(SEMANTICS_MASK_FUNCTION_CALL)) {
        visitor->enterFunctionCallNode(visitor, node);
    }
    if (node->reference) {
        muil_visit_node(visitor, node->reference);
    }
    if (node->arguments) {
        for (ASTNode *arg = node->arguments; arg != null; arg = arg->next) {
            muil_visit_node(visitor, arg);
        }
    }
    if (has(SEMANTICS_MASK_FUNCTION_CALL)) {
        visitor->exitFunctionCallNode(visitor, node);
    }
}


void muil_visit_node(SemanticsPass *visitor, ASTNode *node) {
    if (node == null) {
        verror("Node is null");
        return;
    }
    switch (node->nodeType) {
        case AST_COMPONENT:muil_visit_component_node(visitor, &node->data.component);
            break;
        case AST_SCOPE:muil_visit_scope_node(visitor, &node->data.scope);
            break;
        case AST_PROPERTY:muil_visit_property_node(visitor, &node->data.property);
            break;
        case AST_LITERAL:muil_visit_literal_node(visitor, &node->data.literal);
            break;
        case AST_ASSIGNMENT:muil_visit_assignment_node(visitor, &node->data.assignment);
            break;
        case AST_ARRAY:muil_visit_array_node(visitor, &node->data.array);
            break;
        case AST_BINARY_OP:muil_visit_binary_op_node(visitor, &node->data.binaryOp);
            break;
        case AST_REFERENCE:muil_visit_reference_node(visitor, &node->data.reference);
            break;
        case AST_FUNCTION_CALL:muil_visit_function_call_node(visitor, &node->data.functionCall);
            break;
    }
}

void muil_visit(SemanticsPass *visitor, ProgramAST *program) {
    if (program == null) {
        verror("ProgramAST is null");
        return;
    }
    if (visitor == null) {
        verror("ASTVisitor is null");
        return;
    }
    
    if (has(SEMANTICS_MASK_PROGRAM)) {
        visitor->enterProgramNode(visitor, program);
    }
    // Visit the root node and it's children
    muil_visit_node(visitor, program->root);
    if (has(SEMANTICS_MASK_PROGRAM)) {
        visitor->exitProgramNode(visitor, program);
    }
}


void muil_set_visitor(SemanticsPass *visitor, SemanticsPassMask mask, void *enter, void *exit) {
    if (visitor == null) {
        verror("ASTVisitor is null");
        return;
    }
    visitor->type_mask |= mask;
    switch (mask) {
        case SEMANTICS_MASK_PROGRAM:visitor->enterProgramNode = enter;
            visitor->exitProgramNode = exit;
            break;
        case SEMANTICS_MASK_COMPONENT:visitor->enterComponentNode = enter;
            visitor->exitComponentNode = exit;
            break;
        case SEMANTICS_MASK_PROPERTY:visitor->enterPropertyNode = enter;
            visitor->exitPropertyNode = exit;
            break;
        case SEMANTICS_MASK_LITERAL:visitor->enterLiteralNode = enter;
            visitor->exitLiteralNode = exit;
            break;
        case SEMANTICS_MASK_ASSIGNMENT:visitor->enterAssignmentNode = enter;
            visitor->exitAssignmentNode = exit;
            break;
        case SEMANTICS_MASK_ARRAY:visitor->enterArrayNode = enter;
            visitor->exitArrayNode = exit;
            break;
        case SEMANTICS_MASK_SCOPE:visitor->enterScopeNode = enter;
            visitor->exitScopeNode = exit;
            break;
        case SEMANTICS_MASK_BINARY_OP:visitor->enterBinaryOpNode = enter;
            visitor->exitBinaryOpNode = exit;
            break;
        case SEMANTICS_MASK_REFERENCE:visitor->enterReferenceNode = enter;
            visitor->exitReferenceNode = exit;
            break;
        case SEMANTICS_MASK_FUNCTION_CALL:visitor->enterFunctionCallNode = enter;
            visitor->exitFunctionCallNode = exit;
            break;
        case SEMANTICS_MASK_TYPE:visitor->enterType = enter;
            visitor->exitType = exit;
            break;
        default:verror("Unknown visitor type mask: %d", mask);
            break;
    }
}


b8 muil_has_visitor(SemanticsPass *visitor, SemanticsPassMask mask) {
    return (visitor->type_mask & mask) == mask;
}

void *muil_get_visitor_enter(SemanticsPass *visitor, SemanticsPassMask mask) {
    switch (mask) {
        case SEMANTICS_MASK_PROGRAM:return visitor->enterProgramNode;
        case SEMANTICS_MASK_COMPONENT:return visitor->enterComponentNode;
        case SEMANTICS_MASK_PROPERTY:return visitor->enterPropertyNode;
        case SEMANTICS_MASK_LITERAL:return visitor->enterLiteralNode;
        case SEMANTICS_MASK_ASSIGNMENT:return visitor->enterAssignmentNode;
        case SEMANTICS_MASK_ARRAY:return visitor->enterArrayNode;
        case SEMANTICS_MASK_SCOPE:return visitor->enterScopeNode;
        case SEMANTICS_MASK_BINARY_OP:return visitor->enterBinaryOpNode;
        case SEMANTICS_MASK_REFERENCE:return visitor->enterReferenceNode;
        case SEMANTICS_MASK_FUNCTION_CALL:return visitor->enterFunctionCallNode;
        case SEMANTICS_MASK_TYPE:return visitor->enterType;
        default:verror("Unknown visitor type mask: %d", mask);
            break;
    }
}

void *muil_get_visitor_exit(SemanticsPass *visitor, SemanticsPassMask mask) {
    switch (mask) {
        case SEMANTICS_MASK_PROGRAM:return visitor->exitProgramNode;
        case SEMANTICS_MASK_COMPONENT:return visitor->exitComponentNode;
        case SEMANTICS_MASK_PROPERTY:return visitor->exitPropertyNode;
        case SEMANTICS_MASK_LITERAL:return visitor->exitLiteralNode;
        case SEMANTICS_MASK_ASSIGNMENT:return visitor->exitAssignmentNode;
        case SEMANTICS_MASK_ARRAY:return visitor->exitArrayNode;
        case SEMANTICS_MASK_SCOPE:return visitor->exitScopeNode;
        case SEMANTICS_MASK_BINARY_OP:return visitor->exitBinaryOpNode;
        case SEMANTICS_MASK_REFERENCE:return visitor->exitReferenceNode;
        case SEMANTICS_MASK_FUNCTION_CALL:return visitor->exitFunctionCallNode;
        case SEMANTICS_MASK_TYPE:return visitor->exitType;
        default:verror("Unknown visitor type mask: %d", mask);
            break;
    }
}
