#include <stdio.h>
#include <string.h>

void Rtrim(char *string) {
    size_t len = 0;
    if (string == nullptr)
        return;

    len = strlen(string);
    while (len > 0 && string[len - 1] == ' ')
        string[--len] = 0;
    return;
}

void Ltrim(char *string) {
    size_t len = 0;
    len = strlen(string);
    char *tmp = string;
    if ((*tmp) != ' ')
        return;

    while ((*tmp) != '\0') {
        if ((*tmp) == ' ')
            tmp++;
        else
            break;
    }

    if ((*tmp) == '\0') {
        *string = '\0';
        return;
    }

    char *tmp2 = string;
    while ((*tmp) != '\0') {
        (*tmp2) = (*tmp);
        tmp++;
        tmp2++;
    }
    (*tmp2) = '\0';
    return;
}