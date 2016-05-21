### System dependent content ###
GCC_FOLDER:=/opt/esp-open-sdk/xtensa-lx106-elf
	
ESPTOOL_PY:=$(GCC_FOLDER)/bin/esptool.py
FW_TOOL:=$(GCC_FOLDER)/bin/esptool

SDK:=/media/sf_linux/SDK15
PORT:=/dev/ttyUSB0


### Non system dependent content bellow this line ###
FW_FILE_1:=0x00000.bin
FW_FILE_2:=0x40000.bin
TARGET_OUT:=image.elf
	
SRCS:=driver/uart.c \
	user/ws2812_i2s.c \
	user/user_main.c \
	user/network.c \


XTLIB:=$(SDK)/lib
XTGCCLIB:=$(GCC_FOLDER)/lib/gcc/xtensa-lx106-elf/4.8.2/libgcc.a
FOLDERPREFIX:=$(GCC_FOLDER)/bin
PREFIX:=$(FOLDERPREFIX)/xtensa-lx106-elf-
CC:=$(PREFIX)gcc

CFLAGS:=-mlongcalls -Iinclude -Iuser -Idriver -I$(SDK)/include -Os -DICACHE_FLASH

LDFLAGS_CORE:=\
	-nostdlib \
	-Wl,--relax -Wl,--gc-sections \
	-L$(XTLIB) \
	-L$(XTGCCLIB) \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libssl.a \
	$(SDK)/lib/libupgrade.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libwpa.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/libphy.a \
	$(SDK)/lib/libcrypto.a \
	$(SDK)/lib/libmain.a \
	$(SDK)/lib/libpp.a \
	$(XTGCCLIB) \
	-T $(SDK)/ld/eagle.app.v6.ld

LINKFLAGS:= \
	$(LDFLAGS_CORE) \
	-B$(XTLIB)


### TARGETS ####
.PHONY: all
all : $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)

$(TARGET_OUT) : $(SRCS)
	$(PREFIX)gcc $(CFLAGS) $^  -flto $(LINKFLAGS) -o $@

$(FW_FILE_1): $(TARGET_OUT)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

$(FW_FILE_2): $(TARGET_OUT)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -es .irom0.text $@ -ec

.PHONY: burn
burn : $(FW_FILE_1) $(FW_FILE_2)
	$(ESPTOOL_PY) --port $(PORT) write_flash 0x00000 0x00000.bin 0x40000 0x40000.bin

.PHONY: clean
clean :
	rm -rf user/*.o driver/*.o $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)
