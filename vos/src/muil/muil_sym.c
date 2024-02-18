///**
// * Created by jraynor on 2/17/2024.
// */
//#include <stdio.h>
//#include "muil_sym.h"
//#include "core/vmem.h"
//#include "core/vlogger.h"
//#include "containers/dict.h"
//#include "containers/stack.h"
//#include "muil_dump.h"
//#include "core/vstring.h"
//
//
//struct TypeTable {
//    // The typeTable is used to store the types of all variables and properties in the program.
//    // We can push and pop scopes to the stack
//    Stack *scope;
//};
//
//struct TypeAnalyzer {
//    // The base IRVisitor struct is used to store the function pointers for visiting different types of nodes in the IR.
//    ASTVisitor base;
//    // The typeTable is used to store the types of all variables and properties in the program.
//    Dict *typeTable;
//    // The scope stack is used to keep track of the current scope during type checking.
//    Stack *scopeStack;
//    // The errorStack is used to store any errors that occur during type checking.
//    Stack *errorStack;
//};
//
//
//// Checks if 'type' is valid for 'superType'
//static b8 is_type_valid_for_type(Type *type, Type *superType) {
//    // Basic type compatibility
//    if (type->kind == TYPE_BASIC && superType->kind == TYPE_BASIC) {
//        return strings_equal(type->data.name, superType->data.name);
//    }
//
//    // Array type compatibility
//    if (type->kind == TYPE_ARRAY && superType->kind == TYPE_ARRAY) {
//        return is_type_valid_for_type(type->data.array.elementType, superType->data.array.elementType);
//    }
//
//    // Union type compatibility
//    if (superType->kind == TYPE_UNION) {
//        // For 'type' to be valid for a union 'superType', it must be valid for at least one of the union members.
//        return is_type_valid_for_type(type, superType->data.binary.lhs) ||
//               is_type_valid_for_type(type, superType->data.binary.rhs);
//    }
//
//    // Intersection type compatibility
//    if (superType->kind == TYPE_INTERSECTION) {
//        // For 'type' to be valid for an intersection 'superType', it must be valid for all intersection members.
//        return is_type_valid_for_type(type, superType->data.binary.lhs) &&
//               is_type_valid_for_type(type, superType->data.binary.rhs);
//    }
//
//    // Function type compatibility
//    if (type->kind == TYPE_FUNCTION && superType->kind == TYPE_FUNCTION) {
//        // Validate both input and output types
//        return is_type_valid_for_type(type->data.binary.lhs, superType->data.binary.lhs) &&
//               is_type_valid_for_type(type->data.binary.rhs, superType->data.binary.rhs);
//    }
//
//    // Tuple type compatibility
//    if (type->kind == TYPE_TUPLE && superType->kind == TYPE_TUPLE) {
//        // Simplified check: Ensure the tuples have the same number of elements and each corresponding element matches.
//        // This is a placeholder; actual implementation would iterate and compare each tuple element.
//        Type *currentType = type->data.tupleTypes;
//        Type *currentSuperType = superType->data.tupleTypes;
//        while (currentType && currentSuperType) {
//            if (!is_type_valid_for_type(currentType, currentSuperType)) {
//                return false;
//            }
//            currentType = currentType->next;
//            currentSuperType = currentSuperType->next;
//        }
//        // Both should reach their end at the same time for the types to match
//        return !currentType && !currentSuperType;
//    }
//
//    // If none of the conditions match, the types are not compatible
//    return false;
//}
//
//
//static b8 validateType(TypeAnalyzer *self, Type *type);
//
///**
// * @brief Visits a component node in the AST.
// *
// * This function is used to visit a component node in the AST during type checking.
// * It adds the component's properties to the type table and pushes the component's scope onto the scope stack.
// *
// * @param self A pointer to the TypeCheckingVisitor object.
// * @param node A pointer to the ComponentNode being visited.
// */
//static void visitComponentNode(TypeAnalyzer *self, ComponentNode *node) {
//    // For now we're just registering the component in the type table, we will validate it's types in a later pass
//    if (dict_contains(self->typeTable, node->name)) {
//        stack_push(self->errorStack, string_format("Component already exists in type table: %s", node->name));
//        return;
//    }
//    if (!node->super) {
//        // Always give void as the super type if it's not defined
//        node->super = dict_get(self->typeTable, "void");
//    }
//    dict_set(self->typeTable, node->name, node->super);
//}
//
//static void validateTypes(TypeAnalyzer *self) {
//    // Iterate through the type table and validate each type
//    DictIterator iter = dict_iterator(self->typeTable);
//    while (dict_next(&iter)) {
//        Type *type = iter.entry->value;
//        if (!validateType(self, type)) {
//            // If the type is not valid, push an error onto the error stack
//            char *lastError[256];
//            stack_pop(self->errorStack, &lastError);
//            stack_push(self->errorStack, string_format("Invalid type: %s. Error: %s", iter.entry->key, lastError));
//            // remove it from the type table
//            dict_remove(self->typeTable, iter.entry->key);
//        }
//    }
//}
//
//static Type *evalExpressionType(TypeAnalyzer *self, ASTNode *node) {
//    // Evaluate the type of the expression
//    if (node->nodeType == AST_LITERAL) {
//        // Lookup the type of the literal in the type table
//        LiteralNode lit = node->data.literal;
//        if (lit.type == LITERAL_STRING) {
//            return dict_get(self->typeTable, "string");
//        } else if (lit.type == LITERAL_NUMBER) {
//            return dict_get(self->typeTable, "float");
//        } else if (lit.type == LITERAL_BOOLEAN) {
//            return dict_get(self->typeTable, "bool");
//        }
//    }
//    if (node->nodeType == AST_REFERENCE) {
//        // resolve the reference to a type
//        ASTNode *current = node;
//        ReferenceNode ref = current->data.reference;
//        if (ref.reference == null) {
//            stack_push(self->errorStack, "Reference has no reference");
//            return null;
//        }
//        return evalExpressionType(self, ref.reference);
//    }
//    if (node->nodeType == AST_BINARY_OP) {
//        BinaryOpNode binaryOpNode = node->data.binaryOp;
//        Type *lhsType = evalExpressionType(self, binaryOpNode.left);
//        Type *rhsType = evalExpressionType(self, binaryOpNode.right);
//        if (lhsType == null || rhsType == null) {
//            stack_push(self->errorStack, "Invalid type in binary operation");
//            return null;
//        }
//        // Perform type checking for the binary operation
//        switch (binaryOpNode.operator) {
//            case TOKEN_PLUS:
//            case TOKEN_MINUS:
//            case TOKEN_STAR:
//            case TOKEN_SLASH:
//                // Arithmetic operations
//                if (lhsType->kind != TYPE_BASIC || rhsType->kind != TYPE_BASIC) {
//                    stack_push(self->errorStack, "Invalid types for arithmetic operation");
//                    return null;
//                }
//                if (!strings_equal(lhsType->data.name, rhsType->data.name)) {
//                    stack_push(self->errorStack, "Arithmetic operation with different types");
//                    return null;
//                }
//                return lhsType;
////            case TOKEN_EQUAL_EQUAL:
////            case TOKEN_BANG_EQUAL:
////                // Equality operations
////                if (!is_type_valid_for_type(lhsType, rhsType)) {
////                    stack_push(self->errorStack, "Invalid types for equality operation");
////                    return null;
////                }
//                return dict_get(self->typeTable, "void");
//            default:stack_push(self->errorStack, "Unknown binary operation");
//                return null;
//        }
//    }
//    if (node->nodeType == AST_PROPERTY) {
//        PropertyNode propertyNode = node->data.property;
//        // If it has a defined type, return that type
//        if (propertyNode.type != null) {
//            return propertyNode.type;
//        }
//        // If it has a value, return the type of the value
//        if (propertyNode.value != null) {
//            return evalExpressionType(self, propertyNode.value);
//        }
//        // If it has neither, return null
//        stack_push(self->errorStack, string_format("Property %s has no type or value", propertyNode.name));
//
//    } else if (node->nodeType == AST_ASSIGNMENT) {
//        AssignmentNode assignmentNode = node->data.assignment;
//        // If it has a value, return the type of the value
//        if (assignmentNode.value != null) {
//            return evalExpressionType(self, assignmentNode.value);
//        } else {
//            stack_push(self->errorStack, "Assignment has no value");
//            return dict_get(self->typeTable, "void");
//        }
//    } else if (node->nodeType == AST_FUNCTION_CALL) {
//        FunctionCallNode functionCallNode = node->data.functionCall;
//        // If it has a reference, return the type of the reference
//        if (functionCallNode.reference != null) {
//            return evalExpressionType(self, functionCallNode.reference);
//        }
//    }
//    stack_push(self->errorStack, "Unknown expression type");
//    return null;
//}
//
//// Example of visiting a property node
//static void visitAssignment(TypeAnalyzer *self, AssignmentNode *node) {
//    // The type is semi-ambiguous. We may have to infer the type from the expression
//    if (node->value == null) {
//        stack_push(self->errorStack, "Assignment has no value");
//        return;
//    }
//    Type *type = evalExpressionType(self, node->value);
//    if (type == null) {
//        stack_push(self->errorStack, "Failed to infer type from expression");
//        return;
//    }
//}
//
//static void visitPropertyNode(TypeAnalyzer *self, PropertyNode *node) {
//    //The type is semi-ambiguous. We may have to infer the type from the expression
//    if (node->type == null) {
////        Type *type = evalExpressionType(self, node->value);
////        if (type == null) {
////            stack_push(self->errorStack, "Failed to infer type from expression");
////            return;
////        }
////        node->type = type;
//    }
//}
//
//static b8 validateType(TypeAnalyzer *self, Type *type) {
//    if (type == null) {
//        stack_push(self->errorStack, "Type is null");
//        return false;
//    }
//    // Depending on type kind, we perform different visitations or actions
//    switch (type->kind) {
//        case TYPE_BASIC:
////            b8 result = dict_contains(self->typeTable, type->data.name);
////            if (!result) {
////                stack_push(self->errorStack, string_format("Type not found in type table: %s", type->data.name));
////            }
////            return result;
//            return dict_contains(self->typeTable, type->data.name);
//        case TYPE_ARRAY:
//            // Recursively validate the element type of the array
//            return validateType(self, type->data.array.elementType);
//        case TYPE_UNION:
//            // Recursively validate both sides of the union
//            return validateType(self, type->data.binary.lhs) && validateType(self, type->data.binary.rhs);
//        case TYPE_INTERSECTION:
//            // Recursively validate both sides of the intersection
//            return validateType(self, type->data.binary.lhs) && validateType(self, type->data.binary.rhs);
//        case TYPE_FUNCTION:
//            // Recursively validate the input and output types
//            return validateType(self, type->data.binary.lhs) && validateType(self, type->data.binary.rhs);
//        case TYPE_TUPLE:
//            // Recursively validate each type in the tuple
//            for (Type *elem = type->data.tupleTypes; elem != null; elem = elem->next) {
//                if (!validateType(self, elem)) {
//                    return false;
//                }
//            }
//            return true;
//        default:stack_push(self->errorStack, "Unknown Type Kind");
//            return false;
//    }
//}
//
//
///**
// * @brief Registers the primitive types in the type table.
// *
// * This function is used to register the primitive types in the type table during initialization.
// *
// * @param self A pointer to the TypeCheckingVisitor object.
// */
//
//Type *type_new_basic(const char *name) {
//    Type *type = vnew(Type);
//    type->kind = TYPE_BASIC;
//    type->data.name = string_duplicate(name);
//    return type;
//}
//
//
//static void register_primitive_types(TypeAnalyzer *self) {
//    // Register the primitive types in the type table
//    dict_set(self->typeTable, "int", type_new_basic("int"));
//    dict_set(self->typeTable, "float", type_new_basic("float"));
//    dict_set(self->typeTable, "bool", type_new_basic("bool"));
//    dict_set(self->typeTable, "string", type_new_basic("string"));
//    dict_set(self->typeTable, "void", type_new_basic("void"));
//}
//
//static void destroy_primitive_types(TypeAnalyzer *self) {
//    // Destroy the primitive types in the type table
//    // free the name of the type
//    Type *int_type = dict_get(self->typeTable, "int");
//    Type *float_type = dict_get(self->typeTable, "float");
//    Type *bool_type = dict_get(self->typeTable, "bool");
//    Type *string_type = dict_get(self->typeTable, "string");
//    Type *void_type = dict_get(self->typeTable, "void");
//
//    kbye(int_type->data.name, string_length(int_type->data.name) + 1);
//    kbye(float_type->data.name, string_length(float_type->data.name) + 1);
//    kbye(bool_type->data.name, string_length(bool_type->data.name) + 1);
//    kbye(string_type->data.name, string_length(string_type->data.name) + 1);
//    kbye(void_type->data.name, string_length(void_type->data.name) + 1);
//
//    kbye(int_type, sizeof(Type));
//    kbye(float_type, sizeof(Type));
//    kbye(bool_type, sizeof(Type));
//    kbye(string_type, sizeof(Type));
//    kbye(void_type, sizeof(Type));
//
//    dict_remove(self->typeTable, "int");
//    dict_remove(self->typeTable, "float");
//    dict_remove(self->typeTable, "bool");
//    dict_remove(self->typeTable, "string");
//    dict_remove(self->typeTable, "void");
//}
//
//char *dump_type_table(Dict *typeTable) {
//    //draws a header for the type table
//
//
//    // Adjusting the initial frame characters to match the new width
//    char *result = string_duplicate(
//            "\n┌────────────────┬─────────────────────────────────────────────┐\n"); // Adjusted top border for correct alignment
//    // adding the header
//    result = string_concat(result,
//                           "│ Type           │ Value                                       │\n"); // Adjusted header for new width
//    // Adjusting the middle border to match the new width
//    result = string_concat(result,
//                           "├────────────────┼─────────────────────────────────────────────┤\n"); // Adjusted middle border for new width
//
//    DictIterator iter = dict_iterator(typeTable);
//    u32 count = dict_size(typeTable);
//    u32 index = 0;
//    while (dict_next(&iter)) {
//        char *hash_key = string_format("%s: ", iter.entry->key);
//        char *value = parser_dump_type(iter.entry->value);
//
//        // Keeping the format for the line with the adjusted right border
//        result = string_concat(result,
//                               string_format("│ %-14s │ %-43s │", hash_key, value)); // Output adjusted for new width
//        //Dont draw a line under the last element
//        if (++index != count - 1) {
//            result = string_concat(result,
//                                   "\n├────────────────┼─────────────────────────────────────────────┤\n"); // Adjusted middle border for new width
//        } else {
//            result = string_concat(result, "\n");
//        }
//        // Adjusting to draw a line under the key and value with vertical lines aligned
////        result = string_concat(result, "\n├────────────────┼─────────────────────────────────────────────┤\n"); // Adjusted middle border for new width
//    }
//
//
//    // Adjusting the bottom border to match the new width and alignment
//    result = string_concat(result,
//                           "└────────────────┴─────────────────────────────────────────────┘\n"); // Adjusted bottom border for correct alignment
//    return result;
//}
//
//void visit_program_ast(TypeAnalyzer *self, ProgramAST *program) {
//    // This is for the second pass of the type checker
//    while (!stack_is_empty(self->errorStack)) {
//        char *error[256];
//        stack_pop(self->errorStack, &error);
//        verror("%s", error);
//    }
//    validateTypes(self);
//    // This is for the second pass of the type checker
//    while (!stack_is_empty(self->errorStack)) {
//        char *error[256];
//        stack_pop(self->errorStack, &error);
//        verror("%s", error);
//    }
//    vinfo("Type table: %s", dump_type_table(self->typeTable));
//}
//
//
///**
// * @brief Creates a new TypeCheckingVisitor object.
// *
// * @return A new TypeCheckingVisitor object.
// */
//VAPI TypeAnalyzer *typechecker_new() {
//    TypeAnalyzer *visitor = vnew(TypeAnalyzer);
//    visitor->typeTable = dict_new();
//    Stack *scopeStack = vnew(Stack);
//    stack_create(scopeStack, sizeof(Type *));
//    visitor->scopeStack = scopeStack;
//
//    Stack *errorStack = vnew(Stack);
//    stack_create(errorStack, 256);
//    visitor->errorStack = errorStack;
//    visitor->base.visitComponentNode = (void (*)(ASTVisitor *, ComponentNode *)) visitComponentNode;
//    visitor->base.visitPropertyNode = (void (*)(ASTVisitor *, PropertyNode *)) visitPropertyNode;
//    visitor->base.visitProgramAST = (void (*)(ASTVisitor *, ProgramAST *)) visit_program_ast;
//    visitor->base.visitAssignmentNode = (void (*)(ASTVisitor *, AssignmentNode *)) visitAssignment;
//    register_primitive_types(visitor);
//    return visitor;
//}
//
///**
// * @brief Frees a TypeCheckingVisitor object.
// *
// * @param visitor A pointer to the TypeCheckingVisitor object to free.
// */
//VAPI void type_checker_free(TypeAnalyzer *visitor) {
//    if (visitor) {
//        // output the error stack
//
//
//        if (visitor->typeTable) {
//            dict_destroy(visitor->typeTable);
//        }
//        if (visitor->scopeStack) {
//            stack_destroy(visitor->scopeStack);
//        }
//        if (visitor->errorStack) {
//            stack_destroy(visitor->errorStack);
//        }
//        destroy_primitive_types(visitor);
//        kbye(visitor, sizeof(TypeAnalyzer));
//    }
//}