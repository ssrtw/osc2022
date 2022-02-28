#include "compiler.h"
#ifndef _STRING_H
#define _STRING_H

int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, int len);
uint strlen(const char* s);
uint isspace(char c);
void trim(char* str);
void memset(char* s, char value, uint size);
void strins(char* s, char c, int pos);
void strdel(char* s, int pos);

#endif