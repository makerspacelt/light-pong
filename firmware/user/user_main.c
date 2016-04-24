#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "espconn.h"
#include "ws2812_i2s.h"

#define procTaskPrio        0
#define procTaskQueueLen    1

static volatile os_timer_t some_timer;
uint8_t leds[20*3];
int led = 0;
signed char dir = 1;

void user_rf_pre_init(void)
{
	//nothing.
}

//Tasks that happen all the time.

os_event_t    procTaskQueue[procTaskQueueLen];

static void ICACHE_FLASH_ATTR procTask(os_event_t *events)
{
	//CSTick( 0 );
	system_os_post(procTaskPrio, 0, 0 );
}

//Timer event.
static void ICACHE_FLASH_ATTR myTimer(void *arg)
{
    int i = 0;
    for (i = 0; i < 20; i++)
    {
        if (i == led) {
            leds[i*3] = 0; 
            leds[i*3+1] = 0xff;
            leds[i*3+2] = 0;
        } else {
            leds[i*3] = 0x05;
            leds[i*3+1] = 0;
            leds[i*3+2] = 0;
        }
    }
    
    if (dir == 1 && led == 19)
    {
        dir = -1;
    } else if (dir == -1 && led == 0) {
        dir = 1;
    }
    
    led += dir;
    
    ws2812_push( leds, sizeof( leds ) );
}

void ICACHE_FLASH_ATTR charrx( uint8_t c )
{
	//Called from UART.
}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	uart0_sendStr("\r\nesp8266 ws2812 driver\r\n");
        

	//Add a process
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);

	//Timer example
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)myTimer, NULL);
	os_timer_arm(&some_timer, 10, 1);

        
	ws2812_init();
	system_os_post(procTaskPrio, 0, 0 );
}


//There is no code in this project that will cause reboots if interrupts are disabled.
void EnterCritical()
{
}

void ExitCritical()
{
}
