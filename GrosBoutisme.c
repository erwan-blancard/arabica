#include <stdio.h>

//entier en big endian
unsigned int convertToBigEndian(unsigned int num) {
    unsigned char *ptr = (unsigned char *)&num;
    unsigned int bigEndianNumber = 0;

    //copie les octets en commençant par le dernier (big endian)
    for (int i = 0; i < sizeof(unsigned int); ++i) {
        bigEndianNumber = (bigEndianNumber << 8) | *(ptr + i);
    }

    return bigEndianNumber;
}

int main() {
    unsigned int number = 0x12345678; //nombre en little endian
    unsigned int bigEndianNumber = convertToBigEndian(number);
    
    printf("Little Endian: 0x%X\n", number);
    printf("Big Endian:    0x%X\n", bigEndianNumber);
    
    return 0;
}
