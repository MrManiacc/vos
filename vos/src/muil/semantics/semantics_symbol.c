/**
 * Created by jraynor on 2/18/2024.
 */

#include "muil/muil.h"
#include "lib/vmem.h"
#include "core/vlogger.h"
#include "containers/dict.h"


void symtab_ignored_enter(SymtabPass *visitor, void *node) {}

void symtab_program_exit(SymtabPass *visitor, ProgramAST *node);

void symtab_component_enter(SymtabPass *visitor, CompoundDeclaration *node);

void symtab_assign_exit(SymtabPass *visitor, PropertyAssignmentNode *node);

void symtab_component_exit(SymtabPass *visitor, CompoundDeclaration *node);

void symtab_property_enter(SymtabPass *visitor, PropertyDeclaration *node);


implement_pass(SymtabPass, Scope *scope, {
    pass->scope = vnew(Scope);
    pass->scope->name = "global";
    pass->scope->symbols = dict_new();
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_COMPONENT, symtab_component_enter, symtab_component_exit);
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_PROGRAM, symtab_ignored_enter, symtab_program_exit);
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_PROPERTY, symtab_property_enter, symtab_ignored_enter);
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_ASSIGNMENT, symtab_ignored_enter, symtab_assign_exit);
//    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_ASSIGNMENT, symtab_assign_enter, symtab_assign_exit);
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
    //Sets the scope for the node
    parser_get_node(node)->userData = visitor->scope;
    vinfo("Successfully define property in symbol table: %s", node->name);
}

void symtab_assign_exit(SymtabPass *visitor, PropertyAssignmentNode *node) {
    ASTNode *ast = parser_get_node(node);
    if (!ast) {
        verror("Failed to get ast node for assignment");
        return;
    }
    
    // We check if the reference is already defined in the symbol table
    if (dict_contains(visitor->scope->symbols, node->assignee->data.reference.name)) {
        //do nothing if it does, we'll type check it later
        return;
    }
    
    //If we're defining something and it doesn't exist, we'll define it here
    dict_set(visitor->scope->symbols, node->assignee->data.reference.name, ast);
    vinfo("Defined untyped reference in symbol table: %s", node->assignee->data.reference.name);
    //We could do an immediate type check here, but we'll wait until the type pass so that we're
    //not just checking against the current scope, but all scopes.
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
    vinfo("Successfully define component in symbol table: %s", node->name);
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
//        vinfo("Scope [%s]-\n%s", scope->name, parser_dump_node(ast_node));
        if (ast_node->nodeType == AST_COMPONENT_DECLARE) {
            print_scope(ast_node->userData);
        }
    }
}

void symtab_program_exit(SymtabPass *visitor, ProgramAST *node) {
    Scope *root = visitor->scope;
    while (root->parent) root = root->parent;
    // Here *should* be the start of the next pass.
    // Lets see if we have a next pass, and if we do let's set it's scope
    if (visitor->base.next) {
        //Save the pointer to the scope, this will be passed to the next pass
        visitor->base.userData = visitor->scope;
    }
}



