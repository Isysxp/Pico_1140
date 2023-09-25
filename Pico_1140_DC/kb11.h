#pragma once
#include "kt11.h"
#include "unibus.h"
#include <array>
#include <stdint.h>

enum { FLAGN = 8, FLAGZ = 4, FLAGV = 2, FLAGC = 1 };

class KB11 {
  public:
    void step();
    void reset(uint16_t start, int bootdev);

    void trapat(uint16_t vec);

    // interrupt schedules an interrupt.
    void interrupt(uint8_t vec, uint8_t pri);
    void printstate();
    void ptstate();

    // mode returns the current cpu mode.
    // 0: kernel, 1: supervisor, 2: illegal, 3: user
    constexpr inline uint16_t currentmode() { return (PSW >> 14); }

    // previousmode returns the previous cpu mode.
    // 0: kernel, 1: supervisor, 2: illegal, 3: user
    constexpr inline uint16_t previousmode() { return ((PSW >> 12) & 3); }

    // returns the current CPU interrupt priority.
    constexpr inline uint16_t priority() { return ((PSW >> 5) & 7); }

    // pop the top interrupt off the itab.
    void popirq();

    struct intr {
        uint8_t vec;
        uint8_t pri;
    };

    int rflag;

    std::array<intr, 32> itab;

    KT11 mmu;
    UNIBUS unibus;
    bool print=false;
    bool wtstate;
    uint16_t read16(uint16_t va);
    void write16(uint16_t va, uint16_t v);
    uint16_t PC;               // holds R[7] during instruction execution
    uint16_t PSW;              // processor status word
    uint16_t R[8]; // R0-R7

  private:
    uint16_t oldPSW;
    uint16_t stacklimit, switchregister, displayregister;
    std::array<uint16_t, 4>
        stackpointer; // Alternate R6 (kernel, super, illegal, user)


    inline bool N() { return PSW & FLAGN; }
    inline bool Z() { return PSW & FLAGZ; }
    inline bool V() { return PSW & FLAGV; }
    inline bool C() { return PSW & FLAGC; }
    inline void setZ(const bool b) {
        if (b)
            PSW |= FLAGZ;
    }

    inline uint16_t fetch16() {
        const auto val = read16(R[7]);
        R[7] += 2;
        return val;
    }

    inline void push(const uint16_t v) {
        R[6] -= 2;
        write16(R[6], v);
    }

    inline uint16_t pop() {
        const auto val = read16(R[6]);
        R[6] += 2;
        return val;
    }

    template <auto len> inline uint16_t DA(const uint16_t instr) {
        static_assert(len == 1 || len == 2);
        if (!(instr & 070)) {
            rflag++;
            return (instr & 7);
        }
        return fetchOperand<len>(instr);
    }

    template <auto len> uint16_t fetchOperand(const uint16_t instr) {
        const auto mode = (instr >> 3) & 7;
        const auto reg = instr & 7;

        uint16_t addr;
        switch (mode) {
        case 0: // Mode 0: Registers don't have a virtual address so trap!
            trap(4);
        case 1: // Mode 1: (R)
            return R[reg];
        case 2: // Mode 2: (R)+ including immediate operand #x
            addr = R[reg];
            R[reg] += (reg >= 6) ? 2 : len;
            return addr;
        case 3: // Mode 3: @(R)+
            addr = R[reg];
            R[reg] += 2;
            return read16(addr);
        case 4: // Mode 4: -(R)
            R[reg] -= (reg >= 6) ? 2 : len;
            addr = R[reg];
            return addr;
        case 5: // Mode 5: @-(R)
            R[reg] -= 2;
            addr = R[reg];
            return read16(addr);
        case 6: // Mode 6: d(R)
            addr = fetch16();
            addr = addr + R[reg];
            return addr;
        default: // 7 Mode 7: @d(R)
            addr = fetch16();
            addr = addr + R[reg];
            return read16(addr);
        }
    }

    template <auto len> constexpr uint16_t SS(const uint16_t instr) {
        static_assert(len == 1 || len == 2);
        if (!((instr >> 6) & 070)) {
            // If register mode just get register value
            return R[(instr >> 6) & 7] & max<len>();
        }
        const auto addr = fetchOperand<len>(instr >> 6);
        if constexpr (len == 2) {
            return read16(addr);
        }
        if (addr & 1) {
            return read16(addr & ~1) >> 8;
        }
        return read16(addr & ~1) & 0xFF;
    }

