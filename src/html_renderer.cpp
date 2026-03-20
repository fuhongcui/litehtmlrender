#include "html_renderer.h"
#include "font_manager.h"
#include "container_cairo_ft.h"
#include <litehtml.h>
#include <cairo.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <map>
#include <algorithm>
#include <cstdio>

namespace fs = std::filesystem;

// .cfg file parser (same as reference render2png.cpp)
class html_config
{
    std::map<std::string, std::string> m_data;
public:
    explicit html_config(const std::string& html_file)
    {
        fs::path cfg_path = html_file + ".cfg";
        if (fs::exists(cfg_path)) {
            std::ifstream infile(cfg_path);
            if (infile.is_open()) {
                for (std::string line; std::getline(infile, line);) {
                    auto parts = litehtml::split_string(line, ":");
                    if (parts.size() == 2) {
                        m_data.emplace(
                            litehtml::trim(parts[0], litehtml::split_delims_spaces),
                            litehtml::trim(parts[1], litehtml::split_delims_spaces)
                        );
                    }
                }
            }
        }
    }

    int get_int(const std::string& key, int default_value)
    {
        auto iter = m_data.find(key);
        if (iter != m_data.end()) {
            try { return std::stoi(iter->second); }
            catch (...) { return default_value; }
        }
        return default_value;
    }

    bool get_bool(const std::string& key, bool default_value)
    {
        auto iter = m_data.find(key);
        if (iter != m_data.end()) {
            if (iter->second == "true") return true;
            if (iter->second == "false") return false;
        }
        return default_value;
    }
};

static bool do_render(const std::string& html_content,
                      const std::string& base_path,
                      const std::string& png_file,
                      const std::string& html_file_path,
                      const RenderOptions& opts)
{
    FontManager fm;
    if (!fm.init(opts.font_dir)) {
        fprintf(stderr, "ERROR: Failed to initialize FontManager\n");
        return false;
    }

    int screen_width = opts.width;
    int screen_height = opts.height > 0 ? opts.height : 600;

    ContainerCairoFT container(&fm, base_path, screen_width, screen_height, opts.dpi,
                               opts.default_font.c_str());

    auto doc = litehtml::document::createFromString(html_content, &container);
    if (!doc) {
        fprintf(stderr, "ERROR: Failed to parse HTML\n");
        return false;
    }

    // Load .cfg file if html_file_path is provided
    html_config cfg(html_file_path);

    int best_width = doc->render(screen_width);

    if (best_width > 0 && cfg.get_bool("bestfit", true)) {
        best_width = cfg.get_int("width", best_width);
        std::swap(screen_width, best_width);
        doc->render(screen_width);
        std::swap(screen_width, best_width);
    }

    int width  = cfg.get_int("width",  doc->width()  > 0 ? doc->width()  : 1);
    int height = cfg.get_int("height", doc->height() > 0 ? doc->height() : 1);

    auto surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to create Cairo surface\n");
        return false;
    }

    auto cr = cairo_create(surface);

    cairo_save(cr);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_fill(cr);
    cairo_restore(cr);

    litehtml::position clip(0, 0, width, height);
    doc->draw((litehtml::uint_ptr)cr, 0, 0, &clip);

    cairo_surface_flush(surface);
    cairo_destroy(cr);

    cairo_status_t status = cairo_surface_write_to_png(surface, png_file.c_str());
    cairo_surface_destroy(surface);

    if (status != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to write PNG: %s\n", cairo_status_to_string(status));
        return false;
    }

    return true;
}

bool render_html_to_png(const std::string& html_file,
                        const std::string& png_file,
                        const RenderOptions& opts)
{
    std::ifstream ifs(html_file, std::ios::binary);
    if (!ifs.is_open()) {
        fprintf(stderr, "ERROR: Cannot open file: %s\n", html_file.c_str());
        return false;
    }

    std::stringstream ss;
    ss << ifs.rdbuf();

    std::string base_path = fs::path(html_file).parent_path().string();
    return do_render(ss.str(), base_path, png_file, html_file, opts);
}

bool render_html_string_to_png(const std::string& html_content,
                               const std::string& png_file,
                               const std::string& base_path,
                               const RenderOptions& opts)
{
    return do_render(html_content, base_path, png_file, "", opts);
}
