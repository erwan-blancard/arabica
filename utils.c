#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "types.h"


typedef struct {
    int count;
    char **tokens;
} TOKEN_LIST;


// TODO must output like this:
//
// SET_CHAR 'c' 1 my_str_var
// -> {SET_CHAR, 'c', 1, my_str_var}
//
// LOAD_STR "Hello"
// -> {LOAD_STR, "Hello"}
//
// TODO do negative numbers
TOKEN_LIST extract_tokens(char *line) {
    TOKEN_LIST result = {0, NULL};



    return result;
}


int is_number(char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        if (!isdigit(str[i]))
            return 0;
    }
    return 1;
}


int is_signed_number(char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        if (!isdigit(str[i]) && (i == 0 && str[i] != '-'))
            return 0;
    }
    return 1;
}


//entier en big endian
uint32_t convertToBigEndian(uint32_t num) {
    unsigned char *ptr = (unsigned char *)&num;
    uint32_t bigEndianNumber = 0;

    //copie les octets en commençant par le dernier (big endian)
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        bigEndianNumber = (bigEndianNumber << 8) | *(ptr + i);
    }

    return bigEndianNumber;
}


int arg_is_char(char *token) {
    return (strlen(token) == 3 && token[0] == '\'' && token[2] == '\'');
}


int arg_is_str(char *token) {
    int len = strlen(token);
    return (len > 1 && token[0] == '\"' && token[len-1] == '\"');
}


void *parse_argument_to_uint8(char *token) {
    if (is_number(token)) {
        int num = atoi(token);
        // if value range is valid
        if (num <= UINT8_MAX) {
            uint8_t num8 = (uint8_t)num;
            void *arg = malloc(sizeof(uint8_t));
            if (arg != NULL) {
                uint8_t *ptr_uint8 = &num8;
                memcpy(arg, ptr_uint8, sizeof(uint8_t));
                return arg;
            }
        }
    }
    return NULL;
}


void *parse_argument_to_int32(char *token) {
    if (is_signed_number(token)) {
        int32_t num32 = (int32_t)atoi(token);
        void *arg = malloc(sizeof(int32_t));
        if (arg != NULL) {
            int32_t *ptr_uint32 = &num32;
            memcpy(arg, ptr_uint32, sizeof(int32_t));
            return arg;
        }
    }
    return NULL;
}


void *parse_argument_to_char(char *token) {
    // if '<char>'
    if (arg_is_char(token)) {
        void *arg = malloc(sizeof(char));
        if (arg != NULL) {
            memcpy(arg, token+(sizeof(char)), sizeof(char));
            return arg;
        }
    }
    return NULL;
}


void *parse_argument_to_str(char *token) {
    // if "<string>"
    if (arg_is_str(token)) {
        int sublen = strlen(token)-2+1;
        void *arg = malloc(sizeof(char) * (sublen));
        if (arg != NULL) {
            char term = '\0';
            if (sublen-1 > 0) {
                memcpy(arg, token+(sizeof(char)), sizeof(char) * (sublen-1));
                memcpy(arg+sizeof(char) * (sublen-1), &term, sizeof(char));     // null term
            } else {
                memcpy(arg, &term, sizeof(char));     // null term
            }
            return arg;
        }
    }
    return NULL;
}