    constexpr inline void branch(const uint16_t instr) {
        if (instr & 0x80) {
            R[7] += (instr | 0xff00) << 1;
        } else {
            R[7] += (instr & 0xff) << 1;
        }
    }

    constexpr inline void writePSW(const uint16_t psw) {
        stackpointer[currentmode()] = R[6];
        PSW = psw;
        R[6] = stackpointer[currentmode()];
    }

    // kernelmode pushes the current processor mode and switchs to kernel.
    constexpr inline void kernelmode() {
        writePSW((PSW & 0007777) | (currentmode() << 12));
    }

    template <auto l> constexpr inline uint16_t read(const uint16_t a) {
        static_assert(l == 1 || l == 2);

        if (rflag) {
            if constexpr (l == 2) {
                return R[a & 7];
            }
            else {
                return R[a & 7] & 0xFF;
            }
        }
        if constexpr (l == 2) {
            return read16(a);
        }
        if (a & 1) {
            return read16(a & ~1) >> 8;
        }
        return read16(a) & 0xFF;
    }

    template <auto l> constexpr void write(const uint16_t a, const uint16_t v) {
        static_assert(l == 1 || l == 2);
        auto vl = v;

        if (rflag) {
            auto r = a & 7;
            if constexpr (l == 2) {
                R[r] = vl;
            }
            else {
                R[r] &= 0xFF00;
                R[r] |= vl & 0xFF;
            }
            return;
        }
        if constexpr (l == 2) {
            write16(a, vl);
            return;
        }
        if (a & 1) {
            write16(a & ~1, (read16(a & ~1) & 0xff) | (vl << 8));
        }
        else {
            write16(a, (read16(a) & 0xFF00) | (vl & 0xFF));
        }
    }

    template <auto l> constexpr inline uint16_t max() {
        static_assert(l == 1 || l == 2);
        if constexpr (l == 2) {
            return 0xffff;
        } else {
            return 0xff;
        }
    }

    template <auto l> constexpr inline uint16_t msb() {
        static_assert(l == 1 || l == 2);
        if constexpr (l == 2) {
            return 0x8000;
        } else {
            return 0x80;
        }
    }

    // CMP 02SSDD, CMPB 12SSDD
    template <auto l> void CMP(const uint16_t instr) {
        const auto src = SS<l>(instr);
        const auto da = DA<l>(instr);
        const auto dst = read<l>(da);
        const auto sval = (src - dst) & max<l>();
        PSW &= 0xFFF0;
        if (sval == 0) {
            PSW |= FLAGZ;
        }
        if (sval & msb<l>()) {
            PSW |= FLAGN;
        }
        if (((src ^ dst) & msb<l>()) && (!((dst ^ sval) & msb<l>()))) {
            PSW |= FLAGV;
        }
        if (src < dst) {
            PSW |= FLAGC;
        }
    }

    // Set N & Z clearing V (C unchanged)
    template <auto len> inline void setNZ(const uint16_t v) {
        static_assert(len == 1 || len == 2);
        PSW &= (0xFFF0 | FLAGC);
        if (v & msb<len>()) {
            PSW |= FLAGN;
        }
        if ((v & max<len>()) == 0) {
            PSW |= FLAGZ;
        }
    }

    // Set N, Z & V (C unchanged)
    template <auto len> inline void setNZV(const uint16_t v) {
        setNZ<len>(v);
        if (v == msb<len>()) {
            PSW |= FLAGV;
        }
    }

    // Set N, Z & C clearing V
    template <auto len> inline void setNZC(const uint16_t v) {
        static_assert(len == 1 || len == 2);
        PSW &= 0xFFF0;
        if (v & msb<len>()) {
            PSW |= FLAGN;
        }
        if (v == 0) {
            PSW |= FLAGZ;
        }
        PSW |= FLAGC;
    }

    template <auto l> void BIC(const uint16_t instr) {
        const auto src = SS<l>(instr);
        const auto da = DA<l>(instr);
        const auto dst = read<l>(da);
        auto uval = (max<l>() ^ src) & dst;
        write<l>(da, uval);
        PSW &= 0xFFF1;
        setZ(uval == 0);
        if (uval & msb<l>()) {
            PSW |= FLAGN;
        }
    }

