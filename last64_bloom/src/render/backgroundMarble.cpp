/**
 * @copyright 2025 - Max Bebök
 * @license MIT
 */
#include "backgroundMarble.h"
#include "../utils/profiler.h"
#include "../memory/savegame.h"
#include <libdragon.h>
#include <cmath>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Fixed-point sine lookup table (256 entries for 0 to 2π)
// Values are in 8.8 fixed-point format (scaled by 256)
namespace {
    constexpr int SINE_TABLE_SIZE = 256;
    int16_t sineTable[SINE_TABLE_SIZE];
    bool sineTableInitialized = false;
    
    // Initialize sine lookup table
    void initSineTable() {
        if (sineTableInitialized) return;
        for (int i = 0; i < SINE_TABLE_SIZE; i++) {
            float angle = (float)i * (2.0f * M_PI / SINE_TABLE_SIZE);
            sineTable[i] = (int16_t)(sinf(angle) * 256.0f);
        }
        sineTableInitialized = true;
    }
    
    // Fast sine lookup using fixed-point input (8.8 format)
    // Input: angle in range [0, 256) representing [0, 2π)
    inline int16_t fastSin(uint32_t angle) {
        return sineTable[angle & 0xFF];
    }
    
    // Fast cosine lookup (cos(x) = sin(x + π/2))
    inline int16_t fastCos(uint32_t angle) {
        return sineTable[(angle + 64) & 0xFF];
    }
}

BackgroundMarble::BackgroundMarble()
    : marbleTime(0.0f), theme(PaletteTheme::RED)
{
    initSineTable();
    
    // Use total level ups from save game as randomness seed
    // This value increases over gameplay, providing different patterns
    uint32_t levelUps = SaveGame::get_total_level_ups();
    phaseOffset = (float)(levelUps % 100) * 1.88f; // Arbitrary scaling for variation
    paletteLerp = 1.0f;
    start_base_r = target_base_r = base_r;
    start_base_g = target_base_g = base_g;
    start_base_b = target_base_b = base_b;
    start_accent_r = target_accent_r = accent_r;
    start_accent_g = target_accent_g = accent_g;
    start_accent_b = target_accent_b = accent_b;
}

BackgroundMarble::~BackgroundMarble()
{
}

void BackgroundMarble::reset()
{
    marbleTime = 0.0f;
}

void BackgroundMarble::setTheme(PaletteTheme newTheme)
{
    // Capture current palette as start for interpolation
    start_base_r = base_r; start_base_g = base_g; start_base_b = base_b;
    start_accent_r = accent_r; start_accent_g = accent_g; start_accent_b = accent_b;

    theme = newTheme;
    // Update target palette based on theme
    switch (theme) {
        case PaletteTheme::RED: // default burgundy
            target_base_r = 55;  target_base_g = 25;  target_base_b = 28;
            target_accent_r = 110; target_accent_g = 45; target_accent_b = 55;
            break;
        case PaletteTheme::GREEN: // deep green
            target_base_r = 24;  target_base_g = 50;  target_base_b = 36;
            target_accent_r = 70; target_accent_g = 120; target_accent_b = 90;
            break;
        case PaletteTheme::PINK: // vibrant pink
            target_base_r = 70;  target_base_g = 24;  target_base_b = 48;
            target_accent_r = 170; target_accent_g = 70;  target_accent_b = 150;
            break;
        case PaletteTheme::GREY: // muted greys for menus
            target_base_r = 42;  target_base_g = 42;  target_base_b = 42;
            target_accent_r = 96; target_accent_g = 96; target_accent_b = 96;
            break;
        case PaletteTheme::GOLD: // warm gold for main menu
            target_base_r = 60;  target_base_g = 46;  target_base_b = 18;
            target_accent_r = 196; target_accent_g = 160; target_accent_b = 64;
            break;
        case PaletteTheme::RAINBOW: // animated rainbow (values modulated in draw)
            target_base_r = 80;  target_base_g = 80;  target_base_b = 80;
            target_accent_r = 160; target_accent_g = 160; target_accent_b = 160;
            break;
    }
    paletteLerp = 0.0f; // start fade
}

