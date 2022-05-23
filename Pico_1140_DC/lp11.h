#pragma once
#include <stdint.h>

class LP11 {

  public:
    void poll();
    void reset();
    uint16_t read16(uint32_t a);
    void write16(uint32_t a, uint16_t v);

  private:
    uint16_t lps;
    uint16_t lpb;
    uint16_t count;
};