/**
 * Created by jraynor on 2/19/2024.
 */
#include "lexer.h"
#include "lib/string.h"
#include "lib/platform.h"


//
// Created by jwraynor on 2/15/2024.
//
#include <ctype.h>
#include <string.h>
#include <stdio.h>

static Token makeTok(ProgramSource *result, TokType type, const char *start, u32 length, u32 line, u32 column);

static void ensureCapacity(ProgramSource *result);

static Token errorTok(ProgramSource *result, const char *message, u32 line, u32 column);

static void addTok(ProgramSource *result, Token token);

static int isAlpha(char c);

static int isDigit(char c);

static void lexString(ProgramSource *result, const char **current, u32 *line, u32 *column);

static void lexNumber(ProgramSource *result, const char **current, u32 *line, u32 *column);

static void lexIdentifier(ProgramSource *result, const char **current, u32 *line, u32 *column);

static void skipComment(ProgramSource *result, const char **current, u32 *line);

const char *lexer_token_type_name(TokType type) {
    switch (type) {
        case TOK_EOF:return "EOF";
        case TOK_ERROR:return "Error";
        case TOK_IDENTIFIER:return "Identifier";
        case TOK_STRING:return "String";
        case TOK_ARROW:return "Arrow";
        case TOK_NUMBER:return "Number";
        case TOK_LPAREN:return "Left Parenthesis";
        case TOK_RPAREN:return "Right Parenthesis";
        case TOK_LBRACE:return "Left Brace";
        case TOK_RBRACE:return "Right Brace";
        case TOK_LT:return "Less Than";
        case TOK_GT:return "Greater Than";
        case TOK_DOT:return "Dot";
        case TOK_COMMA:return "Comma";
        case TOK_WHILE:return "While";
        case TOK_FOR:return "For";
        case TOK_WHEN:return "When";
        case TOK_TYPE:return "Type";
            // Add more token types here as needed
        case TOK_COLON:return "Colon";
        case TOK_DELIMITER:return "Delimiter";
        case TOK_EQUALS:return "Equals";
        case TOK_PIPE:return "Pipe";
        case TOK_LBRACKET:return "Left Bracket";
        case TOK_RBRACKET:return "Right Bracket";
        case TOK_STAR:return "*";
        case TOK_SLASH:return "/";
        case TOK_PERCENT:return "%";
        case TOK_AMPERSAND:return "&";
        case TOK_BANG:return "!";
        case TOK_QUESTION:return "?";
        case TOK_PLUS:return "+";
        case TOK_MINUS:return "-";
        default:return "Unknown";
    }
}

// Dynamically allocate or resize the token array
static Token *lastToken = NULL;


