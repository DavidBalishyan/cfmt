# cfmt - C Code Formatter

A fast, modular C code formatter written in **pure C17**. No C++, no libclang, no external dependencies.

---

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Formatting Rules](#formatting-rules)
- [Examples](#examples)
- [Architecture](#architecture)
- [Testing](#testing)
- [License](#license)

---

## Features

- Format `.c` and `.h` files deterministically (idempotent output)
- Whitespace normalization - 4-space indentation, no tabs
- Brace placement (same-line or next-line)
- Operator spacing - spaces around binary operators: `a + b`, `x == y`
- Pointer alignment - `int *ptr` style with context-aware asterisk spacing
- Control keyword spacing - `if (...)`, `while (...)`, `for (...)`, `switch (...)`
- Comment preservation - both `//` and `/* */` styles, including multi-line block comments
- Preprocessor directive handling - left-aligned `#include`, `#define`, etc.
- Trailing whitespace removal
- Redundant blank line removal
- Recursive directory processing
- Check mode for CI integration (exit code 1 if changes needed)
- stdin/stdout pipe support
- Custom configuration via file

## Requirements

- C17 compiler (GCC 8+, Clang 10+, or MSVC 2022+)
- CMake >= 3.14
- POSIX or Windows (via msys2/MinGW)

## Installation

### Using Ninja (recommended)

```sh
git clone https://github.com/DavidBalishyan/cfmt.git
cd cfmt
cmake --preset ninja
cmake --build --preset ninja
```

### Using Unix Makefiles


```sh
git clone https://github.com/DavidBalishyan/cfmt.git
cd cfmt
cmake -B build -G "Unix Makefiles"
make -C build
```

### Manual build

```sh
mkdir build && cd build
cmake .. -G Ninja        # or omit -G to use Makefiles
cmake --build .
```

The binary is placed at `build/cfmt` (or `build/release/cfmt` for the release preset). Optionally copy it to your PATH:

```sh
cp build/cfmt /usr/local/bin/
```

### Building with tests

With presets (tests are enabled by default in the `ninja` and `make` presets):

```sh
cmake --build --preset ninja
ctest --preset ninja
```

Or manually:

```sh
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Build presets

| Preset | Generator | Build type | Tests | Binary dir |
|--------|-----------|------------|-------|------------|
| `ninja` | Ninja | Debug | ON | `build/` |
| `make` | Unix Makefiles | Debug | ON | `build/` |
| `release` | Ninja | Release | OFF | `build/release/` |
| `debug` | Ninja | Debug | ON | `build/debug/` |

---

## Usage

```
cfmt [options] <file|directory>
```

### Options

| Flag | Description |
|------|-------------|
| `--check` | Check mode - report files that would change, exit 1 if any |
| `--config <path>` | Path to configuration file |
| `-r`, `--recursive` | Process directories recursively |
| `--help` | Print usage information and exit |

### Modes

**Single file** - formats the file in place:

```sh
cfmt main.c
```

**Recursive directory** - formats all `.c` and `.h` files:

```sh
cfmt -r src/
```

**Check mode** - useful for CI pipelines. Exits with code 1 if any file would be reformatted:

```sh
cfmt --check .
cfmt --check -r src/
```

**stdin/stdout** - read from stdin, write formatted output to stdout:

```sh
cat test.c | cfmt
cfmt < test.c > formatted.c
```

**Custom config** - override config file path:

```sh
cfmt --config /path/to/.cfmt.conf file.c
```

### Exit codes

| Code | Meaning |
|------|---------|
| 0 | Success - all files formatted (or no changes needed in check mode) |
| 1 | Changes needed (check mode) or I/O error |

---

## Configuration

cfmt looks for a config file in this order:

1. Path passed via `--config <path>`
2. `.cfmt.conf` in the current directory
3. `~/.cfmt.conf`

### Configuration file format

Lines follow `key = value` syntax. Blank lines and `#` comments are ignored.

```ini
# Indentation width in spaces
indent_width = 4

# Maximum line length (currently informational/reserved)
max_line_width = 100

# Place opening brace on same line as control statement (true)
# or on the next line (false)
brace_same_line = true

# Place `*` adjacent to the type name: `int* ptr` (false)
# or adjacent to the variable name: `int *ptr` (true)
pointer_left = true

# Replace tabs with spaces
no_tabs = true
```

### Default configuration

```c
indent_width           = 4
max_line_width         = 100
brace_same_line        = true
pointer_left           = true
no_tabs                = true
```

### Key reference

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `indent_width` | int | `4` | Number of spaces per indentation level |
| `max_line_width` | int | `100` | Soft line length limit |
| `brace_same_line` | bool | `true` | `if (...) {` vs `if (...)\n{` |
| `pointer_left` | bool | `true` | `int *ptr` vs `int* ptr` |
| `no_tabs` | bool | `true` | Emit spaces instead of tabs |

---

## Formatting Rules

cfmt applies the following transformations deterministically. The formatter is **idempotent** - running it twice on the same file produces identical output.

### Indentation

- Bodies inside `{}` are indented one level deeper than their enclosing block
- Each level uses `indent_width` spaces (default: 4)
- Preprocessor directives (`#include`, `#define`, etc.) are always left-aligned (column 0)

### Braces

When `brace_same_line = true`:
- Opening braces stay on the same line: `int main() {`
- Empty blocks `{}` are preserved on a single line
- `} else {` is placed on the same line

When `brace_same_line = false`:
- Opening braces move to the next line

### Operators

- Spaces around binary operators: `a + b`, `x == y`, `*ptr` (dereference has no leading space)
- No spaces around unary operators: `!flag`, `~mask`
- Context-aware pointer asterisks: `int *ptr` in declarations, `*ptr = 1` in expressions
- No space before `,`, `;`, `.`, `->`
- Space before `{`, `?`
- Inc/decrement (`++`, `--`) attach to identifiers/numbers without spaces

### Control keywords

A space is inserted after: `if`, `while`, `for`, `switch`, `return`

No space after function names: `func()` vs `if ()`

### Comments

- `//` line comments stay on their own line with proper indentation
- `/* */` block comments are preserved as-is, including multi-line blocks
- Single-line block comments inline with code are kept inline
- Multi-line block comments are re-indented line by line

### Preprocessor

- `#include`, `#define`, `#ifdef`, etc. are always placed at column 0
- A blank line separates preprocessor directives from surrounding code where appropriate

### Empty lines

- Consecutive blank lines are collapsed (though the engine avoids inserting excessive newlines)
- Trailing whitespace on any line is stripped
- A single trailing newline is always present in the output

### Context tracking

The formatter tracks:
- **Brace depth** - for indentation
- **Parenthesis depth** - to detect `for(...)` headers (semicolons inside `for` don't trigger newlines)
- **Declaration context** - to distinguish `int *ptr` (declaration) from `*ptr` (dereference)
- **For-header context** - semicolons inside `for(...)` don't emit newlines

---

## Examples

### Before

```c
#include <stdio.h>
#include <stdlib.h>

typedef struct point {
    int x;
    int y;
} point_t;

static int calculate(int a, int b, int op) {
    int result;
    switch (op) {
        case 0: result = a + b;
        break;
        case 1: result = a - b;
        break;
        case 2: result = a * b;
        break;
        case 3: if (b != 0) {
            result = a / b;
        } else {
            fprintf(stderr, "division by zero\n");
            result = 0;
        }

        break;
        default: result = 0;
        break;
    }

    return result;
}

static void print_point(const point_t *p) {
    if (!p) {
        printf("point is NULL\n");
        return;
    }

    printf("point: (%d, %d)\n", p->x, p->y);
}

int main(int argc, char **argv) {
    point_t pt = {
        .x = 10, .y = 20
    };
    print_point(&pt);
    for (int i = 0; i < 5; i++) {
        int val = calculate(i, i + 1, i % 4);
        printf("calculate(%d, %d, %d) = %d\n", i, i + 1, i % 4, val);
    }
    /* demonstrate pointer arithmetic */
    int arr[] = {
        1, 2, 3, 4, 5
    };
    int *ptr = arr;
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += *ptr++;
    }

    printf("sum = %d\n", sum);
    return 0;
}
```

### After (default config)

The file above is already formatted by cfmt - running cfmt on it yields identical output (idempotent). A truly unformatted version would gain operator spacing (`a+b` → `a + b`), keyword spacing (`if(x)` → `if (x)`), pointer normalization (`int*ptr` → `int *ptr`), proper indentation, and trailing whitespace removal.

---

## Architecture

```
cfmt
├── src/
│   ├── main.c                  Entry point
│   ├── cli/
│   │   ├── cli.h               CLI option types and API
│   │   └── cli.c               Argument parsing, config loading, file dispatch
│   ├── tokenizer/
│   │   ├── token.h              Token type enum and token structure
│   │   ├── token.c              Token type name helpers
│   │   ├── tokenizer.h          Tokenizer state and API
│   │   └── tokenizer.c          Character-by-character scanner
│   ├── formatter/
│   │   ├── formatter.h          Formatter state, config, and API
│   │   └── formatter.c          Token-driven formatting engine
│   └── utils/
│       ├── buffer.h             Dynamic growable buffer
│       ├── buffer.c
│       ├── file_utils.h         File I/O, directory walking, path helpers
│       ├── file_utils.c
│       ├── string_utils.h       Character classification helpers
│       └── string_utils.c
├── tests/
│   ├── test_framework.h         Minimal test harness (ASSERT, RUN_TEST, etc.)
│   ├── test_tokenizer.c         Tokenizer unit tests
│   └── test_formatter.c         Formatter unit tests
├── examples/
│   ├── example_unformatted.c    Sample unformatted source
│   └── example1.c               Sample formatted output
├── CMakeLists.txt               Build configuration
└── .cfmt.conf                   Default config file
```

### Data flow

```
Source file (.c/.h)
    │
    ▼
[file_read] - reads entire file into memory
    │
    ▼
[tokenizer] - single-pass lexical scan → flat token array
    │
    ▼
[formatter] - token-driven, single-pass formatting → output buffer
    │
    ▼
[file_write] - writes formatted output back (or stdout / check)
```

### Module details

#### CLI (`cli.h` / `cli.c`)

Parses command-line arguments and configuration file. Dispatches to three modes:
- **stdin mode** - reads stdin, writes formatted output to stdout
- **single file mode** - reads a `.c`/`.h` file, formats in place
- **recursive directory mode** - walks a directory tree, processes all `.c`/`.h` files

Configuration is loaded from the default locations or `--config` path, with each key parsed by `parse_config_line()`.

#### Tokenizer (`tokenizer.h` / `tokenizer.c`)

A character-by-character scanner that produces a flat array of tokens. Handles:
- Identifiers and C keywords (via `is_c_keyword()`)
- Integer, hex, float, and octal literals
- String literals (with escape sequence support)
- Character literals
- All punctuation tokens (`{`, `}`, `(`, `)`, `[`, `]`, `;`, `,`, `.`, `->`, `...`, `?`, `:`)
- Operators (single-, double-, and triple-character)
- Preprocessor directives (from `#` to end of line)
- Line comments (`//`)
- Block comments (`/* */`)

#### Formatter (`formatter.h` / `formatter.c`)

A token-driven formatting engine that walks the token stream once and emits formatted output. Maintains:
- **Brace depth** - for correct indentation
- **Parenthesis depth** - for detecting `for(...)` headers
- **Declaration context** - to distinguish `int *ptr` from arithmetic `a * b`
- **For-header context** - semicolons inside `for(...)` don't emit newlines

Key formatting decisions are made by three helper functions:
- `newlines_before()` - determines if blank lines should separate syntax constructs
- `space_before()` - determines spacing between adjacent tokens (operators, keywords, punctuation)
- `ptr_asterisk_space_before()` - context-aware spacing for `*` (declaration vs dereference vs multiplication)

#### Utilities

- **buffer** - dynamic char buffer with geometric growth. Provides append, trim, and detach operations.
- **file_utils** - file read/write, source file detection (`.c`/`.h`), directory walking with callback.
- **string_utils** - character classification (`str_isident`, `str_isdigit`, `str_isspace`, etc.).

---

## Testing

cfmt includes a minimal header-only test framework (`tests/test_framework.h`) and two test suites:

```sh
# Build and run
cmake .. -DBUILD_TESTS=ON
make
ctest --output-on-failure

# Run individually
./build/test_tokenizer
./build/test_formatter
```

### Test coverage

**Tokenizer** - 21 tests covering:
- Empty input, identifiers, keywords
- Integer, hex, and float literals
- String and character literals (including escape sequences)
- All punctuation types
- Operators (single and compound)
- Arrow, ellipsis
- Line and block comments
- Preprocessor directives
- Ternary `?` and `:`

**Formatter** - 16 tests covering:
- Simple function formatting
- Nested indentation
- Operator spacing
- Control keyword spacing
- Pointer declaration vs arithmetic vs dereference
- Line and block comment preservation
- Preprocessor handling
- Empty function bodies
- For-loop formatting
- Trailing whitespace removal
- Idempotency
- Struct formatting
- Deeply nested braces

To add a new test:

```c
static void test_my_feature(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("input code here", &out, cfg);
    ASSERT(strstr(out.data, "expected output") != NULL);
    buffer_free(&out);
}

// In main():
RUN_TEST(test_my_feature);
```

---

## Contributing

1. Ensure the formatter remains **idempotent** - formatting a file twice should produce the same output.
2. Keep it **pure C17** - no C++, no libclang, no external dependencies.
3. Run the full test suite before opening a PR: `cmake .. -DBUILD_TESTS=ON && make && ctest`.
4. Format your own source code with cfmt before committing.
5. Preserve the single-pass design for both tokenizer and formatter.

---

## License

[GPL-3.0](LICENSE)
