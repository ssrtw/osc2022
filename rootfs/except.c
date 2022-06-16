int main() {
    char test[10];
    test[0] = 'h';
    test[1] = 'e';
    test[2] = 'l';
    test[3] = 'l';
    test[4] = 'o';
    test[5] = '\n';
    asm volatile(
        "mov x1, %0\n\t"
        "mov x2, 6\n\t"
        "mov x3, 48763\n\t"
        "mov x8, 2\n\t"  // sys_uart_write
        "svc 0" ::"r"(test));
    asm volatile(
        "mov x8, 5\n\t"  // sys_exit
        "svc 0");
}