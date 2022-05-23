#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <cstdlib>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ff.h"
#include "f_util.h"
#include "avr11.h"
#include "kb11.h"

KB11 cpu;
int kbdelay = 0;
int clkdelay = 0;
FIL bload;

/* Binary loader.

   Loader format consists of blocks, optionally preceded, separated, and
   followed by zeroes.  Each block consists of:

        001             ---
        xxx              |
        lo_count         |
        hi_count         |
        lo_origin        > count bytes
        hi_origin        |
        data byte        |
        :                |
        data byte       ---
        checksum

   If the byte count is exactly six, the block is the last on the tape, and
   there is no checksum.  If the origin is not 000001, then the origin is
   the PC at which to start the program.
*/

int f_xgetc(FIL *fl)
{
    unsigned char buf[2];
    UINT bctr;

    FRESULT fr=f_read(fl,buf,1,&bctr);
    if (fr==FR_OK)
        return buf[0];
    return -1;
}

int32_t sim_load(FIL *bld)
{
    uint32_t c[6], d, i, cnt, csum;
    uint32_t org;


    do {                                                    /* block loop */
        csum = 0;                                           /* init checksum */
        for (i = 0; i < 6; ) {                              /* 6 char header */
            if ((c[i] = f_xgetc(bld))==-1)
                return 1;
            if ((i != 0) || (c[i] == 1))                    /* 1st must be 1 */
                csum = csum + c[i++];                       /* add into csum */
        }
        cnt = (c[3] << 8) | c[2];                           /* count */
        org = (c[5] << 8) | c[4];                           /* origin */
        if (cnt < 6)                                        /* invalid? */
            return 1;
        if (cnt == 6) {                                     /* end block? */
            return 0;
        }
        for (i = 6; i < cnt; i++) {                         /* exclude hdr */
            if ((d = f_xgetc(bld))==-1)                /* data char */
                return 1;
            csum = csum + d;                                /* add into csum */
            if (org & 1)
                d = (d << 8) | cpu.unibus.core[org >> 1];
            cpu.unibus.core[org>>1] = d;
            org = (org + 1) & 0177777;                      /* inc origin */
        }
        if ((d = f_xgetc(bld))==-1)                    /* get csum */
            return 1;
        csum = csum + d;                                    /* add in */
    } while ((csum & 0377) == 0);                       /* result mbz */
    return 1;
}


uint16_t binload(const char* fnm)
{
    uint16_t rsl;

	FRESULT fr = f_open(&bload, fnm, FA_READ | FA_WRITE);
	if (FR_OK != fr && FR_EXIST != fr) {
		printf("f_open(%s) error: %s (%d)\n", fnm, FRESULT_str(fr), fr);
		while (1) ;
	}
    rsl = sim_load(&bload);
    f_close(&bload);
    return rsl;
}

void setup(char *disk) {

    if (strstr(disk,"maindec")) {
        cpu.reset(0200);
        //cpu.print=true;           // Uncomment to start continuous print of cpu steps
        if (binload(disk))
            printf("Load fail\r\n");
        else
            printf("Loaded tape:%s\r\n",disk);
        return;
    }
	if (cpu.unibus.rk11.rk05.obj.lockid)
		return;
	FRESULT fr = f_open(&cpu.unibus.rk11.rk05, disk, FA_READ | FA_WRITE);
	if (FR_OK != fr && FR_EXIST != fr) {
		printf("f_open(%s) error: %s (%d)\n", disk, FRESULT_str(fr), fr);
		while (1) ;
	}
	cpu.reset(02002);
    printf("Ready\n");
    
}

jmp_buf trapbuf;

void loop0();

[[noreturn]] void trap(uint16_t vec) { longjmp(trapbuf, vec); }

void loop() {
    auto vec = setjmp(trapbuf);
    if (vec == 0) {
        loop0();
    } else {
        cpu.trapat(vec);
    }
}

void loop0() {
    while (true) {
        cpu.step();
        if ((cpu.itab[0].vec > 0) && (cpu.itab[0].pri >= cpu.priority())) {
            cpu.trapat(cpu.itab[0].vec);
            cpu.popirq();
            return; // exit from loop to reset trapbuf
        }
        cpu.unibus.rk11.step();
        if (kbdelay++ == 1000) {
            cpu.unibus.cons.poll();
            kbdelay = 0;
        }
        if (clkdelay++ == 7000) {       // Cal MClock 200M, build = release
            cpu.unibus.kw11.tick();
            clkdelay = 0;
        }
    }
}

int startup(int argc, char *argv) {
    setup(argv);
    while (1)
        loop();
}

void panic() {
    cpu.printstate();
    std::abort();
}
