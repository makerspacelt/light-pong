#include "os_type.h"

#include "./io.h"
#include "./ht1632.h"

#include "./font_3x5_numbers_a.h"
#include "./font_5x10_numbers_a.h"
#include "./font_6x12_numbers_a.h"


static char ht1632_cs1_pin;
static char ht1632_wr_pin;
static char ht1632_data_pin;


static char ht1632_fb[HT1632_WIDTH][HT1632_HEIGHT/4] = {0};




void ht1632_start_transaction() {
	io_pin_set(ht1632_cs1_pin, 0);
}
void ht1632_end_transaction() {
	io_pin_set(ht1632_cs1_pin,  1);
	io_pin_set(ht1632_wr_pin,   1);
	io_pin_set(ht1632_data_pin, 1);
}

void ht1632_send_bit(bool bit) {
	io_pin_set(ht1632_wr_pin, 0);
	io_pin_set(ht1632_data_pin, (bit>0));
	asm("nop; nop; nop; nop;");
	io_pin_set(ht1632_wr_pin, 1);
	asm("nop; nop; nop; nop;");
}

void ht1632_send_msb(int data, char len) {
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

int ht1632_addr_from_x_y(char x, char y) {
	return ((x)*4+(y)/4);
}

void ht1632_init(char cs1_pin, char wr_pin, char data_pin) {
	ht1632_cs1_pin = cs1_pin;
	ht1632_wr_pin = wr_pin;
	ht1632_data_pin = data_pin;

	ht1632_send_cmd(HT1632_CMD_SYSDIS);
	ht1632_send_cmd(HT1632_CMD_COMS01);
	ht1632_send_cmd(HT1632_CMD_SYSEN);
	ht1632_send_cmd(HT1632_CMD_LEDON);
	ht1632_send_cmd(HT1632_CMD_PWM(1));
}



void ht1632_clear() {
	char x,y = 0;
	for (x=0; x<HT1632_WIDTH; x++) {
		for (y=0; y<HT1632_HEIGHT; y+=4) {
			ht1632_fb[x][y/4] = 0b0000;
		}
	}
}

void ht1632_invert() {
	char x,y = 0;
	for (x=0; x<HT1632_WIDTH; x++) {
		for (y=0; y<HT1632_HEIGHT; y+=4) {
			ht1632_fb[x][y/4] ^= 0b1111;
		}
	}
}

void ht1632_render() {
	char x,y = 0;
	for (x=0; x<HT1632_WIDTH; x++) {
		for (y=0; y<HT1632_HEIGHT; y+=4) {
			ht1632_send_data(ht1632_addr_from_x_y(x, y), ht1632_fb[x][y/4]);
		}
	}
}



void ht1632_set_pixel_at(char x, char y) {
	ht1632_fb[x][y/4] |= 1 << (3-(y%4));
}
void ht1632_clear_pixel_at(char x, char y) {
	ht1632_fb[x][y/4] &= ~(1 << (3-(y%4)));
}
void ht1632_pixel_at(char x, char y, bool val) {
	if (val) {
		ht1632_set_pixel_at(x,y);
	} else {
		ht1632_clear_pixel_at(x,y);
	}
}

void ht1632_draw_image(const char * _img, char _width, char _height, char _x, char _y, int offset) {
	char char_x, char_y, screen_x, screen_y, addr, val = 0;
	char col_bytes = (_height&0b11)?(_height>>2)+1:(_height>>2);

	for (char_x=0; char_x<_width; char_x++) {
		for (char_y=0; char_y<_height; char_y++) {
			addr = offset + (char_x*col_bytes) + (char_y/4);
			val = (_img[addr] >> (char_y%4)) & 1;

			screen_x = _x+char_x;
			screen_y = _y+char_y;
			ht1632_pixel_at(screen_x, screen_y, val);
		}
	}
}

void ht1632_draw_score(char p1, char p2) {

	char p1n1 = p1/10;
	char p1n2 = p1%10;
	char p2n1 = p2/10;
	char p2n2 = p2%10;

	if (p1n1 > 0) {
		ht1632_draw_image(FONT_6X12, 6, 12, 0, 0, p1n1*FONT_6X12_BYTES_PER_CHAR);
		ht1632_draw_image(FONT_6X12, 6, 12, 7, 0, p1n2*FONT_6X12_BYTES_PER_CHAR);
	} else {
		ht1632_draw_image(FONT_6X12, 6, 12, 0, 0, p1n2*FONT_6X12_BYTES_PER_CHAR);
	}

	if (p2n1 > 0) {
		ht1632_draw_image(FONT_6X12, 6, 12, 11, 4, p2n1*FONT_6X12_BYTES_PER_CHAR);
	}
		ht1632_draw_image(FONT_6X12, 6, 12, 18, 4, p2n2*FONT_6X12_BYTES_PER_CHAR);
}

