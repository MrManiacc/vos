/**
 * Created by jraynor on 2/18/2024.
 */

#include "muil/muil.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "containers/dict.h"


struct SymbolPass {
    // The base visitor
    ASTVisitor base;
    // The symbol table
    Dict *symbolTable;
};


 void symbol_enter_program(SymbolPass  *visitor, ProgramAST *node) {
     vinfo("\n\t\t\t\t\t\t\t-> [Symbol pass] Entering program node")

}

 void symbol_exit_program(SymbolPass *visitor, ProgramAST *node) {
    vinfo("\n\t\t\t\t\t\t\t-> [Symbol pass] Exiting program node")
}

// Creates a new symbol pass object, setting up the base visitor
VAPI ASTVisitor muil_pass_symbol_new() {
    SymbolPass visitor;
    visitor.base.type_mask = 0;
    muil_set_visitor(&visitor.base, VISITOR_TYPE_MASK_PROGRAM, symbol_enter_program, symbol_exit_program);
    return visitor.base; // We could also cast this to an ASTVisitor and return it
}
