## Light-Pong ESP8266 firmware
### Description
Light-Pong firmware code for ESP8266 mcu build around Frankenstein monster of [CNLohr ws8212 driver](https://github.com/cnlohr/esp8266ws2812i2s) witch utilizes Espressif System example of mp3 decoder to build a DMA I2S led driver.

Utilizing DMA for led driving it should be enough cpu time for game orientated tasks

### Build instructions
Build process isdefined in __Makefile__
This software uses __xtensa-lx106__ tool chain and __Espressif System__ SDK
The easiest way to obtain the whole tool chain is to use [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk)

Standalone installation of __esp-open-sdk__ comes with ESP8266_NONOS_SDK_V1.5.2 which should work with no problems, but if there is a need to use a different SDK version, compile __esp-open-sdk__ with 

`make STANDALONE=n`

And download required [SDK](http://bbs.espressif.com/viewtopic.php?f=46&t=850) manually

Additional tool __esptool__ (not mixed with esptool.py) will be needed until I'll find a way to generate bin files from elf. Until then download it from [Christian Klippel's github](https://github.com/igrr/esptool-ck/releases)

After tool chain is ready, don't forget to edit header of __Makefile__ to adopt it to your system

__NOTE__: esptool.py (flasher) inside of `xtensa-lx106-elf/bin` can be overclocked with
`ESP_ROM_BAUD    = 460800` With short enough cables from usb->uart ttl to chip it has no problems with higher speeds than default one. ESP8266 boot loader will auto detect used speed as well.


## Licensing
Main license file is located in root of this repository, all additional licensed software licenses will reside in __3rd_party__ sub folder

