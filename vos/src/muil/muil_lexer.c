//
// Created by jwraynor on 2/15/2024.
//
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "muil_lexer.h"
#include "platform/platform.h"

static Token makeToken(ProgramSource *result, TokenType type, const char *start, u32 length, u32 line, u32 column);

static void ensureCapacity(ProgramSource *result);

static Token errorToken(ProgramSource *result, const char *message, u32 line, u32 column);

static void addToken(ProgramSource *result, Token token);

static int isAlpha(char c);

static int isDigit(char c);

static void lexString(ProgramSource *result, const char **current, u32 *line, u32 *column);

static void lexNumber(ProgramSource *result, const char **current, u32 *line, u32 *column);

static void lexIdentifier(ProgramSource *result, const char **current, u32 *line, u32 *column);

static void skipComment(ProgramSource *result, const char **current, u32 *line);

const char *get_token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF:return "EOF";
        case TOKEN_ERROR:return "Error";
        case TOKEN_IDENTIFIER:return "Identifier";
        case TOKEN_STRING:return "String";
        case TOKEN_NUMBER:return "Number";
        case TOKEN_LPAREN:return "Left Parenthesis";
        case TOKEN_RPAREN:return "Right Parenthesis";
        case TOKEN_LBRACE:return "Left Brace";
        case TOKEN_RBRACE:return "Right Brace";
        case TOKEN_LT:return "Less Than";
        case TOKEN_GT:return "Greater Than";
        case TOKEN_DOT:return "Dot";
        case TOKEN_COMMA:return "Comma";
        case TOKEN_IMPORT:return "Import";
        case TOKEN_COMPONENT:return "Component";
            // Add more token types here as needed
        case TOKEN_COLON:return "Colon";
        case TOKEN_DELIMITER:return "Delimiter";
        case TOKEN_EQUALS:return "Equals";
        case TOKEN_PIPE:return "Pipe";
        case TOKEN_LBRACKET:return "Left Bracket";
        case TOKEN_RBRACKET:return "Right Bracket";
        case TOKEN_STAR:return "Asterisk";
        case TOKEN_SLASH:return "Slash";
        case TOKEN_PERCENT:return "Percent";
        case TOKEN_AMPERSAND:return "Ampersand";
        case TOKEN_BANG:return "Bang";
        case TOKEN_QUESTION:return "Question";
        case TOKEN_PLUS:return "Plus";
        case TOKEN_MINUS:return "Minus";
        default:return "Unknown";
    }
}

// Dynamically allocate or resize the token array
static Token *lastToken = NULL;


// LexerResult lexInput implementation
ProgramSource lexer_analysis_from_mem(const char *source, size_t length) {
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
            case '*' :addToken(&result, makeToken(&result, TOKEN_STAR, start, 1, line, column));
                break;
            case '/' :addToken(&result, makeToken(&result, TOKEN_SLASH, start, 1, line, column));
                break;
            case '%' :addToken(&result, makeToken(&result, TOKEN_PERCENT, start, 1, line, column));
                break;
            case '&' :addToken(&result, makeToken(&result, TOKEN_AMPERSAND, start, 1, line, column));
                break;
            case '!' :addToken(&result, makeToken(&result, TOKEN_BANG, start, 1, line, column));
                break;
            case '?' :addToken(&result, makeToken(&result, TOKEN_QUESTION, start, 1, line, column));
                break;
            case '+' :addToken(&result, makeToken(&result, TOKEN_PLUS, start, 1, line, column));
                break;
            case '-' :addToken(&result, makeToken(&result, TOKEN_MINUS, start, 1, line, column));
                break;
            case '<' :addToken(&result, makeToken(&result, TOKEN_LT, start, 1, line, column));
                break;
            case '>' :addToken(&result, makeToken(&result, TOKEN_GT, start, 1, line, column));
                break;
            case '.' :addToken(&result, makeToken(&result, TOKEN_DOT, start, 1, line, column));
                break;
            case '(':addToken(&result, makeToken(&result, TOKEN_LPAREN, start, 1, line, column));
                break;
            case ')':addToken(&result, makeToken(&result, TOKEN_RPAREN, start, 1, line, column));
                break;
            case '{':addToken(&result, makeToken(&result, TOKEN_LBRACE, start, 1, line, column));
                break;
            case '}':addToken(&result, makeToken(&result, TOKEN_RBRACE, start, 1, line, column));
                break;
            case '[':addToken(&result, makeToken(&result, TOKEN_LBRACKET, start, 1, line, column));
                break;
            case ']':addToken(&result, makeToken(&result, TOKEN_RBRACKET, start, 1, line, column));
                break;
            case ',':addToken(&result, makeToken(&result, TOKEN_COMMA, start, 1, line, column));
                break;
            case ':':addToken(&result, makeToken(&result, TOKEN_COLON, start, 1, line, column));
                break;
            case '=':addToken(&result, makeToken(&result, TOKEN_EQUALS, start, 1, line, column));
                break;
            case '|':addToken(&result, makeToken(&result, TOKEN_PIPE, start, 1, line, column));
                break;
            case '"':lexString(&result, &current, &line, &column);
                break;
            case ';':
            case '\n':
                // only add if the last token was not a delimiter
                if (lastToken == NULL || lastToken->type != TOKEN_DELIMITER)
                    addToken(&result, makeToken(&result, TOKEN_DELIMITER, start, 1, line, column));
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
                    addToken(&result, errorToken(&result, "Unexpected character.", line, column));
                }
                break;
        }
    }
    
    // Add EOF token at the end
    addToken(&result, makeToken(&result, TOKEN_EOF, current, 0, line, (u32) (current - lineStart) + 1));
    return result;
}


