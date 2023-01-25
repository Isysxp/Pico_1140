#pragma once
#include <stdint.h>
#include <stdio.h>
#include "ff.h"

class RK11 {

  public:
	FIL rk05;
    uint16_t read16(uint32_t a);
    void write16(uint32_t a, uint16_t v);
    void reset();
    void step();
	FRESULT fr;

  private:
	uint16_t rkds, rker, rkcs, rkwc, rkba, rkda;
	UINT bcnt;
    uint32_t drive, sector, surface, cylinder,rkba18,rkdelay;

    void rknotready();
    void rkready();
    void readwrite();
    void seek();
};
