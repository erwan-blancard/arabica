#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_CODE_SIZE 65536 // Taille maximale du code

// Structure pour représenter une instruction Arabica
typedef struct {
    uint8_t opcode;
    uint8_t args[4];
} Instruction;

// Tableau des instructions compilées
Instruction code[MAX_CODE_SIZE];
uint32_t code_size = 0;

// Ajouter une instruction au code compilé
void add_instruction(uint8_t opcode, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4) {
    if (code_size >= MAX_CODE_SIZE) {
        fprintf(stderr, "Erreur : Dépassement de la taille maximale du code\n");
        exit(EXIT_FAILURE);
    }

    Instruction instr;
    instr.opcode = opcode;
    instr.args[0] = arg1;
    instr.args[1] = arg2;
    instr.args[2] = arg3;
    instr.args[3] = arg4;
    code[code_size++] = instr;
}

// Compiler le programme Arabica en bytecode
void compile_program(const char* program_filename, const char* bytecode_filename) {
    // Logique de compilation à implémenter
    // Vous devrez lire le programme depuis le fichier, analyser les instructions, et appeler add_instruction() pour chaque instruction

    // Exemple :
    add_instruction(0x01, 0x00, 0x00, 0x00, 0x2A); // LOAD_VAL 42
    add_instruction(0x15, 0x0B, 0x00, 0x00, 0x00); // LOAD_STR (taille 11)
    // Ajoutez d'autres instructions ici...
}

// Générer le bytecode dans un fichier
void generate_bytecode(const char* bytecode_filename) {
    FILE* bytecode_file = fopen(bytecode_filename, "wb");
    if (!bytecode_file) {
        perror("Erreur lors de la création du fichier bytecode");
        exit(EXIT_FAILURE);
    }

    // Écrire l'en-tête du fichier bytecode
    const char* program_name = "HELLO";
    fwrite("CODE", sizeof(char), 4, bytecode_file);
    fwrite(&code_size, sizeof(uint32_t), 1, bytecode_file);
    fwrite(program_name, sizeof(char), 16, bytecode_file);

    // Écrire les instructions compilées
    fwrite(code, sizeof(Instruction), code_size, bytecode_file);

    fclose(bytecode_file);
}

int main() {
    compile_program("example_program.arabica", "example_program.bytecode");
    generate_bytecode("example_program.bytecode");
    return 0;
}