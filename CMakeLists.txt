cmake_minimum_required(VERSION 3.12)

# Change your executable name to something creative!
set(NAME picodon) # <-- Name your project/executable here!

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format
        -Wno-unused-function
        -Wno-maybe-uninitialized
        )

# Add your source files
add_executable(${NAME}
    main.cpp mastodon.c mastdisplay.c tls_client.c JPEGDEC.cpp cJSON.c font8.c # <-- Add source files here!
)

# Include required libraries
# This assumes `pimoroni-pico` is stored alongside your project


# Don't forget to link the libraries you need!
target_link_libraries(${NAME}
	pico_stdlib
	hardware_dma
	hardware_pio
	hardware_pwm
	pico_cyw43_arch_lwip_threadsafe_background
	pico_lwip_mbedtls
	pico_mbedtls
    pico_multicore
    pico_scanvideo_dpi
)

target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR} )

target_compile_definitions(${NAME} PRIVATE 
    PICO_CORE1_STACK_SIZE=0x400 
    PICO_SCANVIDEO_PLANE1_VARIABLE_FRAGMENT_DMA=1
    PICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS=280
)

pico_enable_stdio_usb(${NAME} 1)
pico_enable_stdio_uart(${NAME} 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

# Set up files for the release packages
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)
