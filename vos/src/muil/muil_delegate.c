/**
 * Created by jraynor on 2/18/2024.
 */

#include "muil/muil.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "muil/muil_visitor.h"
#include "containers/darray.h"
#include "core/vmutex.h"

typedef struct LinkedPass {
    SemanticsPass *visitor;
    struct LinkedPass *next;
} LinkedPass;

typedef struct DelegateVisitor {
    SemanticsPass base;
    LinkedPass *head; // Head of the linked list for traversal
    LinkedPass *tail; // Tail of the linked list for easy addition
} DelegateVisitor;

struct PassManager {
    DelegateVisitor *delegate;
    kmutex *mutex;
};

static void initialize_delegate_visitor(PassManager *manager);

PassManager *muil_pass_manager_new() {
    DelegateVisitor *delegate = vnew(struct DelegateVisitor);
    PassManager *manager = vnew(PassManager);
    manager->delegate = delegate;
    manager->mutex = vnew(kmutex);
    kmutex_create(manager->mutex);
    initialize_delegate_visitor(manager);
    return manager;
}

void muil_pass_manager_add(PassManager *manager, SemanticsPass *visitor) {
    if (!manager) {
        verror("Pass manager or visitor is null");
        return;
    }
    kmutex_lock(manager->mutex);
    
    // Create a new pass and set it up
    LinkedPass *pass = vnew(LinkedPass);
    pass->visitor = visitor;
    pass->next = null;
    
    // Append the new pass to the end of the list
    if (manager->delegate->tail) {
        manager->delegate->tail->next = pass;
    } else {
        manager->delegate->head = pass;
    }
    manager->delegate->tail = pass;
    
    kmutex_unlock(manager->mutex);
}


//Runs all the passes in the pass manager
void muil_pass_manager_run(PassManager *manager, ProgramAST *root) {
    if (!manager || !root) {
        verror("Pass manager or root node is null");
        return;
    }
    kmutex_lock(manager->mutex);
    muil_visit((SemanticsPass *) manager->delegate, root);
    kmutex_unlock(manager->mutex);
}

void muil_pass_manager_destroy(PassManager *manager) {
    if (manager) {
        kmutex_destroy(manager->mutex);
        // Iterate through the list of passes and free them
        for (LinkedPass *current = manager->delegate->head; current != null;) {
            LinkedPass *next = current->next;
            kfree(current, sizeof(LinkedPass), MEMORY_TAG_ARRAY);
            current = next;
        }
        vdelete(manager);
    }
}


// ===================================================================================
// ========================== Delegation to visitors =================================
// ===================================================================================
static void delegate_visitor_enter(DelegateVisitor *manager, SemanticsPassMask mask, void *node) {
    for (LinkedPass *current = manager->head; current != null; current = current->next) {
        SemanticsPass *visitor = current->visitor;
        if (muil_has_visitor(visitor, mask)) {
            void
            (*enter)(SemanticsPass *, void *) = (void (*)(SemanticsPass *, void *)) muil_get_visitor_enter(visitor, mask);
            if (enter) {
                enter(visitor, node);
            }
        }
    }
}

static void delegate_visitor_exit(DelegateVisitor *manager, SemanticsPassMask mask, void *node) {
    for (LinkedPass *current = manager->head; current != null; current = current->next) {
        SemanticsPass *visitor = current->visitor;
        if (muil_has_visitor(visitor, mask)) {
            void (*exit)(SemanticsPass *, void *) = (void (*)(SemanticsPass *, void *)) muil_get_visitor_exit(visitor, mask);
            if (exit) {
                exit(visitor, node);
            }
        }
    }
}


static void delegate_visitor_program_enter(DelegateVisitor *manager, ProgramAST *program) {
    delegate_visitor_enter(manager, SEMANTICS_MASK_PROGRAM, program);
}

static void delegate_visitor_program_exit(DelegateVisitor *manager, ProgramAST *program) {
    delegate_visitor_exit(manager, SEMANTICS_MASK_PROGRAM, program);
}

static void delegate_visitor_component_enter(DelegateVisitor *manager, CompoundDeclaration *node) {
    delegate_visitor_enter(manager, SEMANTICS_MASK_COMPONENT, node);
}

static void delegate_visitor_component_exit(DelegateVisitor *manager, CompoundDeclaration *node) {
    delegate_visitor_exit(manager, SEMANTICS_MASK_COMPONENT, node);
}

static void delegate_visitor_property_enter(DelegateVisitor *manager, PropertyDeclaration *node) {
    delegate_visitor_enter(manager, SEMANTICS_MASK_PROPERTY, node);
}

static void delegate_visitor_property_exit(DelegateVisitor *manager, PropertyDeclaration *node) {
    delegate_visitor_exit(manager, SEMANTICS_MASK_PROPERTY, node);
}

static void delegate_visitor_literal_enter(DelegateVisitor *manager, LiteralNode *node) {
    delegate_visitor_enter(manager, SEMANTICS_MASK_LITERAL, node);
}

static void delegate_visitor_literal_exit(DelegateVisitor *manager, LiteralNode *node) {
    delegate_visitor_exit(manager, SEMANTICS_MASK_LITERAL, node);
}

