#include <string.h>
#include <ctype.h>

// strndup is not a C standard and is not in Windows
// https://github.com/msys2/MINGW-packages/issues/4999
#if defined(_WIN32) || defined(_WIN64)
#include <stdlib.h>

char * strndup(const char * src, size_t size) {
    size_t len = strnlen(src, size);
    len = len < size ? len : size;
    char * dst = malloc(len + 1);
    if (!dst)
        return NULL;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

#endif

char *cross_strndup(const char * src, size_t size) { return strndup(src, size); }

// https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
char *trim(char *s) {
    int l = strlen(s);

    while(isspace(s[l - 1])) --l;
    while(*s && isspace(*s)) ++s, --l;

    return strndup(s, l);
}