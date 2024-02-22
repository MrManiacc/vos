/**
 * Created by jraynor on 2/15/2024.
 */
#pragma once

#include "muil_parser.h"
#include "muil_dump.h"
#include "muil_visitor.h"
#include "lib/vmem.h"

// =============================================================================
// PassManager - Manages the execution of visitors
// =============================================================================
typedef struct PassManager PassManager;

typedef struct Scope {
    struct Dict *symbols;
    struct Scope *parent;
    char *name;
} Scope;
typedef enum PassExecutionType {
    PASS_EXECUTE_CONSECUTIVE,// Allows this pass to be ran consecutively, this means the pass will
    // wait until the previous pass has finished before running
    PASS_EXECUTE_PARALLEL, // Allows this pass to be ran in parallel, meaning that it's visit methods
    // will run right after the previous pass's given visitor method
    PASS_EXECUTE_CONCURRENT // Not yet implemented, will allow for multi-threaded execution. Similar to parallel
} PassExecutionType;

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
void muil_pass_manager_add(PassManager *manager, SemanticsPass *visitor, PassExecutionType type);

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
// SymbolPass - Symbol resolution visitor
// =============================================================================
// The symbol pass is responsible for resolving symbols in the AST.
// It is a visitor that is added to the pass manager and
// is executed before the type checking pass.
// =============================================================================
define_pass(SymtabPass);



// =============================================================================
// ReferencesPass - Reference resolution visitor
// =============================================================================
// The references pass is responsible for resolving references in the AST.
// It is a visitor that is added to the pass manager and
// is executed after the symbol resolution pass.
// =============================================================================
define_pass(ReferencesPass);

// =============================================================================
// TypePass - Type checking visitor
// =============================================================================
// The type pass is responsible for checking the types of all nodes in the AST.
// It is a visitor that is added to the pass manager and
// is executed after the symbol resolution pass.
// =============================================================================
define_pass(TypePass);