static void delegate_visitor_assignment_enter(DelegateVisitor *manager, PropertyAssignmentNode *node) {
    delegate_visitor_enter(manager, SEMANTICS_MASK_ASSIGNMENT, node);
}

static void delegate_visitor_assignment_exit(DelegateVisitor *manager, PropertyAssignmentNode *node) {
    delegate_visitor_exit(manager, SEMANTICS_MASK_ASSIGNMENT, node);
}

static void delegate_visitor_array_enter(DelegateVisitor *manager, ArrayNode *node) {
    delegate_visitor_enter(manager, SEMANTICS_MASK_ARRAY, node);
}

static void delegate_visitor_array_exit(DelegateVisitor *manager, ArrayNode *node) {
    delegate_visitor_exit(manager, SEMANTICS_MASK_ARRAY, node);
}

static void delegate_visitor_scope_enter(DelegateVisitor *manager, ScopeNode *node) {
    delegate_visitor_enter(manager, SEMANTICS_MASK_SCOPE, node);
}

static void delegate_visitor_scope_exit(DelegateVisitor *manager, ScopeNode *node) {
    delegate_visitor_exit(manager, SEMANTICS_MASK_SCOPE, node);
}

static void delegate_visitor_binary_op_enter(DelegateVisitor *manager, BinaryOpNode *node) {
    delegate_visitor_enter(manager, SEMANTICS_MASK_BINARY_OP, node);
}

static void delegate_visitor_binary_op_exit(DelegateVisitor *manager, BinaryOpNode *node) {
    delegate_visitor_exit(manager, SEMANTICS_MASK_BINARY_OP, node);
}

static void delegate_visitor_reference_enter(DelegateVisitor *manager, ReferenceNode *node) {
    delegate_visitor_enter(manager, SEMANTICS_MASK_REFERENCE, node);
}

static void delegate_visitor_reference_exit(DelegateVisitor *manager, ReferenceNode *node) {
    delegate_visitor_exit(manager, SEMANTICS_MASK_REFERENCE, node);
}

static void delegate_visitor_function_call_enter(DelegateVisitor *manager, FunctionCallNode *node) {
    delegate_visitor_enter(manager, SEMANTICS_MASK_FUNCTION_CALL, node);
}

static void delegate_visitor_function_call_exit(DelegateVisitor *manager, FunctionCallNode *node) {
    delegate_visitor_exit(manager, SEMANTICS_MASK_FUNCTION_CALL, node);
}

static void delegate_visitor_type_enter(DelegateVisitor *manager, TypeSymbol *node) {
    delegate_visitor_enter(manager, SEMANTICS_MASK_TYPE, node);
}

static void delegate_visitor_type_exit(DelegateVisitor *manager, TypeSymbol *node) {
    delegate_visitor_exit(manager, SEMANTICS_MASK_TYPE, node);
}


static void initialize_delegate_visitor(PassManager *manager) {
    DelegateVisitor *delegate = manager->delegate;
    delegate->base.type_mask = 0;
    muil_set_visitor(&delegate->base, SEMANTICS_MASK_PROGRAM, (void *) delegate_visitor_program_enter,
                     (void *) delegate_visitor_program_exit);
    muil_set_visitor(&delegate->base, SEMANTICS_MASK_COMPONENT, (void *) delegate_visitor_component_enter,
                     (void *) delegate_visitor_component_exit);
    muil_set_visitor(&delegate->base, SEMANTICS_MASK_PROPERTY, (void *) delegate_visitor_property_enter,
                     (void *) delegate_visitor_property_exit);
    
    muil_set_visitor(&delegate->base, SEMANTICS_MASK_LITERAL, (void *) delegate_visitor_literal_enter,
                     (void *) delegate_visitor_literal_exit);
    
    muil_set_visitor(&delegate->base, SEMANTICS_MASK_ASSIGNMENT, (void *) delegate_visitor_assignment_enter,
                     (void *) delegate_visitor_assignment_exit);
    
    muil_set_visitor(&delegate->base, SEMANTICS_MASK_ARRAY, (void *) delegate_visitor_array_enter,
                     (void *) delegate_visitor_array_exit);
    
    muil_set_visitor(&delegate->base, SEMANTICS_MASK_SCOPE, (void *) delegate_visitor_scope_enter,
                     (void *) delegate_visitor_scope_exit);
    
    muil_set_visitor(&delegate->base, SEMANTICS_MASK_BINARY_OP, (void *) delegate_visitor_binary_op_enter,
                     (void *) delegate_visitor_binary_op_exit);
    
    muil_set_visitor(&delegate->base, SEMANTICS_MASK_REFERENCE, (void *) delegate_visitor_reference_enter,
                     (void *) delegate_visitor_reference_exit);
    
    muil_set_visitor(&delegate->base, SEMANTICS_MASK_FUNCTION_CALL, (void *) delegate_visitor_function_call_enter,
                     (void *) delegate_visitor_function_call_exit);
    
    muil_set_visitor(&delegate->base, SEMANTICS_MASK_TYPE, (void *) delegate_visitor_type_enter,
                     (void *) delegate_visitor_type_exit);
    delegate->head = null;
    delegate->tail = null;
}

