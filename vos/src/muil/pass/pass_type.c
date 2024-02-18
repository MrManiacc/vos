/**
 * Created by jraynor on 2/18/2024.
 */

#include "muil/muil.h"
#include "core/vmem.h"
#include "core/vlogger.h"

struct TypePass {
    // The base visitor must be first so we can cast to it
    ASTVisitor base;
};


void type_check_enter_program(TypePass *visitor, ProgramAST *node) {
    vinfo("\n\t\t\t\t\t\t\t-> [Type pass] Entering program node")
}

void type_check_exit_program(TypePass *visitor, ProgramAST *node) {
    vinfo("\n\t\t\t\t\t\t\t-> [Type pass] Exiting program node")
}

//muil_pass_type_check_new - Creates a new type pass object, setting up the base visitor
ASTVisitor muil_pass_type_check_new() {
    TypePass visitor;
    visitor.base.type_mask = 0;
    muil_set_visitor(&visitor.base, VISITOR_TYPE_MASK_PROGRAM, type_check_enter_program, type_check_exit_program);
    return visitor.base; // We could also cast this to an ASTVisitor and return it
}