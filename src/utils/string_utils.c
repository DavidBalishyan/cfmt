#include "string_utils.h"

bool str_isident_start(char c) {
    return (c >= 'a'&& c <= 'z') ||(c >= 'A'&& c <= 'Z') || c == '_';
}

bool str_isident(char c) {
    return str_isident_start(c) ||(c >= '0'&& c <= '9');
}

bool str_isdigit(char c) {
    return c >= '0'&& c <= '9';
}

bool str_isxdigit(char c) {
    return str_isdigit(c) ||(c >= 'a'&& c <= 'f') ||(c >= 'A'&& c <= 'F');
}

bool str_isspace(char c) {
    return c == ' '|| c == '\t'|| c == '\n'|| c == '\r'|| c == '\f'|| c == '\v';
}
