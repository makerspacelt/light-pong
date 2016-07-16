DISPLAY = 1

CC = xtensa-lx106-elf-gcc
CFLAGS = -I. -mlongcalls -Os -DICACHE_FLASH -DDISPLAY=$(DISPLAY)
LDLIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -Wl,--end-group -lgcc
LDFLAGS = -Teagle.app.v6.ld


build: 0x00000.bin 0x40000.bin

display: display.o ht1632.o uart.o network.o io.o


display.o: display.c

ht1632.o: ht1632.c

uart.o: uart.c

network.o: network.c

io.o: io.c


0x00000.bin: display
	esptool -eo display -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

0x40000.bin: display
	esptool -eo display -es .irom0.text $@ -ec

flash: 0x00000.bin 0x40000.bin
	sudo /home/user/bin/esptool.py --baud 921600 write_flash 0 0x00000.bin 0x40000 0x40000.bin 0x7C000 ../data_init.bin

connect:
	sudo minicom --noinit -b 921600 -D /dev/ttyUSB0

clean:
	rm -f display *.o *.bin
