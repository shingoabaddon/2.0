#pragma once

#include <stddef.h>

// strnlen and strtok_r are listed in the firmware's SDK but marked
// disabled (never actually exported), so calling them directly leaves the
// app with unresolved imports at runtime. Minimal drop-in replacements.
size_t cc_strnlen(const char* s, size_t maxlen);
char* cc_strtok_r(char* str, const char* delim, char** saveptr);
