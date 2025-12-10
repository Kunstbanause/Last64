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
    enum class PaletteTheme { RED, GREEN, PINK, GREY, GOLD, RAINBOW };
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
    // Palette interpolation
    float paletteLerp{1.0f};
    // Lerp endpoints
    uint8_t start_base_r{55}, start_base_g{25}, start_base_b{28};
    uint8_t start_accent_r{110}, start_accent_g{45}, start_accent_b{55};
    uint8_t target_base_r{55}, target_base_g{25}, target_base_b{28};
    uint8_t target_accent_r{110}, target_accent_g{45}, target_accent_b{55};
    // Active (lerped) palette values
    uint8_t base_r{55}, base_g{25}, base_b{28};
    uint8_t accent_r{110}, accent_g{45}, accent_b{55};
};
