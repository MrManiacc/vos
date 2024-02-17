/**
 * Created by jraynor on 2/17/2024.
 */
#include <stdio.h>
#include "muil_sym.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "containers/dict.h"
#include "containers/stack.h"
#include "muil_dump.h"

struct TypeCheckingVisitor {
    // The base IRVisitor struct is used to store the function pointers for visiting different types of nodes in the IR.
    IRVisitor base;
    // The typeTable is used to store the types of all variables and properties in the program.
    Dict *typeTable;
    // The scope stack is used to keep track of the current scope during type checking.
    Stack *scopeStack;
};

/**
 * @brief Visits a component node in the AST.
 *
 * This function is used to visit a component node in the AST during type checking.
 * It adds the component's properties to the type table and pushes the component's scope onto the scope stack.
 *
 * @param self A pointer to the TypeCheckingVisitor object.
 * @param node A pointer to the ComponentNode being visited.
 */
static void visitComponentNode(TypeCheckingVisitor *self, ComponentNode *node) {
//    vinfo("Visiting component: %s", node->name);
    printf("Visiting component: %s\n", node->name);
    
}

static void visitType(TypeCheckingVisitor *self, Type *type) {
    char *type_name = parser_dump_type(type);
    printf("Visiting type: %s\n", type_name);
}



/**
 * @brief Creates a new TypeCheckingVisitor object.
 *
 * @return A new TypeCheckingVisitor object.
 */
VAPI TypeCheckingVisitor *typechecker_new() {
    TypeCheckingVisitor *visitor = vnew(TypeCheckingVisitor);
    visitor->base.visitComponentNode = (void (*)(IRVisitor *, ComponentNode *)) visitComponentNode;
    visitor->base.visitType = (void (*)(IRVisitor *, Type *)) visitType;
    return visitor;
}

/**
 * @brief Frees a TypeCheckingVisitor object.
 *
 * @param visitor A pointer to the TypeCheckingVisitor object to free.
 */
VAPI void type_checker_free(TypeCheckingVisitor *visitor) {
    if (visitor) {
        if (visitor->typeTable) {
            dict_destroy(visitor->typeTable);
        }
        if (visitor->scopeStack) {
            stack_destroy(visitor->scopeStack);
        }
        kbye(visitor, sizeof(TypeCheckingVisitor));
    }
}