#include "can_commander_compat.h"

#include <string.h>

size_t cc_strnlen(const char* s, size_t maxlen) {
    const char* found = memchr(s, '\0', maxlen);
    return found ? (size_t)(found - s) : maxlen;
}

char* cc_strtok_r(char* str, const char* delim, char** saveptr) {
    char* start = str ? str : *saveptr;
    if(!start) {
        return NULL;
    }

    start += strspn(start, delim);
    if(*start == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    char* end = start + strcspn(start, delim);
    if(*end) {
        *end = '\0';
        *saveptr = end + 1;
    } else {
        *saveptr = NULL;
    }
    return start;
}
