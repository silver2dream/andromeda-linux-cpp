#include "andro_conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "andro_func.h"

CConfig *CConfig::instance = nullptr;

CConfig::CConfig() {}

CConfig::~CConfig() {
    std::vector<LPCConfItem>::iterator pos;
    for (pos = ConfigItems.begin(); pos != ConfigItems.end(); ++pos) {
        delete (*pos);
    }
    ConfigItems.clear();
}

bool CConfig::Load(const char *confName) {
    FILE *fp;
    fp = fopen(confName, "r");
    if (fp == nullptr)
        return false;

    char lineBuf[501];

    while (!feof(fp)) {
        if (fgets(lineBuf, 500, fp) == nullptr) {
            continue;
        }

        if (lineBuf[0] == 0) {
            continue;
        }

        if (*lineBuf == ';' || *lineBuf == ' ' || *lineBuf == '#' || *lineBuf == '\t' || *lineBuf == '\n') {
            continue;
        }

    lblprocstring:
        if (strlen(lineBuf) > 0) {
            if (lineBuf[strlen(lineBuf) - 1] == 10 || lineBuf[strlen(lineBuf) - 1] == 13 || lineBuf[strlen(lineBuf) - 1] == 32) {
                lineBuf[strlen(lineBuf) - 1] = 0;
                goto lblprocstring;
            }
        }
        if (lineBuf[0] == 0)
            continue;
        if (*lineBuf == '[')
            continue;

        char *tmp = strchr(lineBuf, '=');
        if (tmp != nullptr) {
            LPCConfItem confitem = new CConfItem;
            memset(confitem, 0, sizeof(CConfItem));
            strncpy(confitem->ItemName, lineBuf, (int)(tmp - lineBuf));
            strcpy(confitem->ItemContent, tmp + 1);

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

const char *CConfig::GetString(const char *itemName) {
    std::vector<LPCConfItem>::iterator pos;
    for (pos = ConfigItems.begin(); pos != ConfigItems.end(); ++pos) {
        if (strcasecmp((*pos)->ItemName, itemName) == 0)
            return (*pos)->ItemContent;
    }
    return nullptr;
}

int CConfig::GetInt(const char *itemname, const int def) {
    std::vector<LPCConfItem>::iterator pos;
    for (pos = ConfigItems.begin(); pos != ConfigItems.end(); ++pos) {
        if (strcasecmp((*pos)->ItemName, itemname) == 0)
            return atoi((*pos)->ItemContent);
    }
    return def;
}