include ../var.mak

SRC = kernel
IMAGE_NAME = kernel8
QEMU_SERIAL = -serial null -serial pty #stdio

SRCC = $(wildcard $(SRC_DIR)/*.c)
SRCS = $(wildcard $(SRC_DIR)/*.s)
LIBC = $(wildcard $(LIB_DIR)/*.c)
LIBS = $(wildcard $(LIB_DIR)/*.s)

OBJS = $(SRCC:$(SRC_DIR)/%.c=$(BUILD_DIR)/$(SRC)_%_c.o) \
       $(SRCS:$(SRC_DIR)/%.s=$(BUILD_DIR)/$(SRC)_%_s.o) \
       $(LIBC:$(LIB_DIR)/%.c=$(BUILD_DIR)/$(LIB)_%_c.o) \
       $(LIBS:$(LIB_DIR)/%.s=$(BUILD_DIR)/$(LIB)_%_s.o)

all: $(SRC_DIR)/$(IMAGE_NAME).img

$(BUILD_DIR)/$(LIB)_%_s.o: $(LIB_DIR)/%.s
	$(CC) $(CFLAGS) -I $(LIB_DIR) -c $< -o $@
$(BUILD_DIR)/$(LIB)_%_c.o: $(LIB_DIR)/%.c
	$(CC) $(CFLAGS) -I $(LIB_DIR) -c $< -o $@

$(BUILD_DIR)/$(SRC)_%_s.o: $(SRC_DIR)/%.s
	$(CC) $(CFLAGS) -I $(LIB_DIR) -c $< -o $@
$(BUILD_DIR)/$(SRC)_%_c.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I $(LIB_DIR) -c $< -o $@

$(SRC_DIR)/$(IMAGE_NAME).img: $(OBJS)
	$(LD) -nostdlib -nostartfiles $^ -T link.ld -o $(SRC_DIR)/$(IMAGE_NAME).elf
	$(OC) -O binary $(SRC_DIR)/$(IMAGE_NAME).elf $(SRC_DIR)/$(IMAGE_NAME).img

run:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -serial null -serial stdio -initrd ../initramfs.cpio -dtb ../bcm2710-rpi-3-b-plus.dtb

runtty:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -serial null -serial pty

debug:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -S -s -serial stdio

debugtty:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -S -s -serial pty

clean:
	rm $(SRC_DIR)/$(IMAGE_NAME).elf $(SRC_DIR)/$(IMAGE_NAME).img \
		$(BUILD_DIR)/$(SRC)_*.o $(BUILD_DIR)/$(LIB)_*.o >/dev/null 2>/dev/null || true
