cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(udelay
  udelay.c
  read_buf.pio
  modulate.pio
)

pico_generate_pio_header(udelay
  ${CMAKE_CURRENT_LIST_DIR}/read_buf.pio
)

pico_generate_pio_header(udelay
  ${CMAKE_CURRENT_LIST_DIR}/modulate.pio
)


pico_enable_stdio_usb(udelay 1)
pico_enable_stdio_uart(udelay 0)

pico_add_extra_outputs(udelay)

target_link_libraries(udelay
  pico_stdlib
  hardware_dma
  hardware_pio
)
