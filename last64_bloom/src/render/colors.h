#ifndef LAST64_BLOOM_COLORS_H
#define LAST64_BLOOM_COLORS_H

#include <cstdint>

namespace Colors {

const uint32_t player[] = {
    0x180404FF, // Red
    0x1C190BFF, // Yellow
    0x001118FF, // Blue
    0x071506FF  // Green
};

// Define test colors (RGBA8 format) 10% intensity 
const uint32_t testColors[] = {
    0x030205FF,
    0x070106FF,
    0x0F0209FF,
    0x180404FF,
    0x1B0D04FF,
    0x1A1302FF,
    0x1C190BFF,
    0x1C1A19FF,
    0x011912FF,
    0x001118FF,
    0x02090FFF,
    0x07080CFF,
    0x0A0C0FFF,
    0x071506FF
};

const uint32_t testColors2D[] = {
    0x1C162DFF,
    0x3D083BFF,
    0x841252FF,
    0xDA2424FF,
    0xF77622FF,
    0xECAB11FF,
    0xFEE761FF,
    0xFFEEE5FF,
    0x07E5A0FF,
    0x0099DBFF,
    0x124E89FF,
    0x3E4A6DFF,
    0x5A7088FF,
    0x43BD35FF
};

// const int32_t playerCHASM[] = {
//     0x090E15FF, // Green
//     0x120910FF, // Orange
//     0x080400FF, // Red
//     0xFFFF00FF  // Yellow
// };

// const uint32_t goldfire[] = {
//     0x14121AFF, // #46425e
//     0x192127FF, // #5b768d
//     0x3B2323FF, // #d17c7c
//     0x45372FFF  // #f6c6a8
// };

// const uint32_t oldplayer[] = {
//     0x080400FF, // Orange
//     0x00FF00FF, // Green
//     0x00FFFFFF, // Cyan
//     0xFFFF00FF  // Yellow
// };

}

#endif // LAST64_BLOOM_COLORS_H
