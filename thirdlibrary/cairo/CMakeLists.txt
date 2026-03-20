cmake_minimum_required(VERSION 3.14)
project(cairo_local C CXX)

# Generate config.h for cairo internals
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/config.h
"#ifndef CAIRO_CONFIG_H
#define CAIRO_CONFIG_H

#define HAVE_STDINT_H   1
#define HAVE_UINT64_T   1

#ifdef __linux__
#define HAVE_UNISTD_H   1
#define HAVE_PTHREAD_H  1
#define HAVE_SYS_MMAN_H 1
#define HAVE_MMAP       1
#define HAVE_FENV_H     1
#define HAVE_DLFCN_H    1
#define HAVE_STRNDUP    1
#define CAIRO_HAS_REAL_PTHREAD 1
#define CAIRO_HAS_PTHREAD 1
#endif

#ifdef __ANDROID__
#define HAVE_UNISTD_H   1
#define HAVE_PTHREAD_H  1
#define HAVE_SYS_MMAN_H 1
#define HAVE_MMAP       1
#define HAVE_FENV_H     1
#define HAVE_STRNDUP    1
#define CAIRO_HAS_REAL_PTHREAD 1
#define CAIRO_HAS_PTHREAD 1
#endif

#endif
")

# Generate cairo-features.h with only the backends we need
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cairo-features.h
"#ifndef CAIRO_FEATURES_H
#define CAIRO_FEATURES_H

#define CAIRO_HAS_IMAGE_SURFACE      1
#define CAIRO_HAS_PNG_FUNCTIONS      1
#define CAIRO_HAS_FT_FONT           1
#define CAIRO_HAS_FC_FONT           0
#define CAIRO_HAS_SVG_SURFACE        0
#define CAIRO_HAS_PDF_SURFACE        0
#define CAIRO_HAS_PS_SURFACE         0
#define CAIRO_HAS_XLIB_SURFACE       0
#define CAIRO_HAS_XLIB_XRENDER_SURFACE 0
#define CAIRO_HAS_XCB_SURFACE        0
#define CAIRO_HAS_XCB_SHM_FUNCTIONS  0
#define CAIRO_HAS_QUARTZ_SURFACE     0
#define CAIRO_HAS_QUARTZ_FONT        0
#define CAIRO_HAS_QUARTZ_IMAGE_SURFACE 0
#define CAIRO_HAS_WIN32_SURFACE      0
#define CAIRO_HAS_WIN32_FONT         0
#define CAIRO_HAS_DWRITE_FONT        0
#define CAIRO_HAS_SCRIPT_SURFACE     0
#define CAIRO_HAS_TEE_SURFACE        0
#define CAIRO_HAS_RECORDING_SURFACE  1
#define CAIRO_HAS_OBSERVER_SURFACE   1
#define CAIRO_HAS_MIME_SURFACE       1
#define CAIRO_HAS_USER_FONT         1

#endif
")

