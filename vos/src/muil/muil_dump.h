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

/**
 * @brief Generates a string representation of the given Type object.
 *
 * This function takes a Type object and converts it into a string representation.
 * The resulting string contains information about the type, such as its name and
 * any associated attributes or modifiers.
 *
 * @param type The Type object to be converted into a string representation.
 *
 * @return A dynamically allocated string containing the string representation of the Type object.
 *         The caller is responsible for freeing the memory allocated for the string.
 *         Returns NULL if the type is NULL or if there is an error during the conversion process.
 *
 * @note The caller is responsible for freeing the memory allocated for the returned string.
 *
 * Example usage:
 * @code{.c}
 * Type *myType = ...; // Create a Type object
 * char *typeString = parser_dump_type(myType);
 * printf("Type string: %s\n", typeString);
 * free(typeString); // Free the memory allocated for the string
 * @endcode
 */
char *parser_dump_type(TypeSymbol *type);