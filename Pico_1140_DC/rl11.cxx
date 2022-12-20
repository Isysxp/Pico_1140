/*

Copyright (c) 2022 Ian Schofield

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
    This is a minmal RL01/02 disk handler
    This is only sufficient to run RT11 at present.
    The major kludge is to deal with sector overruns.
    Normally, one would use error flags to notify the OS.
    This turned out to be quite tricky.
    Note overrun code in step().
*/
#include "rl11.h"
#include "avr11.h"
#include <stdint.h>
#include <stdio.h>
#include "kb11.h"
#include "ff.h"
#include "pico/stdlib.h"

extern KB11 cpu;
extern uint64_t systime;

// This is not good. The conversion of RLDA to a Simh disk image file address is obscure

#define RL_NUMWD        (128)                           /* words/sector */
#define RL_NUMSC        (40)                            /* sectors/surface */
#define RL_NUMSF        (2)                             /* surfaces/cylinder */
#define RL_NUMCY        (256)                           /* cylinders/drive */

#define RLDA_V_SECT     (0)                             /* sector */
#define RLDA_M_SECT     (077)
#define RLDA_V_TRACK    (6)                             /* track */
#define RLDA_M_TRACK    (01777)
#define RLDA_HD0        (0 << RLDA_V_TRACK)
#define RLDA_HD1        (1u << RLDA_V_TRACK)
#define RLDA_V_CYL      (7)                             /* cylinder */
#define RLDA_M_CYL      (0777)
#define RLDA_TRACK      (RLDA_M_TRACK << RLDA_V_TRACK)
#define RLDA_CYL        (RLDA_M_CYL << RLDA_V_CYL)
#define GET_SECT(x)     (((x) >> RLDA_V_SECT) & RLDA_M_SECT)
#define GET_CYL(x)      (((x) >> RLDA_V_CYL) & RLDA_M_CYL)
#define GET_TRACK(x)    (((x) >> RLDA_V_TRACK) & RLDA_M_TRACK)
#define GET_DA(x)       ((GET_TRACK (x) * RL_NUMSC) + GET_SECT (x))

enum
{
    RLOPI = (1 << 10),
    RLDCRC = (1 << 11),
    RLHCRC = (1 << 11),
    RLWCE = (1 << 11),
    RLDLT = (1 << 12),
    RLHNF = (1 << 12),
    RLDE = (1 << 14),
    RLCERR = (1 << 15)
};

enum
{
    DEV_RL_BAE = 0774420,  // RL11 Bus Address Extension Register
    DEV_RL_MP = 0774406,   // RL11 Multipurpose Register
    DEV_RL_DA = 0774404,   // RL11 Disk Address
    DEV_RL_BA = 0774402,   // RL11 Bus Address
    DEV_RL_CS = 0774400,   // RL11 Control Status
};


void RL11::loadboot()
{
    size_t i;

    for (i = 0; i < BOOT_LEN; i++)
        cpu.unibus.write16(02000 + (i * 2), boot_rom[i]);

}

uint16_t RL11::read16(uint32_t a)
{
    //printf("Read:%06o\r\n", a);
    switch (a)
    {
    case DEV_RL_CS:  // Control Status (OR in the bus extension)
        return RLCS | (RLBA & 0600000) >> 12;
    case DEV_RL_MP:  // Multi Purpose
        return RLMP;
    case DEV_RL_BA:  // Bus Address
        return RLBA & 0xFFFF;
    case DEV_RL_DA:  // Disk Address
        return RLDA;
    case DEV_RL_BAE:
        return (RLBA & 0600000) >> 16;
    default:
        //printf("%% rl11::read16 invalid read");
        cpu.trapat(INTBUS);
        //panic();
        return 0;
    }
}

void RL11::rlnotready()
{
#ifdef PIN_OUT_DISK_ACT
    digitalWrite(PIN_OUT_DISK_ACT, LED_ON);
#endif
    //RLDS &= ~(1 << 6);
    RLCS &= ~(1 << 7);
}

void RL11::rlready()
{
    //RLDS |= 1 << 6;
    RLCS |= 1 << 7;
    if (RLCS & (1 << 6))
    {
        cpu.interrupt(INTRL, 5);
    }
#ifdef PIN_OUT_DISK_ACT
    digitalWrite(PIN_OUT_DISK_ACT, LED_OFF);
#endif
}

void rlerror(uint16_t e)
{
}

