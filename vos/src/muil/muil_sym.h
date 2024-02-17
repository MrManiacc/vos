/**
 * Created by jraynor on 2/17/2024.
 */
#pragma once

#include "defines.h"
#include "muil_parser.h"
#include "muil_ir.h"

typedef struct TypeCheckingVisitor TypeCheckingVisitor;

/**
 * @brief Creates a new TypeCheckingVisitor object.
 *
 * @return A new TypeCheckingVisitor object.
 */
VAPI TypeCheckingVisitor *typechecker_new();

/**
 * @brief Frees the TypeCheckingVisitor object
 *
 * This function frees the memory occupied by the TypeCheckingVisitor object and its associated data.
 *
 * @param visitor The TypeCheckingVisitor object to be freed
 */
VAPI void type_checker_free(TypeCheckingVisitor *visitor);