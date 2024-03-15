#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "types.h"
#include "trim.h"
#include "utils.h"

#if defined(_WIN32) || defined(_WIN64)
#define EXEC_NAME "arabica-compiler.exe"
#else
#define EXEC_NAME "arabica-compiler"
#endif

#define MAX_CODE_SIZE UINT32_MAX            // code length in header takes 4 bytes max (uint32)
#define MAX_PROGRAM_NAME_LEN 16             // takes 16 bytes max in header
#define MAX_VARS 255                        // variables are indexed with a 1 byte number (uint8)
#define DEFAULT_PROGRAM_NAME "PROGRAM"
#define EXT_NAME ".cabc"

#define LINE_BUFF_SIZE 256


typedef struct {
    char *name;

    // Will be used to calculate an offset 
    // of the address of the label for jump instructions.
    long alignment;
} LABEL;


void print_help() {
    printf("Usage:\n");
    printf("\t%s <file>\n", EXEC_NAME);
    printf("\t%s <progam-name> <file>\n", EXEC_NAME);
}


long index_of_matching_instruction(char *token, INSTRUCTION *instructions, long count) {
    for (long i = 0; i < count; i++) {
        if (strcmp(token, instructions[i].name) == 0) {
            return i;
        }
    }

    return -1;
}


// TODO free mem
int compile_to_bytecode(const char *source_file, char *program_name) {
    LOAD_INSTRUCTION_RESULT available_instructions = load_language_instructions();

    // intruction load error handling
    if (available_instructions.retcode != RET_OK) {
        if (available_instructions.reason != NULL)
            printf("Error loading language instructions: %s\n", available_instructions.reason);
        return available_instructions.retcode;
    }

    FILE *src = fopen(source_file, "r");
    if (src == NULL) {
        printf("Could not open source file %s !\n", source_file);
        return -1;
    }

    // TODO improve
    int source_name_len = strlen(source_file);
    int ext_name_len = strlen(EXT_NAME);

    char *out_file_name = (char*)malloc(sizeof(char) * (source_name_len+ext_name_len-1));

    if (out_file_name == NULL) {
        printf("Could not create output file !\n");
        return -1;
    }

    strcpy(out_file_name, source_file);

    for (int i = 0; i < ext_name_len; i++) {
        out_file_name[source_name_len+i] = EXT_NAME[i];
    }

    FILE *out = fopen(out_file_name, "wb");
    if (out == NULL) {
        printf("Could not create output file %s !\n", out_file_name);
        return -1;
    }

    long code_size = 0;

    long instr_count = 0;
    INSTRUCTION *instructions = NULL;

    int var_count = 0;
    char **variables = (char**)malloc(sizeof(char*) * MAX_VARS);
    
    long label_count = 0;
    LABEL *labels = NULL;


    char line_buff[LINE_BUFF_SIZE];
    long line_number = 0;
    while (fgets(line_buff, LINE_BUFF_SIZE, src) != NULL) {
        TOKEN_LIST token_list = extract_tokens(line_buff);

        if (token_list.count == 0) {
            line_number++;
            continue;
        }

        // check if label
        if (token_list.count == 1 && token_list.tokens[0][strlen(token_list.tokens[0])-1] == ':') {

            if (label_count == 0) {
                labels = (LABEL*)malloc(sizeof(LABEL)*(label_count+1));
                // check if malloc failed
                if (labels == NULL) {
                    printf("Failed to allocate memory for label !");
                    return -1;
                }
            } else {
                LABEL *tmp = (LABEL*)realloc(labels, sizeof(LABEL)*(label_count+1));
                // check if realloc failed
                if (tmp == NULL) {
                    printf("Failed to reallocate memory for label !");
                    return -1;
                } else {
                    labels = tmp;
                }
            }

            char *label_name = (char*)malloc(sizeof(char) * strlen(token_list.tokens[0])-1);
            if (label_name == NULL) {
                printf("Failed to allocate memory for label name !");
                return -1;
            }

            strncpy(label_name, token_list.tokens[0], strlen(token_list.tokens[0])-1);
            labels[label_count].name = label_name;
            labels[label_count].alignment = code_size;

        } else {        // if instruction

            // get instruction based on name token
            long instr_index = index_of_matching_instruction(token_list.tokens[0], available_instructions.instructions, available_instructions.instr_count);
            
            if (instr_index == -1) {
                printf("Error: Instruction \"%s\" does not match any defined program instructions ! (Ln:%ld)\n", token_list.tokens[0], line_number);
                return -1;
            }

            // check args count
            if (token_list.count-1 != available_instructions.instructions[instr_index].arg_count) {
                printf("Error: Invalid number of arguments. Expected %d, got %d. (Ln:%ld)\n", available_instructions.instructions[instr_index].arg_count, token_list.count-1, line_number);
                return -1;
            }
            
            // check args
            for (int i = 0; i < available_instructions.instructions[instr_index].arg_count; i++) {
                void *arg = NULL;
                switch (available_instructions.instructions[instr_index].args[i]) {
                case NUM_U8:
                    arg = parse_argument_to_uint8(token_list.tokens[i+1]);
                    break;
                case NUM_32:
                    arg = parse_argument_to_int32(token_list.tokens[i+1]);
                    break;
                case CHAR:
                    arg = parse_argument_to_char(token_list.tokens[i+1]);
                    break;
                case VAR:
                    // if token isn't surrounded with "" or '' and is not an number
                    if (!arg_is_char(token_list.tokens[i+1]) && !arg_is_str(token_list.tokens[i+1]) && !is_signed_number(token_list.tokens[i+1])) {
                        
                        char *var = (char*)malloc(sizeof(char)*strlen(token_list.tokens[i+1]));
                        if (var == NULL) {
                            printf("Failed to allocate memory for variable pointer !");
                            return -1;
                        }

                        strcpy(var, token_list.tokens[i+1]);

                        variables[var_count] = var;
                        var_count++;

                        arg = (void*)var;

                    } else {
                        printf("Error: Could not parse %s to a variable ! (Ln:%ld)\n", token_list.tokens[i+1], line_number);
                        return -1;
                    }
                    break;
                case ADDR:
                    // if token isn't surrounded with "" or '' and is not an number
                    if (!arg_is_char(token_list.tokens[i+1]) && !arg_is_str(token_list.tokens[i+1]) && !is_signed_number(token_list.tokens[i+1])) {
                        
                    } else {
                        printf("Error: Could not parse %s to a label ! (Ln:%ld)\n", token_list.tokens[i+1], line_number);
                        return -1;
                    }
                    break;
                case STR:
                    arg = parse_argument_to_str(token_list.tokens[i+1]);
                    break;
                
                default:
                    break;
                }

                if (arg == NULL) {
                    printf("Error: Could not parse %s to %s ! (Ln:%ld)\n", token_list.tokens[i+1], get_argument_type_name(available_instructions.instructions[instr_index].args[i]), line_number);
                    return -1;
                }

                // append arg to args
            }


        }

        if (token_list.tokens != NULL) {
            for (int i = 0; i < token_list.count; i++)
                free(token_list.tokens[i]);
            
            free(token_list.tokens);
        }

        line_number++;
    }

    return 0;
}


int main(int argc, char const *argv[]) {

    if (argc == 1) {
        printf("No source file provided !\n\n");
        print_help();
        return -1;
    } else if (argc == 2 && (strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_help();
        return 0;
    }

    if (argc >= 2) {
        const char *source_file;
        char *program_name = NULL;

        if (argc == 2) {
            source_file = argv[1];
        } else if (argc == 3) {
            source_file = argv[2];

            program_name = (char*)malloc(MAX_PROGRAM_NAME_LEN);

            if (program_name == NULL) {
                printf("Could not allocate memory to store the program name !\n\n");
                return -1;
            }
            
            // check program name size
            if (strlen(argv[1]) > MAX_PROGRAM_NAME_LEN) {
                strncpy(program_name, argv[1], MAX_PROGRAM_NAME_LEN);
                printf("Info: Program name is too long, it will be truncated to \"%s\".\n", program_name);
            } else {
                strcpy(program_name, argv[1]);
            }

        } else {
            printf("Too much arguments !\n\n");
            print_help();
            return -1;
        }

        int ret = compile_to_bytecode(source_file, program_name);
        if (program_name != NULL)
            free(program_name);
        return ret;
    }

    return 0;
}