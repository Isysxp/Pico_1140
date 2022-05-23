#include <cstdlib>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include "kb11.h"
#include "kl11.h"
#include <stdio.h>
#include "tusb.h"

extern KB11 cpu;

bool keypressed = false;


KL11::KL11() {
}

void KL11::clearterminal() {
	rcsr = 0;
	xcsr = 0x80;
	rbuf = 0;
	xbuf = 0;
	count = 0;
}

int _kbhit()
{
	return tud_cdc_available();
}

void KL11::serial_putchar(char c)
{
	tud_cdc_write_char(c);
	tud_cdc_write_flush();
}
char KL11::serial_getchar()
{
	return tud_cdc_read_char();
}

void KL11::poll() {
	if (!rcvrdone()) {
		// unit not busy
		if (_kbhit() || keypressed) {
			char ch;
			if (ch = KL11::serial_getchar()) {
				rbuf = ch & 0x7f;
				rcsr |= 0x80;
				if (rcsr & 0x40) {
					cpu.interrupt(INTTTYIN, 4);
				}
			}
			else {
				keypressed = false;
			}
		}
	}


	if (xbuf) {
		xcsr |= 0x80;
		xbuf = 0;
		if (xcsr & 0x40) {
			cpu.interrupt(INTTTYOUT, 4);
		}
	}
}

uint16_t KL11::read16(uint32_t a) {
	int i;

	switch (a) {
	case 0777560:
		return rcsr;
	case 0777562:
		rcsr &= ~0x80;
		if (rbuf == 13)
			i = 0;
		return rbuf;
	case 0777564:
		return xcsr;
	case 0777566:
		return 0;
	default:
		printf("kl11: read from invalid address %06o\n", a);
		trap(INTBUS);
	}
}

void KL11::write16(uint32_t a, uint16_t v) {
	switch (a) {
	case 0777560:
		rcsr |= v & (0x40);
		break;
	case 0777562:
		rcsr &= ~0x80;
		break;
	case 0777564:
		// printf("kl11:write16: %06o %06o\n", a, v);
		if (v & 0x40)
			xcsr |= v & (0x40);
		else
			xcsr &= ~(0x40);
		if ((xcsr & 0300) == 0300)
			cpu.interrupt(INTTTYOUT, 4);
		break;
	case 0777566:
		xbuf = v & 0x7f;
		KL11:serial_putchar(xbuf);
		xbuf |= 0200; // Allow for nulls !!!!
		xcsr &= ~0x80;
		break;
	default:
		printf("kl11: write to invalid address %06o\n", a);
		std::abort();
	}
}

