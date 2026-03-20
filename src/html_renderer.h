#ifndef LITEHTMLRENDER_HTML_RENDERER_H
#define LITEHTMLRENDER_HTML_RENDERER_H

#include <string>

struct RenderOptions {
    int width       = 800;
    int height      = 0;    // 0 = auto (use document height)
    double dpi      = 96;
    std::string font_dir;
    std::string default_font = "serif";
};

// Render an HTML file to a PNG image.
// Returns true on success, false on failure.
bool render_html_to_png(const std::string& html_file,
                        const std::string& png_file,
                        const RenderOptions& opts = {});

// Render an HTML string to a PNG image.
// base_path is used to resolve relative URLs in the HTML.
bool render_html_string_to_png(const std::string& html_content,
                               const std::string& png_file,
                               const std::string& base_path = "",
                               const RenderOptions& opts = {});

#endif
