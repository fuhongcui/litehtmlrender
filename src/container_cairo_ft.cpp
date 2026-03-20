#include "container_cairo_ft.h"
#include <cmath>
#include <array>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_TABLES_H

ContainerCairoFT::ContainerCairoFT(FontManager* fm, const std::string& base_path,
                                   int screen_width, int screen_height, double dpi,
                                   const char* default_font)
    : m_fm(fm)
    , m_base_path(base_path)
    , m_default_font(default_font ? default_font : "serif")
    , m_screen_width(screen_width)
    , m_screen_height(screen_height)
    , m_dpi(dpi)
{
}

ContainerCairoFT::~ContainerCairoFT()
{
    clear_images();
}

// ---------------------------------------------------------------------------
// Font creation
// ---------------------------------------------------------------------------

litehtml::uint_ptr ContainerCairoFT::create_font(
    const litehtml::font_description& descr,
    const litehtml::document* doc,
    litehtml::font_metrics* fm)
{
    bool italic = (descr.style == litehtml::font_style_italic);

    litehtml::string_vector tokens;
    litehtml::split_string(descr.family, tokens, ",");

    FT_Face ft_face = nullptr;
    for (auto& name : tokens) {
        litehtml::trim(name, " \t\r\n\f\v\"\'");
        if (name.empty()) continue;
        ft_face = m_fm->find_face(name, descr.weight, italic);
        if (ft_face) break;
    }
    if (!ft_face) {
        ft_face = m_fm->find_face(m_default_font, descr.weight, italic);
    }

    cairo_font_face_t* cr_font = nullptr;
    if (ft_face) {
        cr_font = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
    } else {
        // Fallback to Cairo toy font when no FreeType fonts are loaded
        cairo_font_slant_t slant = italic ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL;
        cairo_font_weight_t weight_enum = (descr.weight >= 600) ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL;
        cr_font = cairo_toy_font_face_create("sans-serif", slant, weight_enum);
    }
    if (!cr_font) return 0;

    cairo_matrix_t font_matrix, ctm;
    cairo_matrix_init_scale(&font_matrix, descr.size, descr.size);
    cairo_matrix_init_identity(&ctm);

    auto* opts = m_fm->font_options();
    cairo_scaled_font_t* scaled = cairo_scaled_font_create(cr_font, &font_matrix, &ctm, opts);

    auto* ret = new cairo_font_ft();
    ret->cr_font = cr_font;
    ret->scaled_font = scaled;
    ret->ft_face = ft_face;
    ret->size = descr.size;
    ret->weight = descr.weight;
    ret->italic = italic;
    ret->strikeout = (descr.decoration_line & litehtml::text_decoration_line_line_through) != 0;
    ret->underline = (descr.decoration_line & litehtml::text_decoration_line_underline) != 0;
    ret->overline  = (descr.decoration_line & litehtml::text_decoration_line_overline) != 0;
    ret->decoration_color = descr.decoration_color;
    ret->decoration_style = descr.decoration_style;

    if (fm) {
        cairo_font_extents_t extents;
        cairo_scaled_font_extents(scaled, &extents);

        fm->font_size   = descr.size;
        fm->ascent      = (int)std::round(extents.ascent);
        fm->descent     = (int)std::round(extents.descent);
        fm->height      = fm->ascent + fm->descent;
        fm->draw_spaces = (descr.decoration_line != litehtml::text_decoration_line_none);
        fm->sub_shift   = descr.size / 5;
        fm->super_shift = descr.size / 3;

        cairo_text_extents_t text_ext;
        cairo_scaled_font_text_extents(scaled, "x", &text_ext);
        fm->x_height = (int)std::round(text_ext.height);
        if (fm->x_height == fm->height)
            fm->x_height = fm->x_height * 4 / 5;

        cairo_scaled_font_text_extents(scaled, "0", &text_ext);
        fm->ch_width = (int)std::round(text_ext.x_advance);

        ret->ascent  = fm->ascent;
        ret->descent = fm->descent;

        // Decoration metrics from FreeType tables
        double scale = descr.size / (ft_face ? ft_face->units_per_EM : 1000.0);
        int line_thickness = std::max(1, (int)std::round(descr.size / 14.0));

        if (ft_face) {
            TT_OS2* os2 = (TT_OS2*)FT_Get_Sfnt_Table(ft_face, FT_SFNT_OS2);

            auto thinkness = descr.decoration_thickness;
            if (!thinkness.is_predefined() && doc) {
                litehtml::css_length one_em(1.0, litehtml::css_units_em);
                doc->cvt_units(one_em, *fm, 0);
                doc->cvt_units(thinkness, *fm, (int)one_em.val());
                line_thickness = std::max(1, (int)std::round(thinkness.val()));
            }

            ret->underline_thickness = line_thickness;
            ret->underline_position  = (int)std::round(-ft_face->underline_position * scale);

            if (os2) {
                ret->strikethrough_thickness = line_thickness;
                ret->strikethrough_position  = (int)std::round(os2->yStrikeoutPosition * scale);
            } else {
                ret->strikethrough_thickness = line_thickness;
                ret->strikethrough_position  = (int)std::round(fm->ascent * 0.35);
            }
        } else {
            ret->underline_thickness     = line_thickness;
            ret->underline_position      = (int)std::round(fm->descent * 0.4);
            ret->strikethrough_thickness = line_thickness;
            ret->strikethrough_position  = (int)std::round(fm->ascent * 0.35);
        }
        ret->overline_thickness = ret->underline_thickness;
        ret->overline_position  = fm->ascent;
    }

    return (litehtml::uint_ptr)ret;
}

