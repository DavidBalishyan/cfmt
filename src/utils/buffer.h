#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} buffer_t;

void buffer_init(buffer_t *buf);

void buffer_free(buffer_t *buf);

int buffer_ensure(buffer_t *buf, size_t need);

void buffer_clear(buffer_t *buf);

void buffer_append(buffer_t *buf, const char *s, size_t n);

void buffer_append_c(buffer_t *buf, char c);

void buffer_append_s(buffer_t *buf, const char *s);

void buffer_append_n(buffer_t *buf, char c, size_t n);

void buffer_trim_trailing(buffer_t *buf);

char *buffer_detach(buffer_t *buf);
