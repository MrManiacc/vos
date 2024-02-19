/**
 * Created by jraynor on 2/18/2024.
 */

#include "muil/muil.h"
#include "core/vmem.h"
#include "core/vlogger.h"
#include "containers/dict.h"

void reference_pass_ignore(ReferencesPass *visitor, void *node) {}

void reference_pass__program_enter(ReferencesPass *visitor, ProgramAST *node);

void reference_pass_reference_enter(ReferencesPass *visitor, ReferenceNode *node);

void reference_pass_reference_exit(ReferencesPass *visitor, ReferenceNode *node);

// The scpope is defined from the previous pass. This is the scope that will be used to resolve references.
implement_pass(ReferencesPass, Scope *scope, {
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_PROGRAM, reference_pass__program_enter,
            reference_pass_ignore);
    
    muil_set_visitor((SemanticsPass *) pass, SEMANTICS_MASK_REFERENCE, reference_pass_reference_enter,
            reference_pass_reference_exit);
}, {
    //Cleanup
});

void reference_pass__program_enter(ReferencesPass *visitor, ProgramAST *node) {
    // Picking up where we left off in the last pass, we can use the scope from the previous pass.
    // This is the scope that will be used to resolve references.
    visitor->scope = visitor->base.userData; // Assuming the previous pass set the userData to the scope. TODO: error check if needed?
}

Scope *get_node_scope(ASTNode *node) {
    if (!node) return null;
    if (node->nodeType == AST_COMPONENT_DECLARE || node->nodeType == AST_PROPERTY_DECLARE) {
        return node->userData;
    }
    verror("Failed to get scope for node %s", parser_dump_node(node));
    return null;
    // walk up the tree to find the scope
}


ASTNode *process_nested_references(ReferencesPass *visitor, ReferenceNode *node, Scope *startScope) {
    if (!node || !node->name) {
        verror("Invalid node or reference name is null");
        return null;
    }
    
    Scope *scope = startScope ? startScope : visitor->scope;
    ASTNode *resolvedNode = null;
    
    // Attempt to resolve the reference name in the current scope or any parent scope.
    while (scope != null) {
        if (dict_contains(scope->symbols, node->name)) {
            resolvedNode = dict_get(scope->symbols, node->name);
            break;
        }
        scope = scope->parent;
    }
    
    if (!resolvedNode) {
        return null;
    }
    
    // If there is a nested reference, continue the resolution process within the resolved scope.
    if (node->reference && node->reference->nodeType == AST_REFERENCE) {
        Scope *resolvedScope = get_node_scope(resolvedNode); // Fetch the scope of the resolved node.
        if (!resolvedScope) {
            verror("Failed to obtain scope for resolved node %s", node->name);
            return null;
        }
        // Recursively resolve the nested reference within the resolved scope.
        ASTNode *nestedResolvedNode = process_nested_references(visitor, &node->reference->data.reference,
                resolvedScope);
        
        // Here, instead of altering the original node's userData, you may need to handle the nesting differently.
        // For instance, setting the userData of the nested reference or adjusting how you use this data.
        if (nestedResolvedNode) {
            // Optionally, perform any necessary actions with nestedResolvedNode.
            // This could involve linking the resolved nodes or setting userData appropriately.
        }
    }
    
    // The following line may need adjustment based on how you intend to use the resolved information.
    parser_get_node(node)->userData = resolvedNode;
    
    return resolvedNode;
}

void reference_pass_reference_enter(ReferencesPass *visitor, ReferenceNode *node) {
    if (!node || !node->name) {
        verror("Invalid node or reference name is null");
        return;
    }
    ASTNode *resolvedNode = parser_get_node(node)->userData ? parser_get_node(node)->userData
                                                            : process_nested_references(visitor, node, visitor->scope);
    if (resolvedNode) {
        vinfo("Resolved reference %s to %s", node->name, parser_dump_node(resolvedNode));
        //TODO: Do we want to replace reference value with the resolved node? Or just set the userData?
    } else {
        verror("Failed to resolve reference %s", node->name);
    }
}

void reference_pass_reference_exit(ReferencesPass *visitor, ReferenceNode *node) {

}
