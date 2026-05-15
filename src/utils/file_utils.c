#include "file_utils.h"
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

file_status_t file_read(const char *path, buffer_t *out) {
    FILE * f = fopen(path, "rb");
    if (!f) return FILE_ERR_READ;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) {
        fclose(f);
        return FILE_ERR_READ;
    }

    buffer_ensure(out, (size_t) sz + 1);
    size_t nread = fread(out->data, 1, (size_t) sz, f);
    fclose(f);
    out->len = nread;
    out->data[out->len] = '\0';
    return FILE_OK;
}

file_status_t file_write(const char *path, const char *data, size_t len) {
    FILE * f = fopen(path, "wb");
    if (!f) return FILE_ERR_WRITE;
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    return (written == len) ? FILE_OK: FILE_ERR_WRITE;
}

bool file_is_source(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return false;
    return (strcmp(dot, ".c") == 0 || strcmp(dot, ".h") == 0);
}

bool file_is_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

int file_walk_dir(const char *dir_path, file_callback_t cb, void *user) {
    DIR * d = opendir(dir_path);
    if (!d) return -1;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char fullpath[4096];
        if (!file_path_join(dir_path, entry->d_name, fullpath, sizeof(fullpath))) continue;
        if (file_is_dir(fullpath)) {
            file_walk_dir(fullpath, cb, user);
        } else if (file_is_source(entry->d_name)) {
            cb(fullpath, user);
        }
    }

    closedir(d);
    return 0;
}

bool file_path_join(const char *dir, const char *name, char *out, size_t out_size) {
    size_t dlen = strlen(dir);
    size_t nlen = strlen(name);
    if (dlen + 1 + nlen + 1 > out_size) return false;
    memcpy(out, dir, dlen);
    out[dlen] = '/';
    memcpy(out + dlen + 1, name, nlen + 1);
    return true;
}