set(CAIRO_CORE_SOURCES
    src/cairo.c
    src/cairo-analysis-surface.c
    src/cairo-arc.c
    src/cairo-array.c
    src/cairo-atomic.c
    src/cairo-base64-stream.c
    src/cairo-base85-stream.c
    src/cairo-bentley-ottmann.c
    src/cairo-bentley-ottmann-rectangular.c
    src/cairo-bentley-ottmann-rectilinear.c
    src/cairo-botor-scan-converter.c
    src/cairo-boxes.c
    src/cairo-boxes-intersect.c
    src/cairo-cache.c
    src/cairo-cff-subset.c
    src/cairo-clip.c
    src/cairo-clip-boxes.c
    src/cairo-clip-polygon.c
    src/cairo-clip-region.c
    src/cairo-clip-surface.c
    src/cairo-clip-tor-scan-converter.c
    src/cairo-color.c
    src/cairo-composite-rectangles.c
    src/cairo-compositor.c
    src/cairo-contour.c
    src/cairo-damage.c
    src/cairo-debug.c
    src/cairo-default-context.c
    src/cairo-deflate-stream.c
    src/cairo-device.c
    src/cairo-error.c
    src/cairo-fallback-compositor.c
    src/cairo-fixed.c
    src/cairo-font-face.c
    src/cairo-font-face-twin.c
    src/cairo-font-face-twin-data.c
    src/cairo-font-options.c
    src/cairo-freed-pool.c
    src/cairo-freelist.c
    src/cairo-gstate.c
    src/cairo-hash.c
    src/cairo-hull.c
    src/cairo-image-compositor.c
    src/cairo-image-info.c
    src/cairo-image-source.c
    src/cairo-image-surface.c
    src/cairo-line.c
    src/cairo-lzw.c
    src/cairo-mask-compositor.c
    src/cairo-matrix.c
    src/cairo-mempool.c
    src/cairo-mesh-pattern-rasterizer.c
    src/cairo-misc.c
    src/cairo-mono-scan-converter.c
    src/cairo-mutex.c
    src/cairo-no-compositor.c
    src/cairo-observer.c
    src/cairo-output-stream.c
    src/cairo-paginated-surface.c
    src/cairo-path.c
    src/cairo-path-bounds.c
    src/cairo-path-fill.c
    src/cairo-path-fixed.c
    src/cairo-path-in-fill.c
    src/cairo-path-stroke.c
    src/cairo-path-stroke-boxes.c
    src/cairo-path-stroke-polygon.c
    src/cairo-path-stroke-traps.c
    src/cairo-path-stroke-tristrip.c
    src/cairo-pattern.c
    src/cairo-pdf-operators.c
    src/cairo-pdf-shading.c
    src/cairo-pen.c
    src/cairo-polygon.c
    src/cairo-polygon-intersect.c
    src/cairo-polygon-reduce.c
    src/cairo-raster-source-pattern.c
    src/cairo-recording-surface.c
    src/cairo-rectangle.c
    src/cairo-rectangular-scan-converter.c
    src/cairo-region.c
    src/cairo-rtree.c
    src/cairo-scaled-font.c
    src/cairo-scaled-font-subsets.c
    src/cairo-shape-mask-compositor.c
    src/cairo-slope.c
    src/cairo-spans.c
    src/cairo-spans-compositor.c
    src/cairo-spline.c
    src/cairo-stroke-dash.c
    src/cairo-stroke-style.c
    src/cairo-surface.c
    src/cairo-surface-clipper.c
    src/cairo-surface-fallback.c
    src/cairo-surface-observer.c
    src/cairo-surface-offset.c
    src/cairo-surface-snapshot.c
    src/cairo-surface-subsurface.c
    src/cairo-surface-wrapper.c
    src/cairo-tag-attributes.c
    src/cairo-tag-stack.c
    src/cairo-time.c
    src/cairo-tor-scan-converter.c
    src/cairo-tor22-scan-converter.c
    src/cairo-toy-font-face.c
    src/cairo-traps.c
    src/cairo-traps-compositor.c
    src/cairo-tristrip.c
    src/cairo-truetype-subset.c
    src/cairo-type1-fallback.c
    src/cairo-type1-glyph-names.c
    src/cairo-type1-subset.c
    src/cairo-type3-glyph-surface.c
    src/cairo-unicode.c
    src/cairo-user-font.c
    src/cairo-version.c
    src/cairo-wideint.c
)

# Feature sources: PNG + FreeType
set(CAIRO_FEATURE_SOURCES
    src/cairo-png.c
    src/cairo-ft-font.c
    src/cairo-colr-glyph-render.c
    src/cairo-svg-glyph-render.c
)

add_library(cairo STATIC ${CAIRO_CORE_SOURCES} ${CAIRO_FEATURE_SOURCES})

target_include_directories(cairo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(cairo PRIVATE CAIRO_COMPILATION)

if(NOT WIN32)
    target_link_libraries(cairo PRIVATE pthread)
endif()

# Dependencies: pixman, freetype, libpng
if(TARGET pixman)
    target_link_libraries(cairo PUBLIC pixman)
endif()
if(TARGET freetype)
    target_link_libraries(cairo PUBLIC freetype)
    target_include_directories(cairo PRIVATE
        $<TARGET_PROPERTY:freetype,INTERFACE_INCLUDE_DIRECTORIES>
    )
endif()
if(TARGET png_static)
    target_link_libraries(cairo PUBLIC png_static)
elseif(TARGET png)
    target_link_libraries(cairo PUBLIC png)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cairo PRIVATE -w)
endif()
