#include "cli.h"
#include "../tokenizer/tokenizer.h"
#include "../formatter/formatter.h"
#include "../utils/file_utils.h"
#include "../utils/buffer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] <file|directory>\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --check       Check mode (exit 1 if changes needed)\n");
    fprintf(stderr, "  --config PATH Config file path\n");
    fprintf(stderr, "  -r, --recursive  Process directories recursively\n");
    fprintf(stderr, "  --help        Show this help\n");
}

static void trim_trailing(char *s) {
    size_t len = strlen(s);
    while (len > 0 &&(s[len - 1] == ' '|| s[len - 1] == '\t'|| s[len - 1] == '\r'|| s[len - 1] == '\n')) {
        s[-- len] = '\0';
    }
}

static bool parse_bool(const char *s) {
    return strcmp(s, "true") == 0 || strcmp(s, "1") == 0;
}

static bool parse_config_line(char *line, format_config_t *config) {
    char *eq = strchr(line, '=');
    if (!eq) return false;
    size_t key_len =(size_t)(eq - line);
    char *val_start = eq + 1;
    while (*val_start == ' '|| *val_start == '\t') val_start++;
    trim_trailing(val_start);
    while (key_len > 0 &&(line[key_len - 1] == ' '|| line[key_len - 1] == '\t')) key_len--;
    if (key_len == 12 && strncmp(line, "indent_width", 12) == 0) {
        config->indent_width = atoi(val_start);
        return true;
    }

    if (key_len == 14 && strncmp(line, "max_line_width", 14) == 0) {
        config->max_line_width = atoi(val_start);
        return true;
    }

    if (key_len == 15 && strncmp(line, "brace_same_line", 15) == 0) {
        config->brace_same_line = parse_bool(val_start);
        return true;
    }

    if (key_len == 12 && strncmp(line, "pointer_left", 12) == 0) {
        config->pointer_left = parse_bool(val_start);
        return true;
    }

    if (key_len == 7 && strncmp(line, "no_tabs", 7) == 0) {
        config->no_tabs = parse_bool(val_start);
        return true;
    }

    return false;
}

static format_config_t load_config(const char *path) {
    format_config_t config = FORMAT_DEFAULT_CONFIG;
    buffer_t buf;
    buffer_init(&buf);
    const char *load_path = path;
    if (!load_path) {
        if (file_read(".cfmt.conf", &buf) != FILE_OK) {
            buffer_clear(&buf);
            const char *home = getenv("HOME");
            if (home) {
                char cpath[4096];
                snprintf(cpath, sizeof(cpath), "%s/.cfmt.conf", home);
                if (file_read(cpath, &buf) != FILE_OK) {
                    buffer_free(&buf);
                    return config;
                }
            } else {
                buffer_free(&buf);
                return config;
            }
        }
    } else {
        if (file_read(load_path, &buf) != FILE_OK) {
            buffer_free(&buf);
            return config;
        }
    }

    char *line = buf.data;
    char *end;
    while (line && *line) {
        end = strchr(line, '\n');
        if (end) *end = '\0';
        while (*line == ' '|| *line == '\t') line++;
        if (*line && *line != '#') parse_config_line(line, &config);
        if (end) line = end + 1;
        else break;
    }

    buffer_free(&buf);
    return config;
}

int cli_parse_args(cli_options_t *opts, int argc, char **argv) {
    memset(opts, 0, sizeof(*opts));
    opts->config = FORMAT_DEFAULT_CONFIG;
    opts->stdin_mode = false;
    if (argc < 2) {
        opts->stdin_mode = true;
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[i], "--check") == 0) {
            opts->check_mode = true;
        } else if (strcmp(argv[i], "--config") == 0) {
            if (i + 1 < argc) {
                opts->config_path = argv[++ i];
            } else {
                fprintf(stderr, "Error: --config requires a path\n");
                return -1;
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
            opts->recursive = true;
        } else if (argv[i][0] != '-') {
            opts->input_path = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    if (opts->config_path) {
        opts->config = load_config(opts->config_path);
    } else {
        opts->config = load_config(NULL);
    }

    if (!opts->input_path) {
        opts->stdin_mode = true;
    }

    return 0;
}

typedef struct {
    format_config_t config;
    bool check_mode;
    int changed_count;
    int file_count;
    int error_count;
} format_job_t;

static void format_file_cb(const char *path, void *user) {
    format_job_t * job =(format_job_t *) user;
    buffer_t src, dst;
    buffer_init(&src);
    buffer_init(&dst);
    if (file_read(path, &src) != FILE_OK) {
        fprintf(stderr, "Error: cannot read %s\n", path);
        job->error_count++;
        buffer_free(&src);
        buffer_free(&dst);
        return;
    }

    token_array_t tokens = tokenize(src.data, src.len);
    if (tokens.count == 0) {
        token_array_free(&tokens);
        buffer_free(&src);
        buffer_free(&dst);
        return;
    }

    formatter_t fmt;
    formatter_init(&fmt, tokens.tokens, tokens.count, &dst, job->config);
    formatter_run(&fmt);
    bool changed =(src.len != dst.len || memcmp(src.data, dst.data, src.len) != 0);
    if (job->check_mode) {
        if (changed) {
            printf("%s: would reformat\n", path);
            job->changed_count++;
        }
    } else {
        if (changed) {
            if (file_write(path, dst.data, dst.len) == FILE_OK) {
                printf("Formatted: %s\n", path);
                job->changed_count++;
            } else {
                fprintf(stderr, "Error: cannot write %s\n", path);
                job->error_count++;
            }
        }
    }

    job->file_count++;
    token_array_free(&tokens);
    buffer_free(&src);
    buffer_free(&dst);
}

int cli_run(const cli_options_t *opts) {
    format_job_t job;
    job.config = opts->config;
    job.check_mode = opts->check_mode;
    job.changed_count = 0;
    job.file_count = 0;
    job.error_count = 0;
    if (opts->stdin_mode) {
        buffer_t src, dst;
        buffer_init(&src);
        buffer_init(&dst);
        char buf[8192];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
            buffer_append(&src, buf, n);
        }

        if (src.len == 0) {
            buffer_free(&src);
            buffer_free(&dst);
            return 0;
        }

        token_array_t tokens = tokenize(src.data, src.len);
        formatter_t fmt;
        formatter_init(&fmt, tokens.tokens, tokens.count, &dst, opts->config);
        formatter_run(&fmt);
        fwrite(dst.data, 1, dst.len, stdout);
        token_array_free(&tokens);
        buffer_free(&src);
        buffer_free(&dst);
        return 0;
    }

    if (file_is_dir(opts->input_path)) {
        if (opts->recursive) {
            file_walk_dir(opts->input_path, format_file_cb, &job);
        } else {
            fprintf(stderr, "Error: %s is a directory. Use -r or --recursive.\n", opts->input_path);
            return 1;
        }
    } else if (file_is_source(opts->input_path)) {
        format_file_cb(opts->input_path, &job);
    } else {
        fprintf(stderr, "Error: %s is not a C source file or directory\n", opts->input_path);
        return 1;
    }

    if (opts->check_mode && job.changed_count > 0) {
        printf("\n%d file(s) would be reformatted\n", job.changed_count);
        return 1;
    }

    if (job.error_count > 0) return 1;
    return 0;
}

