#include "osapi.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "network.h"

void user_rf_pre_init(void){}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
        os_printf("Welcome to Light-Pong Buttons\n");
        
        initNetwork();
}

void EnterCritical()
{
}

void ExitCritical()
{
}
