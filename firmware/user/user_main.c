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

// Define this to remove all UART output
#define NODEBUG

os_event_t procTaskQueue[1];

void user_rf_pre_init(void){}
void nodebug(char c){}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
        
        #ifdef NODEBUG
        os_install_putc1(nodebug);
        #endif
        
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
        
        // Prepare winner animation timer
        os_timer_disarm(&winTimer);
        os_timer_setfn(&winTimer, (os_timer_func_t *)winTimerCallback, NULL);

        #ifdef EMULATE_P1
                player1.assigned = 1;
                player1.button = 1;
                player1.nr = 1;
        #endif
        
        //Init network communications
        initNetwork();

        // Init game (probably better to move timers to game.c as well))
	ws2812_init();
        
        gameMode = WIN;
        os_timer_arm(&winTimer, 40, 1);
        
        //Start input monitor
	system_os_post(0, 0, 0);
}

void EnterCritical()
{
}

void ExitCritical()
{
}
