#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "trim.h"

char *str_with_escaped_chars(char *str);

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
            char *escaped_arg = str_with_escaped_chars(arg);
            free(arg);
            return escaped_arg;
        }
    }
    return NULL;
}


char *str_with_escaped_chars(char *str) {
    int len = (int)strlen(str);
    char *buff = (char*)malloc(sizeof(char) * (len+1));
    if (!buff) {
        printf("Could not parse string with characters escaped for \"%s\": malloc for buffer failed !\n", str);
        return NULL;
    }

    int buff_index = 0;
    int i = 0;
    while (i < len) {
        // char to write
        char c = '\0';

        if (str[i] == '\\' && i+1 < len) {
            switch (str[i+1]) {
            case 'a': c = 0x07;
                break;
            case 'b': c = 0x08;
                break;
            case 'e': c = 0x1B;
                break;
            case 'f': c = 0x0C;
                break;
            case 'n': c = 0x0A;
                break;
            case 'r': c = 0x0D;
                break;
            case 't': c = 0x09;
                break;
            case 'v': c = 0x0B;
                break;
            case '\\': c = 0x5C;
                break;
            case '\'': c = 0x27;
                break;
            case '\"': c = 0x22;
                break;
            case '\?': c = 0x3F;
                break;
            default:
                printf("Fallen in default case for char %c (i:%d, len:%d, str: %s)\n", str[i+1], i-2, len, str);
                c = '\\';
                i--;    // do not skip next char (i += 1 instead of 2)
                break;
            }
            i += 2;     // skip char on next iteration (not if nothing found by switch case block, see default case)
        } else {
            c = str[i];
            i++;
        }

        buff[buff_index] = c;
        buff_index++;

    }

    buff[buff_index] = '\0';

    return buff;
}
