#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;

enum pin_usage {
	POWPIN_EMUv4,
	POWPIN_EMUv5,
	POWPIN_NSX1,
	POWPIN_NSX2,
	POWPIN_HICO,
	POWPIN_UNUSED1,
	POWPIN_UNUSED2,
	POWPIN_UNUSED3,
	POWPIN_COUNT
};
const char * const pin_names[POWPIN_COUNT] = {
	"EMUv4",
	"EMUv5",
	"NSX1",
	"NSX2",
	"HiCo",
	"Unused1",
	"Unused2",
	"Unused3"
};
const uint POW_PINS[POWPIN_COUNT] = {2, 3, 4, 5, 6, 7, 8, 9};

bool pstate[POWPIN_COUNT];
const char * const pstate_names[2] = {
	"OFF",
	"ON"
};

enum flash_patterns {
	WARN_INVALID_INPUT = 1,
	ERR_USB_INIT_FAILED = 5,
	ERR_INVALID_GOTPIN_STATE,
	ERR_INVALID_ERR_STATE,
};

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

_Noreturn void
error(uint count)
{
	printf("ERROR #%u\n", count);
	for (;;) {
		flash(count);
		sleep_ms(2000);
	}
}


int
main(void)
{
	bool pval;
	uint pnum;
	int ch;
	enum {
		init,
		gotval,
	} state;

	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	for (uint i = 0; i < POWPIN_COUNT; i++) {
		gpio_init(POW_PINS[i]);
		gpio_set_dir(POW_PINS[i], GPIO_OUT);
		gpio_put(POW_PINS[i], false);
		pstate[i] = false;
	}

	if (!stdio_usb_init())
		error(ERR_USB_INIT_FAILED);

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
							goto error;
					}
					break;
				case gotval:
					switch (ch) {
						case '0' ... '7':
							pnum = ch - '0';
							if (pstate[pnum] != pval) {
								gpio_put(POW_PINS[pnum], pval);
								pstate[pnum] = pval;
								printf("%s %s\n", pin_names[pnum], pstate_names[pval]);
							}
							else {
								printf("%s %s (Unchanged)\n", pin_names[pnum], pstate_names[pval]);
							}
							state = init;
							break;
						default:
							goto error;
					}
					break;
			}
		}
		else {
			flash(WARN_INVALID_INPUT);
			sleep_ms(250);
		}
		continue;
error:
		state = init;
		puts("ERR");
	}
}
