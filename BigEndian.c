#include <stdio.h>

//convertir entier en big endian
unsigned int convertToBigEndian(unsigned int num) {
    return ((num >> 24) & 0xFF) |       //premier octet à la position du dernier
           ((num >> 8) & 0xFF00) |      //deuxième octet à la position avant-dernière
           ((num << 8) & 0xFF0000) |    //troisième octet à la position du deuxième
           ((num << 24) & 0xFF000000);  //dernier octet à la position du premier
}

int main() {
    unsigned int number = 0x12345678; //nombre en little endian
    unsigned int bigEndianNumber = convertToBigEndian(number);
    
    printf("Little Endian: 0x%X\n", number);
    printf("Big Endian:    0x%X\n", bigEndianNumber);
    
    return 0;
}
