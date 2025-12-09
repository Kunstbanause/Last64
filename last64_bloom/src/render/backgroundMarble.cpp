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
    : marbleTime(0.0f)
{
    initSineTable();
    
    // Use total level ups from save game as randomness seed
    // This value increases over gameplay, providing different patterns
    uint32_t levelUps = SaveGame::get_total_level_ups();
    phaseOffset = (float)(levelUps % 100) * 1.88f; // Arbitrary scaling for variation
    debugf("BackgroundMarble: levelUps=%lu, phaseOffset=%.2f\n", levelUps, phaseOffset);
}

BackgroundMarble::~BackgroundMarble()
{
}

void BackgroundMarble::reset()
{
    marbleTime = 0.0f;
}

void BackgroundMarble::draw(float deltaTime)
{
    ProfileScope profile("Marble");
    
    marbleTime += deltaTime;

    rdpq_set_scissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    rdpq_set_mode_standard();
    rdpq_mode_zbuf(false, false);
    rdpq_mode_persp(false);
    rdpq_mode_filter(FILTER_BILINEAR);

    // Simplified grid with single-iteration distortion, similar-hue palette
    const int cellW = 12;
    const int cellH = 8;
    float phase = (marbleTime + phaseOffset) * 0.45f;

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

            // Balanced palette (dark gray with moderate reddish tint)
            uint8_t base_r = 55;
            uint8_t base_g = 25;
            uint8_t base_b = 28;
            uint8_t accent_r = 110;
            uint8_t accent_g = 45;
            uint8_t accent_b = 55;

            uint8_t r = base_r + (uint8_t)((accent_r - base_r) * pattern);
            uint8_t g = base_g + (uint8_t)((accent_g - base_g) * pattern);
            uint8_t b = base_b + (uint8_t)((accent_b - base_b) * pattern);

            rdpq_set_mode_fill(RGBA32(r, g, b, 0xFF));
            rdpq_fill_rectangle(x, y, x + cellW, y + cellH);
        }
    }

    rdpq_set_mode_standard();
}