void RL11::step()
{
    if (!drun)
        return;
    if (drun-- > 1)
        return;

    drive = (RLCS >> 8) & 3;
    RLCS &= ~1;

    bool w;
    switch ((RLCS & 017) >> 1)
    {
    case 5:  // write data
        w = true;
        break;
    case 6:  // read data
    case 7:  // read no header check
        w = false;
        break;
    default:
        printf("%% unimplemented RL01/2 operation");  //  %#o", ((r.RLCS & 017) >> 1)))
        while (1);
    }

retry:
    int32_t pos = GET_DA(RLDA) * 256;
    int32_t maxwc = (RL_NUMSC - GET_SECT(RLDA)) * RL_NUMWD;
    int16_t wc = 0200000 - RLMP;

    if (wc > maxwc)                                         /* Will there be a track overrun? */
        wc = maxwc;
    RLWC = 65536 - wc;
    //Serial.printf("Disk: Loc:%08o WC:%06o BA:%08o\r\n", GET_DA(RLDA) * 256, wc,RLBA);
	if (FR_OK != (f_lseek(&rl02, pos))) {
        printf("%% rlstep: failed to seek\r\n");
        while (1);
    }
    uint16_t i=0;
    uint16_t val;
    while (RLWC)
    {
        if (w)
        {
            val = cpu.unibus.read16(RLBA);
            //fwrite(&val, 2, 1, rldata);
            f_write(&rl02,&val,2,&bcnt);
        }
        else
        {
            f_read(&rl02,&val,2,&bcnt);
            //fread(&val, 2, 1, rldata);
            cpu.unibus.write16(RLBA, val);
        }
        RLBA += 2;
        RLWC = (RLWC + 1) & 0xFFFF;
        i++;                            // Count of words transferred
    }
    i &= 0377;
    val = 0;
    RLMP = (RLMP + wc) & 0177777;
    if (w && !RLMP) {                   // If write AND IO complete (RLMP==0) fill remaining words in sector with zeros.
        if (i)
            for (i=i;i<256;i++)
                f_write(&rl02,&val,2,&bcnt);
    f_sync(&rl02);
    //fflush(rldata);
    }
    if (RLMP) {                         // Has RLMP (WC) gone to zero. If not, move to next track and continue
        RLDA = (RLDA + 0100) & ~077;
        goto retry;
    }
    RLCS |= 1;
    rlready();
    drun = 0;
}

void RL11::write16(uint32_t a, uint16_t v)
{
    //Serial.printf("%% rlwrite: %06o -> %06o RLCS:%d\r\n",a,v,RLCS & 1);
    switch (a)
    {
    case DEV_RL_CS:  // Control Status
        RLBA = (RLBA & 0xFFFF) | ((v & 060) << 12);
        v &= 01776;  // writable bits
        RLCS &= ~01776;
        RLCS |= v;
        //printf("RLCS:%06o\r\n", (RLCS & 016)>>1);
        if ((RLCS & 0200) == 0)                // CRDY cleared
        {
            switch ((RLCS & 016) >> 1)
            {
            case 0:                     // NOP
            case 1:                     // Write check (ignored)
                break;
            case 2:                     // Get status
                RLCS &= 01777;          // Clear error flags
                if (RLDA & 2) {         // Do get status
                    RLMP = 0235;        // Heads out/On track/RL02
                    rlready();          // Immediate ready
                }
                break;
            case 3:                     // Seek ... just set ready
                RLCS &= 01777;          // Clear error flags
                rlready();
                break;
            case 4:                     // Read header
                rlready();
                break;
            case 5:
            case 6:                    // Read or Write
                RLWC = RLMP;
                drun = 10;             // Defer xfer by 10 cpu cycles
                RLCS &= ~1;            // Clear Ready
                break;
            default:
                printf("%% unimplemented RL02 operation");  // %#o", ((r.RLCS & 017) >> 1)))
                while (1);
            }
        }
        break;
    case DEV_RL_BA:  // Bus Address
        RLBA = (RLBA & 0600000) | (v);
        break;
    case DEV_RL_DA:  // Disk Address
        RLDA = v;
        break;
    case DEV_RL_MP:  // Multi-purpose register
        RLMP = v;
        break;
    case DEV_RL_BAE:  // Data Buffer
        RLBA = (RLBA & 0xffff) | ((v & 077) << 16);
        break;
    default:
        //printf("%% rlwrite16: invalid write");
        cpu.trapat(INTBUS);
        //panic();
    }
}

void RL11::reset()
{
    RLCS = 0x81;
    RLBA = 0;
    RLDA = 0;
    RLMP = 0;
    drun = 0;
}