// LexerResult lexInput implementation
ProgramSource lexer_analysis_from_mem(const char *source, u32 length) {
    ProgramSource result = {NULL, 0, 0};
    const char *start = source;
    const char *current = source;
    const char *lineStart = source; // Track the start of the current line
    int line = 1;
    while (current - source < length) {
        start = current; // Start of the new token
        char c = *current++;
        u32 column = (u32) (current - lineStart) + 1;
        switch (c) {
            case '*' :addTok(&result, makeTok(&result, TOK_STAR, start, 1, line, column));
                break;
            case '/' :
                if (*current == '/' || *current == '*')
                    skipComment(&result, &current, &line);
                else
                    // Add the token (it's a slash, not a comment
                    addTok(&result, makeTok(&result, TOK_SLASH, start, 1, line, column));
                break;
            case '%' :addTok(&result, makeTok(&result, TOK_PERCENT, start, 1, line, column));
                break;
            case '&' :addTok(&result, makeTok(&result, TOK_AMPERSAND, start, 1, line, column));
                break;
            case '!' :addTok(&result, makeTok(&result, TOK_BANG, start, 1, line, column));
                break;
            case '?' :addTok(&result, makeTok(&result, TOK_QUESTION, start, 1, line, column));
                break;
            case '+' :addTok(&result, makeTok(&result, TOK_PLUS, start, 1, line, column));
                break;
            case '-' :
                //Check for arrow
                if (*current == '>') {
                    addTok(&result, makeTok(&result, TOK_ARROW, start, 2, line, column));
                    current++;
                } else addTok(&result, makeTok(&result, TOK_MINUS, start, 1, line, column));
                break;
            case '<' :addTok(&result, makeTok(&result, TOK_LT, start, 1, line, column));
                break;
            case '>' :addTok(&result, makeTok(&result, TOK_GT, start, 1, line, column));
                break;
            case '.' :addTok(&result, makeTok(&result, TOK_DOT, start, 1, line, column));
                break;
            case '(':addTok(&result, makeTok(&result, TOK_LPAREN, start, 1, line, column));
                break;
            case ')':addTok(&result, makeTok(&result, TOK_RPAREN, start, 1, line, column));
                break;
            case '{':addTok(&result, makeTok(&result, TOK_LBRACE, start, 1, line, column));
                break;
            case '}':addTok(&result, makeTok(&result, TOK_RBRACE, start, 1, line, column));
                break;
            case '[':addTok(&result, makeTok(&result, TOK_LBRACKET, start, 1, line, column));
                break;
            case ']':addTok(&result, makeTok(&result, TOK_RBRACKET, start, 1, line, column));
                break;
            case ',':addTok(&result, makeTok(&result, TOK_COMMA, start, 1, line, column));
                break;
            case ':':addTok(&result, makeTok(&result, TOK_COLON, start, 1, line, column));
                break;
            case '=':addTok(&result, makeTok(&result, TOK_EQUALS, start, 1, line, column));
                break;
            case '|':addTok(&result, makeTok(&result, TOK_PIPE, start, 1, line, column));
                break;
            case '"':lexString(&result, &current, &line, &column);
                break;
            case ';':
            case '\n':
                // only add if the last token was not a delimiter
                if (lastToken == NULL || lastToken->type != TOK_DELIMITER)
                    addTok(&result, makeTok(&result, TOK_DELIMITER, start, 1, line, column));
                if (c == '\n') {
                    line++;
                    lineStart = current; // Point to the start of the new line
                    column = 0; // Reset column for the new line
                }
                break;
            default:
                if (isDigit(c)) {
                    lexNumber(&result, &current, &line, &column);
                } else if (isAlpha(c)) {
                    lexIdentifier(&result, &current, &line, &column);
                } else if (c == '/' && (*current == '/' || *current == '*')) {
                    skipComment(&result, &current, &line);
                } else if (isspace(c)) {
                    // Ignore whitespace
                } else {
                    addTok(&result, errorTok(&result, "Unexpected character.", line, column));
                }
                break;
        }
    }
    
    // Add EOF token at the end
    addTok(&result, makeTok(&result, TOK_EOF, current, 0, line, (u32) (current - lineStart) + 1));
    return result;
}


// Create a new token and add it to the lexer result
static Token makeTok(ProgramSource *result, TokType type, const char *start, u32 length, u32 line, u32 column) {
    Token token = {type, start, length, line, column};
    return token;
}

// Create a new error token
static Token errorTok(ProgramSource *result, const char *message, u32 line, u32 column) {
    // Construct a more detailed error message
    char *detailedMessage = string_allocate_empty(
            256); // Ensure this buffer is appropriately sized for your error messages
    snprintf(detailedMessage, 256, "Error at line %d, column %d: %s", line, column, message);
    
    Token token = {TOK_ERROR, detailedMessage, strlen(detailedMessage), line, column};
    return token;
}

// Add a token to the lexer result
static void addTok(ProgramSource *result, Token token) {
    ensureCapacity(result);
    result->tokens[result->count++] = token;
    lastToken = &result->tokens[result->count - 1];
//    printf("Adding token: %s at line: %d\n", getTokTypeName(token.type), token.line);
}

// Check if the character is alphabetic or an underscore
static int isAlpha(char c) {
    return isalpha(c) || c == '_';
}

// Check if the character is a digit
static int isDigit(char c) {
    return isdigit(c);
}

// Lex a string literal
static void lexString(ProgramSource *result, const char **current, u32 *line, u32 *column) {
    const char *start = *current; // Still points to the opening quote
    (*current)++; // Move past the opening quote to start capturing the string content
    
    while (**current != '"' && **current != '\0') {
        if (**current == '\n') {
            (*line)++;
            // Reset column for the new line if you're tracking columns within lines
        }
        (*current)++;
    }
    
    if (**current == '\0') {
        // Handle unterminated string
        addTok(result, errorTok(result, "Unterminated string.", *line, *column));
        return;
    }
    
    // Length calculation should account for the content excluding the surrounding quotes
    u32 length = *current - start;
    
    // Create the string token
    addTok(result, makeTok(result, TOK_STRING, start, length, *line, *column - length));
    
    (*current)++; // Skip the closing quote
    // Update column position here if necessary
}

