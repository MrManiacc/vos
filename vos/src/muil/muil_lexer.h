//
// Created by jwraynor on 2/15/2024.
//
#pragma once

#include "defines.h"


// Token types
typedef enum {
    TOKEN_IMPORT,       // 'Import' keyword
    TOKEN_COMPONENT,    // 'Component' keyword
    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    TOKEN_LBRACKET,     // [
    TOKEN_RBRACKET,     // ]
    TOKEN_LBRACE,       // {
    TOKEN_RBRACE,       // }
    TOKEN_DOT,          // .
    TOKEN_DELIMITER, // Represents a generic delimiter (new line or semicolon)
    TOKEN_COMMA,        // ,
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_STAR,         // *
    TOKEN_SLASH,        // /
    TOKEN_PERCENT,      // %
    TOKEN_AMPERSAND,    // &
    TOKEN_PIPE,         // |
    TOKEN_BANG,         // !
    TOKEN_QUESTION,     // ?
    TOKEN_COLON,        // :
    TOKEN_SEMICOLON,    // ;
    TOKEN_LT,           // <
    TOKEN_GT,           // >
    TOKEN_EQUALS,       // =
    TOKEN_LE,           // <=
    TOKEN_GE,           // >=
    TOKEN_EQ,           // ==
    TOKEN_NEQ,          // !=
    TOKEN_AND,          // &&
    TOKEN_OR,           // ||
    TOKEN_ARROW,        // -> (Could be used for mappings or lambda expressions)
    TOKEN_STRING,       // String literals
    TOKEN_NUMBER,       // Numbers (integers and floats)
    TOKEN_TRUE,         // true
    TOKEN_FALSE,        // false
    TOKEN_NULL,         // null
    TOKEN_ON,           // Event handler prefix (e.g., onClick, onChange)
    TOKEN_BIND,         // Data-binding keyword or similar concept
    TOKEN_IDENTIFIER,   // Identifiers (variable names, function names, etc.)
    TOKEN_ERROR,        // Error token
    TOKEN_EOF           // End of file
} TokenType;

// Token structure
typedef struct {
    TokenType type; // Token type
    const char *start; // Start of the token in the input string
    u32 length; // Length of the token
    u32 line; // Line number for error reporting
    u32 column; // Column number for error reporting
} Token;

// Lexer output structure
typedef struct ProgramSource {
    Token *tokens; // Dynamic array of tokens
    size_t count; // Number of tokens
    size_t capacity; // Capacity of the tokens array
    // Any additional fields for error handling, etc.
} ProgramSource;

/**
 * @brief This function performs lexical analysis on the provided source code and returns the result.
 *
 * The lexer_analysis_from_mem function tokenizes the given source code into a sequence of tokens.
 *
 * @param source The source code to be analyzed.
 * @param length The length of the source code.
 * @return The result of the lexical analysis, which includes the list of tokens and any errors encountered.
 */
ProgramSource lexer_analysis_from_mem(const char *source, size_t length);



/**
 * @brief Prints the tokens in the given LexerResult.
 *
 * This function prints the tokens stored in the LexerResult structure to the console.
 * Each token is printed on a new line.
 *
 * @param result A pointer to the LexerResult structure containing the tokens.
 */
char *lexer_dump_tokens(ProgramSource *result);

/**
 * @brief Returns the name of the given token type.
 *
 * This function takes a TokenType as input and returns the corresponding
 * name of the token type. If the token type is not recognized, "Unknown"
 * is returned.
 *
 * @param type The token type.
 * @return The name of the token type.
 */
const char *get_token_type_name(TokenType type);