#include "string.h"

#include "util.h"

// https://stackoverflow.com/a/34873763/13177700
int strcmp(const char* s1, const char* s2) {
    const uchar* p1 = (const uchar*)s1;
    const uchar* p2 = (const uchar*)s2;
    while (*p1 && *p1 == *p2) ++p1, ++p2;
    return (*p1 > *p2) - (*p2 > *p1);
}

int strncmp(const char* s1, const char* s2, size_t len) {
    const uchar* p1 = (const uchar*)s1;
    const uchar* p2 = (const uchar*)s2;
    while (len--) {
        if (*p1 != *p2)
            return (*p1 > *p2) - (*p2 > *p1);
        p1++, p2++;
    }
    return 0;
}

void* strcpy(char* dst, const char* src) {
    return memcpy(dst, src, strlen(src) + 1);
}

void* memcpy(void* dst, void* src, size_t len) {
    char* d = dst;
    const char* s = src;
    int i;
    for (i = 0; i < len; i++)
        *d++ = *s++;
    return dst;
}

uint32_t strlen(const char* s) {
    uint32_t len = 0;
    while (*s++)
        ++len;
    return len;
}

uint32_t isspace(char c) {
    if (unlikely(c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v'))
        return 1;
    return 0;
}

// https://stackoverflow.com/a/122616/
void trim(char* str) {
    int i;
    int begin = 0;
    uint32_t end = strlen(str) - 1;
    if (end == -1)
        return;

    while (isspace((uchar)str[begin]))
        begin++;

    while ((end >= begin) && isspace((uchar)str[end]))
        end--;

    // Shift all characters back to the start of the string array.
    for (i = begin; i <= end; i++)
        str[i - begin] = str[i];

    str[i - begin] = '\0';  // Null terminate string.
}

void memset(void* addr, byte value, size_t len) {
    char* d = addr;
    int i;
    for (i = 0; i < len; i++)
        *d++ = value;
}

void strins(char* s, char c, size_t pos) {
    uint32_t len = strlen(s);
    for (uint32_t i = len; i > pos; i--) {
        s[i] = s[i - 1];
    }
    s[pos] = c;
}

void strdel(char* s, size_t pos) {
    uint32_t len = strlen(s) - 1;
    for (uint32_t i = pos; i < len; i++) {
        s[i] = s[i + 1];
    }
    s[len] = 0;
}

int atoi(char* str) {
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] > '9' || str[i] < '0') return res;
        res = res * 10 + str[i] - '0';
    }
    return res;
}