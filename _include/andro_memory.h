#ifndef __ANDRO_MEMORY_H__
#define __ANDRO_MEMORY_H__

#include <stddef.h>

class CMemory {
   private:
    CMemory(){};

   public:
    ~CMemory(){};

   private:
    static CMemory *instance;

   public:
    static CMemory *GetInstance() {
        if (instance == nullptr) {
            if (instance == nullptr) {
                instance = new CMemory();
                static CRelease cr;
            }
        }

        return instance;
    }

    class CRelease {
       public:
        ~CRelease() {
            if (CMemory::instance) {
                delete CMemory::instance;
                CMemory::instance = nullptr;
            }
        }
    };

   public:
    void *AllocMemory(int mem_count, bool is_memset);
    void FreeMemeory(void *ptr);
};

#endif