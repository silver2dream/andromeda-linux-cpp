#ifndef __ADNROD_GBLDEF_H__
#define __ADNROD_GBLDEF_H__
typedef struct
{
    char ItemName[50];
    char ItemContent[500];
} CConfItem, *LPCConfItem;

extern char **G_OS_ARGV;
extern char *G_ENVMEM;
extern int G_ENVIRON_LEN;

#endif