#pragma once
#include <stdint.h>

class DL11 {

  public:
    DL11();

    void clearterminal();
    void poll();
    uint16_t read16(uint32_t a);
    void write16(uint32_t a, uint16_t v);
	  void serial_putchar(char c);
	  char serial_getchar();


  private:
    uint16_t rcsr;
    uint16_t rbuf;
    uint16_t xcsr;
    uint8_t xbuf;
    uint16_t count;
    uint16_t iflag;

    inline bool rcvrdone() { return rcsr & 0x80; }
    inline bool xmitready() { return xcsr & 0x80; }
};