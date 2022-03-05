#include "shell.h"

#include "compiler.h"
#include "mbox.h"
#include "reboot.h"
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
        // backspace
        if (unlikely(c == 0x08 || c == 0x7f)) {
            if (idx == 0) continue;  // check if cursor at first position, don't react
            strdel(cmd, --idx);      //當前的前一個要砍掉
            --len;
        }
        // escape
        else if (unlikely(c == 0x1b)) {
            if (uart_getc() != '[') continue;
            c = uart_getc();
            switch (c) {
                case 0x33:
                    // delete
                    // check if cursor at last position, don't react
                    if (uart_getc() == '~' && idx != len) {
                        strdel(cmd, idx);
                        --len;
                    }
                    break;
                case 0x41:
                    // no implementation
                    break;
                case 0x42:
                    // no implementation
                    break;
                case 0x43:
                    // arrow right
                    if (idx < len) {
                        ++idx;
                        uart_puts(ESCAPE_STR);
                        uart_send(c);
                    }
                    break;
                case 0x44:
                    // arrow left
                    if (idx > 0) {
                        --idx;
                        uart_puts(ESCAPE_STR);
                        uart_send(c);
                    }
                    break;
                case 0x46:  // end
                    idx = len;
                    break;
                case 0x48:  // home
                    idx = 0;
                    break;
            }
        }
        // goto head
        else if (unlikely(c == 0x1)) {
            idx = 0;
        }
        // goto tail
        else if (unlikely(c == 0x5)) {
            idx = len;
        }
        // delete to end (ctrl+k)
        else if (unlikely(c == 0xb)) {
            for (int i = idx; i < len; i++)
                cmd[i] = '\0';
            len = idx;
        }
        // check end of line
        else if (unlikely(c == '\n')) {
            cmd[len] = 0;
            uart_puts("\r\n");
            return;
        }
        // if len<CMD_MAX_LEN can insert new char
        else if (len < CMD_MAX_LEN) {
            strins(cmd, c, idx++);
            ++len;
        }
        flush_line(idx);
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
            "reboot\t: reboot rpi\n"
            "clear\t: clear screen\n");
    } else if (!strncmp(cmd, "hello", 5)) {
        uart_puts("Hello World!\n");
    } else if (!strncmp(cmd, "sysinfo", 7)) {
        print_sysinfo();
    } else if (!strncmp(cmd, "reboot", 6)) {
        reboot(0);
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