#include "andro_conf.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <vector>

#include "andro_func.h"

CConfig *CConfig::instance = nullptr;

CConfig::CConfig() = default;

CConfig::~CConfig() {
    std::vector<LPCConfItem>::iterator pos;
    for (pos = ConfigItems.begin(); pos != ConfigItems.end(); ++pos) {
        delete (*pos);
    }
    ConfigItems.clear();
}

bool CConfig::Load(const char *conf_name) {
    FILE *fp;
    fp = fopen(conf_name, "r");
    if (fp == nullptr)
        return false;

    char line_buf[501];

    while (!feof(fp)) {
        if (fgets(line_buf, 500, fp) == nullptr) {
            continue;
        }

        if (line_buf[0] == 0) {
            continue;
        }

        if (*line_buf == ';' || *line_buf == ' ' || *line_buf == '#' || *line_buf == '\t' || *line_buf == '\n') {
            continue;
        }

    lblprocstring:
        // Use strnlen for safety
        size_t line_len = strnlen(line_buf, sizeof(line_buf) - 1);
        if (line_len > 0) {
            if (line_buf[line_len - 1] == 10 || line_buf[line_len - 1] == 13 || line_buf[line_len - 1] == 32) {
                line_buf[line_len - 1] = 0;
                goto lblprocstring;
            }
        }
        if (line_buf[0] == 0)
            continue;
        if (*line_buf == '[')
            continue;

        char *tmp = strchr(line_buf, '=');
        if (tmp != nullptr) {
            auto confitem = new CConfItem;
            // Use memset_s or explicit_bzero if available, otherwise standard memset
            memset(confitem, 0, sizeof(CConfItem));
            
            // Safe string copying with bounds checking
            size_t name_len = tmp - line_buf;
            if (name_len >= sizeof(confitem->ItemName)) {
                name_len = sizeof(confitem->ItemName) - 1;
            }
            strncpy(confitem->ItemName, line_buf, name_len);
            confitem->ItemName[name_len] = '\0'; // Ensure null termination
            
            // Safe content copying
            size_t content_len = strnlen(tmp + 1, sizeof(confitem->ItemContent) - 1);
            strncpy(confitem->ItemContent, tmp + 1, content_len);
            confitem->ItemContent[content_len] = '\0'; // Ensure null termination

            Rtrim(confitem->ItemName);
            Ltrim(confitem->ItemName);
            Rtrim(confitem->ItemContent);
            Ltrim(confitem->ItemContent);

            ConfigItems.push_back(confitem);
        }
    }

    fclose(fp);
    return true;
}

const char *CConfig::GetString(const char *item_name) {
    std::vector<LPCConfItem>::iterator pos;
    for (pos = ConfigItems.begin(); pos != ConfigItems.end(); ++pos) {
        if (strcasecmp((*pos)->ItemName, item_name) == 0)
            return (*pos)->ItemContent;
    }
    return nullptr;
}

int CConfig::GetIntDefault(const char *item_name, const int def) {
    std::vector<LPCConfItem>::iterator pos;
    for (pos = ConfigItems.begin(); pos != ConfigItems.end(); ++pos) {
        if (strcasecmp((*pos)->ItemName, item_name) == 0)
            return atoi((*pos)->ItemContent);
    }
    return def;
}