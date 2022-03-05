include ../make.var
.PHONY: all clean

BUILD_DIR = ../build
KRNL_DIR = .
LIB_DIR = ../lib

all: $(KRNL_DIR)/kernel8.img

# https://blog.csdn.net/liangkaiming/article/details/6267357
SRCC = $(wildcard $(KRNL_DIR)/*.c)
SRCS = $(wildcard $(KRNL_DIR)/*.s)

LIBC = $(wildcard $(LIB_DIR)/*.c)
LIBS = $(wildcard $(LIB_DIR)/*.s)

OBJS = $($(SRCC):$(KRNL_DIR)/%.c = $(BUILD_DIR)/%_c.o) \
       $($(SRCS):$(KRNL_DIR)/%.s = $(BUILD_DIR)/%_s.o) \
       $($(LIBC):$(LIB_DIR)/%.c = $(BUILD_DIR)/%_c.o) \
       $($(LIBS):$(LIB_DIR)/%.s = $(BUILD_DIR)/%_s.o)


$(BUILD_DIR)/%_s.o: $(KRNL_DIR)/%.s $(LIB_DIR)/%.s
	$(CC) $(CFLAGS) -I $(LIB_DIR) -c $< -o $@

$(BUILD_DIR)/%_c.o: $(KRNL_DIR)/%.c $(LIB_DIR)/%.c
	$(CC) $(CFLAGS) -I $(LIB_DIR) -c $< -o $@

$(KRNL_DIR)/kernel8.img: $(OBJS)
	$(LD) -nostdlib -nostartfiles $^ -T link.ld -o $(KRNL_DIR)/kernel8.elf
	$(OC) -O binary $(KRNL_DIR)/kernel8.elf $(KRNL_DIR)/kernel8.img

run:
	$(QEMU) $(QEMUARGS)

clean:
	rm $(KRNL_DIR)/kernel8.elf $(KRNL_DIR)/kernel8.img $(BUILD_DIR)/*.o >/dev/null 2>/dev/null || true