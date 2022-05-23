#include "kt11.h"
#include <stdint.h>
#include <stdio.h>

uint16_t KT11::read16(const uint32_t a) {
    // printf("kt11:read16: %06o\n", a);
    const auto i = ((a & 017) >> 1);
    switch (a & ~037) {
    case 0772200:
        return pages[01][i].pdr;
    case 0772240:
        return pages[01][i].par;
    case 0772300:
        return pages[00][i].pdr;
    case 0772340:
        return pages[00][i].par;
    case 0777600:
        return pages[03][i].pdr;
    case 0777640:
        return pages[03][i].par;
    default:
        printf("mmu::read16 invalid read from %06o\n", a);
        trap(004); // intbus
    }
}

void KT11::write16(const uint32_t a, const uint16_t v) {
    //  printf("kt11:write16: %06o %06o\n", a, v);
    const auto i = ((a & 017) >> 1);
    switch (a & ~037) {
    case 0772200:
        pages[01][i].pdr = v;
        break;
    case 0772240:
        pages[01][i].par = v;
        break;
    case 0772300:
        pages[00][i].pdr = v;
        break;
    case 0772340:
        pages[00][i].par = v;
        break;
    case 0777600:
        pages[03][i].pdr = v;
        break;
    case 0777640:
        pages[03][i].par = v;
        break;
    default:
        printf("mmu::write16 write to invalid address %06o\n", a);
        trap(004); // intbus
    }
}
