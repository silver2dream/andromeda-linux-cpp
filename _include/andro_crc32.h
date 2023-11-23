#ifndef __ANDRO_CRC32_H__
#define __ANDRO_CRC32_H__

#include <cstddef>  //NULL

class CCRC32 {
   private:
    CCRC32();

   public:
    ~CCRC32();

   private:
    static CCRC32* instance;

   public:
    static CCRC32* GetInstance() {
        if (instance == nullptr) {
            if (instance == nullptr) {
                instance = new CCRC32();
                static CRelease cl;
            }
        }
        return instance;
    }

    class CRelease {
       public:
        ~CRelease() {
            if (CCRC32::instance) {
                delete CCRC32::instance;
                CCRC32::instance = nullptr;
            }
        }
    };

   public:
    void InitCRC32Table();
    unsigned int Reflect(unsigned int ref, char ch);
    int GetCRC(unsigned char* buffer, unsigned int dw_size);

   public:
    unsigned int crc32_table[256];
};

#endif