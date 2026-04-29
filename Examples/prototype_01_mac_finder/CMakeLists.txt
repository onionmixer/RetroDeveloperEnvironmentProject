cmake_minimum_required(VERSION 3.16)
project(prototype_01_mac_finder C)

# Build with the Retro68 toolchain. Caller passes:
#   -DCMAKE_TOOLCHAIN_FILE=<repo>/Library/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
# (compile.sh sets this automatically.)

add_application(Hello
    hello.c
    hello.r
)

set_target_properties(Hello PROPERTIES COMPILE_OPTIONS -ffunction-sections)
if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    set_target_properties(Hello PROPERTIES
        LINK_FLAGS "-Wl,-gc-sections -Wl,--mac-single -Wl,--mac-strip-macsbug"
    )
else()
    set_target_properties(Hello PROPERTIES LINK_FLAGS "-Wl,-gc-sections")
endif()
