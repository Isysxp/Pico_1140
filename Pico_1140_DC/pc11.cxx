#include "pc11.h"
#include "avr11.h"
#include <cstdlib>
#include <stdio.h>

uint16_t PC11::read16(uint32_t a) {
    switch (a & 6) {
        case 0: // ptr11.prs  777550
            break;
        case 2: // ptr11.pdb  777552
            prs = prs & ~0x80; // Clear DONE,
            return prb;
    }
    printf("mmu::read16 invalid read from %06o\n", a);
    trap(004);
}