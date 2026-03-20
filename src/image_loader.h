#ifndef LITEHTMLRENDER_IMAGE_LOADER_H
#define LITEHTMLRENDER_IMAGE_LOADER_H

#include <string>
#include <cairo.h>

namespace image_loader {

// Load an image file (PNG/JPEG/BMP/GIF) and return a cairo_surface_t*.
// Caller owns the returned surface and must call cairo_surface_destroy().
// Returns nullptr on failure.
cairo_surface_t* load(const std::string& path);

}

#endif
