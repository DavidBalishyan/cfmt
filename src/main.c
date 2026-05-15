#include "cli/cli.h"

int main(int argc, char **argv) {
    cli_options_t opts;
    if (cli_parse_args(&opts, argc, argv) != 0) {
        return 1;
    }

    return cli_run(&opts);
}
