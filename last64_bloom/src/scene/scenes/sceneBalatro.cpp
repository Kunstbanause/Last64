/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "sceneBalatro.h"
#include "../../main.h"
#include "../../debugMenu.h"
#include "../../render/textureUtils.h"
#include "../../render/debugDraw.h"
#include <t3d/t3d.h>
#include <t3d/tpx.h>
#include <libdragon.h>

namespace {
  // Ambient lighting
  constexpr uint8_t colorAmbient[4] = {0x20, 0x20, 0x20, 0xFF};

  // Static matrix for scene
  T3DMat4FP* sceneMatFP = nullptr;
  
  // Textures
  surface_t fbmTexture;
  surface_t warpTexture;
  surface_t paletteTexture;
  
  // Animation parameters
  float timeCounter = 0.0f;
  float scrollSpeed = 0.05f;
  float rotateSpeed = 0.3f;
  
  // Texture parameters for scrolling and warping
  rdpq_texparms_t fbmTexParams;
  rdpq_texparms_t warpTexParams;
  
  // Debug flags
  bool showFBM = true;
  bool showWarp = false;
  bool showCombined = true;
}

SceneBalatro::SceneBalatro()
{
    // Set up camera
    camera.fov = T3D_DEG_TO_RAD(80.0f);
    camera.near = 1.0f;
    camera.far = 200.0f;
    camera.pos = {0.0f, 0.0f, 50.0f};
    camera.target = {0.0f, 0.0f, 0.0f};

    // Initialize scene matrix
    sceneMatFP = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(sceneMatFP);
    
    // Initialize texture parameters
    fbmTexParams = {0};
    fbmTexParams.s.repeats = REPEAT_INFINITE;  // Repeat texture
    fbmTexParams.t.repeats = REPEAT_INFINITE;  // Repeat texture
    
    warpTexParams = {0};
    warpTexParams.s.repeats = REPEAT_INFINITE; // Repeat texture
    warpTexParams.t.repeats = REPEAT_INFINITE; // Repeat texture
    
    // Load textures with sizes that fit in TMEM
    // Maximum size for RGBA16 in TMEM is 32x32 (2KB)
    // Note: We're loading the 64x64 FBM texture but treating it as 32x32
    // This will only use the first quarter of the texture data
    fbmTexture = load_rgb555_texture("rom:/fbm_noise.rgb555", 32, 32);
    warpTexture = load_rgb555_texture("rom:/warp_map.rgb555", 32, 32);
    // The palette LUT is 256x1, but we'll load it as 128x1 to fit in TMEM
    // This will only use the first half of the palette data
    paletteTexture = load_rgb555_texture("rom:/palette_lut.rgb555", 128, 1);
    
    // Add debug menu entries
    DebugMenu::addEntry({"Show FBM", DebugMenu::EntryType::BOOL, &showFBM});
    DebugMenu::addEntry({"Show Warp", DebugMenu::EntryType::BOOL, &showWarp});
    DebugMenu::addEntry({"Show Combined", DebugMenu::EntryType::BOOL, &showCombined});
}

SceneBalatro::~SceneBalatro()
{
    // Clean up textures
    surface_free(&fbmTexture);
    surface_free(&warpTexture);
    surface_free(&paletteTexture);
    
    // Clean up scene matrix
    if (sceneMatFP) {
        free_uncached(sceneMatFP);
        sceneMatFP = nullptr;
    }
}

void SceneBalatro::updateScene(float deltaTime)
{
    camera.update(deltaTime);
    timeCounter += deltaTime;
    
    // Update texture parameters with animation
    fbmTexParams.s.translate = timeCounter * scrollSpeed;
    fbmTexParams.t.translate = timeCounter * scrollSpeed;
    
    // For the warp texture, we would rotate it around the center
    // This is a simplified version - a full implementation would use more complex math
    warpTexParams.s.translate = timeCounter * rotateSpeed;
    warpTexParams.t.translate = timeCounter * rotateSpeed;
    
    // Ensure the texture parameters are properly initialized
    fbmTexParams.s.repeats = REPEAT_INFINITE;
    fbmTexParams.t.repeats = REPEAT_INFINITE;
    warpTexParams.s.repeats = REPEAT_INFINITE;
    warpTexParams.t.repeats = REPEAT_INFINITE;
}

void SceneBalatro::draw3D(float deltaTime)
{
    camera.attach();
    
    // Clear the screen with a dark navy background
    t3d_screen_clear_color(RGBA32(15, 20, 40, 0xFF));
    t3d_screen_clear_depth();

    t3d_light_set_ambient(colorAmbient);
    t3d_light_set_count(0); // No directional lights, just ambient

    t3d_matrix_push(sceneMatFP);

    // Set up standard mode for texture rendering
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    rdpq_mode_blender(0);
    rdpq_mode_filter(FILTER_BILINEAR);
    rdpq_mode_persp(false);
    
    // Draw based on which debug option is enabled
    // Only draw one section at a time to avoid overlap
    if (showCombined) {
        // Draw the FBM texture for combined effect (full screen)
        rdpq_tex_upload(TILE0, &fbmTexture, &fbmTexParams);
        rdpq_texture_rectangle(TILE0, 0, 0, 320, 240, 0, 0);
    } else if (showWarp) {
        // Draw the warp texture (top right quadrant)
        rdpq_tex_upload(TILE0, &warpTexture, &warpTexParams);
        rdpq_texture_rectangle(TILE0, 160, 0, 320, 120, 0, 0);
    } else if (showFBM) {
        // Draw the FBM texture (top left quadrant)
        rdpq_tex_upload(TILE0, &fbmTexture, &fbmTexParams);
        rdpq_texture_rectangle(TILE0, 0, 0, 160, 120, 0, 0);
    } else {
        // If no debug option is enabled, just show the background
        // This shouldn't happen with the current debug menu setup, but just in case
    }
    
    /*
     * In a complete implementation of the Balatro-style effect, we would:
     * 1. Use multiple texture units (TILE0, TILE1, TILE2) for:
     *    - TILE0: FBM noise texture with scrolling UVs
     *    - TILE1: Warp displacement map with rotating UVs
     *    - TILE2: Palette LUT for color mapping
     * 2. Set up a complex combiner to:
     *    - Apply the warp displacement to the FBM texture coordinates
     *    - Use the displaced FBM values to index into the palette LUT
     *    - Apply a radial gradient overlay for the center bias effect
     * 3. Animate the texture coordinates for:
     *    - Scrolling the FBM noise
     *    - Rotating the warp displacement map around the center
     * 4. Optimize for N64 hardware constraints (limited texture units, etc.)
     */

    t3d_matrix_pop(1);
}

void SceneBalatro::draw2D(float deltaTime)
{
    // Display scene name
    Debug::printf(100, 10, "Balatro Background");
    Debug::printf(100, 20, "Time: %.2f", timeCounter);
    
    // Show which textures are being displayed
    if (showFBM) Debug::printf(10, 30, "FBM Noise: ON");
    if (showWarp) Debug::printf(10, 40, "Warp Map: ON");
    if (showCombined) Debug::printf(10, 50, "Combined: ON");
}