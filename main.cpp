#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/sync.h"

#include "JPEGDEC.h"

#include "secrets.h"

extern "C" {
#include "tls_client.h"
#include "mastodon.h"
#include "mastdisplay.h"
}

// Display control and command buffers
MDDATA display_data;

void core1_func();

static JPEGDEC jpegdec;

#define DISPLAY_ROWS 240
#define DISPLAY_COLS 240

bool connect_wifi() {
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
        printf("failed to initialise\n");
        return false;
    }
    cyw43_arch_enable_sta_mode();

    printf("Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return false;
    } else {
        printf("Connected.\n");
        return true;
    }
}

static int toot_idx = 0;

int jpeg_draw_callback(JPEGDRAW* pDraw) {
    //printf("Drawing %d pixels at %d-%d, %d-%d\n", 
    //        pDraw->iWidth * pDraw->iHeight, pDraw->x, pDraw->iWidth, pDraw->y, pDraw->iHeight, pDraw->pPixels);

    for (int y = 0; y < pDraw->iHeight; ++y) {
        // Should implement our own pixel type in JPEGDEC to allow this.
        //memcpy(&display_data.toot[0].avatar[(pDraw->y + y)*AVATAR_SIZE + pDraw->x], &pDraw->pPixels[y*pDraw->iWidth], pDraw->iWidth*2);
        
        uint16_t* pDest = &display_data.toot[toot_idx].avatar[(pDraw->y + y)*AVATAR_SIZE + pDraw->x];
        uint16_t* pSource = &pDraw->pPixels[y*pDraw->iWidth];
        for (int x = 0; x < pDraw->iWidth; ++x) {
            uint16_t bgr555_pixel = (*pSource >> 11) | (*pSource << 11) | (*pSource & 0x07c0);
            *pDest++ = bgr555_pixel;
            ++pSource;
        }
    }

    return 1;
}

#define TOOT_AVATAR_PATH_LEN 128

void display_avatar(const uint8_t* avatar_jpeg, int jpeg_len, bool booster) {
    if (jpeg_len > 0) {
        printf("Decoding JPEG length %d\n", jpeg_len);
        jpegdec.openRAM(avatar_jpeg, jpeg_len, &jpeg_draw_callback);
        //jpegdec.setPixelType(RGB565_BIG_ENDIAN);
        if (!booster) {
            jpegdec.decode(0, 0, JPEG_SCALE_HALF);
        }
        else {
            jpegdec.decode(90, 90, JPEG_SCALE_EIGHTH);
        }
    }
}

void fill_toot_data(MTOOT* toot) {
    MDTOOT* display_toot = &display_data.toot[toot_idx];

    strncpy(display_toot->toot_id, toot->id, TOOT_ID_LEN - 1);
    strncpy(display_toot->acct_name, toot->originator_acct, MAX_NAME_LEN - 1);
    strncpy(display_toot->content, toot->content, MAX_CONTENT_LEN - 1);

    char booster_avatar_path[TOOT_AVATAR_PATH_LEN] = "";
    if (toot->booster_avatar_path) strcpy(booster_avatar_path, toot->booster_avatar_path);

    int len;
    const uint8_t* jpeg_data = get_avatar_jpeg(toot->originator_avatar_path, &len);
    display_avatar(jpeg_data, len, false);

    if (toot->booster_avatar_path) {
        jpeg_data = get_avatar_jpeg(booster_avatar_path, &len);
        display_avatar(jpeg_data, len, true);
    }
}

int main() {
    // Pixel clock for 720p is 74.25MHz
    set_sys_clock_khz(74250 * 2, true);
    //set_sys_clock_khz(64000 * 4, true);

    stdio_init_all();

    memset(&display_data, 0, sizeof(MDDATA));
    multicore_launch_core1(core1_func);

    while (!connect_wifi()) {
        sleep_ms(10000);
    }

    const char* last_toot_id_ptr = NULL;
    MTOOT last_toot;
    while (true) {
        if (get_home_toot(&last_toot, last_toot_id_ptr, NULL)) {
            if (last_toot.id) {
                last_toot_id_ptr = display_data.toot[0].toot_id;

                toot_idx = 0;
                fill_toot_data(&last_toot);

                while (toot_idx + 1 < NUM_DISPLAY_TOOTS) {
                    if (get_home_toot(&last_toot, NULL, display_data.toot[toot_idx].toot_id)) {
                        ++toot_idx;
                        fill_toot_data(&last_toot);
                    }
                }
            }
        }

        sleep_ms(15000);
    }
}

void core1_func() {
    render_loop(&display_data);
}
