#pragma once
#include "buffer.h"
#include <stdbool.h>

typedef enum {
    FILE_OK, FILE_ERR_READ, FILE_ERR_WRITE, FILE_ERR_STAT, } file_status_t;

file_status_t file_read(const char *path, buffer_t *out);

file_status_t file_write(const char *path, const char *data, size_t len);

bool file_is_source(const char *path);

bool file_is_dir(const char *path);

typedef void(*file_callback_t)(const char *path, void *user);

int file_walk_dir(const char *dir_path, file_callback_t cb, void *user);

bool file_path_join(const char *dir, const char *name, char *out, size_t out_size);