void ContainerCairoFT::delete_font(litehtml::uint_ptr hFont)
{
    auto* fnt = (cairo_font_ft*)hFont;
    if (fnt) {
        cairo_scaled_font_destroy(fnt->scaled_font);
        cairo_font_face_destroy(fnt->cr_font);
        delete fnt;
    }
}

// ---------------------------------------------------------------------------
// HarfBuzz text shaping
// ---------------------------------------------------------------------------

static int utf8_decode(const char* s, uint32_t* out)
{
    auto u = (const unsigned char*)s;
    if (u[0] < 0x80) { *out = u[0]; return 1; }

    int len;
    uint32_t cp;
    if      ((u[0] & 0xE0) == 0xC0) { len = 2; cp = u[0] & 0x1F; }
    else if ((u[0] & 0xF0) == 0xE0) { len = 3; cp = u[0] & 0x0F; }
    else if ((u[0] & 0xF8) == 0xF0) { len = 4; cp = u[0] & 0x07; }
    else { *out = 0xFFFD; return 1; }

    for (int i = 1; i < len; i++) {
        if (u[i] == 0 || (u[i] & 0xC0) != 0x80) {
            *out = 0xFFFD;
            return i > 0 ? i : 1;
        }
        cp = (cp << 6) | (u[i] & 0x3F);
    }
    *out = cp;
    return len;
}

// Shape a single run of text using HarfBuzz with a specific FT_Face
static ShapedRun shape_one_run(FT_Face face, double font_size,
                               const char* text, int len, double x, double y)
{
    ShapedRun run;
    run.face = face;
    run.advance = 0;

    // Set the char size on the FreeType face before HarfBuzz uses it
    FT_Set_Char_Size(face, 0, (FT_F26Dot6)(font_size * 64), 0, 0);

    hb_font_t* hb_font = hb_ft_font_create_referenced(face);

    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, text, len, 0, len);
    hb_buffer_guess_segment_properties(buf);
    hb_shape(hb_font, buf, nullptr, 0);

    unsigned int glyph_count;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    double cursor_x = x;
    run.glyphs.resize(glyph_count);

    for (unsigned int i = 0; i < glyph_count; i++) {
        run.glyphs[i].index = glyph_info[i].codepoint;
        run.glyphs[i].x = cursor_x + glyph_pos[i].x_offset / 64.0;
        run.glyphs[i].y = y - glyph_pos[i].y_offset / 64.0;
        double advance = glyph_pos[i].x_advance / 64.0;
        cursor_x += advance;
        run.advance += advance;
    }

    hb_buffer_destroy(buf);
    hb_font_destroy(hb_font);
    return run;
}

// Resolve the FT_Face for a single codepoint: primary font if it has the glyph,
// otherwise search all loaded fonts for a fallback.
static FT_Face resolve_face_for_char(FT_Face primary, FontManager* fm,
                                     uint32_t codepoint, int weight, bool italic)
{
    if (codepoint < 0x20) return primary;
    if (FT_Get_Char_Index(primary, codepoint) != 0) return primary;
    FT_Face fallback = fm->find_face_for_char(codepoint, weight, italic);
    return fallback ? fallback : primary;
}

