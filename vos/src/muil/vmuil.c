/**
 * Created by jraynor on 2/15/2024.
 */
#include "vmuil.h"

VAPI Visitor muil_create_visitor(
        void (*visitComponent)(ASTNode *node, Visitor *visitor),
        void (*visitProperty)(ASTNode *node, Visitor *visitor),
        void (*visitType)(ASTNode *node, Visitor *visitor)) {
    Visitor visitor;
    visitor.visit_component = visitComponent;
    visitor.visit_property = visitProperty;
    visitor.visit_type = visitType;
    return visitor;
}

VAPI void _visit_component(ASTNode *node, Visitor *visitor) {
    visitor->visit_component(node, visitor);
    
    // Visit the properties of the component
    Property *property = node->data.component.properties;
    while (property != null && visitor->visit_property != null) {
        visitor->visit_property((ASTNode *) property, visitor);
        property = property->next;
    }
}

VAPI void muil_traverse_ast(ASTNode *node, Visitor *visitor) {
    switch (node->type) {
        case AST_COMPONENT:
            _visit_component(node, visitor);
            break;
        case AST_PROPERTY:
            visitor->visit_property(node, visitor);
            break;
        case AST_TYPE:
            visitor->visit_type(node, visitor);
            break;
        default:
            break;
    }
}

VAPI void muil_traverse(ProgramAST *program, Visitor *visitor) {
    ASTNode *node = program->statements;
    while (node != null) {
        muil_traverse_ast(node, visitor);
        node = node->next;
    }
}

