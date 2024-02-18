/**
 * Created by jraynor on 2/15/2024.
 */
#pragma once

#include "muil_parser.h"
#include "muil_dump.h"
#include "muil_visitor.h"

// =============================================================================
// PassManager - Manages the execution of visitors
// =============================================================================
typedef struct PassManager PassManager;

// =============================================================================
// SymbolPass - Symbol resolution visitor
// =============================================================================
typedef struct SymbolPass SymbolPass;

// =============================================================================
// TypePass - Type checking visitor
// =============================================================================
typedef struct TypePass TypePass;

/**
 * @brief Creates a new pass manager.
 *
 * This function creates a new pass manager and returns a pointer to it.
 *
 * @return A pointer to the newly created pass manager.
 */
PassManager *muil_pass_manager_new();

/**
 * @brief Adds a visitor to the pass manager.
 *
 * This function adds a visitor to the pass manager. The visitor will be executed
 * when the pass manager is run.
 *
 * @param manager   The pass manager to which the visitor will be added.
 * @param visitor   The visitor to be added to the pass manager.
 */
void muil_pass_manager_add(PassManager *manager, ASTVisitor visitor);

/**
 * @brief Runs the pass manager on the given ProgramAST.
 *
 * This function runs the pass manager on the given ProgramAST. The pass manager
 * will execute all visitors that have been added to it.
 *
 * @param manager   The pass manager to be run.
 * @param root      The ProgramAST on which the pass manager will be run.
 */
void muil_pass_manager_run(PassManager *manager, ProgramAST *root);

/**
 * @brief Destroys the given pass manager.
 *
 * This function destroys the given pass manager and
 * frees the memory associated with it.
 *
 *  @param manager  The pass manager to be destroyed.
 */
void muil_pass_manager_destroy(PassManager *manager);

// =============================================================================
//                             Symbol Pass
// =============================================================================
VAPI ASTVisitor muil_pass_symbol_new();

// =============================================================================
//                             Type Pass
// =============================================================================
VAPI ASTVisitor muil_pass_type_check_new();


/**
 * @brief A type definition. This is used to define a new type in the symbol table.
 */
typedef struct Type Type;


/**
 * @breif defines a new type in the symbol table
 */
b8 muil_define_type(TypePass *visitor, Type *type);


