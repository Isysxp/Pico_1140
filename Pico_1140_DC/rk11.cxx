#include <assert.h>
#include <cstdlib>
#include <stdint.h>
#include <stdio.h>
#include "ff.h"
#include "f_util.h"
#include "avr11.h"
#include "kb11.h"
#include "rk11.h"

extern KB11 cpu;
#define SETMASK(l, r, m) l = (((l)&~(m)) | ((r)&(m)))

enum RKERROR {
    RKOVR = (1 << 14),
    RKNXD = (1 << 7),
    RKNXC = (1 << 6),
    RKNXS = (1 << 5)
};

uint16_t RK11::read16(const uint32_t a) {
    switch (a) {
    case 0777400:
        // 777400 Drive Status
        return rkds;
    case 0777402:
        // 777402 Error Register
        return rker;
    case 0777404:
        // 777404 Control Status
        return rkcs & 0xfffe; // go bit is read only
    case 0777406:
        // 777406 Word Count
        return rkwc;
    case 0777412:
        return rkda;
    default:
        // printf("rk11::read16 invalid read %06o\n", a);
        trap(INTBUS);
    }
}

void RK11::rknotready() {
    rkds &= ~(1 << 6);
    rkcs &= ~(1 << 7);
}

void RK11::rkready() {
    rkds |= 1 << 6;
    rkcs |= 1 << 7;
    rkcs &= ~1; // no go
}

void RK11::step() {
    if ((rkcs & 01) == 0) {
        // no GO bit
        return;
    }


    switch ((rkcs >> 1) & 7) {
    case 0:
        // controller reset
        reset();
        break;
    case 1: // write
    case 2: // read
        rknotready();
        seek();
        readwrite();
        return;
    case 6: // Drive Reset - falls through to be finished as a seek
        rker = 0;
        [[fallthrough]];
    case 4: // Seek (and drive reset) - complete immediately
        rknotready();
        if (drive != 0) {
            rker |= 0x80; // NXD
            rkcs &= ~1;
        	cpu.interrupt(INTRK, 5);
            return;
        }
        seek();
        rkcs &= ~0x2000; // Clear search complete - reset by rk11_seekEnd
        rkcs |= 0x80;    // set done - ready to accept new command
	    if (rkcs & (1 << 6))
        	cpu.interrupt(INTRK, 5);
        break;
    case 5: // Read Check
        break;
    case 7: // Write Lock - not implemented :-(
        break;
    default:
        printf("unimplemented RK05 operation %06o\n", ((rkcs & 017) >> 1));
	    while (1) ;
    }
}

void RK11::readwrite() {
    if (rkwc == 0) {
        rkready();
        if (rkcs & (1 << 6)) {
            cpu.interrupt(INTRK, 5);
        }
        return;
    }

    bool w = ((rkcs >> 1) & 7) == 1;
    int i;
    int zero=0;
    int32_t pos = (cylinder * 24 + surface * 12 + sector) * 512;
    //if (w)
    //    printf("Write:");
    //else
    //    printf("Read: ");
    //printf(" Block:%d Addr:%o Count:%d RKDA:%o\n", pos / 512, rkba, 65536 - (int)rkwc, rkda);

    if (0) {
        printf("rk11: step: RKCS: %06o RKBA: %06o RKWC: %06o cylinder: %03o "
               "surface: %03o sector: %03o "
               "write: %x: RKER: %06o\n",
               rkcs, rkba, rkwc, cylinder, surface, sector, w, rker);
    }

    for (i = 0; i < 256 && rkwc != 0; i++) {
	    rkba18 = rkba | (rkcs & 060) << 12;     // Include ext addr bits
        if (w) {
            uint16_t val = cpu.unibus.read16(rkba18);
            uint8_t buf[2] = {static_cast<uint8_t>(val & 0xff),
                              static_cast<uint8_t>(val >> 8)};
	        f_write(&rk05, buf, 2, &bcnt); }
	    else {
		    uint8_t buf[2];
		    f_read(&rk05, buf, 2, &bcnt);
            cpu.unibus.write16(rkba18, static_cast<uint16_t>(buf[0]) |
                                         static_cast<uint16_t>(buf[1]) << 8);
        }
        rkba += 2;
        rkwc++;
	    if (rkba == 0)                          // Overflow into ext addr bits
		    SETMASK(rkcs, rkcs + 020, 060);
		}

    if (w && i<256) {                           // Fill remainder of sector with zero
        for (i=i;i<256;i++)
          f_write(&rk05,&zero,2,&bcnt);
    }

    if (w)
        f_sync(&rk05);
    sector++;
    if (sector > 013) {
        sector = 0;
        surface++;
        if (surface > 1) {
            surface = 0;
            cylinder++;
            if (cylinder > 0312) {
                rker |= RKOVR;
                return;
            }
        }
    }
}

void RK11::seek() {
    const uint32_t pos = (cylinder * 24 + surface * 12 + sector) * 512;
	if (FR_OK != (fr = f_lseek(&rk05, pos))) {
		printf(("rkstep: failed to seek\r\n"));
		while (1) ;
	}
}

void RK11::write16(const uint32_t a, const uint16_t v) {
    // printf("rk11:write16: %06o %06o\n", a, v);
    switch (a) {
    case 0777404:
        rkcs =  (v & ~0xf080) | (rkcs & 0xf080); // Bits 7 and 12 - 15 are read only
        break;
    case 0777406:
        rkwc = v;
        break;
    case 0777410:
        rkba = v;
        break;
    case 0777412:
        rkda = v;
        drive = v >> 13;
        cylinder = (v >> 5) & 0377;
        surface = (v >> 4) & 1;
        sector = v & 15;
        break;
    default:
        printf("rk11::write16 invalid write %06o: %06o\n", a, v);
    }
}

void RK11::reset() {
    //printf("rk11: reset\n");
    rkds = 04700; // Set bits 6, 7, 8, 11
    rker = 0;
    rkcs = 0200;
    rkwc = 0;
    rkba = 0;
    rkda = 0;
    drive = cylinder = surface = sector = 0;
}
