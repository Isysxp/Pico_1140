# Pico_1140
A PDP11/40 emulator that will run Unix v5/v6

Introduction and acknowledgements:

This is an updated version of Dave Cheney's CPP11 https://github.com/davecheney/cpp11 which will run on a Pi Pico.
For this project I have selected the Sparkfun Thing Plus board which contains an RP2040 chip, an SD card and 16Mb flash.
See https://www.sparkfun.com/products/17745.
Other hardware may be used including a Pi Pico itself. Wire the SDCard as per the Sparkfun schematic.
The SD card interface uses Karl Kugler's app (https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico) which is included in the repo.
The project environment is for Visual Studio Code. The Pico SDK is also required https://github.com/raspberrypi/pico-sdk. This is not included.

The emulator:

The de facto emulator for most old computers is Simh https://github.com/simh/simh. The size and complexity of the individual machine
apps is such that a direct port to a memory limited system is not feasible. This is the reason for basing the system on a lightweight
emulator as provided by Dave Cheney for the ATMEGA2560 https://github.com/davecheney/avr11 and the C++ version above.
The code has been updated with patches to the KL11/KT11/KB11/RK11 systems so that the Pico SDK services may be used.
The 11/40 FIS has also been added an this is not a true simulation as found in Simh but an optimised emulation.
The emulator will run the instruction test and EIS MAINDECS. It fails the traps tests as a stack limit trap and trace trap are not implemented.
Similarly, the KT11 test fails as the maintainence mode is not implemented. Finally, the FIS test fails as the implementation of this
uses translation of DEC floating point to IEEE 754.
The app is sufficiently accurate to run RT11 V3/V4, Unix V5 and V6 and mini-unix (http://www.tavi.co.uk/unixhistory/mini-unix.html). I would note
that the floating point performance for Unix is rather slow. It will also run MINC Basic for which I have included an RK05 image
which has the limitation that some command will return to the RT11 . prompt. The original version uses a modified RX02 monitor.
There is a KW11 line time clock. The implementation is not good and system time will not be accurate.

Building:

The app is provided as a Visual Studio Code project. Firstly, a copy of the Pico SDK is required in the same top level directory as the app.
The path to the SDK will need to be setup on your system. Also, GCC of at least version 10 is required. This may need to be installed and
selected as the active kit. The build process will create a .uf2 binary in the build directory. This can be copied to the RP2040 via the
virtual drive presented by the RP2040 boot system.
Any number of RK05 disk images may be copied to the root of an SDCard formatted to FAT32. I use a 32Gb card and the SPI system works with this.
The images are in the same format as used by Simh and may be built or modified in Simh prior to use.

Booting:

After inserting an SDCard and resetting the card, a COM port will appear. Connect a terminal app to this eg TeraTerm. The images on the SDCard
will be listed with an index that is used to select the boot volume. 
The system boots using a boot rom in bootrom.h at address 2000. The start address is set in setup(..) in avr11.cxx.
At present, only 1 volume can be attached at a time. Typically, unix images prompt with a '@', then type unix or rkunix to boot.
The switchregister is set by default to 0173030 in KB11::reset which will boot unix in single user mode.
In addition, the app contains a binary loader which can be called from setup(...) as above with a suitable change to the cpu start address.
You can load a binary tape from the SDCard if the name of the tape image starts with 'maindec'. The default start address in 0200;

Unix: It is sooooo easy to trash these old unix volumes! It is a good habit to 'sync' a lot of times just in case something does go wrong!

Update: 20/12/2022

I have now added an RL01/2 driver to the system. By default, the app will attach rt11V.dsk to unit 0. Please make sure this file is on
the SD Card (copy is in images). The logic in this driver is slightly different to the RK11 driver in that I/O is defered for 10 CPU cycles.
This is due to some curious behavior of RT11 in that without this option, the 'R' command does not work correctly whereas 'RUN' does.
After the system has booted into RT11 V4..
.BOOT DL0:
This will bring up RT11 V5 from the RL02 image.

On this image there is a copy of the DECUS c compiler. This only compiles really odltime c. Documentation in images directory.
As a test of this system, you can have a really good laugh and run PDP8.sav...

.r pdp8
Argv: (just press return)
PDP8 Emulator
->B  (need ucase)
->R 200 (need ucase)

. (now you are running the 4K PDP-8 DMS system.)
.FCL
CONGRATULATIONS!!
YOU HAVE SUCCESSFULLY LOADED 'FOCAL,1969' ON A PDP-8 COMPUTER.


SHALL I RETAIN LOG, EXP, ATN ?:YES

PROCEED.

*

So, now you have an emulated PDP-8 runnning on an emulated PDP-11 .... Rabbit hole or what!!!!
See PDP8.COM for the command lines to build this app and PDP8.C for the source code.

Have fun, Ian Schofield Dec 2022.





