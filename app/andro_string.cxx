#include <cstring>

void Rtrim(char *string) {
    if (string == nullptr)
        return;

    // Use strnlen for safety - limit to reasonable string length
    size_t len = strnlen(string, 65536); // Max 64KB string
    
    // If string is at maximum length and not null-terminated, it's unsafe
    if (len == 65536 && string[65536] != '\0') {
        // String is too long or not null-terminated, truncate it
        string[65535] = '\0';
        len = 65535;
    }
    
    while (len > 0 && string[len - 1] == ' ')
        string[--len] = '\0';
}

void Ltrim(char *string) {
    char *tmp = string;
    if (string == nullptr || (*tmp) != ' ')
        return;

    // Find first non-space character
    while ((*tmp) != '\0' && (*tmp) == ' ') {
        tmp++;
    }

    if ((*tmp) == '\0') {
        *string = '\0';
        return;
    }

    // Shift string left to remove leading spaces
    char *tmp2 = string;
    while ((*tmp) != '\0') {
        (*tmp2) = (*tmp);
        tmp++;
        tmp2++;
    }
    (*tmp2) = '\0';
}