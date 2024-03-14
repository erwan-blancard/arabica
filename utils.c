#include <stdint.h>

//entier en big endian
uint32_t convertToBigEndian(uint32_t num) {
    unsigned char *ptr = (unsigned char *)&num;
    uint32_t bigEndianNumber = 0;

    //copie les octets en commençant par le dernier (big endian)
    for (int i = 0; i < sizeof(uint32_t); ++i) {
        bigEndianNumber = (bigEndianNumber << 8) | *(ptr + i);
    }

    return bigEndianNumber;
}