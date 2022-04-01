#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"

#define POW_PORTS 8
const uint LED_PIN = PICO_DEFAULT_LED_PIN;

const uint POW_PINS[POW_PORTS] = {2, 3, 4, 5, 6, 7, 8, 9};
bool pstate[POW_PORTS];

void
flash(uint count)
{
	for (uint i = 0; i < count; i++) {
		if (i) {
			gpio_put(LED_PIN, false);
			sleep_ms(250);
		}
		gpio_put(LED_PIN, true);
		sleep_ms(250);
	}
	gpio_put(LED_PIN, false);
}

void
error(uint count)
{
#ifndef PICO_DEFAULT_LED_PIN
	exit();
#else
	for (;;) {
		flash(count);
		sleep_ms(2000);
	}
#endif
}


int
main(void)
{
#ifdef PICO_DEFAULT_LED_PIN
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
#endif
	bool pval;
	uint pnum;
	int ch;
	enum {
		init,
		gotval,
		gotpin,
		err
	} state;

	for (uint i = 0; i < POW_PORTS; i++) {
		gpio_init(POW_PINS[i]);
		gpio_set_dir(POW_PINS[i], GPIO_OUT);
		gpio_put(POW_PINS[i], false);
		pstate[i] = false;
	}

	if (!stdio_usb_init())
		error(1);

	for (;;) {
		if (stdio_usb_connected()) {
			// Wait for CRLF?
			ch = getchar_timeout_us(10000);
			if (ch == PICO_ERROR_TIMEOUT) {
				state = init;
				continue;
			}
			switch (state) {
				case init:
					switch (ch) {
						case '+':
						case '-':
							pval = ch & 0x02; // 0x02 for +, 0 for -
							state = gotval;
							break;
						default:
							state = err;
							break;
					}
					break;
				case gotval:
					switch (ch) {
						case '0' ... '7':
							pnum = ch - '0';
							state = gotpin;
							break;
						default:
							state = err;
							break;
					}
					break;
				case gotpin:
					error(3);
				case err:
					error(4);
			}
			switch (state) {
				case init:
					error(5);
				case gotval:
					break;
				case gotpin:
					if (pstate[pnum] != pval) {
						gpio_put(POW_PINS[pnum], pval);
						pstate[pnum] = pval;
						puts("OK");
					}
					else {
						puts("NC");
					}
					state = init;
					break;
				case err:
					puts("ERR");
					state = init;
					break;
			}
		}
		else {
			flash(1);
			sleep_ms(250);
		}
	}
}
