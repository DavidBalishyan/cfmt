#include "test_framework.h"
#include "tokenizer/tokenizer.h"
#include <string.h>

static void test_empty_source(void) {
    token_array_t arr = tokenize("", 0);
    ASSERT_EQ(arr.count, 0);
    token_array_free(&arr);
}

static void test_identifier(void) {
    token_array_t arr = tokenize("hello", 5);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_IDENTIFIER);
    ASSERT_EQ(arr.tokens[0].length, 5);
    ASSERT(memcmp(arr.tokens[0].start, "hello", 5) == 0);
    token_array_free(&arr);
}

static void test_keyword(void) {
    token_array_t arr = tokenize("int", 3);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_KEYWORD);
    token_array_free(&arr);
}

static void test_number_int(void) {
    token_array_t arr = tokenize("42", 2);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_NUMBER);
    token_array_free(&arr);
}

static void test_number_hex(void) {
    token_array_t arr = tokenize("0xFF", 4);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_NUMBER);
    token_array_free(&arr);
}

static void test_number_float(void) {
    token_array_t arr = tokenize("3.14", 4);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_NUMBER);
    token_array_free(&arr);
}

static void test_string(void) {
    token_array_t arr = tokenize("\"hello world\"", 13);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_STRING);
    ASSERT_EQ(arr.tokens[0].length, 13);
    token_array_free(&arr);
}

static void test_string_escape(void) {
    token_array_t arr = tokenize("\"hello\\\"world\"", 14);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_STRING);
    token_array_free(&arr);
}

static void test_char(void) {
    token_array_t arr = tokenize("'x'", 3);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_CHAR);
    token_array_free(&arr);
}

static void test_braces(void) {
    token_array_t arr = tokenize("{}", 2);
    ASSERT_EQ(arr.count, 2);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_OPEN_BRACE);
    ASSERT_EQ(arr.tokens[1].type, TOKEN_CLOSE_BRACE);
    token_array_free(&arr);
}

static void test_parens(void) {
    token_array_t arr = tokenize("()", 2);
    ASSERT_EQ(arr.count, 2);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_OPEN_PAREN);
    ASSERT_EQ(arr.tokens[1].type, TOKEN_CLOSE_PAREN);
    token_array_free(&arr);
}

static void test_brackets(void) {
    token_array_t arr = tokenize("[]", 2);
    ASSERT_EQ(arr.count, 2);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_OPEN_BRACKET);
    ASSERT_EQ(arr.tokens[1].type, TOKEN_CLOSE_BRACKET);
    token_array_free(&arr);
}

static void test_semicolon(void) {
    token_array_t arr = tokenize(";", 1);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_SEMICOLON);
    token_array_free(&arr);
}

static void test_comma(void) {
    token_array_t arr = tokenize(",", 1);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_COMMA);
    token_array_free(&arr);
}

static void test_operators(void) {
    token_array_t arr = tokenize("+ - * / = == != < > <= >= && ||", 34);
    ASSERT_EQ(arr.count, 13);
    for (size_t i = 0; i < arr.count; i++) {
        ASSERT_EQ(arr.tokens[i].type, TOKEN_OPERATOR);
    }

    token_array_free(&arr);
}

static void test_arrow(void) {
    token_array_t arr = tokenize("->", 2);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_ARROW);
    token_array_free(&arr);
}

static void test_ellipsis(void) {
    token_array_t arr = tokenize("...", 3);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_ELLIPSIS);
    ASSERT_EQ(arr.tokens[0].length, 3);
    token_array_free(&arr);
}

static void test_ellipsis_in_decl(void) {
    token_array_t arr = tokenize("int printf(const char *fmt, ...);", 34);
    ASSERT(arr.count >= 4);
    int ellipsis_idx =-1;
    for (size_t i = 0; i < arr.count; i++) {
        if (arr.tokens[i].type == TOKEN_ELLIPSIS) ellipsis_idx =(int) i;
    }

    ASSERT_NE(ellipsis_idx, -1);
    token_array_free(&arr);
}

static void test_line_comment(void) {
    token_array_t arr = tokenize("// comment\nint", 14);
    ASSERT_EQ(arr.count, 2);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_COMMENT_LINE);
    ASSERT_EQ(arr.tokens[1].type, TOKEN_KEYWORD);
    token_array_free(&arr);
}

static void test_block_comment(void) {
    token_array_t arr = tokenize("/* comment */", 13);
    ASSERT_EQ(arr.count, 1);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_COMMENT_BLOCK);
    token_array_free(&arr);
}

static void test_preprocessor(void) {
    token_array_t arr = tokenize("#include <stdio.h>\nint x;", 25);
    ASSERT_EQ(arr.count, 4);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_PREPROCESSOR);
    ASSERT_EQ(arr.tokens[1].type, TOKEN_KEYWORD);
    ASSERT_EQ(arr.tokens[2].type, TOKEN_IDENTIFIER);
    ASSERT_EQ(arr.tokens[3].type, TOKEN_SEMICOLON);
    token_array_free(&arr);
}

static void test_question_colon(void) {
    token_array_t arr = tokenize("? :", 3);
    ASSERT(arr.count >= 2);
    ASSERT_EQ(arr.tokens[0].type, TOKEN_QUESTION);
    ASSERT_EQ(arr.tokens[1].type, TOKEN_COLON);
    token_array_free(&arr);
}

int main(void) {
    printf("Tokenizer Tests:\n");
    RUN_TEST(test_empty_source);
    RUN_TEST(test_identifier);
    RUN_TEST(test_keyword);
    RUN_TEST(test_number_int);
    RUN_TEST(test_number_hex);
    RUN_TEST(test_number_float);
    RUN_TEST(test_string);
    RUN_TEST(test_string_escape);
    RUN_TEST(test_char);
    RUN_TEST(test_braces);
    RUN_TEST(test_parens);
    RUN_TEST(test_brackets);
    RUN_TEST(test_semicolon);
    RUN_TEST(test_comma);
    RUN_TEST(test_operators);
    RUN_TEST(test_arrow);
    RUN_TEST(test_ellipsis);
    RUN_TEST(test_ellipsis_in_decl);
    RUN_TEST(test_line_comment);
    RUN_TEST(test_block_comment);
    RUN_TEST(test_preprocessor);
    RUN_TEST(test_question_colon);
    TEST_SUMMARY();
    return tests_failed > 0 ? 1: 0;
}
