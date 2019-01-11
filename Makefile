ifdef DEBUG
DEBUG_FLAGS = -DDEBUG
else
DEBUG_FLAGS =
endif

COMPILE_LINK_FLAGS = -mmcs51 --std-c99
CFLAGS = $(COMPILE_LINK_FLAGS) -c $(DEBUG_FLAGS)
LINK_FLAGS = $(COMPILE_LINK_FLAGS) --model-small

secohmmeter.ihx : secohmmeter.rel display.rel delay.rel eeprom.rel
	sdcc $(LINK_FLAGS) secohmmeter.rel display.rel delay.rel eeprom.rel

debuglc.ihx : debuglc.rel display.rel delay.rel
	sdcc $(LINK_FLAGS) debuglc.rel display.rel delay.rel

delay.rel : delay.c delay.h common.h
	sdcc $(CFLAGS) delay.c

display.rel : display.c display.h delay.h common.h
	sdcc $(CFLAGS) display.c

eeprom.rel : eeprom.c eeprom.h common.h
	sdcc $(CFLAGS) eeprom.c

secohmmeter.rel : secohmmeter.c display.h delay.h eeprom.h common.h
	sdcc $(CFLAGS) secohmmeter.c

debuglc.rel : debuglc.c display.h delay.h common.h
	sdcc $(CFLAGS) debuglc.c

clean :
	rm -f *.rel *.sym *.lst *.asm *.lk *.map *.mem *.ihx *.rst

flash : secohmmeter.ihx
	stcgal -P stc89 -p /dev/ttyUSB0 secohmmeter.ihx