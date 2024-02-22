/**
 * Created by jraynor on 2/19/2024.
 */
#pragma once

#include "lib/defines.h"


// Token types
typedef enum {
    TOK_TYPE,         // 'Import' keyword
    TOK_FOR,          // 'for' keyword
    TOK_WHILE,        // 'while' keyword
    TOK_WHEN,        // 'when' keyword
    TOK_LPAREN,       // (
    TOK_RPAREN,       // )
    TOK_LBRACKET,     // [
    TOK_RBRACKET,     // ]
    TOK_LBRACE,       // {
    TOK_RBRACE,       // }
    TOK_DOT,          // .
    TOK_DELIMITER, // Represents a generic delimiter (new line or semicolon)
    TOK_COMMA,        // ,
    TOK_PLUS,         // +
    TOK_MINUS,        // -
    TOK_STAR,         // *
    TOK_SLASH,        // /
    TOK_PERCENT,      // %
    TOK_AMPERSAND,    // &
    TOK_PIPE,         // |
    TOK_BANG,         // !
    TOK_QUESTION,     // ?
    TOK_COLON,        // :
    TOK_SEMICOLON,    // ;
    TOK_LT,           // <
    TOK_GT,           // >
    TOK_EQUALS,       // =
    TOK_LE,           // <=
    TOK_GE,           // >=
    TOK_EQ,           // ==
    TOK_NEQ,          // !=
    TOK_AND,          // &&
    TOK_OR,           // ||
    TOK_ARROW,        // -> (Could be used for mappings or lambda expressions)
    TOK_STRING,       // String literals
    TOK_NUMBER,       // Numbers (integers and floats)
    TOK_TRUE,         // true
    TOK_FALSE,        // false
    TOK_NULL,         // null
    TOK_ON,           // Event handler prefix (e.g., onClick, onChange)
    TOK_BIND,         // Data-binding keyword or similar concept
    TOK_IDENTIFIER,   // Identifiers (variable names, function names, etc.)
    TOK_ERROR,        // Error token
    TOK_EOF           // End of file
} TokType;

// Token structure
typedef struct {
    TokType type; // Token type
    const char *start; // Start of the token in the input string
    u32 length; // Length of the token
    u32 line; // Line number for error reporting
    u32 column; // Column number for error reporting
} Token;

// Lexer output structure
typedef struct ProgramSource {
    Token *tokens; // Dynamic array of tokens
    u32 count; // Number of tokens
    u32 capacity; // Capacity of the tokens array
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
ProgramSource lexer_analysis_from_mem(const char *source, u32 length);


/**
 * @brief Prints the tokens in the given LexerResult.
 *
 * This function prints the tokens stored in the LexerResult structure to the console.
 * Each token is printed on a new line.
 *
 * @param result A pointer to the LexerResult structure containing the tokens.
 */
char *lexer_dump_tokens(ProgramSource  *result);

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
const char *lexer_token_type_name(TokType type);