double ContainerCairoFT::shape_text_runs(const char* text, cairo_font_ft* fnt,
                                         std::vector<ShapedRun>& runs, double x, double y)
{
    runs.clear();

    if (!fnt->ft_face) {
        cairo_glyph_t* c_glyphs = nullptr;
        int num_glyphs = 0;
        cairo_status_t status = cairo_scaled_font_text_to_glyphs(
            fnt->scaled_font, x, y, text, -1, &c_glyphs, &num_glyphs, nullptr, nullptr, nullptr);
        if (status == CAIRO_STATUS_SUCCESS && c_glyphs && num_glyphs > 0) {
            ShapedRun run;
            run.face = nullptr;
            run.glyphs.assign(c_glyphs, c_glyphs + num_glyphs);
            run.advance = 0;
            if (num_glyphs > 0) {
                run.advance = c_glyphs[num_glyphs - 1].x - x;
                cairo_text_extents_t ext;
                cairo_scaled_font_glyph_extents(fnt->scaled_font, &c_glyphs[num_glyphs - 1], 1, &ext);
                run.advance += ext.x_advance;
            }
            cairo_glyph_free(c_glyphs);
            runs.push_back(std::move(run));
            return runs[0].advance;
        }
        if (c_glyphs) cairo_glyph_free(c_glyphs);
        return 0;
    }

    FT_Face primary = fnt->ft_face;
    int weight = fnt->weight;
    bool italic = fnt->italic;
    double cursor_x = x;
    double total_advance = 0;
    const char* p = text;

    while (*p) {
        const char* run_start = p;

        // Resolve the face for the first character of the run
        uint32_t first_cp;
        int first_len = utf8_decode(p, &first_cp);
        if (first_len == 0) break;

        FT_Face run_face = resolve_face_for_char(primary, m_fm, first_cp, weight, italic);
        p += first_len;

        // Extend the run while consecutive characters resolve to the same FT_Face
        while (*p) {
            uint32_t cp;
            int len = utf8_decode(p, &cp);
            if (len == 0) break;

            FT_Face char_face = resolve_face_for_char(primary, m_fm, cp, weight, italic);
            if (char_face != run_face) break;
            p += len;
        }

        int run_len = (int)(p - run_start);
        if (run_len == 0) break;

        auto run = shape_one_run(run_face, fnt->size, run_start, run_len, cursor_x, y);
        cursor_x += run.advance;
        total_advance += run.advance;
        runs.push_back(std::move(run));
    }

    return total_advance;
}

// ---------------------------------------------------------------------------
// Text measurement
// ---------------------------------------------------------------------------

litehtml::pixel_t ContainerCairoFT::text_width(const char* text, litehtml::uint_ptr hFont)
{
    auto* fnt = (cairo_font_ft*)hFont;
    std::vector<ShapedRun> runs;
    double w = shape_text_runs(text, fnt, runs, 0, 0);
    return (int)std::round(w);
}

// ---------------------------------------------------------------------------
// Decoration line drawing (ported from container_cairo_pango.cpp)
// ---------------------------------------------------------------------------

namespace {

enum class draw_type { DRAW_OVERLINE, DRAW_STRIKETHROUGH, DRAW_UNDERLINE };

inline void draw_single_line(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                             litehtml::pixel_t width, int thickness, draw_type type)
{
    double top;
    switch (type) {
    case draw_type::DRAW_UNDERLINE:     top = y + (double)thickness / 2.0; break;
    case draw_type::DRAW_OVERLINE:      top = y - (double)thickness / 2.0; break;
    case draw_type::DRAW_STRIKETHROUGH: top = y + 0.5; break;
    default: top = y; break;
    }
    cairo_move_to(cr, x, top);
    cairo_line_to(cr, x + width, top);
}

void draw_solid_line(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                     litehtml::pixel_t width, int thickness, draw_type type, litehtml::web_color& color)
{
    draw_single_line(cr, x, y, width, thickness, type);
    cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0);
    cairo_set_line_width(cr, thickness);
    cairo_stroke(cr);
}

void draw_dotted_line(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                      litehtml::pixel_t width, int thickness, draw_type type, litehtml::web_color& color)
{
    draw_single_line(cr, x, y, width, thickness, type);
    std::array<double, 2> dashes{0, thickness * 2.0};
    if (thickness == 1) dashes[1] += thickness / 2.0;
    cairo_set_line_width(cr, thickness);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_dash(cr, dashes.data(), 2, x);
    cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0);
    cairo_stroke(cr);
}

void draw_dashed_line(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                      litehtml::pixel_t width, int thickness, draw_type type, litehtml::web_color& color)
{
    draw_single_line(cr, x, y, width, thickness, type);
    std::array<double, 2> dashes{thickness * 2.0, thickness * 3.0};
    cairo_set_line_width(cr, thickness);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_dash(cr, dashes.data(), 2, x);
    cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0);
    cairo_stroke(cr);
}

