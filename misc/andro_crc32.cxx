#include "andro_crc32.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CCRC32* CCRC32::instance = nullptr;

CCRC32::CCRC32() {
    InitCRC32Table();
}

CCRC32::~CCRC32() {
}

void CCRC32::InitCRC32Table() {
    // This is the official polynomial used by CRC-32 in PKZip, WinZip and Ethernet.
    // unsigned long ulPolynomial = 0x04c11db7;
    unsigned int ulPolynomial = 0x04c11db7;

    // 256 values representing ASCII character codes.
    for (int i = 0; i <= 0xFF; i++) {
        crc32_table[i] = Reflect(i, 8) << 24;
        // if (i == 1)printf("old1--i=%d,crc32_table[%d] = %lu\r\n",i,i,crc32_table[i]);

        for (int j = 0; j < 8; j++) {
            // if(i == 1)
            //{
            //     unsigned int tmp1 = (crc32_table[i] << 1);
            //     unsigned int tmp2 = (crc32_table[i] & (1 << 31) ? ulPolynomial : 0);
            //     unsigned int tmp3 = tmp1 ^ tmp2;
            //     tmp3 += 1;
            //     tmp3 -= 1;
            //
            // }

            crc32_table[i] = (crc32_table[i] << 1) ^ (crc32_table[i] & (1 << 31) ? ulPolynomial : 0);
            // if (i == 1)printf("old3--i=%d,crc32_table[%d] = %lu\r\n",i,i,crc32_table[i]);
        }
        // if (i == 1)printf("old2--i=%d,crc32_table[%d] = %lu\r\n",i,i,crc32_table[i]);
        crc32_table[i] = Reflect(crc32_table[i], 32);
    }
}

unsigned int CCRC32::Reflect(unsigned int ref, char ch) {
    // Used only by Init_CRC32_Table()
    // unsigned long value(0);
    unsigned int value(0);
    // Swap bit 0 for bit 7 , bit 1 for bit 6, etc.
    for (int i = 1; i < (ch + 1); i++) {
        if (ref & 1)
            value |= 1 << (ch - i);
        ref >>= 1;
    }
    return value;
}

int CCRC32::GetCRC(unsigned char* buffer, unsigned int dw_size) {
    // Be sure to use unsigned variables,
    // because negative values introduce high bits
    // where zero bits are required.
    // unsigned long  crc(0xffffffff);
    unsigned int crc(0xffffffff);
    int len;

    len = dw_size;
    // Perform the algorithm on each character
    // in the string, using the lookup table values.
    while (len--)
        crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ *buffer++];
    // Exclusive OR the result with the beginning value.
    return crc ^ 0xffffffff;
}