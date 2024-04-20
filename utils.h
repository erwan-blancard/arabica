#ifndef UTILS
#define UTILS
#include <stdint.h>

typedef struct {
    int count;
    char **tokens;
} TOKEN_LIST;

TOKEN_LIST extract_tokens(char *line);

void strip_ext(char *fname);

int is_number(char *str);
int is_signed_number(char *str);

uint32_t convertToBigEndian(uint32_t num);

int arg_is_char(char *token);
int arg_is_str(char *token);

void *parse_argument_to_uint8(char *token);
void *parse_argument_to_int32(char *token);
void *parse_argument_to_char(char *token);
void *parse_argument_to_str(char *token);
char *str_with_escaped_chars(char *str);
#endif