#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"

#include <string.h>
#include <stdio.h>

#include "mastdisplay.h"
#include "font.h"

#define vga_mode vga_mode_720p_60

#define HEADER_HEIGHT 40
#define LEFT_BORDER_WIDTH 160
#define MIDDLE_BORDER_WIDTH 40
#define TOOT_HEIGHT 200
#define TOOT_SPACING 20
#define TOTAL_TOOT_HEIGHT (TOOT_HEIGHT + TOOT_SPACING)

#define BG_COLOUR 0x20c3

// Make colour from RGB values in range 0-31
static uint16_t make_colour(uint16_t r, uint16_t g, uint16_t b) {
    return (r << PICO_SCANVIDEO_DPI_PIXEL_RSHIFT) | 
           (g << PICO_SCANVIDEO_DPI_PIXEL_GSHIFT) | 
           (b << PICO_SCANVIDEO_DPI_PIXEL_BSHIFT);
}

static uint32_t* blank_line(uint32_t* buf) {
    *buf++ = COMPOSABLE_COLOR_RUN | (BG_COLOUR << 16);
    *buf++ = (vga_mode.width - 3) | (COMPOSABLE_RAW_1P << 16);
    *buf++ = 0 | (COMPOSABLE_EOL_ALIGN << 16);

    return buf;
}

#define FONT (&ubuntu_mono8)
static uint16_t font_colours[16];

static inline __attribute__((always_inline)) uint32_t* add_char(uint32_t* buf, int c, int y) {
    const lv_font_fmt_txt_glyph_dsc_t *g = &FONT->dsc->glyph_dsc[c - 0x20 + 1];
    const uint8_t *b = FONT->dsc->glyph_bitmap + g->bitmap_index;
    int ey = y - FONT_HEIGHT + 3 + g->ofs_y + g->box_h;
    if (ey < 0 || ey >= g->box_h) {
        *buf++ = COMPOSABLE_COLOR_RUN | (BG_COLOUR << 16);
        *buf++ = (2 + FONT_WIDTH_WORDS * 2 - 5) | (COMPOSABLE_RAW_2P << 16);
        *buf++ = BG_COLOUR | (BG_COLOUR << 16);
    }
    else {
        *buf++ = COMPOSABLE_RAW_RUN | (BG_COLOUR << 16);
        *buf++ = (2 + FONT_WIDTH_WORDS * 2 - 3) | (BG_COLOUR << 16);
        int bi = g->box_w * ey;
        for (int x = 0; x < FONT_WIDTH_WORDS * 2; x++) {
            uint32_t pixel;
            int ex = x - g->ofs_x;
            if (ex >= 0 && ex < g->box_w) {
                pixel = bi & 1 ? font_colours[b[bi >> 1] & 0xf] : font_colours[b[bi >> 1] >> 4];
                bi++;
            } else {
                pixel = BG_COLOUR;
            }
            if (!(x & 1)) {
                *buf = pixel;
            } else {
                *buf++ |= pixel << 16;
            }
        }
    }

    return buf;
}

static uint32_t* __not_in_flash_func(toot_line)(uint32_t* buf, MDTOOT* toot, int l) {
    if (l < AVATAR_SIZE) {
        uint16_t* avatar_pixels = &toot->avatar[l*AVATAR_SIZE];
        *buf++ = COMPOSABLE_COLOR_RUN | (BG_COLOUR << 16);
        *buf++ = (LEFT_BORDER_WIDTH - 3) | (COMPOSABLE_RAW_RUN << 16);
        *buf++ = avatar_pixels[0] | ((AVATAR_SIZE-3) << 16);
        memcpy(buf, &avatar_pixels[1], (AVATAR_SIZE-2) * 2);
        buf += (AVATAR_SIZE-2) / 2;
        *buf++ = avatar_pixels[AVATAR_SIZE-1] | (COMPOSABLE_COLOR_RUN << 16);
        if (l < FONT_HEIGHT && toot->acct_name) {
            *buf++ = BG_COLOUR | ((MIDDLE_BORDER_WIDTH - 3) << 16);
            const char* c = toot->acct_name;
            while (*c && *c != '@') {
                buf = add_char(buf, *c++, l);
            }
            int text_len = c - toot->acct_name;

            *buf++ = COMPOSABLE_COLOR_RUN | (BG_COLOUR << 16);
            *buf++ = (vga_mode.width - (LEFT_BORDER_WIDTH + AVATAR_SIZE + MIDDLE_BORDER_WIDTH + text_len * (2 + (FONT_WIDTH_WORDS * 2))) - 3) | (COMPOSABLE_RAW_1P << 16);
            *buf++ = 0 | (COMPOSABLE_EOL_ALIGN << 16);
        }
        else {
            *buf++ = BG_COLOUR | ((vga_mode.width - (LEFT_BORDER_WIDTH + AVATAR_SIZE) - 3) << 16);
            *buf++ = COMPOSABLE_RAW_1P | (0 << 16);
        }
    }
    else {
        *buf++ = COMPOSABLE_COLOR_RUN | (BG_COLOUR << 16);
        *buf++ = (vga_mode.width - 3) | (COMPOSABLE_RAW_1P << 16);
        *buf++ = 0 | (COMPOSABLE_EOL_ALIGN << 16);
    }

    return buf;
}

static void render_scanline(struct scanvideo_scanline_buffer *dest, MDDATA* data) {
    int l = scanvideo_scanline_number(dest->scanline_id);

    uint32_t *buf = dest->data;
    int toot_num = (l - HEADER_HEIGHT) / TOTAL_TOOT_HEIGHT;
    int line_in_toot = l - HEADER_HEIGHT - TOTAL_TOOT_HEIGHT * toot_num;
    if (l < HEADER_HEIGHT || toot_num >= NUM_DISPLAY_TOOTS || line_in_toot >= TOOT_HEIGHT) {
        buf = blank_line(buf);
    }
    else
    {
        buf = toot_line(buf, &data->toot[toot_num], line_in_toot);
    }

    dest->data_used = buf - dest->data;
    dest->status = SCANLINE_OK;
}

static void build_font_colours() {
    for (int i = 0; i < 16; i++) {
        font_colours[i] = (0x0841 * ((i * 3) / 2)) + BG_COLOUR;
    }
}

void render_loop(MDDATA* display_data) {
    build_font_colours();
    scanvideo_setup(&vga_mode);
    scanvideo_timing_enable(true);

    printf("Rendering begin\n");
    while (true) {
        struct scanvideo_scanline_buffer *scanline_buffer = scanvideo_begin_scanline_generation(true);

        render_scanline(scanline_buffer, display_data);
        scanvideo_end_scanline_generation(scanline_buffer);
    }
}

