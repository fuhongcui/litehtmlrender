cmake_minimum_required(VERSION 3.14)
project(pixman C)

set(PIXMAN_VERSION_MAJOR 0)
set(PIXMAN_VERSION_MINOR 42)
set(PIXMAN_VERSION_MICRO 2)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/pixman/pixman-version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/pixman-version.h
    @ONLY
)

set(PIXMAN_SOURCES
    pixman/pixman.c
    pixman/pixman-access.c
    pixman/pixman-access-accessors.c
    pixman/pixman-bits-image.c
    pixman/pixman-combine32.c
    pixman/pixman-combine-float.c
    pixman/pixman-conical-gradient.c
    pixman/pixman-edge.c
    pixman/pixman-edge-accessors.c
    pixman/pixman-fast-path.c
    pixman/pixman-filter.c
    pixman/pixman-general.c
    pixman/pixman-glyph.c
    pixman/pixman-gradient-walker.c
    pixman/pixman-image.c
    pixman/pixman-implementation.c
    pixman/pixman-linear-gradient.c
    pixman/pixman-matrix.c
    pixman/pixman-noop.c
    pixman/pixman-radial-gradient.c
    pixman/pixman-region16.c
    pixman/pixman-region32.c
    pixman/pixman-solid-fill.c
    pixman/pixman-timer.c
    pixman/pixman-trap.c
    pixman/pixman-utils.c
    pixman/pixman-x86.c
    pixman/pixman-arm.c
    pixman/pixman-ppc.c
    pixman/pixman-mips.c
)

add_library(pixman STATIC ${PIXMAN_SOURCES})

target_include_directories(pixman PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/pixman
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(pixman PRIVATE
    PACKAGE="pixman"
    HAVE_BUILTIN_CLZ=1
)

if(NOT WIN32)
    target_compile_definitions(pixman PRIVATE
        HAVE_PTHREADS=1
    )
    target_link_libraries(pixman PRIVATE pthread)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(pixman PRIVATE -w)
endif()
