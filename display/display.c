#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"

#include "./io.h"
#include "./ht1632.h"
#include "./uart.h"
#include "./network.h"


static const char cs1_pin = 13;
static const char wr_pin = 14;
static const char data_pin = 12;

static bool blinky = true;
static char score[] = {0, 0};

static volatile os_timer_t blinky_dot_timer;
static volatile os_timer_t render_timer;


void user_rf_pre_init(void){}

void ICACHE_FLASH_ATTR blinky_callback(void *arg)
{              
	blinky = (blinky)?false:true;
	if (blinky) {
		ht1632_set_pixel_at(23, 0);
	} else {
		ht1632_clear_pixel_at(23, 0);
	}
}


void ICACHE_FLASH_ATTR ScoreReceived(int p1, int p2, int max)
{
	ht1632_draw_score(p1, p2);
}



void ICACHE_FLASH_ATTR render_callback(void *arg)
{
	ht1632_render();
}

void ICACHE_FLASH_ATTR user_init()
{
	gpio_init();

	//uart_init(BIT_RATE_115200, BIT_RATE_115200);
	uart_init(BIT_RATE_921600, BIT_RATE_921600);
	os_printf("Welcome to Light-Pong Display!\n");
  
	initNetwork();

	// setup ht1632 display
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	gpio_output_set(0, 0, (1 << cs1_pin), 0);
	io_pin_set(cs1_pin, 1);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14); 
	gpio_output_set(0, 0, (1 << wr_pin), 0);
	io_pin_set(wr_pin, 1);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12); 
	gpio_output_set(0, 0, (1 << data_pin), 0);
	io_pin_set(data_pin, 1);

	ht1632_init(cs1_pin, wr_pin, data_pin);

	os_timer_setfn(&render_timer, (os_timer_func_t *)render_callback, NULL);
	os_timer_arm(&render_timer, 50, 1);

	ht1632_clear();
	ht1632_draw_score(10, 12);

	os_timer_setfn(&blinky_dot_timer, (os_timer_func_t *)blinky_callback, NULL);
	os_timer_arm(&blinky_dot_timer, 500, 1);

}
