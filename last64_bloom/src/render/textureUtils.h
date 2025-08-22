/**
 * @copyright 2025 - Max Bebök
 * @license MIT
 */
#pragma once

#include <libdragon.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load a binary RGB555 texture file into a surface
 * 
 * @param filename Path to the binary RGB555 file
 * @param width Width of the texture
 * @param height Height of the texture
 * @return surface_t The loaded surface
 */
surface_t load_rgb555_texture(const char* filename, int width, int height);

#ifdef __cplusplus
}
#endif