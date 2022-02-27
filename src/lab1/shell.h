#ifndef _SHELL_H
#define _SHELL_H

#define CMD_PREFIX "> "
#define CMD_MAX_LEN 256

void get_command();
void parse_command();
void shell();

#endif