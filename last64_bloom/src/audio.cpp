#include "audio.h"
#include <libdragon.h>
#include <stdlib.h>

static wav64_t sfx_level_up;
static wav64_t sfx_start;
static wav64_t sfx_join;
static wav64_t sfx_hits[4];

void audio_init_sounds()
{
    wav64_open(&sfx_start,    "rom:/sfx/start.wav64");
    wav64_open(&sfx_join,     "rom:/sfx/join.wav64");
    wav64_open(&sfx_hits[0],  "rom:/sfx/hit1.wav64");
    wav64_open(&sfx_hits[1],  "rom:/sfx/hit2.wav64");
    wav64_open(&sfx_hits[2],  "rom:/sfx/hit3.wav64");
    wav64_open(&sfx_hits[3],  "rom:/sfx/hit4.wav64");
}

void audio_play_sfx(SfxId id)
{
    switch (id)
    {
        case SFX_LEVEL_UP:
           // mixer_ch_stop(0);
            mixer_ch_play(0, &sfx_start.wave);
            break;
        case SFX_START:
            mixer_ch_play(0, &sfx_start.wave);
            break;
        case SFX_JOIN:
           // mixer_ch_stop(1);
            mixer_ch_play(1, &sfx_join.wave);
        case SFX_HIT:
           // mixer_ch_stop(2);
            mixer_ch_play(2, &sfx_hits[rand() % 4].wave);
            break;
    }
}
