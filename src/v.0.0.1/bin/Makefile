###############################################################################
# Makefile for the project mega-eth
###############################################################################

## General Flags
PROJECT = mega-eth
MCU = atmega128
TARGET = mega-eth.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99                                                                                                      -DF_CPU=16000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=mega-eth.map
LDFLAGS += -Wl,-section-start=EXT_SRAM=0x801100


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Include Directories
INCLUDES = -I"E:\Developments\Elektronika\MCU\ATMEL\AVR\AVR-LPC\LPC\v.0.0.7\www.roland-riegel.de\mega-eth_source_20080522\FreeMODBUS\port" -I"E:\Developments\Elektronika\MCU\ATMEL\AVR\AVR-LPC\LPC\v.0.0.7\www.roland-riegel.de\mega-eth_source_20080522\FreeMODBUS\modbus\rtu" -I"E:\Developments\Elektronika\MCU\ATMEL\AVR\AVR-LPC\LPC\v.0.0.7\www.roland-riegel.de\mega-eth_source_20080522\FreeMODBUS\modbus\tcp" -I"E:\Developments\Elektronika\MCU\ATMEL\AVR\AVR-LPC\LPC\v.0.0.7\www.roland-riegel.de\mega-eth_source_20080522\FreeMODBUS\modbus\ascii" -I"E:\Developments\Elektronika\MCU\ATMEL\AVR\AVR-LPC\LPC\v.0.0.7\www.roland-riegel.de\mega-eth_source_20080522\FreeMODBUS\modbus\include" -I"E:\Developments\Elektronika\MCU\ATMEL\AVR\AVR-LPC\LPC\v.0.0.7\www.roland-riegel.de\mega-eth_source_20080522\FreeMODBUS\modbus\functions" 

## Objects that must be built in order to link
OBJECTS = main.o clock.o timer.o fat16.o partition.o sd.o sd_raw.o arp.o ethernet.o icmp.o ip.o net.o tcp.o tcp_queue.o udp.o spi.o uart.o xmem.o clock_sync.o dhcp_client.o httpd.o httpd_modules.o httpd_session.o enc424j600.o mcp23sxx.o mcp23s08.o mcp23s17.o portevent.o portserial.o porttcp.o porttimer.o mb.o mbascii.o mbfunccoils.o mbfuncdiag.o mbfuncdisc.o mbfuncholding.o mbfuncinput.o mbfuncother.o mbutils.o mbcrc.o mbrtu.o mbtcp.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) mega-eth.hex mega-eth.eep mega-eth.lss size

## Compile
main.o: ../main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

clock.o: ../sys/clock.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

timer.o: ../sys/timer.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

fat16.o: ../sd/fat16.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

partition.o: ../sd/partition.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

sd.o: ../sd/sd.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

sd_raw.o: ../sd/sd_raw.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

arp.o: ../net/arp.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ethernet.o: ../net/ethernet.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

icmp.o: ../net/icmp.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ip.o: ../net/ip.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

net.o: ../net/net.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

tcp.o: ../net/tcp.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

tcp_queue.o: ../net/tcp_queue.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

udp.o: ../net/udp.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

spi.o: ../arch/spi.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

uart.o: ../arch/uart.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

xmem.o: ../arch/xmem.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

clock_sync.o: ../app/clock_sync.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

dhcp_client.o: ../app/dhcp_client.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

httpd.o: ../app/httpd.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

httpd_modules.o: ../app/httpd_modules.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

httpd_session.o: ../app/httpd_session.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

enc424j600.o: ../net/enc424j600/enc424j600.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mcp23sxx.o: ../mcp23sxx/mcp23sxx.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mcp23s08.o: ../mcp23sxx/mcp23s08.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mcp23s17.o: ../mcp23sxx/mcp23s17.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

portevent.o: ../FreeMODBUS/port/portevent.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

portserial.o: ../FreeMODBUS/port/portserial.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

porttcp.o: ../FreeMODBUS/port/porttcp.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

porttimer.o: ../FreeMODBUS/port/porttimer.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mb.o: ../FreeMODBUS/modbus/mb.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbascii.o: ../FreeMODBUS/modbus/ascii/mbascii.c
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

mbcrc.o: ../FreeMODBUS/modbus/rtu/mbcrc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbrtu.o: ../FreeMODBUS/modbus/rtu/mbrtu.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbtcp.o: ../FreeMODBUS/modbus/tcp/mbtcp.c
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
	-rm -rf $(OBJECTS) mega-eth.elf dep/* mega-eth.hex mega-eth.eep mega-eth.lss mega-eth.map


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

