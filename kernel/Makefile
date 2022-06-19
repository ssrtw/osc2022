include ../var.mak

SRC = kernel
IMAGE_NAME = kernel8

QEMU_BINDDTB = -initrd ../initramfs.cpio -dtb ../bcm2710-rpi-3-b-plus.dtb
TA_QEMU_BINDDTB = -initrd ../ta_initramfs.cpio -dtb ../bcm2710-rpi-3-b-plus.dtb

SRCC = $(wildcard $(SRC_DIR)/*.c)
SRCS = $(wildcard $(SRC_DIR)/*.S)
LIBC = $(wildcard $(LIB_DIR)/*.c)
LIBS = $(wildcard $(LIB_DIR)/*.S)

OBJS = $(SRCC:$(SRC_DIR)/%.c=$(BUILD_DIR)/$(SRC)_%_c.o) \
       $(SRCS:$(SRC_DIR)/%.S=$(BUILD_DIR)/$(SRC)_%_S.o) \
       $(LIBC:$(LIB_DIR)/%.c=$(BUILD_DIR)/$(LIB)_%_c.o) \
       $(LIBS:$(LIB_DIR)/%.S=$(BUILD_DIR)/$(LIB)_%_S.o)

all: $(SRC_DIR)/$(IMAGE_NAME).img

$(BUILD_DIR)/$(LIB)_%_S.o: $(LIB_DIR)/%.S
	$(CC) $(CFLAGS) -I $(INC_DIR) -c $< -o $@
$(BUILD_DIR)/$(LIB)_%_c.o: $(LIB_DIR)/%.c
	$(CC) $(CFLAGS) -I $(INC_DIR) -c $< -o $@

$(BUILD_DIR)/$(SRC)_%_S.o: $(SRC_DIR)/%.S
	$(CC) $(CFLAGS) -I $(INC_DIR) -c $< -o $@
$(BUILD_DIR)/$(SRC)_%_c.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I $(INC_DIR) -c $< -o $@

$(SRC_DIR)/$(IMAGE_NAME).img: $(OBJS)
	$(LD) -nostdlib -nostartfiles $^ -T link.ld -o $(SRC_DIR)/$(IMAGE_NAME).elf
	$(OC) -O binary $(SRC_DIR)/$(IMAGE_NAME).elf $(SRC_DIR)/$(IMAGE_NAME).img

run:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -serial stdio $(QEMU_BINDDTB)

run_ta:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -serial stdio $(TA_QEMU_BINDDTB)

runtty:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -serial pty $(QEMU_BINDDTB)

debug:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -S -s -serial stdio $(QEMU_BINDDTB)

debug_ta:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -S -s -serial stdio $(TA_QEMU_BINDDTB)

debugtty:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -S -s -serial pty $(QEMU_BINDDTB)

clean:
	rm $(SRC_DIR)/$(IMAGE_NAME).elf $(SRC_DIR)/$(IMAGE_NAME).img \
		$(BUILD_DIR)/$(SRC)_*.o $(BUILD_DIR)/$(LIB)_*.o >/dev/null 2>/dev/null || true
