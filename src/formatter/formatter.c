#include "formatter.h"
#include "../utils/string_utils.h"
#include <string.h>
#include <stdio.h>

void formatter_init(formatter_t *f, token_t *tokens, size_t count, buffer_t *out, format_config_t config) {
    f->tokens = tokens;
    f->count = count;
    f->pos = 0;
    f->out = out;
    f->indent = 0;
    f->col = 0;
    f->paren_depth = 0;
    f->brace_depth = 0;
    f->bracket_depth = 0;
    f->at_line_start = true;
    f->prev_was_space = false;
    f->prev_was_newline = false;
    f->in_decl = false;
    f->saved_in_decl = false;
    f->in_for_header = false;
    f->prev_was_type = false;
    f->prev_type = TOKEN_EOF;
    f->prev_prev_type = TOKEN_EOF;
    f->prev_text = NULL;
    f->prev_length = 0;
    f->prev_prev_length = 0;
    f->config = config;
}

static void emit_indent(formatter_t *f) {
    int spaces = f->indent * f->config.indent_width;
    buffer_append_n(f->out, ' ', spaces);
    f->col += spaces;
}

static void emit_newline(formatter_t *f) {
    buffer_append_c(f->out, '\n');
    f->col = 0;
    f->at_line_start = true;
    f->prev_was_newline = true;
}

static void emit_space(formatter_t *f) {
    buffer_append_c(f->out, ' ');
    f->col++;
    f->prev_was_space = true;
}

static void emit_token_text(formatter_t *f, const char *s, size_t len) {
    buffer_append(f->out, s, len);
    f->col +=(int) len;
    f->at_line_start = false;
    f->prev_was_newline = false;
    f->prev_was_space = false;
}

static bool str_eq(const char *s, size_t n, const char *lit) {
    size_t ln = strlen(lit);
    return n == ln && memcmp(s, lit, ln) == 0;
}

static bool str_eq_any(const char *s, size_t n, const char **lits, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (str_eq(s, n, lits[i])) return true;
    }

    return false;
}

static const char *type_kws[] = {
    "int", "char", "void", "float", "double", "long", "short", "unsigned", "signed", "_Bool", "struct", "union", "enum"
};

static const char *qual_kws[] = {
    "const", "volatile", "restrict"
};

static const char *decl_kws[] = {
    "static", "extern", "typedef", "register", "auto", "inline"
};

static const char *ctrl_space_kws[] = {
    "if", "while", "for", "switch", "return"
};

static bool is_type_kw(const char *s, size_t n) {
    return str_eq_any(s, n, type_kws, sizeof(type_kws) / sizeof(type_kws[0]));
}

static bool is_qual_kw(const char *s, size_t n) {
    return str_eq_any(s, n, qual_kws, sizeof(qual_kws) / sizeof(qual_kws[0]));
}

static bool is_decl_kw(const char *s, size_t n) {
    return str_eq_any(s, n, decl_kws, sizeof(decl_kws) / sizeof(decl_kws[0]));
}

static bool kw_needs_space_after(const char *s, size_t n) {
    return str_eq_any(s, n, ctrl_space_kws, sizeof(ctrl_space_kws) / sizeof(ctrl_space_kws[0]));
}

static bool prev_is_unary_context(const formatter_t *f) {
    token_type_t p = f->prev_prev_type;
    return p == TOKEN_OPEN_PAREN || p == TOKEN_OPEN_BRACKET || p == TOKEN_COMMA || p == TOKEN_SEMICOLON || p == TOKEN_COLON || p == TOKEN_OPEN_BRACE || p == TOKEN_EOF || p == TOKEN_OPERATOR || p == TOKEN_QUESTION || p == TOKEN_ARROW || p == TOKEN_KEYWORD;
}