// Create a new token and add it to the lexer result
static Token makeToken(ProgramSource *result, TokenType type, const char *start, u32 length, u32 line, u32 column) {
    Token token = {type, start, length, line, column};
    return token;
}

// Create a new error token
static Token errorToken(ProgramSource *result, const char *message, u32 line, u32 column) {
    // Construct a more detailed error message
    char *detailedMessage = platform_allocate(256,
                                              false); // Ensure this buffer is appropriately sized for your error messages
    snprintf(detailedMessage, 256, "Error at line %d, column %d: %s", line, column, message);
    
    Token token = {TOKEN_ERROR, detailedMessage, strlen(detailedMessage), line, column};
    return token;
}

// Add a token to the lexer result
static void addToken(ProgramSource *result, Token token) {
    ensureCapacity(result);
    result->tokens[result->count++] = token;
    lastToken = &result->tokens[result->count - 1];
//    printf("Adding token: %s at line: %d\n", getTokenTypeName(token.type), token.line);
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
        addToken(result, errorToken(result, "Unterminated string.", *line, *column));
        return;
    }
    
    // Length calculation should account for the content excluding the surrounding quotes
    u32 length = *current - start;
    
    // Create the string token
    addToken(result, makeToken(result, TOKEN_STRING, start, length, *line, *column - length));
    
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
    
    addToken(result, makeToken(result, TOKEN_NUMBER, start, *current - start, *line, *column));
}

static int checkKeyword(const char *start, size_t length, const char *keyword, TokenType type) {
    return length == strlen(keyword) && strncmp(start, keyword, length) == 0 ? type : TOKEN_IDENTIFIER;
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
    TokenType type = TOKEN_IDENTIFIER; // Default to identifier
    
    // Keyword recognition
    if (length == 4 && checkKeyword(start, length, "true", TOKEN_TRUE) == TOKEN_TRUE) {
        type = TOKEN_TRUE;
    } else if (length == 5 && checkKeyword(start, length, "false", TOKEN_FALSE) == TOKEN_FALSE) {
        type = TOKEN_FALSE;
    } else if (length == 6 && checkKeyword(start, length, "import", TOKEN_IMPORT) == TOKEN_IMPORT) {
        type = TOKEN_IMPORT;
    } else if (length == 9 && checkKeyword(start, length, "component", TOKEN_COMPONENT) == TOKEN_COMPONENT) {
        type = TOKEN_COMPONENT;
    }
    // Add more keywords as necessary
    
    addToken(result, makeToken(result, type, start, length, *line, startColumn));
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
        Token token = result->tokens[i];
        // Don't print the \n value for the delimiter token
        if (token.type == TOKEN_DELIMITER) {
            int written = snprintf(buffer + offset, bufferSize - offset, "Token: %s, Line: %d, Column: %d\n",
                                   get_token_type_name(token.type), token.line, token.column);
            if (written < 0) {
                platform_free(buffer, false);
                return NULL; // Writing error
            }
            offset += written; // Update offset
            continue;
        }
        const char *tokenType = get_token_type_name(token.type);
        int written = snprintf(buffer + offset, bufferSize - offset, "Token: %s, Value: '%.*s', Line: %d, Column: %d\n",
                               tokenType, (int) token.length, token.start, token.line, token.column);
        if (written < 0) {
            platform_free(buffer, false);
            return NULL; // Writing error
        }
        offset += written; // Update offset
    }
    
    return buffer; // Caller is responsible for freeing this buffer
}

