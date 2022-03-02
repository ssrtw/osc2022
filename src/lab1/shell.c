#include "shell.h"

#include "compiler.h"
#include "mbox.h"
#include "stddef.h"
#include "string.h"
#include "uart.h"

#define CMD_PREFIX     "> "
#define CMD_PREFIX_LEN sizeof CMD_PREFIX - 1
#define CURSOR_OFFSET  1
#define CMD_MAX_LEN    256

void get_command();
void parse_command();
void flush_line(size_t cursor);
void print_sysinfo();
char cmd[CMD_MAX_LEN];

void get_command() {
    char c;
    size_t idx = 0, len = 0;
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
            if (c != '[') continue;
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
            uart_puts("\r\n");
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
            "hello\t: print Hello World!\n"
            "sysinfo\t: show board info\n"
            "clear\t: clear screen\n");
    } else if (!strncmp(cmd, "hello", 5)) {
        uart_puts("Hello World!\n");
    } else if (!strncmp(cmd, "sysinfo", 7)) {
        print_sysinfo();
    } else if (!strncmp(cmd, "clear", 5)) {
        uart_puts(ESCAPE_STR "2J\x1b[H");
    } else {
        uart_printf("%s: command not found\n", cmd);
    }
}

void flush_line(size_t cursor) {
    // clear line & restore command
    uart_printf("%s%s", ESCAPE_STR "2K\r" CMD_PREFIX, cmd);
    // set cursor position with CMD_PREFIX offset
    uart_printf("%s%dG", ESCAPE_STR, cursor + CMD_PREFIX_LEN + CURSOR_OFFSET);
}

// show board info
void print_sysinfo() {
    uint revision = 0, addr = 0, memsize = 0;
    // mbox read data
    get_board_revision(&revision);
    get_arm_memory(&addr, &memsize);
    uart_printf("%s%x%s%x%s%x\n",
                "This raspi board revision is:",
                revision, ", start address is: ", addr, ", memory size: ", memsize);
}

void shell() {
    // clear screen
    uart_puts(ESCAPE_STR "2J\x1b[H");
    // send welcome message
    uart_puts("Welcome to ssrtw shell\n");
    print_sysinfo();
    while (1) {
        uart_puts(CMD_PREFIX);
        get_command();
        parse_command();
        memset(cmd, 0, CMD_MAX_LEN);
    }
}