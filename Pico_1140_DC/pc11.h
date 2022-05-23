#include <stdint.h>


class PC11 {

    public:
    uint16_t prs, prb, pps, ppb;

    uint16_t read16(uint32_t a);
};