#pragma once

enum SfxId {
    SFX_LEVEL_UP,
    SFX_START,
    SFX_JOIN,
    SFX_HIT
};

void audio_init_sounds();
void audio_play_sfx(SfxId id);
