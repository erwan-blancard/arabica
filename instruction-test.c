#include <stdio.h>

#include "types.h"


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

    return 0;
}