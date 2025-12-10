/**
 * @copyright 2025 - Max Bebök
 * @license MIT
 * 
 * Marble pattern background renderer
 */
#pragma once
#include <libdragon.h>
#include <cstdint>

class BackgroundMarble {
public:
    enum class PaletteTheme { RED, GREEN, PINK, GREY, GOLD };
    BackgroundMarble();
    ~BackgroundMarble();
    
    // Update and draw the marble background
    // Call this each frame before drawing 3D content
    void draw(float deltaTime);
    
    // Reset animation time
    void reset();

    // Change palette theme (affects base/accent colors)
    void setTheme(PaletteTheme newTheme);
    
private:
    float marbleTime;
    float phaseOffset;  // Random offset for pattern variation
    PaletteTheme theme;
    // Current palette values
    uint8_t base_r{55}, base_g{25}, base_b{28};
    uint8_t accent_r{110}, accent_g{45}, accent_b{55};
};