    template <auto l> void BIS(const uint16_t instr) {
        const auto src = SS<l>(instr);
        const auto da = DA<l>(instr);
        const auto dst = read<l>(da);
        auto uval = src | dst;
        PSW &= 0xFFF1;
        setZ(uval == 0);
        if (uval & msb<l>()) {
            PSW |= FLAGN;
        }
        write<l>(da, uval);
    }

    // CLR 0050DD, CLRB 1050DD
    template <auto l> void CLR(const uint16_t instr) {
        PSW &= 0xFFF0;
        PSW |= FLAGZ;
        write<l>(DA<l>(instr), 0);
    }

    // COM 0051DD, COMB 1051DD
    template <auto l> void COM(const uint16_t instr) {
        const auto da = DA<l>(instr);
        const auto dst = ~read<l>(da);
        write<l>(da, dst);
        PSW &= 0xFFF0;
        if ((dst & msb<l>())) {
            PSW |= FLAGN;
        }
        if ((dst & max<l>()) == 0) {
            PSW |= FLAGZ;
        }
        PSW |= FLAGC;
    }

    // DEC 0053DD, DECB 1053DD
    template <auto l> void _DEC(const uint16_t instr) {
        const auto da = DA<l>(instr);
        const auto oval = read<l>(da) & max<l>();
        const auto uval = (read<l>(da) - 1) & max<l>();
        write<l>(da, uval);
        //setNZV<l>(uval);
        setNZ<l>(uval);
        if (oval == msb<l>()) {
            PSW |= FLAGV;
        }

    }

    // NEG 0054DD, NEGB 1054DD
    template <auto l> void NEG(const uint16_t instr) {
        const auto da = DA<l>(instr);
        const auto dst = (-read<l>(da)) & max<l>();
        write<l>(da, dst);
        PSW &= 0xFFF0;
        if (dst & msb<l>()) {
            PSW |= FLAGN;
        }
        if ((dst & max<l>()) == 0) {
            PSW |= FLAGZ;
        } else {
            PSW |= FLAGC;
        }
        if (dst == msb<l>()) {
            PSW |= FLAGV;
        }
    }

        template <auto l> void _ADC(const uint16_t instr) {
        const auto da = DA<l>(instr);
        auto uval = read<l>(da);
        if (PSW & FLAGC) {
            write<l>(da, (uval + 1) & max<l>());
            PSW &= 0xFFF0;
            if ((uval + 1) & msb<l>()) {
                PSW |= FLAGN;
            }
            setZ(uval == max<l>());
            if (l == 1)
                uval = (uval << 8) | 0xff;
            if (uval == 0077777) {
                PSW |= FLAGV;
            }
            if (uval == 0177777) {
                PSW |= FLAGC;
            }
        }
        else {
            PSW &= 0xFFF0;
            if (uval & msb<l>()) {
                PSW |= FLAGN;
            }
            setZ(uval == 0);
        }
    }


    template <auto l> void SBC(const uint16_t instr) {
        const auto da = DA<l>(instr);
        auto sval = read<l>(da);
        auto qval = sval;
        PSW &= ~(FLAGN | FLAGV | FLAGZ);
        if (PSW & FLAGC) {
            if (sval)
                PSW ^= FLAGC;
            sval = (sval - 1) & max<l>();
            write<l>(da, sval);
        }
        setZ(sval == 0);
        if (qval == msb<l>())
            PSW |= FLAGV;
        if (sval & msb<l>())
            PSW |= FLAGN;

    }

    template <auto l> void ROR(const uint16_t instr) {
        const auto da = DA<l>(instr);
        const auto dst = read<l>(da);
        auto result = dst >> 1;
        if (PSW & FLAGC) {
            result |= msb<l>();
        }
        write<l>(da, result);
        PSW &= 0xFFF0;
        if (dst & 1) {
            // shift lsb into carry
            PSW |= FLAGC;
        }
        if (result & msb<l>()) {
            PSW |= FLAGN;
        }
        if ((result & max<l>()) == 0) {
            PSW |= FLAGZ;
        }
        if (!(PSW & FLAGC) ^ !(PSW & FLAGN)) {
            PSW |= FLAGV;
        }
    }

