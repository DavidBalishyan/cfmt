#include "tokenizer.h"
#include "../utils/string_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary"
};
#define NUM_KEYWORDS (sizeof(keywords) / sizeof(keywords[0]))

typedef struct {
    const char *text;
    size_t len;
} keyword_entry_t;

static keyword_entry_t sorted_keywords[NUM_KEYWORDS];

static int kw_cmp(const void *a, const void *b) {
    const keyword_entry_t *ka =(const keyword_entry_t *) a;
    const keyword_entry_t *kb =(const keyword_entry_t *) b;
    size_t min = ka->len < kb->len ? ka->len: kb->len;
    int cmp = strncmp(ka->text, kb->text, min);
    if (cmp != 0) return cmp;
    if (ka->len < kb->len) return -1;
    if (ka->len > kb->len) return 1;
    return 0;
}

static void init_keywords(void) {
    static bool initialized = false;
    if (initialized) return;
    for (size_t i = 0; i < NUM_KEYWORDS; i++) {
        sorted_keywords[i].text = keywords[i];
        sorted_keywords[i].len = strlen(keywords[i]);
    }

    qsort(sorted_keywords, NUM_KEYWORDS, sizeof(keyword_entry_t), kw_cmp);
    initialized = true;
}

bool is_c_keyword(const char *s, size_t n) {
    init_keywords();
    keyword_entry_t key = {
        s, n
    };
    return bsearch(&key, sorted_keywords, NUM_KEYWORDS, sizeof(keyword_entry_t), kw_cmp) != NULL;
}

void tokenizer_init(tokenizer_t *tz, const char *source, size_t source_len) {
    tz->source = source;
    tz->source_len = source_len;
    tz->pos = 0;
    tz->line = 1;
    tz->col = 1;
    tz->last_line = 1;
    tz->c =(source_len > 0) ? source[0] : '\0';
    tz->at_bol = true;
}

static void advance(tokenizer_t *tz) {
    if (tz->c == '\n') {
        tz->last_line = tz->line;
        tz->line++;
        tz->col = 1;
        tz->at_bol = true;
    } else {
        tz->col++;
    }

    tz->pos++;
    tz->c =(tz->pos < tz->source_len) ? tz->source[tz->pos] : '\0';
}

static char peek(tokenizer_t *tz, size_t ahead) {
    size_t p = tz->pos + ahead;
    return (p < tz->source_len) ? tz->source[p] : '\0';
}

static void skip_whitespace(tokenizer_t *tz) {
    while (tz->c == ' '|| tz->c == '\t'|| tz->c == '\r') {
        advance(tz);
    }
}

static bool scan_preprocessor(tokenizer_t *tz, token_t *out) {
    out->type = TOKEN_PREPROCESSOR;
    out->start = tz->source + tz->pos;
    out->line = tz->line;
    out->col = tz->col;
    while (tz->c && tz->c != '\n') {
        if (tz->c == '\\'&& peek(tz, 1) == '\n') {
            advance(tz);
            advance(tz);
        } else {
            advance(tz);
        }
    }

    out->length =(tz->source + tz->pos) - out->start;
    return true;
}

static bool scan_line_comment(tokenizer_t *tz, token_t *out) {
    out->type = TOKEN_COMMENT_LINE;
    out->start = tz->source + tz->pos;
    out->line = tz->line;
    out->col = tz->col;
    while (tz->c && tz->c != '\n') advance(tz);
    out->length =(tz->source + tz->pos) - out->start;
    return true;
}

static bool scan_block_comment(tokenizer_t *tz, token_t *out) {
    out->type = TOKEN_COMMENT_BLOCK;
    out->start = tz->source + tz->pos;
    out->line = tz->line;
    out->col = tz->col;
    advance(tz);
    advance(tz);
    while (tz->c) {
        if (tz->c == '*'&& peek(tz, 1) == '/') {
            advance(tz);
            advance(tz);
            break;
        }

        advance(tz);
    }

    out->length =(tz->source + tz->pos) - out->start;
    return true;
}

static bool scan_string(tokenizer_t *tz, token_t *out) {
    out->type = TOKEN_STRING;
    out->start = tz->source + tz->pos;
    out->line = tz->line;
    out->col = tz->col;
    advance(tz);
    while (tz->c && tz->c != '"') {
        if (tz->c == '\\') advance(tz);
        advance(tz);
    }

    if (tz->c == '"') advance(tz);
    out->length =(tz->source + tz->pos) - out->start;
    return true;
}

