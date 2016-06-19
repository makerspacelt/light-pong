#include "ets_sys.h"

#include "gpio.h"
#include "os_type.h"

#include "./io.h"


void io_pin_enable(char pin) {
	gpio_output_set(0, 0, (1 << pin), 0);
}
void io_pin_disable(char pin) {
	gpio_output_set(0, 0, 0, (1 << pin));
}
void io_pin_set(char pin, bool status)
{
	if (status) {
		gpio_output_set((1 << pin), 0, 0, 0);
	} else {
		gpio_output_set(0, (1 << pin), 0, 0);
	}
}
void io_pin_clear(char pin)
{
	gpio_output_set(0, (1 << pin), 0, 0);
}

