cmake_minimum_required(VERSION 3.14)
project(litehtmlrender LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

set(THIRDLIB ${CMAKE_CURRENT_SOURCE_DIR}/thirdlibrary)

# ===========================================================================
# Third-party libraries (all built from source)
# Build order respects dependency chain:
#   zlib -> libpng -> pixman -> freetype -> harfbuzz -> cairo
# ===========================================================================

# --- 1. zlib ---
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
add_subdirectory(${THIRDLIB}/zlib zlib)
# Make find_package(ZLIB) find our built zlib
if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()
set(ZLIB_INCLUDE_DIR "${THIRDLIB}/zlib;${CMAKE_CURRENT_BINARY_DIR}/zlib" CACHE PATH "" FORCE)
set(ZLIB_LIBRARY "$<TARGET_FILE:zlibstatic>" CACHE STRING "" FORCE)
set(ZLIB_LIBRARIES ZLIB::ZLIB CACHE STRING "" FORCE)
set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)

# --- 2. libpng ---
set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_STATIC ON CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
set(PNG_TOOLS OFF CACHE BOOL "" FORCE)
set(PNG_EXECUTABLES OFF CACHE BOOL "" FORCE)
set(PNG_BUILD_ZLIB OFF CACHE BOOL "" FORCE)
add_subdirectory(${THIRDLIB}/libpng libpng EXCLUDE_FROM_ALL)

# --- 3. pixman (custom CMakeLists.txt) ---
add_subdirectory(${THIRDLIB}/pixman pixman EXCLUDE_FROM_ALL)

# --- 4. freetype (disable harfbuzz to break circular dependency) ---
set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_ZLIB TRUE CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(${THIRDLIB}/freetype freetype EXCLUDE_FROM_ALL)

# --- 5. harfbuzz ---
set(HB_HAVE_FREETYPE ON CACHE BOOL "" FORCE)
set(HB_HAVE_GLIB OFF CACHE BOOL "" FORCE)
set(HB_HAVE_ICU OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GOBJECT OFF CACHE BOOL "" FORCE)
set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
add_subdirectory(${THIRDLIB}/harfbuzz harfbuzz EXCLUDE_FROM_ALL)

# --- 6. cairo (custom CMakeLists.txt, depends on pixman + freetype + libpng) ---
add_subdirectory(${THIRDLIB}/cairo cairo_build EXCLUDE_FROM_ALL)

# --- 7. litehtml ---
set(LITEHTML_PATH ${THIRDLIB}/litehtml)
if(NOT TARGET litehtml)
    add_subdirectory(${LITEHTML_PATH} litehtml)
endif()

# ===========================================================================
# Cairo container base files (pure Cairo, no Pango)
# ===========================================================================
set(CAIRO_CONTAINER_DIR ${LITEHTML_PATH}/containers/cairo)
set(CAIRO_CONTAINER_SRC
    ${CAIRO_CONTAINER_DIR}/container_cairo.cpp
    ${CAIRO_CONTAINER_DIR}/container_cairo.h
    ${CAIRO_CONTAINER_DIR}/cairo_borders.cpp
    ${CAIRO_CONTAINER_DIR}/cairo_borders.h
    ${CAIRO_CONTAINER_DIR}/conic_gradient.cpp
    ${CAIRO_CONTAINER_DIR}/conic_gradient.h
    ${CAIRO_CONTAINER_DIR}/cairo_images_cache.h
)

# ===========================================================================
# Project sources
# ===========================================================================
set(PROJECT_SRC
    src/font_manager.cpp
    src/font_manager.h
    src/image_loader.cpp
    src/image_loader.h
    src/container_cairo_ft.cpp
    src/container_cairo_ft.h
    src/html_renderer.cpp
    src/html_renderer.h
)

# --- Static library ---
add_library(litehtmlrender_lib STATIC
    ${CAIRO_CONTAINER_SRC}
    ${PROJECT_SRC}
)

target_include_directories(litehtmlrender_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CAIRO_CONTAINER_DIR}
    ${LITEHTML_PATH}/include
    ${LITEHTML_PATH}/src
    ${THIRDLIB}/stb
)

target_link_libraries(litehtmlrender_lib PUBLIC
    litehtml
    cairo
    freetype
    harfbuzz
)

# --- Executable ---
if(NOT ANDROID)
    add_executable(litehtmlrender src/main.cpp)
    target_link_libraries(litehtmlrender PRIVATE litehtmlrender_lib)
endif()
