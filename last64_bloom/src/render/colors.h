#ifndef LAST64_BLOOM_COLORS_H
#define LAST64_BLOOM_COLORS_H

#include <cstdint>

namespace Colors {
// Define test colors (RGBA8 format) 10% intensity 
const uint32_t testColors[] = {
    0x0C1415FF,
    0x091215FF,
    0x090E15FF,
    0x090A15FF,
    0x07090FFF,
    0x06060AFF,
    0x050405FF,
    0x060508FF,
    0x08060BFF,
    0x0C070DFF,
    0x0F080FFF,
    0x120910FF,
    0x160F0CFF,
    0x16140FFF,
    0x0D130DFF,
    0x08110DFF,
    0x07110FFF,
    0x060D0FFF,
    0x081716FF,
    0x170812FF,
    0x17170CFF,
    0x171717FF
};

const uint32_t testColors2D[] = {
    0x85DAEBFF,
    0x5FC9E7FF,
    0x5FA1E7FF,
    0x5F6EE7FF,
    0x4C60AAFF,
    0x444774FF,
    0x32313BFF,
    0x463C5EFF,
    0x5D4776FF,
    0x855395FF,
    0xAB58A8FF,
    0xCA60AEFF,
    0xF3A787FF,
    0xF5DAA7FF,
    0x8DD894FF,
    0x5DC190FF,
    0x4AB9A3FF,
    0x4593A5FF,
    0x5EFDF7FF,
    0xFF5DCCFF,
    0xFDFE89FF,
    0xFFFFFFFF
};

const uint32_t player[] = {
    0x050909FF,
    0x040809FF,
    0x040609FF,
    0x040409FF
};

const uint32_t oldplayer[] = {
    0x080400FF, // Orange
    0x00FF00FF, // Green
    0x00FFFFFF, // Cyan
    0xFFFF00FF // Yellow
};

}

#endif // LAST64_BLOOM_COLORS_H
