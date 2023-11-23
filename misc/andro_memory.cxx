
#include "andro_memory.h"

#include <cstring>

CMemory* CMemory::instance = nullptr;

void* CMemory::AllocMemory(int size, bool is_memset) {
    void* tmpData = static_cast<void*>(new char[size]);
    if (is_memset) {
        memset(tmpData, 0, size);
    }
    return tmpData;
}

void CMemory::FreeMemory(void* ptr) {
    delete[] static_cast<char*>(ptr);
}