void draw_wavy_line(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                    litehtml::pixel_t width, int thickness, draw_type type, litehtml::web_color& color)
{
    int h_pad = 1;
    int brush_height = thickness * 3 + h_pad * 2;
    int brush_width = brush_height * 2 - 2 * thickness;

    double top;
    switch (type) {
    case draw_type::DRAW_UNDERLINE: top = y + (double)brush_height / 2.0; break;
    case draw_type::DRAW_OVERLINE:  top = y - (double)brush_height / 2.0; break;
    default: top = y; break;
    }

    cairo_surface_t* brush_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, brush_width, brush_height);
    cairo_t* brush_cr = cairo_create(brush_surface);
    cairo_set_source_rgba(brush_cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0);
    cairo_set_line_width(brush_cr, thickness);
    double w = thickness / 2.0;
    cairo_move_to(brush_cr, 0, brush_height - (double)thickness / 2.0 - h_pad);
    cairo_line_to(brush_cr, w, brush_height - (double)thickness / 2.0 - h_pad);
    cairo_line_to(brush_cr, brush_width / 2.0 - w, (double)thickness / 2.0 + h_pad);
    cairo_line_to(brush_cr, brush_width / 2.0 + w, (double)thickness / 2.0 + h_pad);
    cairo_line_to(brush_cr, brush_width - w, brush_height - (double)thickness / 2.0 - h_pad);
    cairo_line_to(brush_cr, brush_width, brush_height - (double)thickness / 2.0 - h_pad);
    cairo_stroke(brush_cr);
    cairo_destroy(brush_cr);

    cairo_pattern_t* pattern = cairo_pattern_create_for_surface(brush_surface);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
    cairo_matrix_t pattern_matrix;
    cairo_matrix_init_translate(&pattern_matrix, 0, -top + brush_height / 2.0);
    cairo_pattern_set_matrix(pattern, &pattern_matrix);
    cairo_set_source(cr, pattern);
    cairo_set_line_width(cr, brush_height);
    cairo_move_to(cr, x, top);
    cairo_line_to(cr, x + width, top);
    cairo_stroke(cr);
    cairo_pattern_destroy(pattern);
    cairo_surface_destroy(brush_surface);
}

void draw_double_line(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                      litehtml::pixel_t width, int thickness, draw_type type, litehtml::web_color& color)
{
    cairo_set_line_width(cr, thickness);
    double top1, top2;
    switch (type) {
    case draw_type::DRAW_UNDERLINE:
        top1 = y + (double)thickness / 2.0;
        top2 = top1 + (double)thickness + (double)thickness / 2.0 + 0.5;
        break;
    case draw_type::DRAW_OVERLINE:
        top1 = y - (double)thickness / 2.0;
        top2 = top1 - (double)thickness - (double)thickness / 2.0 - 0.5;
        break;
    case draw_type::DRAW_STRIKETHROUGH:
        top1 = y - (double)thickness + 0.5;
        top2 = y + (double)thickness + 0.5;
        break;
    default:
        top1 = y;
        top2 = y;
        break;
    }
    cairo_move_to(cr, x, top1);
    cairo_line_to(cr, x + width, top1);
    cairo_stroke(cr);
    cairo_move_to(cr, x, top2);
    cairo_line_to(cr, x + width, top2);
    cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0);
    cairo_stroke(cr);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Text drawing
// ---------------------------------------------------------------------------

