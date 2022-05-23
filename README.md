# Pico_1140
A PDP11/40 emulator that will run Unix v5/v6

Introduction and acknowledgements:

This is an updated version of Dave Cheney's CPP11 https://github.com/davecheney/cpp11 which will run on a Pi Pico.
For this project I have selected the Sparkfun Thing Plus board which contains an RP2040 chip, and sd card and 16Mb flash.
See https://www.sparkfun.com/products/17745.
The SD card interface uses Karl Kugler's app (https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico) which is included in the repo.
The project environment is for Visual Studio Code. The Pico SDK is also required https://github.com/raspberrypi/pico-sdk. This is not included.

The emulator:

The de facto emulator for most old computers is Simh https://github.com/simh/simh. The size and complexity of the individual machine
apps is such that a direct port to a memory limited system is not feasible. This is the reason for basing the system on a lightweight
emulator as provided by Dave Cheney for the ATMEGA2560 https://github.com/davecheney/avr11 and the C++ version above.
The code has been updated with patches to the KL11/KT11/KB11/RK11 systems so that the Pico SDK services may be used.
The 11/40 FIS has also been added an this is not a true simulation as found in Simh but an optimised emulation.
The emulator will run the basic MAINDECS
