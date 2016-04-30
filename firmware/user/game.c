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
#include "network.h"

// Leds frame buffer
uint8_t frameBuffer[LEDS*3];

int led = 0;
uint8_t gameMode = 0;
uint8_t isSafe = 0;
uint8_t strobe = 0;

uint8_t player2Button = 1;

struct Player player1;
struct Player player2;

signed char dir = 1;

// Prepare player start zone and game variables
void ICACHE_FLASH_ATTR prepareGame()
{
    isSafe = 0;
    
    int i;
    for (i = 0; i < LEDS; i++ ) {
        frameBuffer[i*3] = 0x00; 
        frameBuffer[i*3+1] = 0x00;
        frameBuffer[i*3+2] = 0x00;
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
        if ((dir == 1 && GPIO_INPUT_GET(0) == 0) || dir == -1 && player2.button == 0) {
            gameMode = 1;
            os_timer_arm(&frameTimer, SPEED, 1);
        }
    }else if(gameMode == 1) {
        if (dir == 1) {
            if (! player2.button) {
                if (isSafe) {
                    led = LEDS - 1;
                    dir = -1;
                } else {
                    gameMode = 3;
                    os_timer_disarm(&frameTimer);
                    os_timer_arm(&scoreTimer, 10, 1); 
                }
            }
        } else {
            if (! GPIO_INPUT_GET(0)) {
                if (isSafe) {
                    led = 0;
                    dir = 1;
                } else {
                    gameMode = 3;
                    os_timer_disarm(&frameTimer);
                    os_timer_arm(&scoreTimer, 10, 1);
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
    if ((strobe % 2) == 0) {
        uint8_t red, green, blue = 0x00;
        if (dir == 1) {
            green = 0xFF;
        } else {
            blue = 0xFF;
        }
        for (i = 0; i < LEDS; i++) {
            frameBuffer[i*3] = green;
            frameBuffer[i*3+1] = red;
            frameBuffer[i*3+2] = blue;
        }
        os_timer_arm(&scoreTimer, 100, 1);
    } else {
        for (i = 0; i < LEDS; i++) {
            frameBuffer[i*3] = 0x00;
            frameBuffer[i*3+1] = 0x00;
            frameBuffer[i*3+2] = 0x00;
        }
        os_timer_arm(&scoreTimer, 50, 1);
    }
    ws2812_push(frameBuffer, sizeof(frameBuffer));
    
    strobe++;
    if (strobe >= 11) {
        os_timer_disarm(&scoreTimer);
        
        strobe = 0;
        dir = 0 - dir;
        prepareGame();
    }
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
        if (i <= led + 3 && i >= led - 3) {
            frameBuffer[i*3] = 0xff; 
            frameBuffer[i*3+1] = 0xff;
            frameBuffer[i*3+2] = 0xff;
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
