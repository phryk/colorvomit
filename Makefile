MCU = atmega2560
TRG = main
SRC = $(TRG).c

CFLAGS	= -g -Os -Wall -Wstrict-prototypes -mcall-prologues -mmcu=$(MCU) -DMCU=$(MCU) -std=c99
LDFLAGS = -mmcu=$(MCU)

CC	= avr-gcc
RM	= rm -f
RN	= mv
CP	= cp
OBJCOPY	= avr-objcopy
SIZE	= avr-size
INCDIR = .
FORMAT = ihex	
OBJ	= $(SRC:.c=.o)

all: $(TRG).elf $(TRG).bin $(TRG).hex $(TRG).eep $(TRG).ok
%.o : %.c 
	$(CC) -c $(CFLAGS) -I$(INCDIR) $< -o $@

%.s : %.c
	$(CC) -S $(CFLAGS) -I$(INCDIR) $< -o $@


%.elf: $(OBJ)
	$(CC) $(OBJ) $(LIB) $(LDFLAGS) -o $@
	
%.bin: %.elf
	$(OBJCOPY) -O binary -R .eeprom $< $@

%.hex: %.elf
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

%.eep: %.elf
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O $(FORMAT) $< $@

$(TRG).o : $(TRG).c

%ok:
	$(SIZE) -A $(TRG).elf

clean:
	$(RM) $(OBJ)
	$(RM) $(SRC:.c=.s)
	$(RM) $(SRC:.c=.lst)
	$(RM) $(TRG).map
	$(RM) $(TRG).elf
	$(RM) $(TRG).cof
	$(RM) $(TRG).obj
	$(RM) $(TRG).a90
	$(RM) $(TRG).sym
	$(RM) $(TRG).eep
	$(RM) $(TRG).hex
	$(RM) $(TRG).bin
	
size:
	$(SIZE) $(TRG).elf
flash: $(TRG).hex
	avrdude -c wiring -p atmega2560 -P /dev/cuaU1 -b 115200 -U flash:w:main.hex
