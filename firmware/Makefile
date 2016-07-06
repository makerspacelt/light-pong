CC = xtensa-lx106-elf-gcc
CFLAGS = -I. -mlongcalls -Iinclude -Iuser -Idriver -Os -DICACHE_FLASH
LDLIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -Wl,--end-group -lgcc
LDFLAGS = -Teagle.app.v6.ld

SRCS:=driver/uart.c \
	user/ws2812_i2s.c \
	user/user_main.c \
	user/game.c \
	user/network.c

build: 0x00000.bin 0x40000.bin

controller.elf : $(SRCS)
	    $(CC) $(CFLAGS) $^  -flto $(LDLIBS) $(LDFLAGS) -o $@

0x00000.bin: controller.elf
	esptool -eo controller.elf -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

0x40000.bin: controller.elf
	esptool -eo controller.elf -es .irom0.text $@ -ec

flash: 0x00000.bin 0x40000.bin
	sudo /home/user/bin/esptool.py --baud 921600 write_flash 0 0x00000.bin 0x40000 0x40000.bin

connect:
	sudo minicom --noinit -b 115200 -D /dev/ttyUSB0

clean:
	rm -f *.elf *.bin */*.o
