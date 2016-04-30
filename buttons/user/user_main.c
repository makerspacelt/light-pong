#include "osapi.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "network.h"
#include "os_type.h"
#include "gpio.h"

os_event_t procTaskQueue[1];
uint8_t button = 1;

void user_rf_pre_init(void){}

// System task which will monitor players input
void ICACHE_FLASH_ATTR inputMonitor(os_event_t *events)
{
    uint8_t current = GPIO_INPUT_GET(0);
    if (current != button) {
        button = current;
        sendButtonData(button);
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
        
        initNetwork();
}

void EnterCritical()
{
}

void ExitCritical()
{
}
