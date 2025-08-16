#pragma once
#include <libdragon.h>

class SFXManager {
public:
    enum SfxId {
        SFX_LEVEL_UP,
        SFX_START,
        SFX_JOIN,
        SFX_HIT
    };

    void init();
    void play(SfxId id);

private:
    static const int HIT_CHANNELS = 12; // We have 16 channels #0-15
    wav64_t sfx_level_up;
    wav64_t sfx_start;
    wav64_t sfx_join;
    wav64_t sfx_hits[4];
    int next_hit_channel = 0;
};

extern SFXManager gSFXManager;