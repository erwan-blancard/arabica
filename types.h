#ifndef TYPES

#define INSTR_MAX_LEN 32

// keywords used for instruction definition
#define NUM_8_KEY "uint8"
#define NUM_32_KEY "uint32"
#define CHAR_KEY "char"
#define VAR_KEY "variable"
#define ADDR_KEY "address"
#define STR_KEY "string"

typedef enum {
    RET_OK,
    RET_BAD_FILE_HANDLE,
    RET_INVALID_SYNTAX,
    RET_MEM_ALLOC_FAILED
} LOAD_RET_CODE;

typedef enum {
    INVALID,    // Only used by get_argument_type_by_keyword()
    NUM_8,      // uint8 number
    NUM_32,     // uint32 number
    CHAR,       // 'c'
    VAR,        // Variable (register 0-255)
    ADDR,       // address or label
    STR         // String
} ARGUMENT_TYPE;

typedef struct {
    char name[INSTR_MAX_LEN];
    int id;
    short arg_count;
    ARGUMENT_TYPE *args;
} INSTRUCTION;

typedef struct {
    LOAD_RET_CODE retcode;
    char *reason;
    int instr_count;
    INSTRUCTION *instructions;
} LOAD_INSTRUCTION_RESULT;

ARGUMENT_TYPE get_argument_type_by_keyword(char *keyword);
char *get_argument_type_name(ARGUMENT_TYPE type);
int is_int(char *str);
LOAD_INSTRUCTION_RESULT load_language_instructions();

#endif