void ContainerCairoFT::draw_text(litehtml::uint_ptr hdc, const char* text,
                                 litehtml::uint_ptr hFont, litehtml::web_color color,
                                 const litehtml::position& pos)
{
    auto* fnt = (cairo_font_ft*)hFont;
    auto* cr = (cairo_t*)hdc;
    cairo_save(cr);
    apply_clip(cr);

    set_color(cr, color);

    double baseline_y = pos.top() + fnt->ascent;
    std::vector<ShapedRun> runs;
    shape_text_runs(text, fnt, runs, pos.left(), baseline_y);

    // Draw each run with its own font face
    for (auto& run : runs) {
        if (run.glyphs.empty()) continue;

        if (run.face && run.face != fnt->ft_face) {
            // Fallback font: create a temporary scaled font
            cairo_font_face_t* fb_font = cairo_ft_font_face_create_for_ft_face(run.face, 0);
            cairo_matrix_t font_matrix, ctm;
            cairo_matrix_init_scale(&font_matrix, fnt->size, fnt->size);
            cairo_matrix_init_identity(&ctm);
            cairo_scaled_font_t* fb_scaled = cairo_scaled_font_create(fb_font, &font_matrix, &ctm, m_fm->font_options());

            cairo_set_scaled_font(cr, fb_scaled);
            set_color(cr, color);
            cairo_show_glyphs(cr, run.glyphs.data(), (int)run.glyphs.size());

            cairo_scaled_font_destroy(fb_scaled);
            cairo_font_face_destroy(fb_font);
        } else {
            // Primary font or toy font
            cairo_set_scaled_font(cr, fnt->scaled_font);
            set_color(cr, color);
            cairo_show_glyphs(cr, run.glyphs.data(), (int)run.glyphs.size());
        }
    }

    litehtml::web_color decoration_color = color;
    if (!fnt->decoration_color.is_current_color) {
        decoration_color = fnt->decoration_color;
    }

    litehtml::pixel_t tw = 0;
    if (fnt->underline || fnt->strikeout || fnt->overline) {
        tw = text_width(text, hFont);
    }

    litehtml::pixel_t text_baseline = pos.height - (int)fnt->descent;

    using draw_func_t = decltype(&draw_solid_line);
    std::array<draw_func_t, litehtml::text_decoration_style_max> draw_funcs{
        draw_solid_line,
        draw_double_line,
        draw_dotted_line,
        draw_dashed_line,
        draw_wavy_line,
    };

    if (fnt->underline) {
        draw_funcs[fnt->decoration_style](cr, pos.left(), pos.top() + text_baseline + fnt->underline_position,
                                          tw, fnt->underline_thickness, draw_type::DRAW_UNDERLINE, decoration_color);
    }
    if (fnt->strikeout) {
        draw_funcs[fnt->decoration_style](cr, pos.left(), pos.top() + text_baseline - fnt->strikethrough_position,
                                          tw, fnt->strikethrough_thickness, draw_type::DRAW_STRIKETHROUGH, decoration_color);
    }
    if (fnt->overline) {
        draw_funcs[fnt->decoration_style](cr, pos.left(), pos.top() + text_baseline - fnt->overline_position,
                                          tw, fnt->overline_thickness, draw_type::DRAW_OVERLINE, decoration_color);
    }

    cairo_restore(cr);
}

// ---------------------------------------------------------------------------
// Image loading (replacing GdkPixbuf)
// ---------------------------------------------------------------------------

cairo_surface_t* ContainerCairoFT::get_image(const std::string& _url)
{
    if (_url.empty()) return nullptr;

    // URL decode
    std::string url;
    for (size_t i = 0; i < _url.length(); i++) {
        if (_url[i] == '%' && i + 2 < _url.length()) {
            uint32_t val;
            if (sscanf(_url.substr(i + 1, 2).c_str(), "%x", &val) == 1) {
                url += static_cast<char>(val);
                i += 2;
                continue;
            }
        }
        url += _url[i];
    }

    cairo_surface_t* surface = m_images.get_image(url);
    if (!surface) {
        surface = image_loader::load(url);
        if (surface) {
            m_images.add_image(url, surface);
            surface = cairo_surface_reference(surface);
        }
    }
    return surface;
}

void ContainerCairoFT::load_image(const char*, const char*, bool) {}

void ContainerCairoFT::set_caption(const char*) {}
void ContainerCairoFT::on_anchor_click(const char*, const litehtml::element::ptr&) {}
void ContainerCairoFT::on_mouse_event(const litehtml::element::ptr&, litehtml::mouse_event) {}
void ContainerCairoFT::set_cursor(const char*) {}

void ContainerCairoFT::set_base_url(const char* base_url)
{
    if (base_url) {
        m_base_path = base_url;
    }
}

void ContainerCairoFT::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl)
{
    std::string path;
    make_url(url.c_str(), baseurl.c_str(), path);

    std::stringstream ss;
    std::ifstream ifs(path, std::ios::binary);
    if (ifs.is_open()) {
        ss << ifs.rdbuf();
        text = ss.str();
    }
}

void ContainerCairoFT::get_viewport(litehtml::position& viewport) const
{
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = m_screen_width;
    viewport.height = m_screen_height;
}

const char* ContainerCairoFT::get_default_font_name() const
{
    return m_default_font.c_str();
}

void ContainerCairoFT::make_url(const char* url, const char* basepath, litehtml::string& out)
{
    if (basepath && *basepath) {
        out = (std::filesystem::path(basepath) / url).string();
    } else if (!m_base_path.empty()) {
        out = (std::filesystem::path(m_base_path) / url).string();
    } else {
        out = url;
    }
}
