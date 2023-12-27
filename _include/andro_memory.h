#ifndef __ANDRO_MEMORY_H__
#define __ANDRO_MEMORY_H__

#include <cstddef>

class CMemory {
   private:
    CMemory(){};

   public:
    ~CMemory()= default;

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
    static void *AllocMemory(int mem_count, bool is_memset);
    static void FreeMemory(void *ptr);
};

#endif