static int ptr_asterisk_space_before(const formatter_t *f) {
    if (f->prev_type == TOKEN_EOF) return 0;
    if (f->prev_type == TOKEN_OPEN_PAREN) return 0;
    if (f->prev_type == TOKEN_OPEN_BRACKET) return 0;
    if (f->prev_type == TOKEN_COMMA) return 1;
    if (f->prev_type == TOKEN_SEMICOLON) return 1;
    if (f->prev_type == TOKEN_OPEN_BRACE) return 1;
    if (f->in_decl && f->prev_type != TOKEN_OPERATOR) return 1;
    if (f->prev_type == TOKEN_OPERATOR) {
        if (f->prev_length == 1) {
            char c = f->prev_text[0];
            if (c == '=') return 1;
            if (c == '('|| c == '[') return 0;
            if (c == '*') return 0;
            if (c == '-'|| c == '+') {
                if (f->prev_prev_type == TOKEN_IDENTIFIER || f->prev_prev_type == TOKEN_NUMBER || f->prev_prev_type == TOKEN_CLOSE_PAREN) return 1;
                return 0;
            }
        }

        return 1;
    }

    if (f->prev_type == TOKEN_IDENTIFIER || f->prev_type == TOKEN_NUMBER) return 1;
    return 0;
}

static int newlines_before(const formatter_t *f, const token_t *tok) {
    if (tok->type == TOKEN_PREPROCESSOR) return 1;
    if (tok->type == TOKEN_COMMENT_LINE) return 1;
    if (tok->type == TOKEN_CLOSE_BRACE) return 1;
    if (f->prev_type == TOKEN_SEMICOLON) {
        if (f->in_for_header) return 0;
        if (tok->type == TOKEN_PREPROCESSOR || tok->type == TOKEN_COMMENT_LINE) return 0;
        if (f->brace_depth == 0) return 1;
        return 0;
    }

    if (f->prev_type == TOKEN_CLOSE_BRACE && f->brace_depth >= 0) {
        if (tok->type == TOKEN_CLOSE_BRACE) return 1;
        return 2;
    }

    if (f->prev_type == TOKEN_PREPROCESSOR) return 1;
    if (f->prev_type == TOKEN_COMMENT_LINE) return 1;
    if (tok->type == TOKEN_COMMENT_BLOCK) {
        if (f->prev_type == TOKEN_CLOSE_BRACE) return 1;
        if (f->prev_type == TOKEN_SEMICOLON) return 1;
    }

    if (f->prev_type == TOKEN_COMMENT_BLOCK) {
        if (f->prev_prev_type == TOKEN_CLOSE_BRACE || f->prev_prev_type == TOKEN_SEMICOLON) return 1;
        return 0;
    }

    return 0;
}

