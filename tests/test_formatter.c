#include "test_framework.h"
#include "tokenizer/tokenizer.h"
#include "formatter/formatter.h"
#include "utils/buffer.h"
#include <string.h>
#include <stdio.h>

static void format_string(const char *input, buffer_t *out, format_config_t config) {
    buffer_t src;
    buffer_init(&src);
    buffer_append_s(&src, input);
    token_array_t tokens = tokenize(src.data, src.len);
    formatter_t fmt;
    formatter_init(&fmt, tokens.tokens, tokens.count, out, config);
    formatter_run(&fmt);
    token_array_free(&tokens);
    buffer_free(&src);
}

static void test_simple_function(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("int main(){return 0;}", &out, cfg);
    ASSERT(strstr(out.data, "int main() {") != NULL);
    ASSERT(strstr(out.data, "    return 0;") != NULL);
    buffer_free(&out);
}

static void test_indentation(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("void f(){if(1){x=1;}}", &out, cfg);
    const char *result = out.data;
    ASSERT(strstr(result, "if (1) {") != NULL);
    ASSERT(strstr(result, "    x = 1;") != NULL);
    buffer_free(&out);
}

static void test_spacing_around_operators(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("int x=1+2;", &out, cfg);
    ASSERT(strstr(out.data, "x = 1 + 2") != NULL);
    buffer_free(&out);
}

static void test_control_keywords(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("if(x){} while(y){} for(;;){} switch(z){}", &out, cfg);
    ASSERT(strstr(out.data, "if (x)") != NULL);
    ASSERT(strstr(out.data, "while (y)") != NULL);
    ASSERT(strstr(out.data, "for (;;)") != NULL);
    ASSERT(strstr(out.data, "switch (z)") != NULL);
    buffer_free(&out);
}

static void test_pointer_decl(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("int*ptr;", &out, cfg);
    ASSERT(strstr(out.data, "int *ptr") != NULL);
    buffer_free(&out);
}

static void test_pointer_arithmetic(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("x=a*b;", &out, cfg);
    ASSERT(strstr(out.data, "a * b") != NULL);
    buffer_free(&out);
}

static void test_deref(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("x=*ptr;", &out, cfg);
    ASSERT(strstr(out.data, "= *ptr") != NULL);
    buffer_free(&out);
}

static void test_line_comment_preserved(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("int x;// this is a comment\nint y;", &out, cfg);
    ASSERT(strstr(out.data, "// this is a comment") != NULL);
    buffer_free(&out);
}

static void test_block_comment(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("int x;/* block */int y;", &out, cfg);
    ASSERT(strstr(out.data, "/* block */") != NULL);
    buffer_free(&out);
}

static void test_preprocessor(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("#include <stdio.h>\nint x;", &out, cfg);
    char *first_line = out.data;
    size_t nl = 0;
    while (first_line[nl] && first_line[nl] != '\n') nl++;
    ASSERT(nl > 0);
    ASSERT(strstr(out.data, "#include <stdio.h>") != NULL);
    ASSERT(strstr(out.data, "int x;") != NULL);
    buffer_free(&out);
}

static void test_empty_function(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("void f(){}", &out, cfg);
    ASSERT(strstr(out.data, "void f() {\n}") != NULL || strstr(out.data, "void f() {}") != NULL);
    buffer_free(&out);
}

static void test_for_loop(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("void f(){for(int i=0;i<10;i++){x=i;}}", &out, cfg);
    ASSERT(strstr(out.data, "for (int i = 0; i < 10; i++)") != NULL);
    buffer_free(&out);
}

static void test_trailing_whitespace(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("int x;   \nint y;", &out, cfg);
    const char *result = out.data;
    const char *first_line_end = strstr(result, "\n");
    if (first_line_end) {
        ptrdiff_t len = first_line_end - result;
        ASSERT(len > 0);
        ASSERT(result[len - 1] != ' ');
    }

    buffer_free(&out);
}

static void test_idempotent(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out1, out2;
    buffer_init(&out1);
    buffer_init(&out2);
    const char *input = "int main(){\nint x=1+2;\nreturn x;\n}\n";
    format_string(input, &out1, cfg);
    format_string(out1.data, &out2, cfg);
    ASSERT_EQ(out1.len, out2.len);
    ASSERT(memcmp(out1.data, out2.data, out1.len) == 0);
    buffer_free(&out1);
    buffer_free(&out2);
}

static void test_struct(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("struct point{int x;int y;};", &out, cfg);
    ASSERT(strstr(out.data, "struct point {") != NULL);
    ASSERT(strstr(out.data, "    int x;") != NULL);
    ASSERT(strstr(out.data, "    int y;") != NULL);
    buffer_free(&out);
}

static void test_nested_braces(void) {
    format_config_t cfg = FORMAT_DEFAULT_CONFIG;
    buffer_t out;
    buffer_init(&out);
    format_string("void f(){if(1){if(2){x=1;}}}", &out, cfg);
    const char *r = out.data;
    ASSERT(strstr(r, "if (1) {") != NULL);
    ASSERT(strstr(r, "    if (2) {") != NULL);
    ASSERT(strstr(r, "        x = 1;") != NULL);
    buffer_free(&out);
}

int main(void) {
    printf("Formatter Tests:\n");
    RUN_TEST(test_simple_function);
    RUN_TEST(test_indentation);
    RUN_TEST(test_spacing_around_operators);
    RUN_TEST(test_control_keywords);
    RUN_TEST(test_pointer_decl);
    RUN_TEST(test_pointer_arithmetic);
    RUN_TEST(test_deref);
    RUN_TEST(test_line_comment_preserved);
    RUN_TEST(test_block_comment);
    RUN_TEST(test_preprocessor);
    RUN_TEST(test_empty_function);
    RUN_TEST(test_for_loop);
    RUN_TEST(test_trailing_whitespace);
    RUN_TEST(test_idempotent);
    RUN_TEST(test_struct);
    RUN_TEST(test_nested_braces);
    TEST_SUMMARY();
    return tests_failed > 0 ? 1: 0;
}
