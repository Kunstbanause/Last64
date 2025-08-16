#include "audio.h"
#include <libdragon.h>

static wav64_t sfx_level_up;
static wav64_t sfx_start;

void audio_init_sounds()
{
    wav64_open(&sfx_level_up, "rom:/sfx/test.wav64");
    wav64_open(&sfx_start, "rom:/sfx/test.wav64");
}

void audio_play_sfx(SfxId id)
{
    switch (id)
    {
        case SFX_LEVEL_UP:
            mixer_ch_play(0, &sfx_level_up.wave);
            break;
        case SFX_START:
            mixer_ch_play(0, &sfx_start.wave);
            break;
    }
}
