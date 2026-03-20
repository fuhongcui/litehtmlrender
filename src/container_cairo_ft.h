#ifndef LITEHTMLRENDER_CONTAINER_CAIRO_FT_H
#define LITEHTMLRENDER_CONTAINER_CAIRO_FT_H

#include <litehtml.h>
#include "container_cairo.h"
#include "cairo_images_cache.h"
#include "font_manager.h"
#include "image_loader.h"
#include <cairo.h>
#include <cairo-ft.h>
#include <hb.h>
#include <hb-ft.h>
#include <string>

struct cairo_font_ft
{
    cairo_font_face_t*      cr_font;
    cairo_scaled_font_t*    scaled_font;
    FT_Face                 ft_face;
    double                  size;
    int                     weight;
    bool                    italic;
    bool                    underline;
    bool                    strikeout;
    bool                    overline;
    double                  ascent;
    double                  descent;
    int                     underline_thickness;
    int                     underline_position;
    int                     strikethrough_thickness;
    int                     strikethrough_position;
    int                     overline_thickness;
    int                     overline_position;
    int                     decoration_style;
    litehtml::web_color     decoration_color;
};

// A shaped run of glyphs from a single font face
struct ShapedRun {
    FT_Face                     face;
    std::vector<cairo_glyph_t>  glyphs;
    double                      advance;
};

class ContainerCairoFT : public container_cairo
{
public:
    ContainerCairoFT(FontManager* fm, const std::string& base_path,
                     int screen_width, int screen_height, double dpi,
                     const char* default_font = "serif");
    ~ContainerCairoFT() override;

    // --- Text/font (replacing Pango) ---
    litehtml::uint_ptr create_font(const litehtml::font_description& descr, const litehtml::document* doc, litehtml::font_metrics* fm) override;
    void delete_font(litehtml::uint_ptr hFont) override;
    litehtml::pixel_t text_width(const char* text, litehtml::uint_ptr hFont) override;
    void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) override;

    // --- container_cairo pure virtuals ---
    cairo_surface_t* get_image(const std::string& url) override;
    double get_screen_dpi() const override { return m_dpi; }
    int get_screen_width() const override { return m_screen_width; }
    int get_screen_height() const override { return m_screen_height; }

    // --- document_container callbacks ---
    void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
    void set_caption(const char* caption) override;
    void set_base_url(const char* base_url) override;
    void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
    void on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event) override;
    void set_cursor(const char* cursor) override;
    void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override;
    void get_viewport(litehtml::position& viewport) const override;
    const char* get_default_font_name() const override;
    void make_url(const char* url, const char* basepath, litehtml::string& out) override;

private:
    // Shape text into runs, splitting by font coverage for fallback
    double shape_text_runs(const char* text, cairo_font_ft* fnt,
                           std::vector<ShapedRun>& runs, double x, double y);

    FontManager*        m_fm;
    std::string         m_base_path;
    std::string         m_default_font;
    int                 m_screen_width;
    int                 m_screen_height;
    double              m_dpi;
    cairo_images_cache  m_images;
};

#endif
