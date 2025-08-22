/**
 * @copyright 2025 - Max Bebök
 * @license MIT
 */
#include "textureUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libdragon.h>

surface_t load_rgb555_texture(const char* filename, int width, int height) {
    // Try to open file
    FILE* file = fopen(filename, "rb");
    if (!file) {
        // If file doesn't exist, create a surface with a test pattern
        surface_t surf = surface_alloc(FMT_RGBA16, width, height);
        uint16_t* buffer = (uint16_t*)surf.buffer;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // Create a test pattern (gradient)
                uint8_t r = (x * 255 / width) & 0xF8;  // Reduce to 5-bit range
                uint8_t g = (y * 255 / height) & 0xF8; // Reduce to 5-bit range
                uint8_t b = 128 & 0xF8;                // Reduce to 5-bit range
                
                // Convert to RGB555
                uint16_t rgb555 = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
                buffer[y * width + x] = rgb555;
            }
        }
        return surf;
    }
    
    // Allocate surface
    surface_t surf = surface_alloc(FMT_RGBA16, width, height);
    
    // Read the binary data
    size_t expected_size = width * height * sizeof(uint16_t);
    size_t read_size = fread(surf.buffer, 1, expected_size, file);
    fclose(file);
    
    // Check if we read the expected amount of data
    if (read_size != expected_size) {
        // Fill with test pattern if read failed
        uint16_t* buffer = (uint16_t*)surf.buffer;
        for (int i = 0; i < width * height; i++) {
            buffer[i] = 0x7C1F; // Test color (pinkish)
        }
    }
    
    return surf;
}