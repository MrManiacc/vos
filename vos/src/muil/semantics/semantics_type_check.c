/**
 * Created by jraynor on 2/18/2024.
 */

#include "muil/muil.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "containers/dict.h"

void type_check_pass_ignore(TypePass *visitor, void *node) {}

void type_check_pass__program_enter(TypePass *visitor, ProgramAST *node);
void type_check_pass_program_exit(TypePass *visitor, ProgramAST *node);

void type_check_visit_assignment_enter(TypePass *visitor, PropertyAssignmentNode *node);

static TypeSymbol *evalulate_node_type(ASTNode *node, Scope *scope);


// The scpope is defined from the previous pass. This is the scope that will be used to resolve type_checks.
implement_pass(TypePass, Scope *scope, {
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_PROGRAM, type_check_pass__program_enter,
            type_check_pass_program_exit);
    
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_ASSIGNMENT, type_check_visit_assignment_enter,
            type_check_pass_ignore);
    
}, {
    //Cleanup
});

void type_check_pass__program_enter(TypePass *visitor, ProgramAST *node) {
    // Picking up where we left off in the last pass, we can use the scope from the previous pass.
    // This is the scope that will be used to resolve type_checks.
    visitor->scope = visitor->base.userData; // Assuming the previous pass set the userData to the scope. TODO: error check if needed?
}


static TypeSymbol *get_native_type(ASTLiteralType type) {
    //Creates three static types for the native types
    static TypeSymbol *intType = null;
    static TypeSymbol *boolType = null;
    static TypeSymbol *stringType = null;
    if (!intType) {
        intType = vnew(TypeSymbol);
        intType->kind = TYPE_BASIC;
        intType->data.name = "int";
    }
    if (!boolType) {
        boolType = vnew(TypeSymbol);
        boolType->kind = TYPE_BASIC;
        boolType->data.name = "bool";
    }
    if (!stringType) {
        stringType = vnew(TypeSymbol);
        stringType->kind = TYPE_BASIC;
        stringType->data.name = "string";
    }
    switch (type) {
        case LITERAL_NUMBER :return intType;
        case LITERAL_STRING:return boolType;
        case LITERAL_BOOLEAN:return stringType;
        default:verror("Failed to get native type for %d", type);
            return null;
    }
    verror("Failed to get native type for %d", type);
    return null;
}


static TypeSymbol *get_reference_type(char *name, Scope *scope) {
    if (!scope) {
        verror("Invalid scope");
        return null;
    }
    if (!name) {
        verror("Invalid name");
        return null;
    }
    ASTNode *node = dict_get(scope->symbols, name);
    if (!node) {
        // Try top scope
        if (scope->parent) {
            return get_reference_type(name, scope->parent);
        }
        verror("Failed to resolve reference %s", name);
        return null;
    }
    return evalulate_node_type(node, scope);
}

static TypeSymbol *get_binary_expr_type(ASTNode *node, Scope *scope) {
    if (!node) return null;
    if (node->nodeType != AST_BINARY_OP) {
        verror("Invalid node type: %d", node->nodeType);
        return null;
    }
    ASTNode *left = node->data.binaryOp.left;
    ASTNode *right = node->data.binaryOp.right;
    TypeSymbol *leftType = evalulate_node_type(left, scope);
    TypeSymbol *rightType = evalulate_node_type(right, scope);
    if (!leftType || !rightType) {
        verror("Failed to resolve type for binary op");
        return null;
    }
    // We need to check if the types are compatible
    if (leftType->kind != rightType->kind) {
        verror("Incompatible types for binary op");
        return null;
    }
    return leftType;
}

static TypeSymbol *get_function_call_type(ASTNode *node, Scope *scope) {
    if (!node) return null;
    if (node->nodeType != AST_FUNCTION_CALL) {
        verror("Invalid node type: %d", node->nodeType);
        return null;
    }
    // We need to resolve the function call and check the return type of the function.
    TypeSymbol *functionType = get_reference_type(node->data.functionCall.name, scope);
    if (!functionType) {
        verror("Failed to resolve function call");
        return null;
    }
    return functionType;
}

static TypeSymbol *get_array_type(ASTNode *node, Scope *scope) {
    if (!node) return null;
    if (node->nodeType != AST_ARRAY) {
        verror("Invalid node type: %d", node->nodeType);
        return null;
    }
    // We need to check the types of the elements in the array.
    ASTNode *elements = node->data.array.elements;
    if (!elements) {
        verror("Invalid array elements");
        return null;
    }
    // We need to check the types of the elements in the array.
    return evalulate_node_type(elements, scope);
}


static TypeSymbol *evalulate_node_type(ASTNode *node, Scope *scope) {
    if (!node) return null;
    switch (node->nodeType) {
        case AST_LITERAL:return get_native_type(node->data.literal.type);
        case AST_REFERENCE:return get_reference_type(node->data.reference.name, scope);
        case AST_BINARY_OP:return get_binary_expr_type(node, scope);
        case AST_FUNCTION_CALL:return get_function_call_type(node, scope);
        case AST_ARRAY:return get_array_type(node, scope);
        case AST_COMPONENT_DECLARE: return node->data.compound.super;
        case AST_PROPERTY_DECLARE: return node->data.property.type;
        case AST_TYPE:return node->data.type.type;
        default: return null;
    }
}

void type_check_visit_assignment_enter(TypePass *visitor, PropertyAssignmentNode *node) {
    // Here we check if the type of the assignment is valid. We may need check the reference type and the assignment type
    // if it's a reference type, we need to resolve the reference and check the type of the resolved node.
    // if it's a literal type, we need to check the type of the literal.
    // if it's a function call, we need to check the return type of the function.
    
    // We can use the visitor->scope to resolve the reference.

//    vinfo("Type checking assignment: %s", parser_dump_node(node->assignee));
    // If the property does have a type, we need to make sure the assignment type matches the property type.
    // We can skip the type check if the node already has a type.
    //We'll need to do some complicated type checking here because of our union, intersection, and tuple types.
    TypeSymbol *assignment_type = evalulate_node_type(node->assignee, visitor->scope);
    if (!assignment_type)
        assignment_type = evalulate_node_type(node->assignment, visitor->scope);
    if (!assignment_type) {
        verror("Failed to resolve type for assignment");
        return;
    }
    //Assign the type to the node's assignee
    if (node->assignee->nodeType == AST_PROPERTY_DECLARE)
        node->assignee->data.property.type = assignment_type;
    else if (node->assignee->nodeType == AST_COMPONENT_DECLARE)
        node->assignee->data.compound.super = assignment_type;
    else if (node->assignee->nodeType == AST_TYPE)
        node->assignee->data.type.type = assignment_type;
    else if (node->assignee->nodeType == AST_REFERENCE)
        node->assignee->data.reference.type = assignment_type;
    else
        verror("Invalid node type for assignment");
    
}


void type_check_pass_program_exit(TypePass *visitor, ProgramAST *node) {
    vinfo("Tree after type check pass: %s", parser_dump(node));
}