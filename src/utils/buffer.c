#include "buffer.h"
#define BUF_INIT_CAP 4096

void buffer_init(buffer_t *buf) {
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
}

void buffer_free(buffer_t *buf) {
    free(buf->data);
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
}

int buffer_ensure(buffer_t *buf, size_t need) {
    if (buf->cap >= need) return 0;
    size_t newcap = buf->cap ? buf->cap: BUF_INIT_CAP;
    while (newcap < need) newcap *= 2;
    char *p = realloc(buf->data, newcap);
    if (!p) return -1;
    buf->data = p;
    buf->cap = newcap;
    return 0;
}

void buffer_clear(buffer_t *buf) {
    buf->len = 0;
}

void buffer_append(buffer_t *buf, const char *s, size_t n) {
    if (n == 0) return;
    if (buffer_ensure(buf, buf->len + n + 1) != 0) return;
    memcpy(buf->data + buf->len, s, n);
    buf->len += n;
    buf->data[buf->len] = '\0';
}

void buffer_append_c(buffer_t *buf, char c) {
    buffer_append(buf, &c, 1);
}

void buffer_append_s(buffer_t *buf, const char *s) {
    buffer_append(buf, s, strlen(s));
}

void buffer_append_n(buffer_t *buf, char c, size_t n) {
    for (size_t i = 0; i < n; i++) buffer_append_c(buf, c);
}

void buffer_trim_trailing(buffer_t *buf) {
    while (buf->len > 0 &&(buf->data[buf->len - 1] == ' '|| buf->data[buf->len - 1] == '\t'|| buf->data[buf->len - 1] == '\n')) {
        buf->len--;
    }

    buf->data[buf->len] = '\0';
}

char *buffer_detach(buffer_t *buf) {
    char *p = buf->data;
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
    return p;
}
