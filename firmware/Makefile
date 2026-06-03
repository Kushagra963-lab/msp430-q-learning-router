CC ?= cc
MSP430_CC ?= msp430-elf-gcc
MSP430_MCU ?= msp430g2553
MSPDEBUG ?= mspdebug

CFLAGS := -std=c99 -Wall -Wextra -Werror -Iinclude
BUILD_DIR := build
SOURCES := src/ql_router.c src/main.c

.PHONY: all host firmware flash clean

all: host

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

host: $(BUILD_DIR)
	$(CC) $(CFLAGS) -DHOST_SIMULATION $(SOURCES) -o $(BUILD_DIR)/ql-router-sim

firmware: $(BUILD_DIR)
	$(MSP430_CC) $(CFLAGS) -mmcu=$(MSP430_MCU) $(SOURCES) -o $(BUILD_DIR)/ql-router.elf

flash: firmware
	$(MSPDEBUG) rf2500 "prog $(BUILD_DIR)/ql-router.elf"

clean:
	rm -rf $(BUILD_DIR)

