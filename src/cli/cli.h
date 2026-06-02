#ifndef CLI_H
#define CLI_H
#include "../formatter/formatter.h"
#include <stdbool.h>

typedef struct {
    bool check_mode;
    bool recursive;
    bool stdin_mode;
    const char *config_path;
    const char *input_path;
    format_config_t config;
} cli_options_t;

int cli_parse_args(cli_options_t *opts, int argc, char **argv);

int cli_run(const cli_options_t *opts);
#endif /* CLI_H */