// Lex a number (integer for simplicity, extend for floats)
// Extended lexNumber to handle floating-point numbers
static void lexNumber(ProgramSource *result, const char **current, u32 *line, u32 *column) {
    const char *start = *current - 1;
    while (isDigit(**current)) (*current)++;
    
    // Look for a fractional part
    if (**current == '.' && isDigit(*(*current + 1))) {
        (*current)++; // Consume the '.'
        while (isDigit(**current)) (*current)++;
    }
    
    addTok(result, makeTok(result, TOK_NUMBER, start, *current - start, *line, *column));
}

static int checkKeyword(const char *start, size_t length, const char *keyword, TokType type) {
    return length == strlen(keyword) && strncmp(start, keyword, length) == 0 ? type : TOK_IDENTIFIER;
}

// Enhanced lexIdentifier to distinguish keywords
static void lexIdentifier(ProgramSource *result, const char **current, u32 *line, u32 *column) {
    const char *start = *current - 1;
    u32 startColumn = *column - 1; // Adjust for the already incremented column
    
    while (isAlpha(**current) || isDigit(**current)) {
        (*current)++;
        (*column)++;
    }
    
    size_t length = *current - start;
    TokType type = TOK_IDENTIFIER; // Default to identifier
    
    // Keyword recognition
    if (length == 4 && checkKeyword(start, length, "true", TOK_TRUE) == TOK_TRUE) {
        type = TOK_TRUE;
    } else if (length == 5 && checkKeyword(start, length, "false", TOK_FALSE) == TOK_FALSE) {
        type = TOK_FALSE;
    } else if (length == 6 && checkKeyword(start, length, "while", TOK_WHILE) == TOK_WHILE) {
        type = TOK_WHILE;
    } else if (length == 9 && checkKeyword(start, length, "for", TOK_FOR) == TOK_FOR) {
        type = TOK_FOR;
    } else if (length == 4 && checkKeyword(start, length, "when", TOK_WHEN) == TOK_WHEN) {
        type = TOK_WHEN;
    } else if (length == 4 && checkKeyword(start, length, "type", TOK_TYPE) == TOK_TYPE) {
        type = TOK_TYPE;
    }
    
    // Add more keywords as necessary
    
    addTok(result, makeTok(result, type, start, length, *line, startColumn));
}

static void ensureCapacity(ProgramSource *result) {
    if (result->capacity == 0) {
        result->
                capacity = 8; // Initial capacity
        result->
                tokens = (Token *) platform_allocate(result->capacity * sizeof(Token), false);
    } else if (result->count >= result->capacity) {
        result->capacity *= 2;
        result->tokens = (Token *) platform_reallocate(result->tokens, result->capacity * sizeof(Token), false);
    }
}

// Function to skip single and multi-line comments
static void skipComment(ProgramSource *result, const char **current, u32 *line) {
    if (**current == '/') {
        // Single-line comment
        (*current)++;
        while (**current != '\n' && **current != '\0') (*current)++;
        if (**current == '\n') line++;
    } else if (**current == '*') {
        // Multi-line comment
        (*current)++;
        while (**current != '\0' && !(**current == '*' && *(*current + 1) == '/')) {
            if (**current == '\n') (*line)++;
            (*current)++;
        }
        if (**current != '\0') (*current) += 2; // Skip past the closing */
    }
}


char *lexer_dump_tokens(ProgramSource *result) {
// Estimate buffer size
    size_t bufferSize = 256; // Start with a reasonable size
    for (size_t i = 0; i < result->count; i++) {
        bufferSize += 64; // Assume each token description takes up to 64 characters
    }
    
    char *buffer = (char *) platform_allocate(bufferSize, false);
    if (buffer == NULL) return NULL; // Allocation failed
    
    size_t offset = 0; // Current offset in the buffer
    for (size_t i = 0; i < result->count; i++) {
        Token tok = result->tokens[i];
        // Don't print the \n value for the delimiter token
        if (tok.type == TOK_DELIMITER) {
            int written = snprintf(buffer + offset, bufferSize - offset, "Tok: %s, Line: %d, Column: %d\n",
                    lexer_token_type_name(tok.type), tok.line, tok.column);
            if (written < 0) {
                platform_free(buffer, false);
                return NULL; // Writing error
            }
            offset += written; // Update offset
            continue;
        }
        const char *tokenType = lexer_token_type_name(tok.type);
        int written = snprintf(buffer + offset, bufferSize - offset, "Tok: %s, Value: '%.*s', Line: %d, Column: %d\n",
                tokenType, (int) tok.length, tok.start, tok.line, tok.column);
        if (written < 0) {
            platform_free(buffer, false);
            return NULL; // Writing error
        }
        offset += written; // Update offset
    }
    
    return buffer; // Caller is responsible for freeing this buffer
}

