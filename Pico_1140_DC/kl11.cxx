#include <cstdlib>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include "kb11.h"
#include "kl11.h"
#include <stdio.h>
#include "tusb.h"
#include "pico/stdlib.h"

extern KB11 cpu;

bool keypressed = false;

KL11::KL11()
{
}

void KL11::clearterminal()
{
	rcsr = 0;
	xcsr = 0x80;
	rbuf = 0;
	xbuf = 0;
	count = 0;
}

static int _kbhit()
{
	return tud_cdc_available() || uart_is_readable(uart0);
}

void KL11::serial_putchar(char c)
{
	if (tud_cdc_connected()) {
		tud_cdc_write_char(c);
		tud_cdc_write_flush();
	}
	while (!uart_is_writable(uart0));
	uart_putc(uart0,c);
}
char KL11::serial_getchar()
{
	if (tud_cdc_available())
		return tud_cdc_read_char();
	return uart_getc(uart0);
}

void KL11::poll()
{
	if (!rcvrdone())
	{
		// unit not busy
		if (count++)
			if (_kbhit() || keypressed)
			{
				char ch;
				count = 0;
				if (ch = KL11::serial_getchar())
				{
					rbuf = ch & 0x7f;
					rcsr |= 0x80;
					if (rcsr & 0x40)
					{
						cpu.interrupt(INTTTYIN, 4);
					}
				}
				else
				{
					keypressed = false;
				}
			}
	}

	if (xbuf)
	{
		xcsr |= 0x80;
		xbuf = 0;
		if (xcsr & 0x40)
		{
			cpu.interrupt(INTTTYOUT, 4);
		}
	} 
	else {
		if (iflag == 1) {
			cpu.interrupt(INTTTYOUT, 4);
			iflag = 2;
		}
	}
}

uint16_t KL11::read16(uint32_t a)
{

	switch (a)
	{
	case 0777560:
		return rcsr;
	case 0777562:
		rcsr &= ~0x80;
		return rbuf;
	case 0777564:
		return xcsr;
	case 0777566:
		return xbuf;
	default:
		// printf("kl11: read from invalid address %06o\n", a);
		trap(INTBUS);
	}
}

void KL11::write16(uint32_t a, uint16_t v)
{
	switch (a)
	{
	case 0777560:
		rcsr = ((rcsr & 0200) ^ (v & ~0200));
		break;
	case 0777562:
		rcsr &= ~0x80;
		break;
	case 0777564:
		xcsr = ((xcsr & 0200) ^ (v & ~0200));
		if ((xcsr & 0300) == 0300 && iflag == 0)
			iflag = 1;
		if (iflag == 2)
			iflag = 0;
		break;
	case 0777566:
		xbuf = v & 0x7f;
		serial_putchar(xbuf);
		xbuf |= 0200; // Allow for nulls !!!!
		xcsr &= ~0x80;
		iflag = 0;
		break;
	default:
		printf("kl11: write to invalid address %06o\n", a);
		std::abort();
	}
}
