#include <stdio.h>

#include "types.h"
#include "utils.h"


// print the instructions loaded
int main() {
    LOAD_INSTRUCTION_RESULT result = load_language_instructions();

    if (result.retcode == RET_BAD_FILE_HANDLE) {
        printf("Bad file handle");
    } else if (result.retcode == RET_INVALID_SYNTAX) {
        printf("Invalid Syntax: %s", result.reason);
    } else {
        printf("\nFound Instructions:\n");
        for (int i = 0; i < result.instr_count; i++) {
            INSTRUCTION instr = result.instructions[i];
            printf("----- %s -----\n", instr.name);
            printf("- ID: %d\n", instr.id);
            printf("- ARGS: %d\n", instr.arg_count);
            if (instr.arg_count > 0) {
                for (int j = 0; j < instr.arg_count; j++) {
                    printf("  - %s\n", get_argument_type_name(instr.args[j]));
                }
            }
            printf("\n");
        }
    }

    // TOKEN_LIST token_list = extract_tokens("   LOAD_STR \"Hellooo there\" s ");
    // printf("Count: %d\n", token_list.count);

    // if (token_list.count == -1) {
    //     printf("Mem error !");
    //     return -1;
    // }

    // for (int i = 0; i < token_list.count; i++) {
    //     printf("\"%s\"\n", token_list.tokens[i]);
    // }

    // int32_t s_num = -1;
    // uint32_t us_num = (uint32_t)s_num;

    // printf("%d\n", us_num);

    // printf("%d\n", convertToBigEndian(-1));
    // printf("%d\n", convertToBigEndian(1));

    return 0;
}