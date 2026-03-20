#include "font_manager.h"
#include <filesystem>
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include FT_TRUETYPE_TABLES_H

namespace fs = std::filesystem;

FontManager::~FontManager()
{
    if (m_font_options) {
        cairo_font_options_destroy(m_font_options);
    }
    for (auto& [name, entries] : m_fonts) {
        for (auto& entry : entries) {
            FT_Done_Face(entry.ft_face);
        }
    }
    if (m_ft_lib) {
        FT_Done_FreeType(m_ft_lib);
    }
}

std::string FontManager::to_lower(const std::string& s)
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool FontManager::init(const std::string& font_dir)
{
    if (FT_Init_FreeType(&m_ft_lib) != 0) {
        fprintf(stderr, "ERROR: FT_Init_FreeType failed\n");
        return false;
    }

    m_aliases["serif"]      = "noto serif";
    m_aliases["sans-serif"] = "noto sans";
    m_aliases["fantasy"]    = "noto sans";
    m_aliases["cursive"]    = "noto sans";
    m_aliases["monospace"]  = "noto sans mono";

    if (!font_dir.empty()) {
        scan_directory(font_dir);
    }

    m_font_options = cairo_font_options_create();
    cairo_font_options_set_antialias(m_font_options, CAIRO_ANTIALIAS_GRAY);
    cairo_font_options_set_hint_style(m_font_options, CAIRO_HINT_STYLE_NONE);
    cairo_font_options_set_subpixel_order(m_font_options, CAIRO_SUBPIXEL_ORDER_RGB);

    return true;
}

void FontManager::scan_directory(const std::string& dir)
{
    if (dir.empty() || !fs::exists(dir)) return;

    std::error_code ec;
    for (auto const& entry : fs::recursive_directory_iterator(dir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        auto ext = to_lower(entry.path().extension().string());
        if (ext == ".ttf" || ext == ".otf" || ext == ".ttc" || ext == ".otc") {
            std::string canonical = fs::canonical(entry.path(), ec).string();
            if (ec) canonical = entry.path().string();
            if (m_loaded_paths.count(canonical)) continue;
            m_loaded_paths.insert(canonical);
            load_font_file(entry.path().string());
        }
    }
}

void FontManager::load_font_file(const std::string& path)
{
    FT_Face probe;
    if (FT_New_Face(m_ft_lib, path.c_str(), -1, &probe) != 0) {
        if (FT_New_Face(m_ft_lib, path.c_str(), 0, &probe) != 0) {
            fprintf(stderr, "WARNING: Failed to load font: %s\n", path.c_str());
            return;
        }
        long num_faces = probe->num_faces;
        std::string family = to_lower(probe->family_name ? probe->family_name : "unknown");
        FontEntry entry;
        entry.ft_face = probe;
        entry.weight = (probe->style_flags & FT_STYLE_FLAG_BOLD) ? 700 : 400;
        entry.italic = (probe->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
        m_fonts[family].push_back(entry);

        for (long i = 1; i < num_faces; i++) {
            FT_Face face;
            if (FT_New_Face(m_ft_lib, path.c_str(), i, &face) != 0) continue;
            family = to_lower(face->family_name ? face->family_name : "unknown");
            FontEntry e;
            e.ft_face = face;
            e.weight = (face->style_flags & FT_STYLE_FLAG_BOLD) ? 700 : 400;
            e.italic = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
            m_fonts[family].push_back(e);
        }
        return;
    }

    long num_faces = probe->num_faces;
    FT_Done_Face(probe);

    for (long i = 0; i < num_faces; i++) {
        FT_Face face;
        if (FT_New_Face(m_ft_lib, path.c_str(), i, &face) != 0) continue;

        std::string family = to_lower(face->family_name ? face->family_name : "unknown");

        FontEntry entry;
        entry.ft_face = face;
        entry.weight = (face->style_flags & FT_STYLE_FLAG_BOLD) ? 700 : 400;
        entry.italic = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;

        m_fonts[family].push_back(entry);
    }
}

void FontManager::add_alias(const std::string& alias, const std::string& target)
{
    m_aliases[to_lower(alias)] = to_lower(target);
}

std::string FontManager::resolve_alias(const std::string& family) const
{
    std::string key = to_lower(family);
    auto it = m_aliases.find(key);
    if (it != m_aliases.end()) {
        return it->second;
    }
    return key;
}

FT_Face FontManager::find_face(const std::string& family, int weight, bool italic)
{
    std::string resolved = resolve_alias(family);

    auto it = m_fonts.find(resolved);
    if (it == m_fonts.end() || it->second.empty()) {
        if (!m_fonts.empty()) {
            return m_fonts.begin()->second[0].ft_face;
        }
        return nullptr;
    }

    const FontEntry* best = &it->second[0];
    int best_score = INT_MAX;

    for (auto& entry : it->second) {
        int score = std::abs(entry.weight - weight) + (entry.italic != italic ? 1000 : 0);
        if (score < best_score) {
            best_score = score;
            best = &entry;
        }
    }

    return best->ft_face;
}

FT_Face FontManager::find_face_for_char(uint32_t codepoint, int weight, bool italic)
{
    const FontEntry* best = nullptr;
    int best_score = INT_MAX;

    for (auto& [name, entries] : m_fonts) {
        for (auto& entry : entries) {
            if (FT_Get_Char_Index(entry.ft_face, codepoint) != 0) {
                int score = std::abs(entry.weight - weight) + (entry.italic != italic ? 1000 : 0);
                if (score < best_score) {
                    best_score = score;
                    best = &entry;
                }
            }
        }
    }

    return best ? best->ft_face : nullptr;
}
