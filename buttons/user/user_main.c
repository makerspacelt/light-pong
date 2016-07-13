#include "osapi.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "network.h"
#include "os_type.h"
#include "gpio.h"
#include "ws2812_i2s.h"

#define LEDS 14
#define VOLTAGE 3600

os_event_t procTaskQueue[1];

volatile os_timer_t debounceTimer;
volatile os_timer_t ledTimer;

uint8_t state = 1;
uint8_t debounceValue = 1;
uint8_t lastSent = 1;

uint8_t frameBuffer[LEDS*3];
uint8_t led = 0;
uint32_t vdd = 4200;

void user_rf_pre_init(void){}

// System task which will monitor players input
void ICACHE_FLASH_ATTR inputMonitor(os_event_t *events)
{   
    if (lastSent != state) {
        if (sendButtonData(state) == 0) {
            lastSent = state;
        }
        
        system_os_post(0, 0, 0);
        return;
    }
    
    uint8_t current = GPIO_INPUT_GET(0);
    
    if (current != state) {
        debounceValue = current;
        
        os_timer_arm(&debounceTimer, 10, 0);       
        return;
    }
    
    system_os_post(0, 0, 0);
}

void ICACHE_FLASH_ATTR debounceCallback(void *arg)
{
    uint8_t current = GPIO_INPUT_GET(0);
    if (debounceValue == current) {
        state = current;
        
        if (sendButtonData(state) == 0) {
            lastSent = state;
        }
    }
    
    system_os_post(0, 0, 0 );
}

void ledCallback(void *arg)
{
    int i;
    vdd = system_get_vdd33() * 1000 / 1024;
    
    
    for (i = 0; i < LEDS; i++ ) {
        frameBuffer[i*3] = 0x00;
        frameBuffer[i*3+1] = 0x00;
        frameBuffer[i*3+2] = 0x00;

        if (vdd < VOLTAGE) {
            if (i % 4 == 0) {
                frameBuffer[i*3+1] = 0x10;
            }
        } else if (!connected) {
            if (i % 2 == 0) {
                frameBuffer[i*3] = 0x20;
                frameBuffer[i*3+1] = 0x20;
                frameBuffer[i*3+2] = 0x20;
            }
        } else {
            if (!state) {
                if (player == 1) {
                    frameBuffer[i*3+2] = 0x30;
                } else {
                    frameBuffer[i*3] = 0x10;
                    frameBuffer[i*3+1] = 0x35;
                }
            } else if (i == led) {
                if (player == 1) {
                    frameBuffer[i*3+2] = 0xFF;
                } else {
                    frameBuffer[i*3] = 0x50;
                    frameBuffer[i*3+1] = 0xFF;
                }
            }
        }
    }
    
    ws2812_push(frameBuffer, sizeof(frameBuffer));
    
    if (connected) {
        led++;
    }
    
    if (led > LEDS) {
        led = 0;
        os_printf("Voltage: %d\n", vdd);
    }
}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
        os_printf("Welcome to Light-Pong Buttons\n");
        
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
        PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
        gpio_output_set(0, 0, 0, BIT0);
        
        ws2812_init();
        
        //Prepare frame timer
	os_timer_disarm(&ledTimer);
	os_timer_setfn(&ledTimer, (os_timer_func_t *)ledCallback, NULL);
        os_timer_arm(&ledTimer, 30, 1);

        
        //Register game input monitor task
	system_os_task(inputMonitor, 0, procTaskQueue, 1);
        
        //Prepare frame timer
	os_timer_disarm(&debounceTimer);
	os_timer_setfn(&debounceTimer, (os_timer_func_t *)debounceCallback, NULL);
        
        initNetwork();
}

void EnterCritical()
{
}

void ExitCritical()
{
}
