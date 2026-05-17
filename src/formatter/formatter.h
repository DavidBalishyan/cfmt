#ifndef FORMATTER_H
#define FORMATTER_H
#include "../tokenizer/token.h"
#include "../utils/buffer.h"
#include <stdbool.h>

typedef struct {
    int indent_width;
    int max_line_width;
    bool spaces_around_operators;
    bool pointer_left;
    bool brace_same_line;
    bool no_tabs;
} format_config_t;
#define FORMAT_DEFAULT_CONFIG \
    (format_config_t) { \
        .indent_width = 4, \
        .max_line_width = 100, \
        .spaces_around_operators = true, \
        .pointer_left = true, \
        .brace_same_line = true, \
        .no_tabs = true, \
    }

typedef struct {
    token_t * tokens;
    size_t count;
    size_t pos;
    buffer_t * out;
    int indent;
    int col;
    int paren_depth;
    int brace_depth;
    int bracket_depth;
    bool at_line_start;
    bool prev_was_space;
    bool prev_was_newline;
    bool in_decl;
    bool saved_in_decl;
    bool in_for_header;
    bool prev_was_type;
    token_type_t prev_type;
    token_type_t prev_prev_type;
    const char *prev_text;
    size_t prev_length;
    size_t prev_prev_length;
    format_config_t config;
} formatter_t;

void formatter_init(formatter_t *f, token_t *tokens, size_t count, buffer_t *out, format_config_t config);

void formatter_run(formatter_t *f);
#endif /* FORMATTER_H */