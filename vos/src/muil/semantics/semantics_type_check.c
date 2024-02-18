/**
 * Created by jraynor on 2/18/2024.
 */

#include "muil/muil.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "containers/dict.h"


void type_check_enter_program(TypePass *visitor, ProgramAST *node) {
}

void type_check_exit_program(TypePass *visitor, ProgramAST *node) {
}


implement_pass(TypePass, Dict *symbols, {
    if (!free) {
        pass->symbols = dict_new();
        pass->base.type_mask = 0;
        muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_PROGRAM, type_check_enter_program,
                         type_check_exit_program);
    } else {
        dict_destroy(pass->symbols);
    }
});