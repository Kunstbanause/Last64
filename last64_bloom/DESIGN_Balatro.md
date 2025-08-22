/**
 * Balatro-style N64 background implementation
 * 
 * This file contains notes and references for implementing the Balatro-style background
 * effect on the N64 using libdragon and Tiny3D.
 * 
 * Concept:
 * - Pre-render 64x64 tileable FBM noise texture
 * - Pre-render 32x32 warp displacement map
 * - Use libdragon/Tiny3D to render fullscreen quad
 * - Apply scrolling base UVs and rotating warp UVs from center
 * - Combine textures using RDP texture combiners for marble effect
 * - Apply radial gradient overlay for center bias
 * - Use 256x1 RGB555 palette LUT for final colors
 * - Target 30fps with fixed-point math
 * 
 * Implementation Plan:
 * 1. Generate textures using the Python script in assets/background.py
 *    - Output FBM noise as RGB555
 *    - Output warp displacement map as RGB555
 *    - Output palette LUT as 256x1 RGB555
 * 2. Load these textures in SceneBalatro
 * 3. Set up a fullscreen quad with appropriate vertex data
 * 4. Implement custom RDP combiner mode to achieve the effect
 *    - Use texture coordinates for scrolling and warping
 *    - Apply palette lookup
 * 5. Render the quad in draw3D()
 * 
 * Key challenges:
 * - Efficiently generating and loading textures in correct format
 * - Correctly setting up RDP combiner for multi-texture effects
 * - Managing texture coordinates for scrolling and warping
 * - Applying radial gradient in hardware or as additional texture
 * - Integrating palette lookup with combiner
 * 
 * Reference materials:
 * - https://github.com/DragonMinded/libdragon/blob/master/include/rdpq.h
 * - https://github.com/DragonMinded/libdragon/wiki/RDP-Combiner-Recipes
 * - Tiny3D examples and documentation
 * - assets/background.py for reference on effect generation
 */