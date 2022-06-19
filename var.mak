.PHONY: all clean

OS=$(shell lsb_release -si)
ARMPREFIX = aarch64-elf
LD = $(ARMPREFIX)-ld
CC = $(ARMPREFIX)-gcc
OC = $(ARMPREFIX)-objcopy
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector -g

QEMU = qemu-system-aarch64
# Arch Linux
QEMUARGS = -M raspi3b -display none -serial null
# Ubuntu
ifeq ($(OS),Ubuntu)
	QEMUARGS = -M raspi3 -display none -serial null
endif

BUILD = build
LIB = lib
INC = include
BUILD_DIR = ../$(BUILD)
LIB_DIR = ../$(LIB)
SRC_DIR = ../$(SRC)
INC_DIR = ../$(INC)