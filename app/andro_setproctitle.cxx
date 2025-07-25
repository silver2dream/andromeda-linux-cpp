#include <cstring>
#include <unistd.h>
#include <algorithm>

#include "andro_global.h"

void init_proctitle() {
    int i;
    for (i = 0; environ[i]; i++) {
        // Use strnlen for safety - limit to reasonable environment variable length
        size_t env_len = strnlen(environ[i], 4096); // Max 4KB per env var
        G_ENVIRON_LEN += env_len + 1;
    }

    G_ENVMEM = new char[G_ENVIRON_LEN];
    memset(G_ENVMEM, 0, G_ENVIRON_LEN);

    char *tmp = G_ENVMEM;

    for (i = 0; environ[i]; i++) {
        // Use strnlen and strncpy for safety
        size_t env_len = strnlen(environ[i], 4096);
        size_t size = env_len + 1;
        
        // Ensure we don't overflow G_ENVMEM
        if (tmp + size > G_ENVMEM + G_ENVIRON_LEN) {
            break; // Prevent buffer overflow
        }
        
        strncpy(tmp, environ[i], env_len);
        tmp[env_len] = '\0'; // Ensure null termination
        environ[i] = tmp;
        tmp += size;
    }
    return;
}

void set_proctitle(const char *title) {
    // Input validation
    if (!title) {
        return; // Handle NULL pointer
    }
    
    // Use strnlen to safely calculate title length with maximum limit
    // Maximum process title length is typically 16 bytes on Linux, but we allow more
    const size_t MAX_TITLE_LEN = 255;
    size_t ititlelen = strnlen(title, MAX_TITLE_LEN);
    
    // If title is at maximum length and not null-terminated, it's unsafe
    if (ititlelen == MAX_TITLE_LEN && title[MAX_TITLE_LEN] != '\0') {
        // Title is too long or not null-terminated, truncate it
        ititlelen = MAX_TITLE_LEN - 1; // Leave space for null terminator
    }

    size_t environlen = 0;
    for (int i = 0; G_OS_ARGV[i]; i++) {
        // Use strnlen for safety
        environlen += strnlen(G_OS_ARGV[i], 4096) + 1;
    }

    size_t esy = G_ENV_NEED_MEM + G_ARGV_NEED_MEM;
    if (esy <= ititlelen) {
        return; // Not enough space
    }

    // Ensure G_OS_ARGV[0] exists and has enough space
    if (!G_OS_ARGV[0]) {
        return;
    }

    G_OS_ARGV[1] = NULL;
    char *tmp = G_OS_ARGV[0];
    
    // Use strncpy instead of strcpy for safety
    strncpy(tmp, title, ititlelen);
    tmp[ititlelen] = '\0'; // Ensure null termination
    tmp += ititlelen;

    size_t cha = esy - ititlelen;
    memset(tmp, 0, cha);
    return;
}