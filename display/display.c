#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"

#include "./font_3x5_numbers_a.h"
#include "./font_5x10_numbers_a.h"

static const int cs1_pin = 13;
static const int wr_pin = 14;
static const int data_pin = 12;

static bool blinky = true;
static char score[] = {0, 0};

static volatile os_timer_t blinky_dot_timer;
static volatile os_timer_t score_timer;


static int ht1632_cs1_pin;
static int ht1632_wr_pin;
static int ht1632_data_pin;

static const int ht1632_width = 24;
static const int ht1632_height = 16;


#define HT1632_CMD_SYSDIS 0b00000000  /* CMD= 0000-0000-x Turn off oscil */
#define HT1632_CMD_SYSEN  0b00000001  /* CMD= 0000-0001-x Enable system oscil */
#define HT1632_CMD_LEDOFF 0b00000010  /* CMD= 0000-0010-x LED duty cycle gen off */
#define HT1632_CMD_LEDON  0b00000011  /* CMD= 0000-0011-x LEDs ON */
#define HT1632_CMD_BLOFF  0b00001000  /* CMD= 0000-1000-x Blink ON */
#define HT1632_CMD_BLON   0b00001001  /* CMD= 0000-1001-x Blink Off */
#define HT1632_CMD_SLVMD  0b00010000  /* CMD= 0001-00xx-x Slave Mode */
#define HT1632_CMD_MSTMD  0b00010100  /* CMD= 0001-01xx-x Master Mode */
#define HT1632_CMD_RCCLK  0b00011000  /* CMD= 0001-10xx-x Use on-chip clock */
#define HT1632_CMD_EXTCLK 0b00011100  /* CMD= 0001-11xx-x Use external clock */
#define HT1632_CMD_COMS00 0b00100000  /* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS01 0b00100100  /* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS10 0b00101000  /* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS11 0b00101100  /* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_PWM_T  0b10100000  /* CMD= 101x-PPPP-x PWM duty cycle - template*/
#define HT1632_CMD_PWM(lvl) (HT1632_CMD_PWM_T | (lvl-1))
  /* Produces the correct command from the given value of lvl. lvl = [0..15] */
#define HT1632_CMD_LEN    8  /* Commands are 8 bits long, excluding the trailing bit */
#define HT1632_ADDR_LEN   7  /* Addresses are 7 bits long */
#define HT1632_WORD_LEN   4  /* Words are 4 bits long */



void io_pin_enable(int pin) {
	gpio_output_set(0, 0, (1 << pin), 0);
}
void io_pin_disable(int pin) {
	gpio_output_set(0, 0, 0, (1 << pin));
}
void io_pin_set(int pin, bool status)
{
	if (status) {
		gpio_output_set((1 << pin), 0, 0, 0);
	} else {
		gpio_output_set(0, (1 << pin), 0, 0);
	}
}
void io_pin_clear(int pin)
{
	gpio_output_set(0, (1 << pin), 0, 0);
}


void ht1632_start_transaction() {
	io_pin_set(cs1_pin, 0);
}
void ht1632_end_transaction() {
	io_pin_set(cs1_pin,  1);
	io_pin_set(wr_pin,   1);
	io_pin_set(data_pin, 1);
}

void ht1632_send_bit(int bit) {
	io_pin_set(wr_pin, 0);
	io_pin_set(data_pin, (bit>0));
	asm("nop;");
	io_pin_set(wr_pin, 1);
	asm("nop;");
}

void ht1632_send_msb(int data, int len) {
	int j,t;
	for(j=len-1, t = 1 << (len - 1); j>=0; --j, t >>= 1){
		ht1632_send_bit((data&t));
	}
}

void ht1632_send_cmd(char cmd) {
	ht1632_start_transaction();

	ht1632_send_msb(0b100, 3);
	ht1632_send_msb(cmd, HT1632_CMD_LEN);
	ht1632_send_bit(0);

	ht1632_end_transaction();
}

void ht1632_send_data(char addr, char data) {
	ht1632_start_transaction();
	
	ht1632_send_msb(0b101 , 3);
	ht1632_send_msb(addr , HT1632_ADDR_LEN);
	ht1632_send_msb(data , HT1632_WORD_LEN);
	
	ht1632_end_transaction();
}

int ht1632_addr_from_x_y(int x, int y) {
	return ((x)*4+(y)/4);
}

void ht1632_init(int cs1_pin, int wr_pin, int data_pin) {
	ht1632_cs1_pin = cs1_pin;
	ht1632_wr_pin = wr_pin;
	ht1632_data_pin = data_pin;
	
	ht1632_send_cmd(HT1632_CMD_SYSDIS);
	ht1632_send_cmd(HT1632_CMD_COMS01);
	ht1632_send_cmd(HT1632_CMD_SYSEN);
	ht1632_send_cmd(HT1632_CMD_LEDON);
	ht1632_send_cmd(HT1632_CMD_PWM(0));
}

void ht1632_clear() {
	int x,y = 0;
	for (x=0; x<ht1632_width; x++) {
		for (y=0; y<ht1632_height; y+=4) {
			ht1632_send_data(ht1632_addr_from_x_y(x, y), 0b0000);
		}
	}
}

void ht1632_render() {
	int x,y = 0;
	for (x=0; x<ht1632_width; x++) {
		for (y=0; y<ht1632_height; y+=4) {
			ht1632_send_data(ht1632_addr_from_x_y(x, y), 0b0000);
		}
	}
}

void blinky_callback(void *arg)
{
	blinky = (blinky)?false:true;
	if (blinky) {
		ht1632_send_data(ht1632_addr_from_x_y(23,15), 0b0001);
	} else {
		ht1632_send_data(ht1632_addr_from_x_y(23,15), 0b0000);
	}
}

void score_callback(void *arg)
{
	if (score[0] < score[1]) {
		score[0]++;
	} else {
		score[1]++;
	}
	if (score[0] > 0b1111) {
		score[0] = score[1] = 0;
	}
	ht1632_send_data(ht1632_addr_from_x_y(0,0), score[0]);
	ht1632_send_data(ht1632_addr_from_x_y(1,0), score[1]);
}

void ICACHE_FLASH_ATTR user_init()
{
	gpio_init();

  
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

	
	ht1632_clear();
	ht1632_send_data(ht1632_addr_from_x_y(23,0), 0b1000);
	ht1632_send_data(ht1632_addr_from_x_y(23,15), 0b0001);
	ht1632_send_data(ht1632_addr_from_x_y(0,15), 0b0001);

	
	os_timer_setfn(&blinky_dot_timer, (os_timer_func_t *)blinky_callback, NULL);
	os_timer_arm(&blinky_dot_timer, 500, 1);

	os_timer_setfn(&score_timer, (os_timer_func_t *)score_callback, NULL);
	os_timer_arm(&score_timer, 100, 1);
}
