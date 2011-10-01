# Makefile
# (C) 2011 Akafugu Corporation
#
# This program is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU General Public License for more details.

# Define your programmer in this file: ~/user.mk
-include ~/user.mk

SILENT ?= @
CROSS ?= avr-

MCU ?= atmega328p
F_CPU ?= 16000000

TARGET = main

SRCS = main.c \
	../twi.c \
	../twi-lowlevel.c \
	../rtc.c \
	buffer.c \
	uart.c


# Default values
FEATURE_SET_TIME ?= YES
FEATURE_CHARACTERS ?= YES
FEATURE_CHANGE_TWI_ADDRESS ?= YES
FEATURE_SHOW_ADDRESS_ON_STARTUP ?= YES
FEATURE_LOWERCASE ?= YES

ifeq ($(MCU), attiny4313)
  FEATURE_CHANGE_TWI_ADDRESS ?= YES
  FEATURE_SHOW_ADDRESS_ON_NO_DATA ?= YES
endif

ifneq ($(DEFAULT_BRIGHTNESS), )
  CFLAGS += -DDEFAULT_BRIGHTNESS=$(DEFAULT_BRIGHTNESS)
endif

# These will automatically be checked if they are set to YES
SPECIAL_DEFS += DEMO \
	FEATURE_SET_TIME \
	FEATURE_CHARACTERS \
	FEATURE_CHANGE_TWI_ADDRESS \
	FEATURE_SHOW_ADDRESS_ON_STARTUP \
	FEATURE_LOWERCASE
	  

OBJS = $(SRCS:.c=.o)

ifneq ($(CROSS), )
  CC = $(CROSS)gcc
  CXX = $(CROSS)g++
  OBJCOPY = $(CROSS)objcopy
  OBJDUMP = $(CROSS)objdump
  SIZE = $(CROSS)size
endif

ifneq ($(F_CPU),)
  CFLAGS += -DF_CPU=$(F_CPU)
endif

## Special defines

define CHECK_ANSWER
  ifeq ($$($(1)), YES)
    CFLAGS += -D$(1)
  endif
endef

$(foreach i,$(SPECIAL_DEFS),$(eval $(call CHECK_ANSWER,$(i))))

##

OPT=s

CFLAGS += -g -O$(OPT) \
-ffreestanding -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums \
-Wall -Wstrict-prototypes \
-Wa,-adhlns=$(<:.c=.lst) -std=gnu99 -mmcu=$(MCU) 


LDFLAGS = -Wl-Map=$(TARGET).map,--cref

all: $(TARGET).elf size

size: $(TARGET).elf
	$(SILENT) $(SIZE) -C --mcu=$(MCU) $(TARGET).elf 

ifneq ($(wildcard $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).eep $(OBJS:%.o=%.d)), )
clean:
	-rm $(wildcard $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).eep $(OBJS:%.o=%.d) $(OBJS:%.o=%.lst))
else
clean:
	@echo "Nothing to clean."
endif

.SECONDARY:

%.elf: $(OBJS)
	@echo "[$(TARGET)] Linking:" $@...
	$(SILENT) $(CC) $(CFLAGS) $(OBJS) --output $@ $(LDFLAGS)

%.o : %.cpp
	@echo "[$(TARGET)] Compiling:" $@... 
	$(SILENT) $(CXX) $(CXXFLAGS) -MMD -MF $(@:%.o=%.d) -c $< -o $@

%.o : %.c
	@echo "[$(TARGET)] Compiling:" $@...
	$(SILENT) $(CC) $(CFLAGS) -MMD -MF $(@:%.o=%.d) -c $< -o $@

%.d : %.cpp
	@echo "[$(TARGET)] Generating dependency:" $@...
	$(SILENT) $(CXX) $(CXXFLAGS) -MM -MT $(addsuffix .o, $(basename $@)) -MF $@ $<

%.d : %.c
	@echo "[$(TARGET)] Generating dependency:" $@...
	$(SILENT) $(CC) $(CFLAGS) -MM -MT $(addsuffix .o, $(basename $@)) -MF $@ $<

###############

## Programming

AVRDUDE := avrdude

AVRDUDE_FLAGS += -p $(MCU)
ifneq ($(AVRDUDE_PORT), )
  AVRDUDE_FLAGS += -P $(AVRDUDE_PORT)
endif
ifneq ($(AVRDUDE_PROGRAMMER), )
  AVRDUDE_FLAGS += -c $(AVRDUDE_PROGRAMMER)
endif
ifneq ($(AVRDUDE_SPEED), )
  AVRDUDE_FLAGS += -b $(AVRDUDE_SPEED)
endif

#Add more verbose output if we dont have SILENT set
ifeq ($(SILENT), )
  AVRDUDE_FLAGS += -v -v
endif

# Fuses for internal 8MHz oscillator
ifeq ($(MCU), atmega328p)
  AVRDUDE_WRITE_FUSE = -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m
endif
ifeq ($(MCU), atmega88)
  AVRDUDE_WRITE_FUSE = -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m
endif
ifeq ($(MCU), atmega8)
  AVRDUDE_WRITE_FUSE = -U lfuse:w:0xe4:m -U hfuse:w:0xd9:m 
endif
ifeq ($(MCU), $(filter $(MCU), attiny2313 attiny4313))
  AVRDUDE_WRITE_FUSE := -U lfuse:w:0xe4:m -U hfuse:w:0xdb:m
  #AVRDUDE_WRITE_FUSE := -U lfuse:w:0x64:m # 1Mhz
  #AVRDUDE_WRITE_FUSE := -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m # attiny4313 only 4MHz
endif

ifneq ($(AVRDUDE_PROGRAMMER), )
flash: $(TARGET).hex $(TARGET).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$(TARGET).hex -U eeprom:w:$(TARGET).eep

fuse:
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FUSE) 

%.hex: %.elf
	@echo "[$(TARGET)] Creating flash file:" $@...
	$(SILENT) $(OBJCOPY) -O ihex -R .eeprom $< $@

%.eep: %.elf
	@echo "[$(TARGET)] Creating eeprom file:" $@...
	$(SILENT) $(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O ihex $< $@
else
FLASH_MSG="You need to set AVRDUDE_PROGRAMMER/AVRDUDE_PORT/AVRDUDE_SPEED in ~/user.mk"
flash:
	@echo $(FLASH_MSG)

fuse:
	@echo $(FLASH_MSG)
endif

###############

# Check which .o files we already have and include their dependency files.
PRIOR_OBJS := $(wildcard $(OBJS))
include $(PRIOR_OBJS:%.o=%.d)
