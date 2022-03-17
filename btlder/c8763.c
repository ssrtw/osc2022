#include "c8763.h"

#include "uart.h"

#define PROTOCOL_HEADER_LEN sizeof protocol_header - 1
byte protocol_header[] = "c8763";

void c8763_reader(void *__dtb) {
    size_t address = 0x80000;
    int img_size = 0, i;
    byte *kernel = (void *)address;
    byte data;
    while (1) {
        // 讀取header
        byte idx = 0;
        while (1) {
            data = uart_read();
            if (data != protocol_header[idx])
                idx = 0;
            ++idx;
            uart_send(data);
            if (idx == 5) break;
        }
        for (i = 0; i < 4; i++) {
            byte b = uart_read();
            img_size <<= 8;
            // big endian
            img_size |= (int)b;
        }

        for (i = 0; i < img_size; i++) {
            byte b = uart_read();
            *(kernel + i) = b;
            uart_send(b);
        }
        void (*run_kernel)(void *) = (void *)kernel;
        run_kernel(__dtb);
    }
}