cmake_minimum_required(VERSION 3.14)
project(cfmt VERSION 1.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

set(SOURCES
    src/main.c
    src/cli/cli.c
    src/tokenizer/token.c
    src/tokenizer/tokenizer.c
    src/formatter/formatter.c
    src/utils/buffer.c
    src/utils/string_utils.c
    src/utils/file_utils.c
)

add_executable(cfmt ${SOURCES})

target_include_directories(cfmt PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    set(TEST_SOURCES
        tests/test_tokenizer.c
        tests/test_formatter.c
        src/tokenizer/token.c
        src/tokenizer/tokenizer.c
        src/formatter/formatter.c
        src/utils/buffer.c
        src/utils/string_utils.c
        src/utils/file_utils.c
    )

    add_executable(test_tokenizer
        tests/test_tokenizer.c
        src/tokenizer/token.c
        src/tokenizer/tokenizer.c
        src/utils/buffer.c
        src/utils/string_utils.c
    )
    target_include_directories(test_tokenizer PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
    )

    add_executable(test_formatter
        tests/test_formatter.c
        src/tokenizer/token.c
        src/tokenizer/tokenizer.c
        src/formatter/formatter.c
        src/utils/buffer.c
        src/utils/string_utils.c
    )
    target_include_directories(test_formatter PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
    )

    enable_testing()
    add_test(NAME test_tokenizer COMMAND test_tokenizer)
    add_test(NAME test_formatter COMMAND test_formatter)
endif()
