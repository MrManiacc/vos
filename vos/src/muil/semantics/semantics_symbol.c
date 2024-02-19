/**
 * Created by jraynor on 2/18/2024.
 */

#include "muil/muil.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "containers/dict.h"


typedef struct Scope {
    Dict *symbols;
    struct Scope *parent;
    char *name;
} Scope;


void symtab_program_enter(SymtabPass *visitor, ProgramAST *node);

void symtab_program_exit(SymtabPass *visitor, ProgramAST *node);


void symtab_component_enter(SymtabPass *visitor, CompoundDeclaration *node);

void symtab_component_exit(SymtabPass *visitor, CompoundDeclaration *node);

void symtab_property_enter(SymtabPass *visitor, PropertyDeclaration *node);

void symtab_property_exit(SymtabPass *visitor, PropertyDeclaration *node);

void symtab_assign_enter(SymtabPass *visitor, PropertyAssignmentNode *node);

void symtab_assign_exit(SymtabPass *visitor, PropertyAssignmentNode *node);

implement_pass(SymtabPass, Scope *scope, {
    pass->scope = vnew(Scope);
    pass->scope->name = "global";
    pass->scope->symbols = dict_new();
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_COMPONENT, symtab_component_enter, symtab_component_exit);
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_PROGRAM, symtab_program_enter, symtab_program_exit);
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_PROPERTY, symtab_property_enter, symtab_property_exit);
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_ASSIGNMENT, symtab_assign_enter, symtab_assign_exit);
}, {
                   vdelete(pass->scope);
               });

void symtab_property_enter(SymtabPass *visitor, PropertyDeclaration *node) {
    ASTNode *ast_node = parser_get_node(node);
    if (!ast_node) {
        verror("Failed to get ast node for property %s", node->name);
        return;
    }
    if (!node->name) {
        verror("Property name is null");
        return;
    }
    if (dict_contains(visitor->scope->symbols, node->name)) {
        verror("A property with the name %s already exists in scope %s", node->name, visitor->scope->name);
        return;
    }
    dict_set(visitor->scope->symbols, node->name, ast_node);
}

void symtab_assign_enter(SymtabPass *visitor, PropertyAssignmentNode *node) {
//    ASTNode *ast_node = parser_get_node(node);
//    if (!ast_node) {
//        verror("Failed to get ast node for assignment");
//        return;
//    }
//    if (!node->assignee) {
//        verror("Assignee is null");
//        return;
//    }
//    ASTNode *assignee = node->assignee;
//    if (assignee->nodeType != AST_REFERENCE) {
//        return;
//    }
//    ReferenceNode ref = node->assignee->data.reference;
//    if (!ref.name) {
//        verror("Reference name is null");
//        return;
//    }
//    if (dict_contains(visitor->scope->symbols, ref.name)) {
//        verror("A symbol with the name %s already exists in scope %s", ref.name, visitor->scope->name);
//        return;
//    }
//    dict_set(visitor->scope->symbols, ref.name, ast_node);
}

void symtab_assign_exit(SymtabPass *visitor, PropertyAssignmentNode *node) {

}

void symtab_property_exit(SymtabPass *visitor, PropertyDeclaration *node) {

}


void symtab_component_enter(SymtabPass *visitor, CompoundDeclaration *node) {
    ASTNode *ast_node = parser_get_node(node);
    if (!ast_node) {
        verror("Failed to get ast node for component %s", node->name);
        return;
    }
    // First we define the scope for the component
    Scope *parent_scope = visitor->scope;

    if (!parent_scope) {
        verror("Parent scope is null for component %s", node->name);
        return;
    }
    if (!node->name) {
        verror("Component name is null");
        return;
    }
    if (dict_contains(parent_scope->symbols, node->name)) {
        verror("A component with the name %s already exists in scope %s", node->name, parent_scope->name);
        return;
    }

    // Define the ast node in the symbol table
    dict_set(parent_scope->symbols, node->name, ast_node);
    // Push a new scope here
    Scope *scope = vnew(Scope);
    scope->name = node->name;
    scope->symbols = dict_new();
    scope->parent = parent_scope;
    visitor->scope = scope;
    ast_node->userData = scope;
//    vinfo("Successfully define component in symbol table: %s", node->name);
}

void symtab_component_exit(SymtabPass *visitor, CompoundDeclaration *node) {
    //Pop the scope here
    Scope *parent = visitor->scope->parent;
    if (parent) visitor->scope = parent;
}

void print_scope(Scope *scope) {
    if (!scope) return;
    DictIter iter = dict_iterator(scope->symbols);
    while (dict_next(&iter)) {
        Entry *entry = iter.entry;
        ASTNode *ast_node = entry->value;
        if (!ast_node) {
            vwarn(" failed to retrieve symbol Symbol: %s", entry->key);
            continue;
        }
        vinfo("Scope [%s]-\n%s", scope->name, parser_dump_node(ast_node));
        if (ast_node->nodeType == AST_COMPONENT_DECLARE) {
            print_scope(ast_node->userData);
        }
    }
}

void symtab_program_exit(SymtabPass *visitor, ProgramAST *node) {
    Scope *root = visitor->scope;
    while (root->parent) root = root->parent;
    print_scope(visitor->scope);
}

void symtab_program_enter(SymtabPass *visitor, ProgramAST *node) {

}

