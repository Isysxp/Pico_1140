The UF2 image is built for terminal I/O via Serial 1 and 2 on the Sparkfun Thing+ board
Serial0: GP0 and GP1 Serial1: GP20 and GP21.
AND I/O via 2 USB ports. You may use either or both.

ATTN: The repo source code is setup to wait for a USB connection... See Pico_1140.cxx


Pico_1140_USB.uf2:  (Default) This build will wait for a USB connection to the master comm port.
Pico_1140_UART.uf2: This build will not wait for a USB conection and data is immediately
                    streamed from the serial ports.