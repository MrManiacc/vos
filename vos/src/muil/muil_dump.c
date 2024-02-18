/**
 * Created by jraynor on 2/17/2024.
 */
#pragma once

#include "muil_dump.h"
#include "core/vstring.h"

// =============================================================================
// Dump Functions for Each AST Node Type
// =============================================================================
// Forward declarations for recursive dumping
void dump_ast_node(ASTNode *node, StringBuilder *sb, int indentLevel, int isLast);


void append_indent(StringBuilder *sb, int indentLevel, int isLast) {
    for (int i = 0; i < indentLevel; ++i) {
        sb_appendf(sb, "│   "); // Vertical line remains the same for simplicity
    }
    if (indentLevel > 0) {
        // Using DEC Line Drawing characters directly
        sb_appendf(sb, "%s── ", isLast ? "└" : "├"); // Modify these as needed based on your terminal's encoding support
    }
}

// Utility function for type dumping (handles composite types)
void dump_type(TypeAST *type, StringBuilder *sb) {
    if (!type) {
        sb_appendf(sb, "null");
        return;
    }
    if (type->alias) {
        sb_appendf(sb, "%s : ", type->alias);
    }
    switch (type->kind) {
        case TYPE_BASIC:sb_appendf(sb, "%s", type->data.name);
            break;
        case TYPE_ARRAY:dump_type(type->data.array.elementType, sb);
            sb_appendf(sb, "[]");
            break;
        case TYPE_UNION:dump_type(type->data.binary.lhs, sb);
            sb_appendf(sb, " | ");
            dump_type(type->data.binary.rhs, sb);
            break;
        case TYPE_INTERSECTION:dump_type(type->data.binary.lhs, sb);
            sb_appendf(sb, " & ");
            dump_type(type->data.binary.rhs, sb);
            break;
        case TYPE_FUNCTION:dump_type(type->data.binary.lhs, sb);
            sb_appendf(sb, " -> ");
            dump_type(type->data.binary.rhs, sb);
            break;
        case TYPE_TUPLE:sb_appendf(sb, "(");
            TypeAST *current = type->data.tupleTypes;
            while (current) {
                dump_type(current, sb);
                current = current->next;
                if (current) sb_appendf(sb, ", ");
            }
            sb_appendf(sb, ")");
            break;
        default:sb_appendf(sb, "Unknown Type");
    }
}

// Specific dumping functions for each node type
void dump_literal(ASTNode *node, StringBuilder *sb, int indentLevel, int isLast) {
    append_indent(sb, indentLevel, isLast);
    sb_appendf(sb, "Literal: ");
    switch (node->data.literal.type) {
        case LITERAL_NUMBER:sb_appendf(sb, "%f\n", node->data.literal.value.numberValue);
            break;
        case LITERAL_STRING:sb_appendf(sb, "%s\n", node->data.literal.value.stringValue);
            break;
        case LITERAL_BOOLEAN:sb_appendf(sb, "%s\n", node->data.literal.value.booleanValue ? "true" : "false");
            break;
    }
}

void dump_property(ASTNode *node, StringBuilder *sb, int indentLevel, int isLast) {
    append_indent(sb, indentLevel, isLast);
    sb_appendf(sb, "Property: %s, Type: ", node->data.property.name);
    dump_type(node->data.property.type, sb);
    sb_appendf(sb, "\n");
    if (node->data.property.value) {
        dump_ast_node(node->data.property.value, sb, indentLevel + 1, 1); // Property value is always last
    }
}

void dump_component(ASTNode *node, StringBuilder *sb, int indentLevel, int isLast) {
    append_indent(sb, indentLevel, isLast);
    sb_appendf(sb, "Compound: %s\n", node->data.component.name);
    if (node->data.component.super) {
        append_indent(sb, indentLevel + 1, 0);
        sb_appendf(sb, "Type: ");
        dump_type(node->data.component.super, sb);
        sb_appendf(sb, "\n");
    }
    if (node->data.component.body) {
        dump_ast_node(node->data.component.body, sb, indentLevel + 1, 1); // Component body is always last
    }
}

void dump_scope(ASTNode *node, StringBuilder *sb, int indentLevel, int isLast) {
    append_indent(sb, indentLevel, isLast);
    sb_appendf(sb, "Scope:\n"); // No need to print the scope node itself, just its children
    for (ASTNode *stmt = node->data.scope.body; stmt != null; stmt = stmt->next) {
        dump_ast_node(stmt, sb, indentLevel + 1, stmt->next == null);
    }
}

