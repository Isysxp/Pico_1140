cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(Pico_1140_DC C CXX ASM)
pico_sdk_init()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER_FORCED)
set(CMAKE_CXX_COMPILER_FORCED)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
if (TARGET tinyusb_device)
    add_executable(Pico_1140 avr11.cxx disasm.cxx getline.cxx kb11.cxx kl11.cxx kt11.cxx kw11.cxx lp11.cxx pc11.cxx Pico_1140.cxx rk11.cxx rl11.cxx unibus.cxx dl11.cxx hw_config.c)

    # pull in common dependencies
    target_link_libraries(Pico_1140 pico_stdlib FatFs_SPI)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(Pico_1140 1)
    pico_enable_stdio_uart(Pico_1140 1)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(Pico_1140)

    # add url via pico_set_program_url
#     example_auto_set_url(Pico_1140)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()
add_subdirectory(no-OS-FatFS-SD-SPI-RPi-Pico-master/FatFs_SPI)
add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=1)
add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0)
