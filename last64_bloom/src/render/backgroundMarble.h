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
private:
    float marbleTime;
    
public:
    BackgroundMarble();
    ~BackgroundMarble();
    
    // Update and draw the marble background
    // Call this each frame before drawing 3D content
    void draw(float deltaTime);
    
    // Reset animation time
    void reset();
};