void dump_assignment(ASTNode *node, StringBuilder *sb, int indentLevel, int isLast) {
    append_indent(sb, indentLevel, isLast);
    sb_appendf(sb, "Assignment: %s = \n", node->data.assignment.variableName);
    dump_ast_node(node->data.assignment.value, sb, indentLevel + 1, 1); // Assignment value is always last
}

void dump_array(ASTNode *node, StringBuilder *sb, int indentLevel, int isLast) {
    append_indent(sb, indentLevel, isLast);
    sb_appendf(sb, "Array:\n");
    for (ASTNode *elem = node->data.array.elements; elem != null; elem = elem->next) {
        dump_ast_node(elem, sb, indentLevel + 1, elem->next == null);
    }
}

void dump_binary_op(ASTNode *node, StringBuilder *sb, int indentLevel, int isLast) {
    append_indent(sb, indentLevel, isLast);
    sb_appendf(sb, "Binary Op: %s\n", lexer_token_type_name(node->data.binaryOp.operator));
    dump_ast_node(node->data.binaryOp.left, sb, indentLevel + 1, 0);
    dump_ast_node(node->data.binaryOp.right, sb, indentLevel + 1, 1);
}

void dump_reference(ASTNode *node, StringBuilder *sb, int indentLevel, int isLast) {
    append_indent(sb, indentLevel, isLast);
    sb_appendf(sb, "Reference: %s, Type: \n", node->data.reference.name);
    ASTNode *ref = node->data.reference.reference;
    if (ref) {
        dump_ast_node(ref, sb, indentLevel + 1, 1);
//        dump_type(&ref->data.type, sb);
    } else {
        sb_appendf(sb, "null");
    }
}

void dump_function_call(ASTNode *node, StringBuilder *sb, int indentLevel, int isLast) {
    append_indent(sb, indentLevel, isLast);
    sb_appendf(sb, "Function Call: %s\n", node->data.functionCall.name);
    if (node->data.functionCall.reference) {
        dump_ast_node(node->data.functionCall.reference, sb, indentLevel + 1, 0);
    }
    ASTNode *args = node->data.functionCall.arguments;
    while (args) {
        dump_ast_node(args, sb, indentLevel + 1, args->next == null);
        args = args->next;
    }
}

// Main recursive function to dump AST nodes
void dump_ast_node(ASTNode *node, StringBuilder *sb, int indentLevel, int isLast) {
    if (!node) return;
    
    switch (node->nodeType) {
        case AST_LITERAL:dump_literal(node, sb, indentLevel, isLast);
            break;
        case AST_PROPERTY:dump_property(node, sb, indentLevel, isLast);
            break;
        case AST_COMPONENT:dump_component(node, sb, indentLevel, isLast);
            break;
        case AST_SCOPE:dump_scope(node, sb, indentLevel, isLast);
            break;
        case AST_ASSIGNMENT:dump_assignment(node, sb, indentLevel, isLast);
            break;
        case AST_ARRAY:dump_array(node, sb, indentLevel, isLast);
            break;
        case AST_BINARY_OP:dump_binary_op(node, sb, indentLevel, isLast);
            break;
        case AST_REFERENCE:dump_reference(node, sb, indentLevel, isLast);
            break;
        case AST_FUNCTION_CALL:dump_function_call(node, sb, indentLevel, isLast);
            break;
    }

//     Uncomment if siblings are handled outside of child nodes
    if (node->next && node->nodeType == AST_SCOPE) {
        dump_ast_node(node->next, sb, indentLevel, !node->next->next);
    }
}

char *parser_dump_node(ASTNode *node) {
    StringBuilder *sb = sb_new();
    dump_ast_node(node, sb, 0, 1);
    char *result = sb_build(sb);
    sb_free(sb);
    return result;
}

// Entry point for dumping the AST to a string
char *parser_dump(ProgramAST *root) {
    return parser_dump_node(root->root);
}

char *parser_dump_type(TypeAST *type) {
    StringBuilder *sb = sb_new();
    dump_type(type, sb);
    char *result = sb_build(sb);
    sb_free(sb);
    return result;
}
