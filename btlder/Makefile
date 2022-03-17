include ../var.mak

SRC = btlder
IMAGE_NAME = bootloader
QEMU_SERIAL = -serial null -serial pty

SRCC = $(wildcard $(SRC_DIR)/*.c)
SRCS = $(wildcard $(SRC_DIR)/*.s)
LIBC = uart.c
LIBC:= $(LIBC:%.c=$(LIB_DIR)/%.c)

OBJS = $(SRCC:$(SRC_DIR)/%.c=$(BUILD_DIR)/$(SRC)_%_c.o) \
       $(SRCS:$(SRC_DIR)/%.s=$(BUILD_DIR)/$(SRC)_%_s.o) \
       $(LIBC:$(LIB_DIR)/%.c=$(BUILD_DIR)/$(LIB)_%_c.o) \


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
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -serial null -serial stdio

runtty:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -serial null -serial pty

debug:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -S -s -serial stdio

debugtty:
	$(QEMU) $(QEMUARGS) -kernel $(IMAGE_NAME).img -S -s -serial pty

clean:
	rm $(SRC_DIR)/$(IMAGE_NAME).elf $(SRC_DIR)/$(IMAGE_NAME).img \
		$(BUILD_DIR)/$(SRC)_*.o $(BUILD_DIR)/$(LIB)_*.o >/dev/null 2>/dev/null || true
