# Source and include
BUILD_DIR=build
SRCS=$(wildcard src/*.c)
OBJS_REV1=$(SRCS:%.c=build/rev1/%.o)
OBJS_REV2=$(SRCS:%.c=build/rev2/%.o)
INC=-I ./inc

# MCU configuration
# Set MCU type, clock frequency and programmer
MCU=atmega8
CLOCK_FREQ=16000000
PROG_STR=usbasp

BOARD_REVISION=1

# Compiler flags
CFLAGS=-std=c11 -Wall -Wextra -Werror -mmcu=$(MCU) -DF_CPU=$(CLOCK_FREQ)
OPT_FLAGS_REV1=-O3 -g -DDEBUG -DBOARD_REV=1
OPT_FLAGS_REV2=-O3 -g -DDEBUG -DBOARD_REV=2

# Compiler and utility tools
OBJCOPY=avr-objcopy
CC=avr-gcc

# Project configuration
PROJ_NAME=sflap_controller_fw
PROJ_BLD_REV1=$(BUILD_DIR)/$(PROJ_NAME)_rev1
PROJ_BLD_REV2=$(BUILD_DIR)/$(PROJ_NAME)_rev2

TEMP_EEPROM_DUMP=eeprom.hex


ifeq ($(REV),1)
	PROJ_BLD=$(PROJ_BLD_REV1)
else
	ifeq ($(REV),2)
		PROJ_BLD=$(PROJ_BLD_REV2)
	endif
endif

# Rules

# Build firmware for both revisions
all: $(PROJ_BLD_REV1).elf $(PROJ_BLD_REV2).elf 

$(PROJ_BLD_REV1).elf: $(OBJS_REV1)
	$(CC) -o $@ $^ $(INC) $(CFLAGS) $(OPT_FLAGS_REV1) $(MCU_FLAGS)
	$(OBJCOPY) -j .text -j .data -O ihex $@ $(PROJ_BLD_REV1).hex
	$(OBJCOPY) -j .text -j .data -O binary $@ $(PROJ_BLD_REV1).bin

$(PROJ_BLD_REV2).elf: $(OBJS_REV2)
	$(CC) -o $@ $^ $(INC) $(CFLAGS) $(OPT_FLAGS_REV2) $(MCU_FLAGS)
	$(OBJCOPY) -j .text -j .data -O ihex $@ $(PROJ_BLD_REV2).hex
	$(OBJCOPY) -j .text -j .data -O binary $@ $(PROJ_BLD_REV2).bin

build/rev1/%.o: %.c
	mkdir -p build/rev1/src
	$(CC) -c -o $@ $(INC) $(CFLAGS) $(OPT_FLAGS_REV1) $(MCU_FLAGS) $<
build/rev2/%.o: %.c
	mkdir -p build/rev2/src
	$(CC) -c -o $@ $(INC) $(CFLAGS) $(OPT_FLAGS_REV2) $(MCU_FLAGS) $<



release: OPT_FLAGS=-O2 -DNDEBUG
release: $(PROJ_BLD_REV1).elf $(PROJ_BLD_REV2).elf

fuse:
	avrdude -c $(PROG_STR) -p $(MCU) -U lfuse:w:0xDF:m  -U hfuse:w:0xCA:m -B 125kHz

flash-clean:
	#avrdude -c $(PROG_STR) -p $(MCU) -U eeprom:r:$(TEMP_EEPROM_DUMP):i
	avrdude -c $(PROG_STR) -p $(MCU) -U flash:w:$(PROJ_BLD).hex:i

flash-update:
	avrdude -c $(PROG_STR) -p $(MCU) -U eeprom:r:$(TEMP_EEPROM_DUMP):i
	avrdude -c $(PROG_STR) -p $(MCU) -U flash:w:$(PROJ_BLD).hex:i
	avrdude -c $(PROG_STR) -p $(MCU) -U eeprom:w:$(TEMP_EEPROM_DUMP):i

flash-eeprom:
	avrdude -c $(PROG_STR) -p $(MCU) -U eeprom:w:$(TEMP_EEPROM_DUMP):i


clear-address:
	avrdude -c $(PROG_STR) -p $(MCU) -U eeprom:r:$(TEMP_EEPROM_DUMP):i
	sed -i 's/:20000000..../:200000000000/' $(TEMP_EEPROM_DUMP)
	avrdude -c $(PROG_STR) -p $(MCU) -U eeprom:w:$(TEMP_EEPROM_DUMP):i

flash-debug:
	avrdude -c $(PROG_STR) -p $(MCU) -U flash:w:$(PROJ_BLD).elf:e

clean:
	rm -rf build
	rm $(TEMP_EEPROM_DUMP)

.PHONY = clean, release, flash, flash-debug