#ifndef LITEHTMLRENDER_FONT_MANAGER_H
#define LITEHTMLRENDER_FONT_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cairo.h>

struct FontEntry {
    FT_Face ft_face;
    int weight;
    bool italic;
};

class FontManager {
public:
    FontManager() = default;
    ~FontManager();

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    bool init(const std::string& font_dir);

    FT_Face find_face(const std::string& family, int weight = 400, bool italic = false);
    FT_Face find_face_for_char(uint32_t codepoint, int weight = 400, bool italic = false);
    cairo_font_options_t* font_options() { return m_font_options; }
    FT_Library ft_library() { return m_ft_lib; }

    void add_alias(const std::string& alias, const std::string& target);

private:
    void scan_directory(const std::string& dir);
    void load_font_file(const std::string& path);
    std::string resolve_alias(const std::string& family) const;
    static std::string to_lower(const std::string& s);

    FT_Library m_ft_lib = nullptr;
    std::unordered_map<std::string, std::vector<FontEntry>> m_fonts;
    std::unordered_map<std::string, std::string> m_aliases;
    std::unordered_set<std::string> m_loaded_paths; // avoid loading same file twice
    cairo_font_options_t* m_font_options = nullptr;
};

#endif
