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

#define LINE_BUFF_SIZE 512


typedef struct {
    char *name;

    // Will be used to calculate an offset 
    // of the address of the label for jump instructions.
    uint32_t alignment;
} LABEL;


typedef struct {
    INSTRUCTION *instr;
    void **args;
} PreprocessedInstruction;


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


void write_instruction(PreprocessedInstruction prep, FILE *f) {
    fwrite(&prep.instr->id, sizeof(uint8_t), 1, f);     // id

    for (int i = 0; i < prep.instr->arg_count; i++) {   // args
        switch (prep.instr->args[i]) {
        case NUM_32:
        case ADDR:
            int32_t val = (int32_t)convertToBigEndian(*(uint32_t*)prep.args[i]);
            // int32_t val = *(int32_t*)prep.args[i];
            // printf("%d\n", val);
            // uint32_t uval = convertToBigEndian((val+UINT32_MAX+1));
            fwrite(&val, sizeof(uint32_t), 1, f);
            break;
        case STR:
            char *str = (char*)prep.args[i];
            int len = (int)strlen(str);
            fwrite(&len, sizeof(uint8_t), 1, f);
            fwrite(str, sizeof(char), len, f);
            break;
        case NUM_U8:
        case CHAR:
        case VAR:
            fwrite(prep.args[i], sizeof(uint8_t), 1, f);
            break;
        default:
            break;
        }
    }
}


