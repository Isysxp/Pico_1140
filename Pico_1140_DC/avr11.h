#pragma once
#include <stdint.h>

// interrupts
enum INTVEC {
    INTBUS = 0004,
    INTINVAL = 0010,
    INTDEBUG = 0014,
    INTIOT = 0020,
    INTTTYIN = 0060,
    INTTTYOUT = 0064,
    INTFAULT = 0250,
    INTCLOCK = 0100,
    INTRK = 0220,
    INTFIS = 0244,
    INTRL = 0160,
    INTDLR = 0300,
    INTDLT = 0304
};

[[ noreturn ]] void trap(uint16_t num);



