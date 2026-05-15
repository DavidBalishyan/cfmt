# cfmt - C Code Formatter

A fast, modular C code formatter written entirely in C17. No C++, no libclang dependencies.

## Features

- Format `.c` and `.h` files
- Whitespace normalization (4-space indentation, no tabs)
- Brace placement normalization (same-line style)
- Pointer spacing: `int *ptr` style
- Spaces around binary operators: `a + b`, `x == y`
- Space after control keywords: `if (...)`, `while (...)`
- Comment preservation (both `//` and `/* */`)
- Preprocessor directive alignment
- Trailing whitespace removal
- Redundant empty line removal
- Deterministic and idempotent output
- Recursive directory processing
- Check mode (CI integration)
- stdin/stdout support

## Building

```sh
mkdir build && cd build
cmake ..
make
```

Or with tests:

```sh
cmake .. -DBUILD_TESTS=ON
make && ctest
```

## Usage

```sh
# Format a single file
cfmt main.c

# Format all files in directory recursively
cfmt -r src/

# Check mode (exit 1 if changes needed)
cfmt --check .

# Read from stdin, write to stdout
cat test.c | cfmt

# Custom config file
cfmt --config /path/to/.cfmt.conf file.c
```

## Configuration

Create `~/.cfmt.conf` or pass `--config <path>`:

```
indent_width = 4
max_line_width = 100
brace_same_line = true
pointer_left = true
no_tabs = true
```

## Architecture

```
src/
  tokenizer/    Lexical analysis / token stream generation
  formatter/    Indentation, spacing, line wrapping engine
  cli/          Command-line interface and config parsing
  utils/        File I/O, dynamic buffers, string helpers
tests/          Unit tests (tokenizer + formatter)
examples/       Example inputs and formatted outputs
```

### Tokenizer

Character-by-character scanner producing a flat token array. Handles identifiers, keywords, numbers, strings, chars, operators, preprocessor directives, and comments.

### Formatter

Token-driven formatting engine. Walks the token stream, maintains formatting context (brace depth, declaration context, parentheses tracking), and applies spacing/indentation rules deterministically.

## Performance

- Reads entire file into memory
- Single pass tokenization
- Single pass formatting
- Minimal allocations with reusable buffers
- Efficient handling of large files

## Requirements

- C17 compiler (GCC, Clang)
- CMake >= 3.14
- POSIX system (Linux, macOS)
