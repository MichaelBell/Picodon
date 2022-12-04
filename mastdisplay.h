#pragma once

#define AVATAR_SIZE 120
#define TOOT_ID_LEN 20
#define MAX_NAME_LEN 64
#define MAX_CONTENT_LEN 512
#define NUM_DISPLAY_TOOTS 3

typedef struct mastodon_display_toot_tag {
    char toot_id[TOOT_ID_LEN];
    uint16_t avatar[AVATAR_SIZE*AVATAR_SIZE];
    char display_name[MAX_NAME_LEN];
    char acct_name[MAX_NAME_LEN];
    char content[MAX_CONTENT_LEN];
} MDTOOT;


typedef struct mastodon_display_data_tag {
    MDTOOT toot[NUM_DISPLAY_TOOTS];
} MDDATA;

// Called on core 1, never returns
// This renders the display data constantly.
// Display data is updated from the core 0 - there is no locking
void render_loop(MDDATA* display_data);
