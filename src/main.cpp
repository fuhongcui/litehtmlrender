#include "html_renderer.h"
#include <cstdio>

int main(int argc, char* argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <input.html> <output.png> <font_dir>\n", argv[0]);
        return 1;
    }

    RenderOptions opts;
    opts.font_dir = argv[3];

    if (render_html_to_png(argv[1], argv[2], opts)) {
        printf("OK. Saved to: %s\n", argv[2]);
        return 0;
    }

    fprintf(stderr, "ERROR: Rendering failed\n");
    return 1;
}
