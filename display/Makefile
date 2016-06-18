CC = xtensa-lx106-elf-gcc
CFLAGS = -I. -mlongcalls
LDLIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -Wl,--end-group -lgcc
LDFLAGS = -Teagle.app.v6.ld


display: display.o

display.o: display.c

0x00000.bin: display
	esptool -eo display -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

0x40000.bin: display
	esptool -eo display -es .irom0.text $@ -ec

flash: 0x00000.bin 0x40000.bin
	sudo /home/user/bin/esptool.py --baud 921600 write_flash 0 0x00000.bin 0x40000 0x40000.bin

clean:
	rm -f display display.o 0x00000.bin 0x40000.bin
