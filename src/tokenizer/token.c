#include "token.h"

const char *token_type_name(token_type_t type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_NEWLINE: return "NEWLINE";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_CHAR: return "CHAR";
        case TOKEN_PREPROCESSOR: return "PREPROCESSOR";
        case TOKEN_COMMENT_LINE: return "COMMENT_LINE";
        case TOKEN_COMMENT_BLOCK: return "COMMENT_BLOCK";
        case TOKEN_OPEN_BRACE: return "OPEN_BRACE";
        case TOKEN_CLOSE_BRACE: return "CLOSE_BRACE";
        case TOKEN_OPEN_PAREN: return "OPEN_PAREN";
        case TOKEN_CLOSE_PAREN: return "CLOSE_PAREN";
        case TOKEN_OPEN_BRACKET: return "OPEN_BRACKET";
        case TOKEN_CLOSE_BRACKET: return "CLOSE_BRACKET";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_DOT: return "DOT";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_ELLIPSIS: return "ELLIPSIS";
        case TOKEN_OPERATOR: return "OPERATOR";
        case TOKEN_COLON: return "COLON";
        case TOKEN_QUESTION: return "QUESTION";
        case TOKEN_HASH: return "HASH";
    }

    return "UNKNOWN";
}
