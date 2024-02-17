/**
 * Created by jraynor on 2/17/2024.
 */
#include "muil_ir.h"
#include "core/vlogger.h"
#include "muil_dump.h"

VAPI void ir_visit_ast_node(IRVisitor *visitor, ASTNode *node) {
    if (node == null) {
        verror("Node is null")
        return;
    }
    switch (node->nodeType) {
        case AST_COMPONENT:ir_visit_component_node(visitor, &node->data.component);
            break;
        case AST_SCOPE:ir_visit_scope_node(visitor, &node->data.scope);
            break;
        case AST_PROPERTY:ir_visit_property_node(visitor, &node->data.property);
            break;
        case AST_LITERAL:ir_visit_literal_node(visitor, &node->data.literal);
            break;
        case AST_ASSIGNMENT:ir_visit_assignment_node(visitor, &node->data.assignment);
            break;
        case AST_ARRAY:ir_visit_array_node(visitor, &node->data.array);
            break;
        case AST_BINARY_OP:ir_visit_binary_op_node(visitor, &node->data.binaryOp);
            break;
    }
}

VAPI void ir_visit_component_node(IRVisitor *visitor, ComponentNode *node) {
    vinfo("Visiting component: %s", node->name)
    if (visitor->visitComponentNode) {
        visitor->visitComponentNode(visitor, node);
    }
    if (node->extends) {
        ir_visit_type(visitor, node->extends);
    }
    if (node->body) {
        ir_visit_ast_node(visitor, node->body);
    }
}

VAPI void ir_visit_scope_node(IRVisitor *visitor, ScopeNode *node) {
    vinfo("Visiting scope")
    if (visitor->visitScopeNode != null) {
        visitor->visitScopeNode(visitor, node);
    }
    for (ASTNode *stmt = node->nodes; stmt != null; stmt = stmt->next) {
        ir_visit_ast_node(visitor, stmt);
    }
}

VAPI void ir_visit_property_node(IRVisitor *visitor, PropertyNode *node) {
    vinfo("Visiting property: %s", node->name)
    if (visitor->visitPropertyNode) {
        visitor->visitPropertyNode(visitor, node);
    }
    if (node->type) {
        ir_visit_type(visitor, node->type);
    }
    if (node->value) {
        ir_visit_ast_node(visitor, node->value);
    }
}

VAPI void ir_visit_literal_node(IRVisitor *visitor, LiteralNode *node) {
    switch (node->type) {
        case LITERAL_STRING:vinfo("Literal: %s", node->value.stringValue);
            break;
        case LITERAL_NUMBER:vinfo("Literal: %f", node->value.numberValue);
            break;
        case LITERAL_BOOLEAN:vinfo("Literal: %s", node->value.booleanValue ? "true" : "false");
            break;
    }
    if (visitor->visitLiteralNode) {
        visitor->visitLiteralNode(visitor, node);
    }
}

VAPI void ir_visit_assignment_node(IRVisitor *visitor, AssignmentNode *node) {
    vinfo("Visiting assignment: %s", node->variableName)
    if (visitor->visitAssignmentNode) {
        visitor->visitAssignmentNode(visitor, node);
    }
    ir_visit_ast_node(visitor, node->value);
}

VAPI void ir_visit_array_node(IRVisitor *visitor, ArrayNode *node) {
    vinfo("Visiting array")
    if (visitor->visitArrayNode) {
        visitor->visitArrayNode(visitor, node);
    }
    for (ASTNode *elem = node->elements; elem != null; elem = elem->next) {
        ir_visit_ast_node(visitor, elem);
    }
}

VAPI void ir_visit_binary_op_node(IRVisitor *visitor, BinaryOpNode *node) {
    vinfo("Visiting binary op node: %s", lexer_token_type_name(node->operator))
    if (visitor->visitBinaryOpNode) {
        visitor->visitBinaryOpNode(visitor, node);
    }
    ir_visit_ast_node(visitor, node->left);
    ir_visit_ast_node(visitor, node->right);
}

VAPI void ir_visit_type(IRVisitor *visitor, Type *type) {
    if (type == null) {
        verror("Type is null");
        return;
    }

    
    
    // Depending on type kind, we perform different visitations or actions
    switch (type->kind) {
        case TYPE_BASIC:break;
        case TYPE_ARRAY:
            // Recursively visit the element type of the array
            ir_visit_type(visitor, type->data.array.elementType);
            break;
        case TYPE_UNION:
            // Recursively visit both sides of the union
            ir_visit_type(visitor, type->data.binary.lhs);
            ir_visit_type(visitor, type->data.binary.rhs);
            break;
        case TYPE_INTERSECTION:
            // Recursively visit both sides of the intersection
            ir_visit_type(visitor, type->data.binary.lhs);
            ir_visit_type(visitor, type->data.binary.rhs);
            break;
        case TYPE_FUNCTION:
            // Recursively visit the input and output types
            ir_visit_type(visitor, type->data.binary.lhs); // Input type
            ir_visit_type(visitor, type->data.binary.rhs); // Output type
            break;
        case TYPE_TUPLE:
            // Recursively visit each type in the tuple
            for (Type *elem = type->data.tupleTypes; elem != null; elem = elem->next) {
                ir_visit_type(visitor, elem);
            }
            break;
        default:verror("Unknown Type Kind: %d", type->kind);
            break;
    }
    // If the visitor has a specific function for visiting types, call it
    if (visitor->visitType) {
        visitor->visitType(visitor, type);
    }
}


VAPI void ir_visit_tree(IRVisitor *visitor, ProgramAST *node) {
    vinfo("Visiting program AST")
    if (visitor->visitProgramAST) {
        
        visitor->visitProgramAST(visitor, node);
    }
    ASTNode *current = node->root;
    while (current != null) {
        ir_visit_ast_node(visitor, current);
        current = current->next;
    }
}
