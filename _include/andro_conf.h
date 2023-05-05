#ifndef __ADNROD_CONF_H__
#define __ADNROD_CONF_H__

#include <vector>

#include "andro_global.h"

class CConfig {
   private:
    CConfig();

   public:
    ~CConfig();

   private:
    static CConfig *instance;

   public:
    std::vector<LPCConfItem> ConfigItems;

   public:
    bool Load(const char *confName);
    const char *GetString(const char *itemname);
    int GetInt(const char *itemname, const int def);

   public:
    static CConfig *GetInstance() {
        if (instance == nullptr) {
            instance = new CConfig();
            static CRelease cr;
        }
        return instance;
    }

    class CRelease {
       public:
        ~CRelease() {
            if (CConfig::instance) {
                delete CConfig::instance;
                CConfig::instance = nullptr;
            }
        }
    };
};
#endif