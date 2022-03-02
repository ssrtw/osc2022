#ifndef _STRING_H
#define _STRING_H

#include "compiler.h"
#include "stddef.h"

#define ESCAPE_STR "\x1b["

int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t len);
uint strlen(const char* s);
uint isspace(char c);
void trim(char* str);
void memset(char* s, char value, size_t size);
void strins(char* s, char c, size_t pos);
void strdel(char* s, size_t pos);

#endif