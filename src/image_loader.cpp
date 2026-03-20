#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "image_loader.h"
#include <cstring>

namespace image_loader {

cairo_surface_t* load(const std::string& path)
{
    int w, h, channels;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4); // force RGBA
    if (!data) {
        return nullptr;
    }

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        stbi_image_free(data);
        cairo_surface_destroy(surface);
        return nullptr;
    }

    cairo_surface_flush(surface);
    unsigned char* dest = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

    // Convert stb RGBA (R,G,B,A bytes) to Cairo ARGB32 (premultiplied, native endian)
    for (int y = 0; y < h; y++) {
        const unsigned char* src_row = data + y * w * 4;
        uint32_t* dest_row = reinterpret_cast<uint32_t*>(dest + y * stride);
        for (int x = 0; x < w; x++) {
            unsigned char r = src_row[x * 4 + 0];
            unsigned char g = src_row[x * 4 + 1];
            unsigned char b = src_row[x * 4 + 2];
            unsigned char a = src_row[x * 4 + 3];

            // Premultiply alpha
            if (a == 255) {
                dest_row[x] = (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
            } else if (a == 0) {
                dest_row[x] = 0;
            } else {
                r = (unsigned char)((uint32_t(r) * a + 127) / 255);
                g = (unsigned char)((uint32_t(g) * a + 127) / 255);
                b = (unsigned char)((uint32_t(b) * a + 127) / 255);
                dest_row[x] = (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
            }
        }
    }

    cairo_surface_mark_dirty(surface);
    stbi_image_free(data);
    return surface;
}

}
