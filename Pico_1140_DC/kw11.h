#pragma once
#include <stdint.h>

class KW11 {
  public:
    void write16(uint32_t a, uint16_t v);
    uint16_t read16(uint32_t a);

    KW11();
    void tick();

  private:
    uint16_t csr;
};