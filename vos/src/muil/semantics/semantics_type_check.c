/**
 * Created by jraynor on 2/18/2024.
 */

#include "muil/muil.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "containers/dict.h"


void type_check_pass_ignore(TypePass *visitor, void *node) {}

void type_check_pass__program_enter(TypePass *visitor, ProgramAST *node);


void type_check_visit_assignment_enter(TypePass *visitor, PropertyAssignmentNode *node);

// The scpope is defined from the previous pass. This is the scope that will be used to resolve type_checks.
implement_pass(TypePass, Scope *scope, {
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_PROGRAM, type_check_pass__program_enter,
            type_check_pass_ignore);
    
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

void type_check_visit_assignment_enter(TypePass *visitor, PropertyAssignmentNode *node) {
    // Here we check if the type of the assignment is valid. We may need check the reference type and the assignment type
    // if it's a reference type, we need to resolve the reference and check the type of the resolved node.
    // if it's a literal type, we need to check the type of the literal.
    // if it's a function call, we need to check the return type of the function.
    
    // We can use the visitor->scope to resolve the reference.
    
    vinfo("Type checking assignment: %s", parser_dump_node(node->assignee));
    // If the property does have a type, we need to make sure the assignment type matches the property type.
    
    //We'll need to do some complicated type checking here because of our union, intersection, and tuple types.
}


