#include <cstdlib>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>

#include "kb11.h"
#include "lp11.h"

extern KB11 cpu;

void LP11::poll() {
    if (!(lps & 0x80)) {
        if (++count > 3000) {
            fputc(lpb & 0x7f, stdout);
            lps |= 0x80;
            if (lps & (1 << 6)) {
                cpu.interrupt(0200, 4);
            }
        }
    }
}

uint16_t LP11::read16(uint32_t a) {
    switch (a) {
    case 0777514:
        return lps;
    case 0777516:
        return 0; // lpb cannot be read
    default:
        printf("lp11: read from invalid address %06o\n", a);
        std::abort();
    }
}

void LP11::write16(uint32_t a, uint16_t v) {
    switch (a) {
    case 0777514:
        if (v & (1 << 6)) {
            lps |= 1 << 6;
        } else {
            lps &= ~(1 << 6);
        }
        if (lps & (1 << 6)) {
            cpu.interrupt(0200, 4);
        }
        break;
    case 0777516:
        lpb = v & 0x7f;
        lps &= 0xff7f;
        count = 0;
        break;
    default:
        printf("lp11: write to invalid address %06o\n", a);
        std::abort();
    }
}

void LP11::reset() {
    lps = 0x80;
    lpb = 0;
}