size_t size_of_instruction(PreprocessedInstruction instr) {
    size_t size = 1;   // id of instruction

    for (int i = 0; i < instr.instr->arg_count; i++) {
        switch (instr.instr->args[i]) {
        case NUM_32:
            size = size + 4;
            break;
        case ADDR:
            size = size + 4;
            break;
        case STR:
            size = size + 1 + strlen((char*)instr.args[i]);
            break;
        default:
            size++;
            break;
        }
    }
    
    return size;
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
    if (!src) {
        printf("Could not open source file %s !\n", source_file);
        return -1;
    }

    // TODO improve
    int source_name_len = strlen(source_file);
    int ext_name_len = strlen(EXT_NAME);

    char *out_file_name = (char*)malloc(sizeof(char) * (source_name_len+ext_name_len+1));
    out_file_name[source_name_len+ext_name_len] = '\0';

    if (!out_file_name) {
        printf("Could not create output file !\n");
        return -1;
    }

    strcpy(out_file_name, source_file);

    for (int i = 0; i < ext_name_len; i++) {
        out_file_name[source_name_len+i] = EXT_NAME[i];
    }

    FILE *out = fopen(out_file_name, "wb");
    if (!out) {
        printf("Could not create output file %s !\n", out_file_name);
        return -1;
    }

    long code_size = 0;

    long instr_count = 0;
    PreprocessedInstruction *prep_instructions;

    int var_count = 0;
    char **variables = (char**)malloc(sizeof(char*) * MAX_VARS);
    
    long label_count = 0;
    LABEL *labels = NULL;


    char line_buff[LINE_BUFF_SIZE];
    size_t line_number = 1;
    while (fgets(line_buff, LINE_BUFF_SIZE, src) != NULL) {
        printf("--------------\nLine: %s\n", line_buff);
        TOKEN_LIST token_list = extract_tokens(line_buff);

        if (token_list.count == -1) {
            printf("Error: Failed to allocate memory when extracting tokens from line ! (Ln:%lld)\n", line_number);
            return -1;
        }

        printf("Count: %d\n", token_list.count);
        for (int i = 0; i < token_list.count; i++) {
            printf("\"%s\"\n", token_list.tokens[i]);
        }

        if (token_list.count == 0) {
            line_number++;
            continue;
        }

        // check if label
        if (token_list.count == 1 && token_list.tokens[0][strlen(token_list.tokens[0])-1] == ':') {
            
            printf("Found label. Token: %s\n", token_list.tokens[0]);
            // check if label name contains " or ', or if label is just ":"
            if (strlen(token_list.tokens[0]) == 1 || strchr(token_list.tokens[0], '\"') || strchr(token_list.tokens[0], '\'')) {
                printf("Error: Invalid label name ! (Ln:%lld)\n", line_number);
                return -1;
            }

            if (label_count == 0) {
                labels = (LABEL*)malloc(sizeof(LABEL)*(label_count+1));
                // check if malloc failed
                if (!labels) {
                    printf("Failed to allocate memory for label !\n");
                    return -1;
                }
            } else {
                LABEL *tmp = (LABEL*)realloc(labels, sizeof(LABEL)*(label_count+1));
                // check if realloc failed
                if (!tmp) {
                    printf("Failed to reallocate memory for label !\n");
                    return -1;
                } else {
                    labels = tmp;
                }
            }

            char *label_name = (char*)malloc(sizeof(char) * strlen(token_list.tokens[0]));
            if (!label_name) {
                printf("Failed to allocate memory for label name !\n");
                return -1;
            }

            strncpy(label_name, token_list.tokens[0], strlen(token_list.tokens[0])-1);
            label_name[strlen(token_list.tokens[0])-1] = '\0';
            printf("Label name: \"%s\"\n", label_name);

            // check if label already exist
            for (int l = 0; l < label_count; l++) {
                if (strcmp(label_name, labels[l].name) == 0) {
                    printf("Error: Duplicated label %s ! (Ln:%lld)\n", label_name, line_number);
                    free(label_name);
                    return -1;
                }
            }

            labels[label_count].name = label_name;
            labels[label_count].alignment = code_size;

            label_count++;

        } else {        // if instruction

            // get instruction based on name token
            long instr_index = index_of_matching_instruction(token_list.tokens[0], available_instructions.instructions, available_instructions.instr_count);
            
            if (instr_index == -1) {
                printf("Error: Instruction \"%s\" does not match any defined program instructions ! (Ln:%lld)\n", token_list.tokens[0], line_number);
                return -1;
            }

            // check args count
            if (token_list.count-1 != available_instructions.instructions[instr_index].arg_count) {
                printf("Error: Invalid number of arguments. Expected %d, got %d. (Ln:%lld)\n", available_instructions.instructions[instr_index].arg_count, token_list.count-1, line_number);
                return -1;
            }

            void **args = NULL;
            if (available_instructions.instructions[instr_index].arg_count) {
                args = (void**)malloc(sizeof(void*)*available_instructions.instructions[instr_index].arg_count);
                if (!args) {
                    printf("Failed to allocate memory for args !\n");
                    return -1;
                }
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
                        
                        // check if var was already declared before
                        char *ptr_var = NULL;
                        for (int v = 0; v < var_count; v++) {
                            if (strcmp(token_list.tokens[i+1], variables[v]) == 0) {
                                ptr_var = variables[v];
                                break;
                            }
                        }

                        if (ptr_var != NULL) {
                            arg = (void*)ptr_var;
                        } else if (var_count+1 > MAX_VARS) {
                            printf("Error: Could not add variable %s: max variable count reached ! (Ln:%lld)\n", token_list.tokens[i+1], line_number);
                            return -1;
                        } else {
                            char *var = (char*)malloc(sizeof(char)*strlen(token_list.tokens[i+1]));
                            if (!var) {
                                printf("Failed to allocate memory for variable pointer !\n");
                                return -1;
                            }

                            strcpy(var, token_list.tokens[i+1]);

                            variables[var_count] = var;
                            var_count++;

                            arg = (void*)var;
                        }

                    } else {
                        printf("Error: Could not parse %s to a variable ! (Ln:%lld)\n", token_list.tokens[i+1], line_number);
                        return -1;
                    }
                    break;
                case ADDR:
                    // if token isn't surrounded with "" or '' and is not an number
                    if (!arg_is_char(token_list.tokens[i+1]) && !arg_is_str(token_list.tokens[i+1]) && !is_signed_number(token_list.tokens[i+1])) {
                        
                        char *labname = (char*)malloc(sizeof(char)*strlen(token_list.tokens[i+1]));
                        if (!labname) {
                            printf("Failed to allocate memory for label pointer !\n");
                            return -1;
                        }

                        strcpy(labname, token_list.tokens[i+1]);

                        arg = (void*)labname;

                    } else {
                        printf("Error: Could not parse %s to a label ! (Ln:%lld)\n", token_list.tokens[i+1], line_number);
                        return -1;
                    }
                    break;
                case STR:
                    arg = parse_argument_to_str(token_list.tokens[i+1]);
                    break;
                
                default:
                    break;
                }

                if (!arg) {
                    printf("Error: Could not parse %s to %s ! (Ln:%lld)\n", token_list.tokens[i+1], get_argument_type_name(available_instructions.instructions[instr_index].args[i]), line_number);
                    return -1;
                }

                // append arg to args
                args[i] = arg;
            }

            if (instr_count == 0) {
                prep_instructions = (PreprocessedInstruction*)malloc(sizeof(PreprocessedInstruction)*(instr_count+1));
                if (!prep_instructions) {
                    printf("Failed to allocate memory for instructions !\n");
                    return -1;
                }
            } else {
                PreprocessedInstruction *tmp = (PreprocessedInstruction*)realloc(prep_instructions, sizeof(PreprocessedInstruction)*(instr_count+1));
                if (!tmp) {
                    free(prep_instructions);
                    printf("Failed to reallocate memory for instructions !\n");
                    return -1;
                } else {
                    prep_instructions = tmp;
                }
            }

            PreprocessedInstruction prep_instr;
            prep_instr.instr = &available_instructions.instructions[instr_index];
            prep_instr.args = args;

            prep_instructions[instr_count] = prep_instr;

            code_size = code_size + size_of_instruction(prep_instr);

            instr_count++;
        }

        if (token_list.tokens != NULL) {
            for (int i = 0; i < token_list.count; i++)
                free(token_list.tokens[i]);
            
            free(token_list.tokens);
        }

        line_number++;
    }

    fclose(src);

    // link labels and variables here

    int32_t alignment = 0;
    for (int i = 0; i < instr_count; i++) {
        for (int a = 0; a < prep_instructions[i].instr->arg_count; a++) {
            if (prep_instructions[i].instr->args[a] == VAR) {
                for (int v = 0; v < var_count; v++) {
                    // compare pointers directly
                    if (prep_instructions[i].args[a] == variables[v]) {
                        uint8_t *var_id = (uint8_t*)malloc(sizeof(uint8_t));
                        if (!var_id) {
                            printf("Failed to allocate memory for variable id !\n");
                            return -1;
                        }
                        *var_id = v;
                        prep_instructions[i].args[a] = var_id;
                    }
                }

            } else if (prep_instructions[i].instr->args[a] == ADDR) {
                int32_t *address = NULL;
                for (int l = 0; l < label_count; l++) {
                    if (strcmp(prep_instructions[i].args[a], labels[l].name) == 0) {
                        address = (int32_t*)malloc(sizeof(int32_t));
                        if (!address) {
                            printf("Failed to allocate memory for address offset !\n");
                            return -1;
                        }
                        // calculate offset from our current pos.
                        // -1 is here because vm cursor is "after" the instruction id in the bytecode file.
                        // ex JMP <label> output: 09<cursor-pos> XX XX XX XX
                        *address = -(alignment - labels[l].alignment) -1;
                        prep_instructions[i].args[a] = address;
                    }
                }

                if (!address) {
                    printf("Error: Label %s doesn't exist !\n", (char*)prep_instructions[i].args[a]);
                    return -1;
                }
            }

        }
        alignment = alignment + size_of_instruction(prep_instructions[i]);
    }

    // write to output file

    // header

    char name[16];
    if (!program_name)
        program_name = DEFAULT_PROGRAM_NAME;
    strncpy(name, program_name, 16);

    fwrite("CODE", sizeof(char), 4, out);
    code_size = convertToBigEndian(code_size);
    fwrite(&code_size, sizeof(uint32_t), 1, out);
    fwrite(name, sizeof(char), 16, out);

    // write instructions

    for (int i = 0; i < instr_count; i++) {
        write_instruction(prep_instructions[i], out);
    }

    fclose(out);

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

            if (!program_name) {
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