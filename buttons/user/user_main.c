#include "osapi.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "network.h"
#include "os_type.h"
#include "gpio.h"

os_event_t procTaskQueue[1];

volatile os_timer_t debounceTimer;

uint8_t button = 1;
uint8_t debounceValue = 1;
uint8_t lastSent = 1;

void user_rf_pre_init(void){}

// System task which will monitor players input
void ICACHE_FLASH_ATTR inputMonitor(os_event_t *events)
{   
    if (lastSent != button) {
        if (sendButtonData(button) == 0) {
            lastSent = button;
        }
        
        system_os_post(0, 0, 0);
        return;
    }
    
    uint8_t current = GPIO_INPUT_GET(0);
    
    if (current != button) {
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
        button = current;
        
        if (sendButtonData(button) == 0) {
            lastSent = button;
        }
    }
    
    system_os_post(0, 0, 0 );
}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
        os_printf("Welcome to Light-Pong Buttons\n");
        
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
        PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
        gpio_output_set(0, 0, 0, BIT0);
        
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
