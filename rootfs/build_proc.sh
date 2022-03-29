aarch64-elf-gcc -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector -c except.s
aarch64-elf-ld -nostdlib -nostartfiles except.o -T linker.ld -o except.elf
aarch64-elf-objcopy -O binary except.elf except.img