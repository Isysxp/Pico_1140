/*
Modified BSD License

Copyright (c) 2021 Chloe Lunn

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

// sam11 software emulation of DEC PDP-11/40 RK11 RK Disk Controller

#pragma once
#include "stdint.h"
#include "ff.h"


class RL11 {

public:
FIL rl02;
void write16(uint32_t a, uint16_t v);
uint16_t read16(uint32_t a);
void reset();
void step();
void rlnotready();
void rlready();
void loadboot();

private:
    uint16_t drive, drun, dtype;
    uint16_t RLWC, RLDA, RLMP, RLCS, RLBAE;
    uint32_t RLBA;
    UINT bcnt;

#define BOOT_START      02000                           /* start */
#define BOOT_ENTRY      (BOOT_START + 002)              /* entry */
#define BOOT_UNIT       (BOOT_START + 010)              /* unit number */
#define BOOT_CSR        (BOOT_START + 020)              /* CSR */
#define BOOT_LEN        (sizeof (boot_rom) / sizeof (uint16_t))

static inline uint16_t boot_rom[] = {
    0042114,                        /* "LD" */
    0012706, BOOT_START,            /* MOV #boot_start, SP */
    0012700, 0000000,               /* MOV #unit, R0 */
    0010003,                        /* MOV R0, R3 */
    0000303,                        /* SWAB R3 */
    0012701, 0174400,               /* MOV #RLCS, R1        ; csr */
    0012761, 0000013, 0000004,      /* MOV #13, 4(R1)       ; clr err */
    0052703, 0000004,               /* BIS #4, R3           ; unit+gstat */
    0010311,                        /* MOV R3, (R1)         ; issue cmd */
    0105711,                        /* TSTB (R1)            ; wait */
    0100376,                        /* BPL .-2 */
    0105003,                        /* CLRB R3 */
    0052703, 0000010,               /* BIS #10, R3          ; unit+rdhdr */
    0010311,                        /* MOV R3, (R1)         ; issue cmd */
    0105711,                        /* TSTB (R1)            ; wait */
    0100376,                        /* BPL .-2 */
    0016102, 0000006,               /* MOV 6(R1), R2        ; get hdr */
    0042702, 0000077,               /* BIC #77, R2          ; clr sector */
    0005202,                        /* INC R2               ; magic bit */
    0010261, 0000004,               /* MOV R2, 4(R1)        ; seek to 0 */
    0105003,                        /* CLRB R3 */
    0052703, 0000006,               /* BIS #6, R3           ; unit+seek */
    0010311,                        /* MOV R3, (R1)         ; issue cmd */
    0105711,                        /* TSTB (R1)            ; wait */
    0100376,                        /* BPL .-2 */
    0005061, 0000002,               /* CLR 2(R1)            ; clr ba */
    0005061, 0000004,               /* CLR 4(R1)            ; clr da */
    0012761, 0177000, 0000006,      /* MOV #-512., 6(R1)    ; set wc */
    0105003,                        /* CLRB R3 */
    0052703, 0000014,               /* BIS #14, R3          ; unit+read */
    0010311,                        /* MOV R3, (R1)         ; issue cmd */
    0105711,                        /* TSTB (R1)            ; wait */
    0100376,                        /* BPL .-2 */
    0042711, 0000377,               /* BIC #377, (R1) */
    0005002,                        /* CLR R2 */
    0005003,                        /* CLR R3 */
    0012704, BOOT_START + 020,        /* MOV #START+20, R4 */
    0005005,                        /* CLR R5 */
    0005007                         /* CLR PC */
};


};  // class rl11