    template <auto l> void ROL(const uint16_t instr) {
        const auto da = DA<l>(instr);
        int32_t sval = read<l>(da) << 1;
        if (PSW & FLAGC) {
            sval |= 1;
        }
        PSW &= 0xFFF0;
        if (sval & (max<l>() + 1)) {
            PSW |= FLAGC;
        }
        if (sval & msb<l>()) {
            PSW |= FLAGN;
        }
        setZ(!(sval & max<l>()));
        if ((sval ^ (sval >> 1)) & msb<l>()) {
            PSW |= FLAGV;
        }
        sval &= max<l>();
        write<l>(da, sval);
    }

    template <auto l> void ASR(const uint16_t instr) {
        const auto da = DA<l>(instr);
        auto uval = read<l>(da);
        PSW &= 0xFFF0;
        if (uval & 1) {
            PSW |= FLAGC;
        }
        uval = (uval & msb<l>()) | (uval >> 1);
        if (uval & msb<l>()) {
            PSW |= FLAGN;
        }
        if (!(PSW & FLAGC) ^ !(PSW & FLAGN)) {
            PSW |= FLAGV;
        }
        setZ(uval == 0);
        write<l>(da, uval);
    }

    template <auto l> void ASL(const uint16_t instr) {
        const auto da = DA<l>(instr);
        // TODO(dfc) doesn't need to be an sval
        int32_t sval = read<l>(da);
        PSW &= 0xFFF0;
        if (sval & msb<l>()) {
            PSW |= FLAGC;
        }
        if (sval & (msb<l>() >> 1)) {
            PSW |= FLAGN;
        }
        if ((sval ^ (sval << 1)) & msb<l>()) {
            PSW |= FLAGV;
        }
        sval = (sval << 1) & max<l>();
        setZ(sval == 0);
        write<l>(da, sval);
    }

    // INC 0052DD, INCB 1052DD
    template <auto l> void INC(const uint16_t instr) {
        const auto da = DA<l>(instr);
        const auto dst = read<l>(da) + 1;
        write<l>(da, dst);
        setNZV<l>(dst);
    }

    // BIT 03SSDD, BITB 13SSDD
    template <auto l> void BIT(const uint16_t instr) {
        const auto src = SS<l>(instr);
        const auto dst = read<l>(DA<l>(instr));
        const auto result = src & dst;
        setNZ<l>(result);
    }

    // TST 0057DD, TSTB 1057DD
    template <auto l> void TST(const uint16_t instr) {
        const auto dst = read<l>(DA<l>(instr));
        PSW &= 0xFFF0;
        if ((dst & max<l>()) == 0) {
            PSW |= FLAGZ;
        }
        if (dst & msb<l>()) {
            PSW |= FLAGN;
        }
    }

    // MOV 01SSDD, MOVB 11SSDD
    template <auto len> void MOV(const uint16_t instr) {
        const auto src = SS<len>(instr);
        if (!(instr & 0x38) && (len == 1)) {
            // Special case: movb sign extends register to word size
            R[instr & 7] = src & 0x80 ? 0xff00 | src : src;
            setNZ<len>(src);
            return;
        }
        setNZ<len>(src);
        write<len>(DA<len>(instr), src);
    }

    void ADD(const uint16_t instr);
    void SUB(const uint16_t instr);
    void JSR(const uint16_t instr);
    void MUL(const uint16_t instr);
    void DIV(const uint16_t instr);
    void ASH(const uint16_t instr);
    void ASHC(const uint16_t instr);
    void XOR(const uint16_t instr);
    void SOB(const uint16_t instr);
    void FIS(const uint16_t instr);
    void JMP(const uint16_t instr);
    void MARK(const uint16_t instr);
    void MFPI(const uint16_t instr);
    void MFPT();
    void MTPS(const uint16_t instr);
    void MFPS(const uint16_t instr);
    void MTPI(const uint16_t instr);
    void RTS(const uint16_t instr);
    void EMTX(const uint16_t instr);
    void SWAB(uint16_t);
    void SXT(uint16_t);
    void RTT();
    void RESET();
    void WAIT();
};