#include "shell.h"

#include "compiler.h"
#include "string.h"
#include "uart.h"

char cmd[CMD_MAX_LEN];

void get_command() {
    char c;
    uint idx = 0;
    while (1) {
        c = uart_getc();
        // https://www.microchip.com/forums/m979711.aspx
        // check backspace
        if (unlikely(c == 0x08 || c == 0x7f)) {
            if (idx == 0) continue;  // check if no any char, make index don't out of bound
            --idx;                   // cmd idx-1
            uart_puts("\b\x20\b");   // clear last char
        }
        // check end of line
        else if (unlikely(c == '\n')) {
            cmd[idx] = 0;
            uart_send('\n');
            break;
        } else {
            // if len==CMD_MAX_LEN and not return char
            if (unlikely(idx == CMD_MAX_LEN)) {
                idx = 0;
                memset(cmd, 0, CMD_MAX_LEN);
                uart_puts("\n\n----------\nCommand too long!(Max length: 255).\n----------\n\n");
            } else {
                cmd[idx++] = c;
                uart_send(c);
            }
        }
    }
}

void parse_command() {
    trim(cmd);
    if (!strncmp(cmd, "help", 4)) {
        uart_puts(
            "help\t: print this help menu\n"
            "hello\t: print Hello World!\n");
    } else if (!strncmp(cmd, "hello", 5)) {
        uart_puts("Hello World!\n");
    } else {
        uart_puts(cmd);
        uart_puts("\t: command not found!\n");
    }
    memset(cmd, 0, CMD_MAX_LEN);
}

void shell() {
    while (1) {
        uart_puts(CMD_PREFIX);
        get_command();
        parse_command();
    }
}