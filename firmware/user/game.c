#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "espconn.h"
#include "ws2812_i2s.h"
#include "gpio.h"
#include "game.h"

// Leds frame buffer
uint8_t frameBuffer[LEDS*3];

int led = 0;
uint8_t gameMode = 0;
uint8_t isSafe = 0;

signed char dir = 1;

// Prepare player start zone and game variables
void ICACHE_FLASH_ATTR prepareGame()
{
    isSafe = 0;
    
    int i;
    for (i = 0; i < LEDS; i++ ) {
        frameBuffer[i*3] = 0; 
        frameBuffer[i*3+1] = 0;
        frameBuffer[i*3+2] = 0;
        if (dir == -1 && i >= LEDS - SAFEZONE) {
            led = LEDS - 1;
            frameBuffer[i*3+2] = 0x30;
        } else if (dir == 1 && i < SAFEZONE) {
            led = 0;
            frameBuffer[i*3] = 0x30;
        }
    }
    
    ws2812_push(frameBuffer, sizeof(frameBuffer));
    gameMode = 0;
}

// System task which will monitor players input
void ICACHE_FLASH_ATTR inputMonitor(os_event_t *events)
{
    if (gameMode == 0) {
        if ((dir == 1 && GPIO_INPUT_GET(0) == 0) || dir == -1 && GPIO_INPUT_GET(2) == 0) {
            gameMode = 1;
            os_timer_arm(&frameTimer, SPEED, 1);
        }
    }else if(gameMode == 1) {
        if (dir == 1) {
            if (! GPIO_INPUT_GET(2)) {
                if (isSafe) {
                    led = LEDS - 1;
                    dir = -1;
                }
            }
        } else {
            if (! GPIO_INPUT_GET(0)) {
                if (isSafe) {
                    led = 0;
                    dir = 1;
                }
            }
        }
    }
    
    system_os_post(0, 0, 0 );
}

// A timer callback used to display fancy score lines
void ICACHE_FLASH_ATTR scoreTimerCallback(void *arg)
{
    int i;
    if (gameMode == 3) {
        for (i = 0; i < LEDS; i++) {
            if ((dir == 1 && i <= led) || (dir == -1 && i >= led)) {
                frameBuffer[i*3] = 0xff;
                frameBuffer[i*3+1] = 0xff;
                frameBuffer[i*3+2] = 0xff;
            } else {
                frameBuffer[i*3] = 0;
                frameBuffer[i*3+1] = 0;
                frameBuffer[i*3+2] = 0;
            }
        }
        
        led += dir;
        if (led == 0 || led == LEDS) {
            gameMode = 4;
            led = LEDS - led;
        }
    } else if (gameMode == 4) {
        for (i = 0; i < LEDS; i++) {
            if ((dir == 1 && i <= led) || (dir == -1 && i >= led)) {
                frameBuffer[i*3] = 0;
                frameBuffer[i*3+1] = 0;
                frameBuffer[i*3+2] = 0;
                
                if (dir == 1) {
                   frameBuffer[i*3] = 0xff; 
                } else {
                    frameBuffer[i*3+2] = 0xff;
                }
            }
        }
        
        led += dir;
        if (led == 0 || led == LEDS) {
            os_timer_disarm(&scoreTimer);
            
            dir = 0 - dir;
            prepareGame();
        }
    }

    ws2812_push(frameBuffer, sizeof(frameBuffer));
}

// A timer callback used to display each frame of moving light in-play
void ICACHE_FLASH_ATTR frameTimerCallback(void *arg)
{
    if (gameMode == 2) {
        gameMode = 3;
        os_timer_disarm(&frameTimer);
        os_timer_arm(&scoreTimer, 10, 1);
        
        if (dir == 1) {
            led = 0;
        } else {
            led = LEDS - 1;
        }
        
        return;
    }
    
    if ((dir == -1 && led < SAFEZONE) || (dir == 1 && led >= LEDS - SAFEZONE)) {
        isSafe = 1;
    } else {
        isSafe = 0;
    }
    
    int i = 0;
    for (i = 0; i < LEDS; i++)
    {
        if (i == led) {
            frameBuffer[i*3] = 0; 
            frameBuffer[i*3+1] = 0xff;
            frameBuffer[i*3+2] = 0;
        } else {
            frameBuffer[i*3] = 0; 
            frameBuffer[i*3+1] = 0;
            frameBuffer[i*3+2] = 0;
            
            if (dir == -1 && led < SAFEZONE && i < SAFEZONE) {
                frameBuffer[i*3] = 0x30;
            } else if (dir == 1 && led >= LEDS - SAFEZONE && i >= LEDS - SAFEZONE) {
                frameBuffer[i*3+2] = 0x30;
            }
        }
    }
    
    if (dir == 1 && led == LEDS - 1) {
        gameMode = 2;
    } else if (dir == -1 && led == 0) {
        gameMode = 2;
    }
    
    led += dir;
    ws2812_push(frameBuffer, sizeof(frameBuffer));
}
