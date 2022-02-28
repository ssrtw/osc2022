ARMPREFIX = aarch64-elf
LD = $(ARMPREFIX)-ld
CC = $(ARMPREFIX)-gcc
OC = $(ARMPREFIX)-objcopy
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector

QEMU = qemu-system-aarch64
QEMUARGS = -M raspi3 -kernel kernel8.img -display none -serial null -serial stdio

all: kernel8.img

%_s.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

%_c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# https://blog.csdn.net/liangkaiming/article/details/6267357
SRCC = $(wildcard *.c)
SRCS = $(wildcard *.s)
OBJS = $(patsubst %.c,%_c.o,$(SRCC)) $(patsubst %.s,%_s.o,$(SRCS))

kernel8.img: $(OBJS)
	$(LD) -nostdlib -nostartfiles $^ -T link.ld -o kernel8.elf
	$(OC) -O binary kernel8.elf kernel8.img

clean:
	rm kernel8.elf kernel8.img *.o >/dev/null 2>/dev/null || true

run:
	$(QEMU) $(QEMUARGS)

debug:
	$(QEMU) $(QEMUARGS) -S -s