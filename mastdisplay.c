#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"

#include <string.h>
#include <stdio.h>

#include "mastdisplay.h"
#include "font.h"

// 720p, reduced blanking - doesn't work correctly on my monitor
#if 0
const scanvideo_timing_t vga_timing_1280x720_60_rb =
        {
                .clock_freq = 64000000,

                .h_active = 1280,
                .v_active = 720,

                .h_front_porch = 48,
                .h_pulse = 32,
                .h_total = 1440,
                .h_sync_polarity = 1,

                .v_front_porch = 3,
                .v_pulse = 5,
                .v_total = 741,
                .v_sync_polarity = 0,

                .enable_clock = 0,
                .clock_polarity = 0,

                .enable_den = 0
        };


const scanvideo_mode_t vga_mode =
        {
                .default_timing = &vga_timing_1280x720_60_rb,
                .pio_program = &video_24mhz_composable,
                .width = 1280,
                .height = 720,
                .xscale = 1,
                .yscale = 1,
        };
#else
#define vga_mode vga_mode_720p_60
#endif

#define DISPLAY_WIDTH 1280
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

static uint32_t sv_blank_line[] = {
    COMPOSABLE_COLOR_RUN | (BG_COLOUR << 16),
    (DISPLAY_WIDTH - 3) | (COMPOSABLE_RAW_1P << 16),
    0 | (COMPOSABLE_EOL_ALIGN << 16)
};

static uint32_t* blank_line(uint32_t* buf) {
    *buf++ = count_of(sv_blank_line);
    *buf++ = (uintptr_t)sv_blank_line;

    *buf++ = 0;
    *buf++ = 0;

    return buf;
}

#define FONT (&ubuntu_mono8)
static __scratch_x("font") uint32_t font_colours[16];

static inline __attribute__((always_inline)) uint32_t* add_char(uint32_t* buf, int c, int y) {
    const lv_font_fmt_txt_glyph_dsc_t* g = &FONT->dsc->glyph_dsc[c - 0x20 + 1];
    const uint8_t *b = FONT->dsc->glyph_bitmap + g->bitmap_index;
    const int ey = y - FONT_HEIGHT + 4 + g->ofs_y + g->box_h;
    if (ey < 0 || ey >= g->box_h || g->box_w == 0) {
        for (int i = 0; i < 5; ++i) {
            *buf++ = BG_COLOUR | (BG_COLOUR << 16);
        }
    }
    else {
        int bi = (g->box_w * ey);

        uint32_t bits = (b[bi >> 2] << 16) | (b[(bi >> 2) + 1] << 8) | b[(bi >> 2) + 2];
        bits >>= 8 - ((bi & 3) << 1);
        bits &= 0xffff & (0xffff << ((8 - g->box_w) << 1));
        bits >>= g->ofs_x << 1;

        *buf++ = font_colours[bits >> 12];
        *buf++ = font_colours[(bits >> 8) & 0xf];
        *buf++ = font_colours[(bits >> 4) & 0xf];
        *buf++ = font_colours[bits & 0xf];
        *buf++ = BG_COLOUR | (BG_COLOUR << 16);
    }

    return buf;
}

static uint32_t sv_left_border[] = {
    COMPOSABLE_COLOR_RUN | (BG_COLOUR << 16),
    (LEFT_BORDER_WIDTH - 5) | (COMPOSABLE_RAW_2P << 16),
    BG_COLOUR | (BG_COLOUR << 16)
};

static uint32_t sv_left_border_avatar[] = {
    COMPOSABLE_COLOR_RUN | (BG_COLOUR << 16),
    (LEFT_BORDER_WIDTH - 4) | (COMPOSABLE_RAW_RUN << 16),
    BG_COLOUR | ((AVATAR_SIZE - 2) << 16)
};

static uint32_t sv_blank_after_avatar[] = {
    COMPOSABLE_COLOR_RUN | (BG_COLOUR << 16),
    (DISPLAY_WIDTH - (LEFT_BORDER_WIDTH + AVATAR_SIZE) - 3) | (COMPOSABLE_RAW_1P << 16),
    0 | (COMPOSABLE_EOL_ALIGN << 16)
};

#define TEXT_BLOCK_WIDTH 400
#define TEXT_LEN (TEXT_BLOCK_WIDTH / 10)

static uint32_t sv_text_block[] = {
    COMPOSABLE_COLOR_RUN | (BG_COLOUR << 16),
    (MIDDLE_BORDER_WIDTH - 4) | (COMPOSABLE_RAW_RUN << 16),
    BG_COLOUR | ((TEXT_BLOCK_WIDTH - 2) << 16)
};

static uint32_t sv_blank_after_text[] = {
    COMPOSABLE_COLOR_RUN | (BG_COLOUR << 16),
    (DISPLAY_WIDTH - (LEFT_BORDER_WIDTH + AVATAR_SIZE + MIDDLE_BORDER_WIDTH + TEXT_BLOCK_WIDTH) - 3) | (COMPOSABLE_RAW_1P << 16),
    0 | (COMPOSABLE_EOL_ALIGN << 16)
};

static uint32_t* __not_in_flash_func(toot_line)(uint32_t* buf, MDTOOT* toot, int l) {
    uint32_t* data_ptr = buf + 12;

    if (l < AVATAR_SIZE) {
        *buf++ = count_of(sv_left_border_avatar);
        *buf++ = (uintptr_t)sv_left_border_avatar;
        *buf++ = AVATAR_SIZE / 2;
        *buf++ = (uintptr_t)&toot->avatar[l*AVATAR_SIZE];
        if (l < FONT_HEIGHT && toot->acct_name) {
            *buf++ = count_of(sv_text_block);
            *buf++ = (uintptr_t)sv_text_block;
            *buf++ = TEXT_BLOCK_WIDTH / 2;
            *buf++ = (uintptr_t)data_ptr;
            *buf++ = count_of(sv_blank_after_text);
            *buf++ = (uintptr_t)sv_blank_after_text;

            const char* c = toot->acct_name;
            int i = 0;
            while (c[i] && i < TEXT_LEN) {
                data_ptr = add_char(data_ptr, c[i++], l);
            }
            for (int j = 0; j < ((TEXT_LEN - i) * 5); ++j) {
                *data_ptr++ = BG_COLOUR | (BG_COLOUR << 16);
            }
        }
        else {
            *buf++ = count_of(sv_blank_after_avatar);
            *buf++ = (uintptr_t)sv_blank_after_avatar;
        }
#if 0
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
            while (*c >= 0x20 && *c < 0x7F && (c - toot->acct_name) < 10) {
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
#endif
    }
    else {
        return blank_line(buf);
    }

    // Terminate
    *buf++ = 0;
    *buf++ = 0;

    return data_ptr;
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
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            font_colours[i*4 + j] = ((0x0841 * i * 8) + BG_COLOUR) | (((0x0841 * j * 8) + BG_COLOUR) << 16);
        }
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

