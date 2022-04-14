#include "shell.h"

#include "compiler.h"
#include "cpio.h"
#include "exec.h"
#include "malloc.h"
#include "mbox.h"
#include "reboot.h"
#include "stddef.h"
#include "string.h"
#include "timer.h"
#include "uart.h"

#define CMD_PREFIX     "> "
#define CMD_PREFIX_LEN sizeof CMD_PREFIX - 1
#define CURSOR_OFFSET  1
#define CMD_MAX_LEN    256

size_t get_line(char *line_prefix);
void parse_command();
void flush_line(char *line_prefix, int prefix_len, size_t cursor);
void print_sysinfo();
char prev_cmd[CMD_MAX_LEN];
char cmd[CMD_MAX_LEN];

size_t get_line(char *line_prefix) {
    uart_puts(line_prefix);
    char c;
    size_t idx = 0, len = 0;
    int prefix_len = strlen(line_prefix);
    while (1) {
        c = uart_async_getc();
        // https://www.microchip.com/forums/m979711.aspx
        // backspace
        if (unlikely(c == 0x08 || c == 0x7f)) {
            if (idx == 0) continue;  // check if cursor at first position, don't react
            strdel(cmd, --idx);      //當前的前一個要砍掉
            --len;
        }
        // escape
        else if (unlikely(c == 0x1b)) {
            if (uart_async_getc() != '[') continue;
            c = uart_async_getc();
            switch (c) {
                case 0x31:  // goto head
                    if (uart_read() == 0x7e)
                        idx = 0;
                    break;
                case 0x33:
                    // delete
                    // check if cursor at last position, don't react
                    if (uart_async_getc() == '~' && idx != len) {
                        strdel(cmd, idx);
                        --len;
                    }
                    break;
                case 0x34:  // goto tail
                    if (uart_read() == 0x7e)
                        idx = len;
                    break;
                    // arrow top
                case 0x41:
                    idx = len = strlen(prev_cmd);
                    strncpy(cmd, prev_cmd, CMD_MAX_LEN);
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
            return len;
        }
        // if len<CMD_MAX_LEN can insert new char
        else if (len < CMD_MAX_LEN) {
            strins(cmd, c, idx++);
            ++len;
        }
        flush_line(line_prefix, prefix_len, idx);
    }
    return len;
}

void parse_command() {
    if (!strlen(cmd))
        return;
    trim(cmd);
    strncpy(prev_cmd, cmd, CMD_MAX_LEN);
    if (!strcmp(cmd, "help")) {
        uart_async_puts(
            "help\t: print this help menu\n"
            "hello\t: print Hello World!\n"
            "sysinfo\t: show board info\n"
            "ls\t: list cpio root files\n"
            "cat\t: read cpio file content\n"
            "timer\t: set timer, timer <seconds> <message>\n"
            "exec\t: excute cpio file\n"
            "reboot\t: reboot rpi\n"
            "clear\t: clear screen\n");
    } else if (!strcmp(cmd, "hello")) {
        uart_puts("Hello World!\n");
    } else if (!strcmp(cmd, "ls")) {
        cpio_ls();
    } else if (!strncmp(cmd, "preempt", 7)) {
    } else if (!strncmp(cmd, "allocator", 9)) {
        allocate_test();
    } else if (!strncmp(cmd, "timer", 5)) {
        char *secs = NULL, *msg = NULL;
        for (char *c = cmd; *c != '\0'; c++) {
            if (*c == ' ') {
                if (secs == NULL)
                    secs = c + 1;
                else {
                    *c = '\0';
                    msg = c + 1;
                    break;
                }
            }
        }
        if (msg != NULL) {
            int s = atoi(secs);
            add_timer_task(timer_alert_callback, s, msg);
        } else {
            uart_async_puts("Timer args error\n");
        }
    } else if (!strcmp(cmd, "exec")) {
        // clear cmd
        memset(cmd, 0, CMD_MAX_LEN);
        // again, read file name
        size_t len = get_line("File name: ");
        int ret_code = cpio_traverse(cmd, &len, cpio_exec);
        // int ret_code = exec("exception_test.img", strlen("exception_test.img"));
        if (ret_code == -1) {
            uart_puts("Error: file not exist\n");
        }
    } else if (!strcmp(cmd, "cat")) {
        // clear cmd
        memset(cmd, 0, CMD_MAX_LEN);
        // again, read file name
        size_t len = get_line("File name: ");
        int ret_code = cpio_traverse(cmd, &len, cpio_catfile);
        if (ret_code == -1) {
            uart_puts("Error: file not exist\n");
        }
    } else if (!strcmp(cmd, "sysinfo")) {
        print_sysinfo();
    } else if (!strcmp(cmd, "reboot")) {
        reboot(1);
    } else if (!strcmp(cmd, "clear")) {
        uart_puts(ESCAPE_STR "2J\x1b[H");
    } else {
        uart_printf("%s: command not found\n", cmd);
    }
}

void flush_line(char *line_prefix, int prefix_len, size_t cursor) {
    // clear line & restore command
    uart_printf("%s%s%s", ESCAPE_STR "2K\r", line_prefix, cmd);
    // set cursor position with CMD_PREFIX offset
    uart_printf("%s%dG", ESCAPE_STR, cursor + prefix_len + CURSOR_OFFSET);
}

// show board info
void print_sysinfo() {
    uint32_t revision = 0, addr = 0, memsize = 0;
    // mbox read data
    get_board_revision(&revision);
    get_arm_memory(&addr, &memsize);
    uart_printf("%s%x%s%x%s%x\n",
                "This raspi board revision is:",
                revision, ", start address is: ", addr, ", memory size: ", memsize);
}

void shell() {
    // // clear screen
    // uart_async_puts(ESCAPE_STR "2J\x1b[H");
    char *welcome_msg = malloc_size(23);
    strncpy(welcome_msg, "Welcome to ssrtw shell", 23);
    // send welcome message
    uart_printf("%s\nMessage heap address: %x\n", welcome_msg, welcome_msg);
    uart_async_puts("hello, async message\n");
    print_sysinfo();
    while (1) {
        get_line(CMD_PREFIX);
        parse_command();
        memset(cmd, 0, CMD_MAX_LEN);
    }
}