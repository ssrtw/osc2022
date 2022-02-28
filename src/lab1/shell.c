#include "shell.h"

#include "compiler.h"
#include "string.h"
#include "uart.h"

#define CMD_PREFIX "> "
#define CMD_PREFIX_LEN sizeof CMD_PREFIX - 1
#define CURSOR_OFFSET 1
#define CMD_MAX_LEN 256

void get_command();
void parse_command();
void flush_line(int cursor);
char cmd[CMD_MAX_LEN];

void get_command() {
    char c;
    uint idx = 0, len = 0;
    while (1) {
        c = uart_getc();
        // https://www.microchip.com/forums/m979711.aspx
        // check backspace
        if (unlikely(c == 0x08 || c == 0x7f)) {
            if (idx == 0) continue;  // check if no any char, make index don't out of bound
            strdel(cmd, --idx);      //當前的前一個要砍掉
            flush_line(idx);
            --len;
        }
        // escape
        else if (unlikely(c == 0x1b)) {
            c = uart_getc();  // get '['
            c = uart_getc();
            if (c == 0x41) {
                // no implementation
            } else if (c == 0x42) {
                // no implementation
            } else {
                // arrow right
                if (c == 0x43 && idx < len) {
                    ++idx;
                    uart_puts("\x1b[");
                    uart_send(c);
                }
                // arrow left
                else if (c == 0x44 && idx > 0) {
                    --idx;
                    uart_puts("\x1b[");
                    uart_send(c);
                }
            }
        }
        // check end of line
        else if (unlikely(c == '\n')) {
            cmd[len] = 0;
            uart_send('\n');
            break;
        }
        // if len<CMD_MAX_LEN can insert new char
        else if (len < CMD_MAX_LEN) {
            strins(cmd, c, idx++);
            flush_line(idx);
            ++len;
        }
    }
}

void parse_command() {
    if (!strlen(cmd))
        return;
    trim(cmd);
    if (!strncmp(cmd, "help", 4)) {
        uart_puts(
            "help\t: print this help menu\n"
            "hello\t: print Hello World!\n");
    } else if (!strncmp(cmd, "hello", 5)) {
        uart_puts("Hello World!\n");
    } else {
        uart_puts(cmd);
        uart_puts(": command not found\n");
    }
}

void flush_line(int cursor) {
    // clear line
    uart_puts("\x1b[2K\r");
    // restore command
    uart_puts(CMD_PREFIX);
    uart_puts(cmd);
    // set cursor position with CMD_PREFIX offset
    uart_puts("\x1b[");
    uart_puti(cursor + CMD_PREFIX_LEN + CURSOR_OFFSET);
    uart_send('G');
}

void shell() {
    while (1) {
        uart_puts(CMD_PREFIX);
        get_command();
        parse_command();
        memset(cmd, 0, CMD_MAX_LEN);
    }
}