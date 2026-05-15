#pragma once
#include "token.h"
#include "../utils/buffer.h"
#include <stdbool.h>

typedef struct {
    token_t * tokens;
    size_t count;
    size_t cap;
} token_array_t;

typedef struct {
    const char *source;
    size_t source_len;
    size_t pos;
    int line;
    int col;
    int last_line;
    char c;
    bool at_bol;
} tokenizer_t;

void tokenizer_init(tokenizer_t *tz, const char *source, size_t source_len);

bool tokenizer_next(tokenizer_t *tz, token_t *out);

void tokenizer_free(token_t *tokens, size_t count);

token_array_t tokenize(const char *source, size_t source_len);

void token_array_free(token_array_t *arr);

bool is_c_keyword(const char *s, size_t n);