static int space_before(const formatter_t *f, const token_t *tok) {
    if (f->prev_type == TOKEN_EOF) return 0;
    if (f->at_line_start) return 0;
    if (tok->type == TOKEN_COMMA || tok->type == TOKEN_SEMICOLON) return 0;
    if (tok->type == TOKEN_CLOSE_PAREN) return 0;
    if (tok->type == TOKEN_CLOSE_BRACKET) return 0;
    if (tok->type == TOKEN_DOT || tok->type == TOKEN_ARROW) return 0;
    if (tok->type == TOKEN_COLON) {
        if (f->prev_type == TOKEN_COLON) return 0;
        if (f->prev_type == TOKEN_NUMBER) return 0;
        if (f->prev_type == TOKEN_IDENTIFIER) return 0;
        if (f->prev_type == TOKEN_KEYWORD) return 0;
        return 1;
    }

    if (tok->type == TOKEN_OPEN_PAREN) {
        if (f->prev_type == TOKEN_KEYWORD) {
            return kw_needs_space_after(f->prev_text, f->prev_length) ? 1: 0;
        }

        return 0;
    }

    if (tok->type == TOKEN_OPEN_BRACKET) return 0;
    if (tok->type == TOKEN_OPEN_BRACE) return 1;
    if (tok->type == TOKEN_QUESTION) return 1;
    if (f->prev_type == TOKEN_COMMA) return 0;
    if (f->prev_type == TOKEN_SEMICOLON) return f->in_for_header ? 1: 0;
    if (f->prev_type == TOKEN_OPEN_PAREN) return 0;
    if (f->prev_type == TOKEN_OPEN_BRACKET) return 0;
    if (f->prev_type == TOKEN_OPEN_BRACE) return 0;
    if (f->prev_type == TOKEN_DOT || f->prev_type == TOKEN_ARROW) return 0;
    if (f->prev_type == TOKEN_QUESTION) return 1;
    if (tok->type == TOKEN_OPERATOR && tok->length == 1 && tok->start[0] == '*') {
        return ptr_asterisk_space_before(f);
    }

    if (tok->type == TOKEN_OPERATOR && tok->length == 2) {
        if ((tok->start[0] == '+'&& tok->start[1] == '+') ||(tok->start[0] == '-'&& tok->start[1] == '-')) {
            if (f->prev_type == TOKEN_IDENTIFIER || f->prev_type == TOKEN_NUMBER || f->prev_type == TOKEN_CLOSE_PAREN || f->prev_type == TOKEN_CLOSE_BRACKET) return 0;
        }
    }

    if (tok->type == TOKEN_OPERATOR) {
        if (f->prev_type == TOKEN_OPERATOR) return 0;
        if (f->prev_type == TOKEN_KEYWORD) {
            if (kw_needs_space_after(f->prev_text, f->prev_length)) return 1;
            return 0;
        }

        if (f->prev_type == TOKEN_IDENTIFIER || f->prev_type == TOKEN_NUMBER || f->prev_type == TOKEN_CLOSE_PAREN || f->prev_type == TOKEN_CLOSE_BRACKET) return 1;
        return 0;
    }

    if (f->prev_type == TOKEN_OPERATOR) {
        if (f->prev_length == 1) {
            char c = f->prev_text[0];
            if (c == '!'|| c == '~') return 0;
            if (c == '+'|| c == '-'|| c == '&'|| c == '*') {
                if (prev_is_unary_context(f)) return 0;
                if (c == '*'&& f->in_decl) return 0;
                return 1;
            }
        }

        return 1;
    }

    if (f->prev_type == TOKEN_KEYWORD) return 1;
    if (f->prev_type == TOKEN_IDENTIFIER) return 1;
    if (f->prev_type == TOKEN_NUMBER) return 1;
    return 1;
}