static bool scan_char(tokenizer_t *tz, token_t *out) {
    out->type = TOKEN_CHAR;
    out->start = tz->source + tz->pos;
    out->line = tz->line;
    out->col = tz->col;
    advance(tz);
    while (tz->c && tz->c != '\'') {
        if (tz->c == '\\') advance(tz);
        advance(tz);
    }

    if (tz->c == '\'') advance(tz);
    out->length =(tz->source + tz->pos) - out->start;
    return true;
}

static bool scan_identifier(tokenizer_t *tz, token_t *out) {
    out->start = tz->source + tz->pos;
    out->line = tz->line;
    out->col = tz->col;
    while (str_isident(tz->c)) advance(tz);
    out->length =(tz->source + tz->pos) - out->start;
    out->type = is_c_keyword(out->start, out->length) ? TOKEN_KEYWORD: TOKEN_IDENTIFIER;
    return true;
}

static bool scan_number(tokenizer_t *tz, token_t *out) {
    out->type = TOKEN_NUMBER;
    out->start = tz->source + tz->pos;
    out->line = tz->line;
    out->col = tz->col;
    if (tz->c == '0'&&(peek(tz, 1) == 'x'|| peek(tz, 1) == 'X')) {
        advance(tz);
        advance(tz);
        while (str_isxdigit(tz->c)) advance(tz);
    } else {
        while (str_isdigit(tz->c)) advance(tz);
    }

    if (tz->c == '.'&& peek(tz, 1) != '.') {
        advance(tz);
        while (str_isdigit(tz->c)) advance(tz);
    }

    if (tz->c == 'e'|| tz->c == 'E') {
        advance(tz);
        if (tz->c == '+'|| tz->c == '-') advance(tz);
        while (str_isdigit(tz->c)) advance(tz);
    }

    if (tz->c == 'f'|| tz->c == 'F'|| tz->c == 'l'|| tz->c == 'L'|| tz->c == 'u'|| tz->c == 'U') {
        advance(tz);
    }

    out->length =(tz->source + tz->pos) - out->start;
    return true;
}

typedef struct {
    const char *text;
    size_t len;
    token_type_t type;
} op_entry_t;

static op_entry_t operators[] = {
{
        "...", 3, TOKEN_ELLIPSIS
    }

    ,  {
        "->", 2, TOKEN_ARROW
    }

    ,  {
        "++", 2, TOKEN_OPERATOR
    }

    ,  {
        "--", 2, TOKEN_OPERATOR
    }

    ,  {
        "<<=", 3, TOKEN_OPERATOR
    }

    ,  {
        ">>=", 3, TOKEN_OPERATOR
    }

    ,  {
        "<<", 2, TOKEN_OPERATOR
    }

    ,  {
        ">>", 2, TOKEN_OPERATOR
    }

    ,  {
        "<=", 2, TOKEN_OPERATOR
    }

    ,  {
        ">=", 2, TOKEN_OPERATOR
    }

    ,  {
        "==", 2, TOKEN_OPERATOR
    }

    ,  {
        "!=", 2, TOKEN_OPERATOR
    }

    ,  {
        "&&", 2, TOKEN_OPERATOR
    }

    ,  {
        "||", 2, TOKEN_OPERATOR
    }

    ,  {
        "+=", 2, TOKEN_OPERATOR
    }

    ,  {
        "-=", 2, TOKEN_OPERATOR
    }

    ,  {
        "*=", 2, TOKEN_OPERATOR
    }

    ,  {
        "/=", 2, TOKEN_OPERATOR
    }

    ,  {
        "%=", 2, TOKEN_OPERATOR
    }

    ,  {
        "&=", 2, TOKEN_OPERATOR
    }

    ,  {
        "|=", 2, TOKEN_OPERATOR
    }

    ,  {
        "^=", 2, TOKEN_OPERATOR
    }

    ,  {
        "=", 1, TOKEN_OPERATOR
    }

    ,  {
        "+", 1, TOKEN_OPERATOR
    }

    ,  {
        "-", 1, TOKEN_OPERATOR
    }

    ,  {
        "*", 1, TOKEN_OPERATOR
    }

    ,  {
        "/", 1, TOKEN_OPERATOR
    }

    ,  {
        "%", 1, TOKEN_OPERATOR
    }

    ,  {
        "&", 1, TOKEN_OPERATOR
    }

    ,  {
        "|", 1, TOKEN_OPERATOR
    }

    ,  {
        "^", 1, TOKEN_OPERATOR
    }

    ,  {
        "~", 1, TOKEN_OPERATOR
    }

    ,  {
        "!", 1, TOKEN_OPERATOR
    }

    ,  {
        "<", 1, TOKEN_OPERATOR
    }

    ,  {
        ">", 1, TOKEN_OPERATOR
    }

    ,  {
        "?", 1, TOKEN_QUESTION
    }

    , 
};

