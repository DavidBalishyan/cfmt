#ifndef TOKEN_H
#define TOKEN_H
#include <stddef.h>

typedef enum {
    TOKEN_EOF, TOKEN_NEWLINE, TOKEN_IDENTIFIER, TOKEN_KEYWORD, TOKEN_NUMBER, TOKEN_STRING, TOKEN_CHAR, TOKEN_PREPROCESSOR, TOKEN_COMMENT_LINE, TOKEN_COMMENT_BLOCK, TOKEN_OPEN_BRACE, TOKEN_CLOSE_BRACE, TOKEN_OPEN_PAREN, TOKEN_CLOSE_PAREN, TOKEN_OPEN_BRACKET, TOKEN_CLOSE_BRACKET, TOKEN_SEMICOLON, TOKEN_COMMA, TOKEN_DOT, TOKEN_ARROW, TOKEN_ELLIPSIS, TOKEN_OPERATOR, TOKEN_COLON, TOKEN_QUESTION, TOKEN_HASH, } token_type_t;

typedef struct {
    token_type_t type;
    const char *start;
    size_t length;
    int line;
    int col;
} token_t;

const char *token_type_name(token_type_t type);
#endif /* TOKEN_H */