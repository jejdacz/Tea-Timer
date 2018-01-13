# MCU
MCU = attiny85

# compiler to use
CC = avr-gcc

# flags to pass compiler
CFLAGS = -Os -DF_CPU=1000000UL -mmcu=$(MCU)

# flags to avr-size
SFLAGS = -C --mcu=$(MCU)

# name for executable
EXE = teatimer

HEX = $(EXE).hex

# space-separated list of header files
HDRS = 

# space-separated list of libraries, if any,
# each of which should be prefixed with -l
LIBS =

# space-separated list of source files
SRCS = teatimer.c

# automatically generated list of object files
OBJS = $(SRCS:.c=.o)

# hex file
$(HEX): $(EXE)
	avr-objcopy -O ihex -R .eeprom $^ $@	
	avr-size $(SFLAGS) $^

# executable file
$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# object files
$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c -o $@ $^
	
# flash
flash: $(HEX)
	avrdude -v -c usbasp -p t85 -U flash:w:$(HEX):i -B 8

# clean
clean:
	rm -f core $(EXE) $(OBJS) $(HEX)
	
.PHONY: clean flash

