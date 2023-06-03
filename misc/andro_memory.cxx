
#include "andro_memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CMemory* CMemory::instance = nullptr;

void* CMemory::AllocMemory(int size, bool is_memset) {
    void* tmpData = static_cast<void*>(new char[size]);
    if (is_memset) {
        memset(tmpData, 0, size);
    }
    return tmpData;
}

void CMemory::FreeMemeory(void* ptr) {
    delete[] static_cast<char*>(ptr);
}