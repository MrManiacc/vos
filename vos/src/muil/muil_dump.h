/**
 * Created by jraynor on 2/17/2024.
 */
#pragma once

#include "muil_parser.h"

/**
 * @brief Generates a string representation of a ProgramAST.
 *
 * This function takes a ProgramAST pointer as input and returns a dynamically allocated
 * string representing the structure and content of the ProgramAST. The string is generated
 * in a format suitable for debugging or display purposes.
 *
 * @param result    A pointer to the ProgramAST structure to be converted to a string.
 * @return          A pointer to a dynamically allocated string representation of the ProgramAST.
 *                  The caller is responsible for freeing the memory when no longer needed.
 */
char *parser_dump(ProgramAST *result);

/**
 * @brief Generates a string representation of an ASTNode.
 *
 * This function takes an ASTNode pointer as input and returns a dynamically allocated
 * string representing the structure and content of the ASTNode. The string is generated
 * in a format suitable for debugging or display purposes.
 *
 * @param node      A pointer to the ASTNode structure to be converted to a string.
 * @return          A pointer to a dynamically allocated string representation of the ASTNode.
 *                  The caller is responsible for freeing the memory when no longer needed.
 */
char *parser_dump_node(ASTNode *node);