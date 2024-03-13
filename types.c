#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "trim.h"


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


ARGUMENT_TYPE get_argument_type_by_keyword(char *keyword) {
    if (strcmp(keyword, NUM_8_KEY) == 0) {
        return NUM_8;
    } else if (strcmp(keyword, NUM_32_KEY) == 0) {
        return NUM_32;
    } else if (strcmp(keyword, VAR_KEY) == 0) {
        return VAR;
    } else if (strcmp(keyword, ADDR_KEY) == 0) {
        return ADDR;
    } else if (strcmp(keyword, STR_KEY) == 0) {
        return STR;
    } else if (strcmp(keyword, CHAR_KEY) == 0) {
        return CHAR;
    }
    
    return INVALID;
}


char *get_argument_type_name(ARGUMENT_TYPE type) {
    switch (type)
    {
    case NUM_8:
        return "NUM_8";
    case NUM_32:
        return "NUM_32";
    case VAR:
        return "VAR";
    case STR:
        return "STR";
    case CHAR:
        return "CHAR";
    case ADDR:
        return "ADDR";
    case INVALID:
        return "INVALID";
    default:
        return "UNKNOWN";
    }
}


int is_int(char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        if (!isdigit(str[i]))
            return 0;
    }
    return 1;
}


LOAD_INSTRUCTION_RESULT load_language_instructions() {
    LOAD_INSTRUCTION_RESULT result = {RET_OK, NULL, 0, NULL};

    FILE *f;

    f = fopen("instr.def", "r");
    
    if (f == NULL) {
        result.retcode = RET_BAD_FILE_HANDLE;
        return result;
    }

    int instr_count = 0;
    INSTRUCTION *instructions = NULL;

    char line_buffer[256];

    // buffers for instruction list
    char instr_name[INSTR_MAX_LEN];
    int instr_id;
    short arg_count;
    ARGUMENT_TYPE *instr_args = NULL;

    // iterate over lines
    while (fgets(line_buffer, 256, f) != NULL) {
        // remove white spaces around text
        strcpy(line_buffer, trim(line_buffer));

        // get the first token (instruction name)

        char *token = strtok(line_buffer, " ");
        // if line is empty, continue
        if (token == NULL)
            continue;

        if (strlen(token) > INSTR_MAX_LEN) {
            result.retcode = RET_INVALID_SYNTAX;
            char reason[] = "Instruction name \"%s\" is too long !";
            result.reason = (char*)malloc(sizeof(char)*(strlen(reason)-2+strlen(token)));
            if (result.reason != NULL)
                sprintf(result.reason, reason, token);
            if (instructions != NULL)
                free(instructions);
            fclose(f);
            return result;
        }
        
        strcpy(instr_name, token);
        
        // get instr ID

        token = strtok(NULL, " ");

        if (token == NULL) {
            result.retcode = RET_INVALID_SYNTAX;
            char reason[] = "Missing ID for instruction \"%s\" !";
            result.reason = (char*)malloc(sizeof(char)*(strlen(reason)-2+INSTR_MAX_LEN));
            if (result.reason != NULL)
                sprintf(result.reason, reason, instr_name);
            if (instructions != NULL)
                free(instructions);
            fclose(f);
            return result;
        }

        if (!is_int(token)) {
            result.retcode = RET_INVALID_SYNTAX;
            char reason[] = "Invalid ID \"%s\" for instruction \"%s\" !";
            result.reason = (char*)malloc(sizeof(char)*(strlen(reason)-4+INSTR_MAX_LEN+strlen(token)));
            if (result.reason != NULL)
                sprintf(result.reason, reason, token, instr_name);
            if (instructions != NULL)
                free(instructions);
            fclose(f);
            return result;
        }
        
        instr_id = atoi(token);

        // get instruction arguments

        arg_count = 0;

        while (arg_count < 256) {
            token = strtok(NULL, " ");
            
            if (token != NULL) {
                ARGUMENT_TYPE arg_type = get_argument_type_by_keyword(token);
                if (arg_type == INVALID) {
                    result.retcode = RET_INVALID_SYNTAX;
                    char reason[] = "Invalid Argument \"%s\" for instruction \"%s\" !";
                    result.reason = (char*)malloc(sizeof(char)*(strlen(reason)-4+INSTR_MAX_LEN+strlen(token)));
                    if (result.reason != NULL)
                        sprintf(result.reason, reason, token, instr_name);
                    if (instructions != NULL)
                        free(instructions);
                    fclose(f);
                    return result;
                }

                if (arg_count == 0) {
                    instr_args = (ARGUMENT_TYPE*)malloc(sizeof(ARGUMENT_TYPE)*(arg_count+1));
                    // check if malloc failed
                    if (instr_args == NULL) {
                        result.retcode = RET_MEM_ALLOC_FAILED;
                        if (instructions != NULL)
                            free(instructions);
                        fclose(f);
                        return result;
                    }
                } else {
                    ARGUMENT_TYPE *tmp = (ARGUMENT_TYPE*)realloc(instr_args, sizeof(ARGUMENT_TYPE)*(arg_count+1));
                    // check if realloc failed
                    if (tmp == NULL) {
                        result.retcode = RET_MEM_ALLOC_FAILED;
                        if (instructions != NULL)
                            free(instructions);
                        fclose(f);
                        free(instr_args);
                        return result;
                    } else {
                        instr_args = tmp;
                    }
                }
                instr_args[arg_count] = arg_type;

                arg_count++;
            } else {
                break;
            }
        }

        if (instr_count == 0) {
            instructions = (INSTRUCTION*)malloc(sizeof(INSTRUCTION)*(instr_count+1));
            // check if malloc failed
            if (instructions == NULL) {
                result.retcode = RET_MEM_ALLOC_FAILED;
                if (instr_args != NULL)
                    free(instr_args);
                fclose(f);
                return result;
            }
        } else {
            INSTRUCTION *tmp = (INSTRUCTION*)realloc(instructions, sizeof(INSTRUCTION)*(instr_count+1));
            // check if realloc failed
            if (tmp == NULL) {
                result.retcode = RET_MEM_ALLOC_FAILED;
                if (instr_args != NULL)
                    free(instr_args);
                free(instructions);
                fclose(f);
                return result;
            } else {
                instructions = tmp;
            }
        }

        INSTRUCTION instruction;

        strcpy(instruction.name, instr_name);
        instruction.id = instr_id;
        instruction.arg_count = arg_count;
        instruction.args = instr_args;

        instructions[instr_count] = instruction;

        instr_count++;

    }
    
    fclose(f);

    result.instr_count = instr_count;
    result.instructions = instructions;
    return result;
}


// TODO move to compiler
// typedef struct {
//     char *name;

//     // Will be used to calculate an offset 
//     // of the address of the label for jump instructions.
//     int alignment;
// } LABEL;