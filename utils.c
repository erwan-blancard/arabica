#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "trim.h"


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
// returns count as -1 on mem error
TOKEN_LIST extract_tokens(char *line) {
    TOKEN_LIST result = {0, NULL};
    size_t line_length = strlen(line);

    size_t i = 0;
    size_t first_valid_pos = 0;
    char in_string = 0;      // if we reached "
    char in_char = 0;        // if we reached '
    while (i < line_length) {
        char parse = 0;     // flag to parse the token at end of loop

        if (line[i] == '\"') {

            if (in_string) {
                parse = 1;
            } else {
                in_string = 1;
            }

        } else if (line[i] == '\'') {

            if (in_char) {
                parse = 1;
            } else {
                in_char = 1;
            }

        } else if (isblank(line[i])) {

            if (!in_string && !in_char) {
                if (first_valid_pos == i) {
                    first_valid_pos++;
                } else {
                    parse = 1;
                }
            }

        } 
        
        if (i != 0 && i == line_length-1 && first_valid_pos-i != 0) {   // parse if we reached the end of the line

            // printf("Parse because end: i=%lld first=%lld\n", i, first_valid_pos);
            if (line[i] != '\n')
                i++;
            parse = 1;

        }

        if (parse) {
            char *token = cross_strndup(line+first_valid_pos, i-first_valid_pos+(in_string || in_char));
            if (result.count == 0) {
                result.tokens = (char**)malloc(sizeof(char*));
                if (!result.tokens) {
                    result.count = -1;
                    return result;
                }
            } else {
                char **tmp = (char**)realloc(result.tokens, sizeof(char*) * (result.count+1));
                if (!tmp) {
                    free(result.tokens);
                    result.count = -1;
                    return result;
                } else {
                    result.tokens = tmp;
                }
            }

            result.tokens[result.count] = token;
            result.count++;

            in_string = 0;
            in_char = 0;
            first_valid_pos = i+1;
        }

        i++;
    }

    return result;
}

// https://stackoverflow.com/questions/43163677/how-do-i-strip-a-file-extension-from-a-string-in-c
void strip_ext(char *fname) {
    char *end = fname + strlen(fname);

    while (end > fname && *end != '.' && *end != '\\' && *end != '/') {
        --end;
    }
    if ((end > fname && *end == '.') &&
        (*(end - 1) != '\\' && *(end - 1) != '/')) {
        *end = '\0';
    }  
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