/**
 * @copyright 2025 - Max Bebök
 * @license MIT
 */
#include "backgroundMarble.h"
#include "../utils/profiler.h"
#include <libdragon.h>
#include <cmath>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

BackgroundMarble::BackgroundMarble()
    : marbleTime(0.0f)
{
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
    float phase = marbleTime * 0.45f;

    for (int y = 0; y < SCREEN_HEIGHT; y += cellH) {
        for (int x = 0; x < SCREEN_WIDTH; x += cellW) {
            float px = (float)x / (float)SCREEN_WIDTH;
            float py = (float)y / (float)SCREEN_HEIGHT;

            // Single distortion pass (minimal math)
            float angle = phase * 0.2f + sinf(py * 3.0f) * 0.3f;
            float c = cosf(angle);
            float s = sinf(angle);
            float px_new = px * c - py * s;
            float py_new = px * s + py * c;

            // Simple turbulence
            px_new += sinf(py_new * 2.5f + phase * 0.6f) * 0.08f;
            py_new += cosf(px_new * 2.0f + phase * 0.5f) * 0.08f;

            // Pattern: single oscillation
            float pattern = 0.5f + 0.5f * sinf(px_new * 3.5f + py_new * 2.8f + phase * 0.7f);

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
