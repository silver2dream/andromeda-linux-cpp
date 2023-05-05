#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "andro_global.h"

void init() {
    int i;
    for (i = 0; environ[i]; i++) {
        G_ENVIRON_LEN += strlen(environ[i]) + 1;
    }

    G_ENVMEM = new char[G_ENVIRON_LEN];
    memset(G_ENVMEM, 0, G_ENVIRON_LEN);

    char *tmp = G_ENVMEM;

    for (i = 0; environ[i]; i++) {
        size_t size = strlen(environ[i]) + 1;
        strcpy(tmp, environ[i]);
        environ[i] = tmp;
        tmp += size;
    }
    return;
}

void setproctitle(const char *title) {
    size_t ititlelen = strlen(title);

    size_t environlen = 0;
    for (int i = 0; G_OS_ARGV[i]; i++) {
        environlen += strlen(G_OS_ARGV[i]) + 1;
    }

    size_t esy = environlen + G_ENVIRON_LEN;
    if (esy <= ititlelen) {
        return;
    }

    G_OS_ARGV[1] = NULL;
    char *tmp = G_OS_ARGV[0];
    strcpy(tmp, title);
    tmp += ititlelen;

    size_t cha = esy - ititlelen;
    memset(tmp, 0, cha);
    return;
}