void formatter_run(formatter_t *f) {
    for (f->pos = 0; f->pos < f->count; f->pos++) {
        token_t * tok =&f->tokens[f->pos];
        token_t * next =(f->pos + 1 < f->count) ? &f->tokens[f->pos + 1] : NULL;
        if (tok->type == TOKEN_PREPROCESSOR) {
            if (!f->at_line_start) emit_newline(f);
            int saved = f->indent;
            f->indent = 0;
            emit_indent(f);
            f->indent = saved;
            emit_token_text(f, tok->start, tok->length);
            emit_newline(f);
            f->prev_prev_type = f->prev_type;
            f->prev_prev_length = f->prev_length;
            f->prev_type = TOKEN_PREPROCESSOR;
            f->prev_text = tok->start;
            f->prev_length = tok->length;
            continue;
        }

        if (tok->type == TOKEN_COMMENT_LINE) {
            int nlb = newlines_before(f, tok);
            if (nlb > 0) emit_newline(f);
            if (nlb > 1) emit_newline(f);
            if (f->at_line_start) emit_indent(f);
            emit_token_text(f, tok->start, tok->length);
            emit_newline(f);
            f->prev_prev_type = f->prev_type;
            f->prev_prev_length = f->prev_length;
            f->prev_type = TOKEN_COMMENT_LINE;
            f->prev_text = tok->start;
            f->prev_length = tok->length;
            continue;
        }

        if (tok->type == TOKEN_COMMENT_BLOCK) {
            bool multi = memchr(tok->start, '\n', tok->length) != NULL;
            if (multi) {
                if (!f->at_line_start) emit_newline(f);
                const char *p = tok->start;
                ptrdiff_t remaining =(ptrdiff_t) tok->length;
                while (remaining > 0) {
                    const char *nl =(const char *) memchr(p, '\n', (size_t) remaining);
                    if (!nl) break;
                    size_t line_len =(size_t)(nl - p);
                    if (line_len > 0) {
                        emit_indent(f);
                        buffer_append(f->out, p, line_len);
                        f->col +=(int) line_len;
                        f->at_line_start = false;
                    }

                    emit_newline(f);
                    remaining -=(ptrdiff_t)(nl - p + 1);
                    p = nl + 1;
                }

                if (remaining > 0) {
                    emit_indent(f);
                    buffer_append(f->out, p, (size_t) remaining);
                    f->col +=(int) remaining;
                    f->at_line_start = false;
                }
            } else {
                if (f->prev_type == TOKEN_CLOSE_BRACE || f->prev_type == TOKEN_SEMICOLON) {
                    if (!f->at_line_start) emit_newline(f);
                    emit_indent(f);
                } else {
                    if (!f->at_line_start && space_before(f, tok)) emit_space(f);
                    if (f->at_line_start) emit_indent(f);
                }

                emit_token_text(f, tok->start, tok->length);
            }

            f->prev_prev_type = f->prev_type;
            f->prev_prev_length = f->prev_length;
            f->prev_type = TOKEN_COMMENT_BLOCK;
            f->prev_text = tok->start;
            f->prev_length = tok->length;
            continue;
        }

        if (tok->type == TOKEN_OPEN_BRACE) {
            if (!f->at_line_start) {
                if (f->config.brace_same_line) {
                    if (space_before(f, tok)) emit_space(f);
                } else {
                    emit_newline(f);
                    emit_indent(f);
                }
            }

            emit_token_text(f, tok->start, tok->length);
            f->brace_depth++;
            f->in_decl = false;
            f->prev_prev_type = f->prev_type;
            f->prev_prev_length = f->prev_length;
            f->prev_type = TOKEN_OPEN_BRACE;
            f->prev_text = tok->start;
            f->prev_length = tok->length;
            if (next && next->type == TOKEN_CLOSE_BRACE) {
                f->pos++;
                token_t * close = next;
                emit_token_text(f, close->start, close->length);
                f->brace_depth--;
                f->prev_prev_type = f->prev_type;
                f->prev_prev_length = f->prev_length;
                f->prev_type = TOKEN_CLOSE_BRACE;
                f->prev_text = close->start;
                f->prev_length = close->length;
            } else {
                emit_newline(f);
                f->indent++;
            }

            continue;
        }

        if (tok->type == TOKEN_CLOSE_BRACE) {
            f->indent--;
            f->in_decl = false;
            bool next_is_typedef_name =(next && next->type == TOKEN_IDENTIFIER &&(f->pos + 2 < f->count) && f->tokens[f->pos + 2].type == TOKEN_SEMICOLON);
            if (!next_is_typedef_name) {
                if (!f->at_line_start) emit_newline(f);
                emit_indent(f);
            }

            emit_token_text(f, tok->start, tok->length);
            f->brace_depth--;
            f->prev_prev_type = f->prev_type;
            f->prev_prev_length = f->prev_length;
            f->prev_type = TOKEN_CLOSE_BRACE;
            f->prev_text = tok->start;
            f->prev_length = tok->length;
            if (next && next->type == TOKEN_KEYWORD && str_eq(next->start, next->length, "else")) {
                emit_space(f);
                f->pos++;
                token_t * else_tok = next;
                emit_token_text(f, else_tok->start, else_tok->length);
                f->prev_prev_type = f->prev_type;
                f->prev_prev_length = f->prev_length;
                f->prev_type = TOKEN_KEYWORD;
                f->prev_text = else_tok->start;
                f->prev_length = else_tok->length;
            } else if (next_is_typedef_name) {
                emit_space(f);
                f->pos++;
                token_t * id_tok = next;
                emit_token_text(f, id_tok->start, id_tok->length);
                f->prev_prev_type = f->prev_type;
                f->prev_prev_length = f->prev_length;
                f->prev_type = TOKEN_IDENTIFIER;
                f->prev_text = id_tok->start;
                f->prev_length = id_tok->length;
            }

            continue;
        }

        if (tok->type == TOKEN_SEMICOLON) {
            if (!f->at_line_start && space_before(f, tok)) emit_space(f);
            if (f->at_line_start) emit_indent(f);
            emit_token_text(f, tok->start, tok->length);
            if (!f->in_for_header) {
                emit_newline(f);
            }

            f->prev_prev_type = f->prev_type;
            f->prev_prev_length = f->prev_length;
            f->prev_type = TOKEN_SEMICOLON;
            f->prev_text = tok->start;
            f->prev_length = tok->length;
            continue;
        }

        if (tok->type == TOKEN_OPEN_PAREN) {
            if (!f->at_line_start && space_before(f, tok)) emit_space(f);
            if (f->at_line_start) emit_indent(f);
            f->paren_depth++;
            emit_token_text(f, tok->start, tok->length);
            f->prev_prev_type = f->prev_type;
            f->prev_prev_length = f->prev_length;
            f->prev_type = TOKEN_OPEN_PAREN;
            f->prev_text = tok->start;
            f->prev_length = tok->length;
            continue;
        }

        if (tok->type == TOKEN_CLOSE_PAREN) {
            if (!f->at_line_start && space_before(f, tok)) emit_space(f);
            if (f->at_line_start) emit_indent(f);
            f->paren_depth--;
            emit_token_text(f, tok->start, tok->length);
            if (f->paren_depth == 0) {
                f->in_for_header = false;
            }

            f->prev_prev_type = f->prev_type;
            f->prev_prev_length = f->prev_length;
            f->prev_type = TOKEN_CLOSE_PAREN;
            f->prev_text = tok->start;
            f->prev_length = tok->length;
            continue;
        }

        int nlb = newlines_before(f, tok);
        if (nlb > 0) {
            if (nlb > 1) emit_newline(f);
            emit_newline(f);
        }

        if (f->at_line_start) {
            emit_indent(f);
        } else if (space_before(f, tok)) {
            emit_space(f);
        }

        emit_token_text(f, tok->start, tok->length);
        if (tok->type == TOKEN_COMMA) {
            emit_space(f);
            if (f->saved_in_decl) {
                f->in_decl = true;
                f->saved_in_decl = false;
            }
        }

        if (tok->type == TOKEN_OPERATOR && tok->length == 1 && tok->start[0] == '='&& f->in_decl) {
            f->saved_in_decl = true;
            f->in_decl = false;
        }

        if (tok->type == TOKEN_KEYWORD) {
            if (str_eq(tok->start, tok->length, "for")) f->in_for_header = true;
            if (is_type_kw(tok->start, tok->length) || is_qual_kw(tok->start, tok->length)) f->in_decl = true;
            if (is_decl_kw(tok->start, tok->length)) f->in_decl = true;
        }

        if (tok->type == TOKEN_CLOSE_BRACE || tok->type == TOKEN_SEMICOLON) {
            if (!f->in_for_header) {
                f->in_decl = false;
                f->saved_in_decl = false;
            }
        }

        if (tok->type == TOKEN_CLOSE_PAREN && f->paren_depth == 0) {
            if (!next ||(next->type != TOKEN_OPEN_BRACE && next->type != TOKEN_KEYWORD)) f->in_decl = false;
        }

        f->prev_prev_type = f->prev_type;
        f->prev_prev_length = f->prev_length;
        f->prev_type = tok->type;
        f->prev_text = tok->start;
        f->prev_length = tok->length;
    }

    buffer_trim_trailing(f->out);
    buffer_append_c(f->out, '\n');
}
