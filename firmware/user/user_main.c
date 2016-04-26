#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "espconn.h"
#include "ws2812_i2s.h"
#include "game.h"
#include "network.h"

os_event_t procTaskQueue[1];
void user_rf_pre_init(void){}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
        os_printf("Welcome to Light-Pong\n");
        
        // GPIO00 as input Player#1
        // GPIO02 as input Player#2
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
        PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
        gpio_output_set(0, 0, 0, BIT0);
        
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
        PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);
        gpio_output_set(0, 0, 0, BIT2);
        
	//Register game input monitor task
	system_os_task(inputMonitor, 0, procTaskQueue, 1);

	//Prepare frame timer
	os_timer_disarm(&frameTimer);
	os_timer_setfn(&frameTimer, (os_timer_func_t *)frameTimerCallback, NULL);
        
        //Prepare score timer
        os_timer_disarm(&scoreTimer);
        os_timer_setfn(&scoreTimer, (os_timer_func_t *)scoreTimerCallback, NULL);

        initNetwork();

        // Init game (probably better to move timers to game.c as well))
	ws2812_init();
        prepareGame();
        
        //Start input monitor
	system_os_post(0, 0, 0);
}

void EnterCritical()
{
}

void ExitCritical()
{
}