static bool scan_operator(tokenizer_t *tz, token_t *out) {
    for (size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++) {
        const char *op = operators[i].text;
        size_t len = operators[i].len;
        if (strncmp(tz->source + tz->pos, op, len) == 0) {
            out->type = operators[i].type;
            out->start = tz->source + tz->pos;
            out->length = len;
            out->line = tz->line;
            out->col = tz->col;
            for (size_t j = 0; j < len; j++) advance(tz);
            return true;
        }
    }

    return false;
}

static bool is_operator_char(char c) {
    const char *op_chars = "+-*/%&|^~!<>=?:.";
    return strchr(op_chars, c) != NULL;
}

bool tokenizer_next(tokenizer_t * tz, token_t * out) {
    skip_whitespace(tz);
    if (!tz->c) {
        out->type = TOKEN_EOF;
        out->start = NULL;
        out->length = 0;
        out->line = tz->line;
        out->col = tz->col;
        return false;
    }

    if (tz->c == '\n') {
        out->type = TOKEN_NEWLINE;
        out->start = tz->source + tz->pos;
        out->length = 1;
        out->line = tz->line;
        out->col = tz->col;
        advance(tz);
        tz->at_bol = true;
        return true;
    }

    if (tz->c == '#'&& tz->at_bol) {
        return scan_preprocessor(tz, out);
    }

    if (tz->c == '/'&& peek(tz, 1) == '/') {
        return scan_line_comment(tz, out);
    }

    if (tz->c == '/'&& peek(tz, 1) == '*') {
        return scan_block_comment(tz, out);
    }

    if (tz->c == '"') {
        return scan_string(tz, out);
    }

    if (tz->c == '\'') {
        return scan_char(tz, out);
    }

    if (str_isident_start(tz->c)) {
        return scan_identifier(tz, out);
    }

    if (tz->c == '.') {
        if (peek(tz, 1) == '.'&& peek(tz, 2) == '.') {
            out->type = TOKEN_ELLIPSIS;
            out->start = tz->source + tz->pos;
            out->length = 3;
            out->line = tz->line;
            out->col = tz->col;
            advance(tz);
            advance(tz);
            advance(tz);
            return true;
        }

        if (str_isdigit(peek(tz, 1))) {
            return scan_number(tz, out);
        }
    }

    if (str_isdigit(tz->c)) {
        return scan_number(tz, out);
    }

    switch (tz->c) {
        case '{' : out->type = TOKEN_OPEN_BRACE;
        break;
        case '}' : out->type = TOKEN_CLOSE_BRACE;
        break;
        case '(' : out->type = TOKEN_OPEN_PAREN;
        break;
        case ')' : out->type = TOKEN_CLOSE_PAREN;
        break;
        case '[' : out->type = TOKEN_OPEN_BRACKET;
        break;
        case ']' : out->type = TOKEN_CLOSE_BRACKET;
        break;
        case ';' : out->type = TOKEN_SEMICOLON;
        break;
        case ',' : out->type = TOKEN_COMMA;
        break;
        case '.' : out->type = TOKEN_DOT;
        break;
        case ':' : out->type = TOKEN_COLON;
        break;
        default: {
            if (is_operator_char(tz->c)) {
                return scan_operator(tz, out);
            }

            out->type = TOKEN_OPERATOR;
            out->start = tz->source + tz->pos;
            out->length = 1;
            out->line = tz->line;
            out->col = tz->col;
            advance(tz);
            return true;
        }
    }

    out->start = tz->source + tz->pos;
    out->length = 1;
    out->line = tz->line;
    out->col = tz->col;
    advance(tz);
    return true;
}

token_array_t tokenize(const char *source, size_t source_len) {
    init_keywords();
    token_array_t arr = {
        NULL, 0, 0
    };
    tokenizer_t tz;
    tokenizer_init(&tz, source, source_len);
    token_t tok;
    while (tokenizer_next(&tz, &tok)) {
        if (tok.type == TOKEN_NEWLINE) continue;
        if (arr.count >= arr.cap) {
            size_t newcap = arr.cap ? arr.cap * 2: 1024;
            token_t * p = realloc(arr.tokens, newcap * sizeof(token_t));
            if (!p) break;
            arr.tokens = p;
            arr.cap = newcap;
        }

        arr.tokens[arr.count++] = tok;
    }

    return arr;
}

void token_array_free(token_array_t *arr) {
    free(arr->tokens);
    arr->tokens = NULL;
    arr->count = 0;
    arr->cap = 0;
}
