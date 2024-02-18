/**
 * Created by jraynor on 2/18/2024.
 */

#include "muil/muil.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "containers/dict.h"


void symtab_program_enter(SymtabPass *visitor, ProgramAST *node) {

}

void symtab_program_exit(SymtabPass *visitor, ProgramAST *node) {

}


implement_pass(SymtabPass, Dict *symbols, {
    if (!free) {
        pass->symbols = dict_new();
        pass->base.type_mask = 0;
        muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_PROGRAM, symtab_program_enter,
                         symtab_program_exit);
    } else {
        dict_destroy(pass->symbols);
    }
});