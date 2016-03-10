/* Keyboard example for Teensy USB Development Board
 * http://www.pjrc.com/teensy/usb_keyboard.html
 * Copyright (c) 2008 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_keyboard.h"
#include "matrix.h"

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

uint8_t keyboard_keys_raw[6] = {0};
bool keys[NKEYS] = {false};
bool keys_prev[NKEYS] = {false};
uint8_t keys_remove[NKEYS] = {0};
uint8_t *layout;

void init_pins(void) {
	uint8_t i;

	for (i = 0; i < NROWS; i++) {
		*row_ddr[i]  |= row_bit[i];
		*row_port[i] |= row_bit[i];
	}

	for (i = 0; i < NCOLS; i++) {
		*col_ddr[i]  &= ~col_bit[i];
		*col_port[i] |= col_bit[i];
	}
}

void shift_array_left(uint8_t *array, uint8_t start, uint8_t end) {
	uint8_t j;
	for (j = start; j < end-1; j++)
		array[j] = array[j+1];
	array[end-1] = 0;
}

void press_key(uint8_t key) {
	if (modifiers[key]) {
		keyboard_modifier_keys |= layout[key];
	} else {
		uint8_t k;
		for (k = 0; k < 6 && keyboard_keys[k]; k++);
		if (k < 6) {
			keyboard_keys[k] = layout[key];
			keyboard_keys_raw[k] = key;
		}
	}
}

void remove_key(uint8_t key) {
	uint8_t k;

	if (modifiers[key]) {
		keyboard_modifier_keys &= ~layout[key];
	} else {
	        for (k = 0; k < 6; k++) {
	                if (keyboard_keys_raw[k] == key) {
				shift_array_left(keyboard_keys_raw, k, 6);
				shift_array_left(keyboard_keys, k, 6);
			}
		}
	}
}

int main(void)
{
	uint8_t f, key;
	uint8_t row, col;

	/* set for 16 MHz clock */
	CPU_PRESCALE(0);
	
	init_pins();

	/* Initialize the USB, and then wait for the host to set configuration.
	 * If the Teensy is powered without a PC connected to the USB port,
	 * this will wait forever.
	 */
	usb_init();
	while (!usb_configured());

	/* Wait for the PC's operating system to load drivers
	 * and do whatever it does to actually be ready for input
	 */
	_delay_ms(100);

	while (1) {
		for (row = 0; row < NROWS; row++) {
			*row_port[row] &= ~row_bit[row];
	
			for (col = 0; col < NCOLS; col++) {
				if ((*col_pin[col] & col_bit[col]) == 0) {
					keys[col * NROWS + row] = true;
				}
			}
		
			*row_port[row] |= row_bit[row];
		}

		layout = layout_nm;

		for (f = 0; f < NFUNCTION_KEYS; f++) {
			if (keys[key_fn[f]]) {
				layout = layout_fn;
				break;
			}
		}

		for (key = 0; key < NKEYS; key++) {
			if (keys[key] && !keys_prev[key]) {
				press_key(key);
				keys_remove[key] = 0;
			} else if (!keys[key] && keys_prev[key]) {
				/* Start to remove the key */
				keys_remove[key] = 0x08;
			}

			if (keys_remove[key] == 0x01) {
				remove_key(key);
			}
		
			keys_remove[key] >>= 1;

			keys_prev[key] = keys[key];
			keys[key] = false;

		}
		usb_keyboard_send();
	}
}