void BackgroundMarble::draw(float deltaTime)
{
    ProfileScope profile("Marble");
    
    marbleTime += deltaTime;
    // Advance palette interpolation (fade between themes)
    if (paletteLerp < 1.0f) {
        paletteLerp += deltaTime * 6.4f; // fade speed
        if (paletteLerp > 1.0f) paletteLerp = 1.0f;
    }
    float tLerp = paletteLerp;
    auto lerp8 = [](uint8_t a, uint8_t b, float t) -> uint8_t {
        float v = a + (b - a) * t;
        if (v < 0.0f) v = 0.0f;
        if (v > 255.0f) v = 255.0f;
        return (uint8_t)v;
    };
    base_r = lerp8(start_base_r, target_base_r, tLerp);
    base_g = lerp8(start_base_g, target_base_g, tLerp);
    base_b = lerp8(start_base_b, target_base_b, tLerp);
    accent_r = lerp8(start_accent_r, target_accent_r, tLerp);
    accent_g = lerp8(start_accent_g, target_accent_g, tLerp);
    accent_b = lerp8(start_accent_b, target_accent_b, tLerp);

    rdpq_set_scissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    rdpq_set_mode_standard();
    rdpq_mode_zbuf(false, false);
    rdpq_mode_persp(false);
    rdpq_mode_filter(FILTER_BILINEAR);

    // Simplified grid with single-iteration distortion, similar-hue palette
    const int cellW = 12;
    const int cellH = 8;
    float phase = (marbleTime + phaseOffset) * 0.45f;

    // Dynamic rainbow palette modulation
    uint8_t dyn_base_r = base_r;
    uint8_t dyn_base_g = base_g;
    uint8_t dyn_base_b = base_b;
    uint8_t dyn_accent_r = accent_r;
    uint8_t dyn_accent_g = accent_g;
    uint8_t dyn_accent_b = accent_b;

    if (theme == PaletteTheme::RAINBOW) {
        float t = marbleTime * 0.9f;
        auto wave = [](float x) -> uint8_t {
            float s = 0.5f + 0.5f * sinf(x);
            int v = (int)(s * 255.0f);
            if (v < 0) v = 0;
            if (v > 255) v = 255;
            return (uint8_t)v;
        };
        dyn_base_r   = wave(t);
        dyn_base_g   = wave(t + 2.094f); // +120 deg
        dyn_base_b   = wave(t + 4.188f); // +240 deg
        dyn_accent_r = wave(t + 0.6f);
        dyn_accent_g = wave(t + 2.694f);
        dyn_accent_b = wave(t + 4.788f);
    }

    for (int y = 0; y < SCREEN_HEIGHT; y += cellH) {
        for (int x = 0; x < SCREEN_WIDTH; x += cellW) {
            float px = (float)x / (float)SCREEN_WIDTH;
            float py = (float)y / (float)SCREEN_HEIGHT;

            // Single distortion pass - using fixed-point lookup
            // Convert angle to 0-255 range for lookup table
            uint32_t pyAngle = (uint32_t)(py * 3.0f * 256.0f / (2.0f * M_PI)) & 0xFF;
            int16_t sinPy = fastSin(pyAngle);  // Result in 8.8 format
            
            uint32_t angle256 = (uint32_t)(phase * 0.2f * 256.0f / (2.0f * M_PI)) + 
                               (sinPy * 77 >> 8);  // 0.3 * 256 = 77
            
            int16_t c_fixed = fastCos(angle256);
            int16_t s_fixed = fastSin(angle256);
            
            // Rotation math with fixed-point (result will be scaled by 256)
            float px_new = (px * c_fixed - py * s_fixed) / 256.0f;
            float py_new = (px * s_fixed + py * c_fixed) / 256.0f;

            // Simple turbulence - using lookup table
            uint32_t turbAngle1 = (uint32_t)((py_new * 2.5f + phase * 0.6f) * 256.0f / (2.0f * M_PI)) & 0xFF;
            uint32_t turbAngle2 = (uint32_t)((px_new * 2.0f + phase * 0.5f) * 256.0f / (2.0f * M_PI)) & 0xFF;
            
            px_new += (fastSin(turbAngle1) / 256.0f) * 0.08f;
            py_new += (fastCos(turbAngle2) / 256.0f) * 0.08f;

            // Pattern: single oscillation using lookup
            uint32_t patternAngle = (uint32_t)((px_new * 3.5f + py_new * 2.8f + phase * 0.7f) * 256.0f / (2.0f * M_PI)) & 0xFF;
            float pattern = 0.5f + 0.5f * (fastSin(patternAngle) / 256.0f);

            // Palette varies by theme (set via setTheme)
            uint8_t r = dyn_base_r + (uint8_t)((dyn_accent_r - dyn_base_r) * pattern);
            uint8_t g = dyn_base_g + (uint8_t)((dyn_accent_g - dyn_base_g) * pattern);
            uint8_t b = dyn_base_b + (uint8_t)((dyn_accent_b - dyn_base_b) * pattern);

            rdpq_set_mode_fill(RGBA32(r, g, b, 0xFF));
            rdpq_fill_rectangle(x, y, x + cellW, y + cellH);
        }
    }

    rdpq_set_mode_standard();
}
