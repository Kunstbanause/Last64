#include "audio.h"
#include <libdragon.h>
#include <stdlib.h>

SFXManager gSFXManager;

void SFXManager::init()
{
    wav64_open(&sfx_level_up, "rom:/sfx/level.wav64");
    wav64_open(&sfx_start,    "rom:/sfx/start.wav64");
    wav64_open(&sfx_join,     "rom:/sfx/join.wav64");
    wav64_open(&sfx_hits[0],  "rom:/sfx/hitA01.wav64");
    wav64_open(&sfx_hits[1],  "rom:/sfx/hitA02.wav64");
    wav64_open(&sfx_hits[2],  "rom:/sfx/hitA03.wav64");
    wav64_open(&sfx_hits[3],  "rom:/sfx/hitA04.wav64");
    wav64_open(&sfx_hits[4],  "rom:/sfx/hitA05.wav64");
    wav64_open(&sfx_hits[5],  "rom:/sfx/hitA06.wav64");
    wav64_open(&sfx_hits[6],  "rom:/sfx/hitA07.wav64");
    wav64_open(&sfx_hits[7],  "rom:/sfx/hitA08.wav64");
    wav64_open(&sfx_hits[8],  "rom:/sfx/hitA09.wav64");
    next_hit_channel = 0;
}

void SFXManager::play(SfxId id)
{
    switch (id)
    {
        case SFX_LEVEL_UP:
            mixer_ch_play(0, &sfx_level_up.wave);
            break;
        case SFX_START:
            mixer_ch_play(0, &sfx_start.wave);
            break;
        case SFX_JOIN:
            mixer_ch_play(1, &sfx_join.wave);
            break;
        case SFX_HIT:
            mixer_ch_play(2 + next_hit_channel, &sfx_hits[rand() % sfx_hits_count].wave);
            next_hit_channel = (next_hit_channel + 1) % HIT_CHANNELS;
            break;
    }
}