###############################################################################
# Makefile for the project main
###############################################################################

## General Flags
PROJECT = main
MCU = atmega32
TARGET = main.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99                                                                 -DF_CPU=16000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=main.map


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Include Directories
INCLUDES = -I"Q:\EAGLE\DC Servo\src\OpenServo" -I"Q:\EAGLE\DC Servo\src\FreeMODBUS" -I"Q:\EAGLE\DC Servo\src\FreeMODBUS\modbus\include" -I"Q:\EAGLE\DC Servo\src\FreeMODBUS\port" -I"Q:\EAGLE\DC Servo\src\FreeMODBUS\modbus\rtu" 

## Objects that must be built in order to link
OBJECTS = main.o pwm.o registers.o eeprom.o pid.o mb.o portevent.o portserial.o porttimer.o mbcrc.o mbrtu.o mbfunccoils.o mbfuncdiag.o mbfuncdisc.o mbfuncholding.o mbfuncinput.o mbfuncother.o mbutils.o current_loop.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) main.hex main.eep main.lss size

## Compile
main.o: ../main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

pwm.o: ../OpenServo/pwm.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

registers.o: ../OpenServo/registers.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

eeprom.o: ../OpenServo/eeprom.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

pid.o: ../pid.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mb.o: ../FreeMODBUS/modbus/mb.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

portevent.o: ../FreeMODBUS/port/portevent.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

portserial.o: ../FreeMODBUS/port/portserial.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

porttimer.o: ../FreeMODBUS/port/porttimer.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbcrc.o: ../FreeMODBUS/modbus/rtu/mbcrc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbrtu.o: ../FreeMODBUS/modbus/rtu/mbrtu.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfunccoils.o: ../FreeMODBUS/modbus/functions/mbfunccoils.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfuncdiag.o: ../FreeMODBUS/modbus/functions/mbfuncdiag.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfuncdisc.o: ../FreeMODBUS/modbus/functions/mbfuncdisc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfuncholding.o: ../FreeMODBUS/modbus/functions/mbfuncholding.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfuncinput.o: ../FreeMODBUS/modbus/functions/mbfuncinput.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfuncother.o: ../FreeMODBUS/modbus/functions/mbfuncother.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbutils.o: ../FreeMODBUS/modbus/functions/mbutils.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

current_loop.o: ../current_loop.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) main.elf dep/* main.hex main.eep main.lss main.map


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

