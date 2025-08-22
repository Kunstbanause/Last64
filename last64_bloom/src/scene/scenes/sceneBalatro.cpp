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
  surface_t fbmTexture;      // 64x64 grayscale noise
  surface_t warpTexture;     // 32x32 RG displacement map
  surface_t paletteTexture;  // 256x1 color LUT
  
  // Animation parameters
  float timeCounter = 0.0f;
  float scrollSpeed = 0.05f;
  float rotateSpeed = 0.3f;
  float spiralStrength = 0.8f;
  // Remove unused variable
  // float marbleFreq = 1.5f;  // Unused - remove to fix warning
  
  // Texture parameters
  rdpq_texparms_t fbmTexParams;
  rdpq_texparms_t warpTexParams;
  rdpq_texparms_t paletteTexParams;
  
  // Debug flags
  bool showFBM = false;
  bool showWarp = false;
  bool showCombined = true;
  bool useAnimation = true;
  
  // Center bias parameters
  float centerBias = 2.0f;
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
    
    // Initialize texture parameters for tiling
    fbmTexParams = {0};
    fbmTexParams.s.repeats = REPEAT_INFINITE;
    fbmTexParams.t.repeats = REPEAT_INFINITE;
    fbmTexParams.s.mirror = false;
    fbmTexParams.t.mirror = false;
    
    warpTexParams = {0};
    warpTexParams.s.repeats = REPEAT_INFINITE;
    warpTexParams.t.repeats = REPEAT_INFINITE;
    warpTexParams.s.mirror = false;
    warpTexParams.t.mirror = false;
    
    paletteTexParams = {0};
    paletteTexParams.s.repeats = REPEAT_INFINITE;
    paletteTexParams.t.repeats = REPEAT_INFINITE;
    
    // Load textures - sizes chosen to fit in N64's 4KB TMEM
    // FBM noise: 32x32 RGB555 (2KB) - reduced from 64x64 to fit TMEM
    fbmTexture = load_rgb555_texture("rom:/fbm_noise.rgb555", 32, 32);
    
    // Warp map: 32x32 RGB555 (2KB) - displacement in R,G channels
    warpTexture = load_rgb555_texture("rom:/warp_map.rgb555", 32, 32);
    
    // Palette LUT: 64x1 RGB555 (128 bytes) - reduced from 256x1 to fit TMEM
    paletteTexture = load_rgb555_texture("rom:/palette_lut.rgb555", 64, 1);
    
    // Add debug menu entries
    DebugMenu::addEntry({"Show FBM Only", DebugMenu::EntryType::BOOL, &showFBM});
    DebugMenu::addEntry({"Show Warp Only", DebugMenu::EntryType::BOOL, &showWarp});
    DebugMenu::addEntry({"Show Combined", DebugMenu::EntryType::BOOL, &showCombined});
    DebugMenu::addEntry({"Use Animation", DebugMenu::EntryType::BOOL, &useAnimation});
    DebugMenu::addEntry({"Scroll Speed", DebugMenu::EntryType::FLOAT, &scrollSpeed});
    DebugMenu::addEntry({"Rotate Speed", DebugMenu::EntryType::FLOAT, &rotateSpeed});
    DebugMenu::addEntry({"Spiral Strength", DebugMenu::EntryType::FLOAT, &spiralStrength});
    DebugMenu::addEntry({"Center Bias", DebugMenu::EntryType::FLOAT, &centerBias});
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
    
    if (useAnimation) {
        timeCounter += deltaTime;
    }
    
    // Update FBM texture scrolling
    fbmTexParams.s.translate = fmodf(timeCounter * scrollSpeed, 1.0f);
    fbmTexParams.t.translate = fmodf(timeCounter * scrollSpeed, 1.0f);
    
    // Update warp texture rotation (simplified - rotates around texture center)
    float angle = timeCounter * rotateSpeed;
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    
    // Simple rotation approximation using translation
    // For true rotation, you'd need vertex shader or pre-computed rotation matrices
    warpTexParams.s.translate = fmodf(0.5f + cos_a * 0.1f, 1.0f);
    warpTexParams.t.translate = fmodf(0.5f + sin_a * 0.1f, 1.0f);
}

void SceneBalatro::draw3D(float deltaTime)
{
    camera.attach();
    
    // Clear with dark navy background
    t3d_screen_clear_color(RGBA32(15, 20, 40, 0xFF));
    t3d_screen_clear_depth();

    t3d_light_set_ambient(colorAmbient);
    t3d_light_set_count(0);

    t3d_matrix_push(sceneMatFP);

    // Set up RDP for texture operations
    rdpq_set_mode_standard();
    rdpq_mode_filter(FILTER_BILINEAR);
    rdpq_mode_persp(false);
    
    if (showCombined) {
        // ===== FULL BALATRO EFFECT =====
        // This is a simplified version - full effect would require custom combiner modes
        
        // Method 1: Simple additive blending of textures
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        
        // First pass: Draw FBM base
        rdpq_tex_upload(TILE0, &fbmTexture, &fbmTexParams);
        rdpq_mode_combiner(RDPQ_COMBINER_TEX);
        rdpq_texture_rectangle(TILE0, 0, 0, 320, 240, 0, 0);
        
        // Second pass: Modulate with warp map
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_tex_upload(TILE1, &warpTexture, &warpTexParams);
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_texture_rectangle(TILE1, 0, 0, 320, 240, 0, 0);
        
        /*
         * ADVANCED IMPLEMENTATION NOTES:
         * 
         * For the full Balatro effect, you would need:
         * 
         * 1. Custom RDP combiner that:
         *    - Uses warp texture (R,G channels) to offset FBM texture coordinates
         *    - Applies marble sine function: sin(freq * warped_coords + fbm_detail)
         *    - Uses result to index into palette LUT
         *    - Applies radial center bias gradient
         * 
         * 2. Multi-pass rendering:
         *    Pass 1: Generate warped coordinates using TILE0 (warp) + TILE1 (fbm)
         *    Pass 2: Apply marble function and center bias
         *    Pass 3: Map through palette LUT (TILE2)
         * 
         * 3. Vertex buffer approach:
         *    - Generate a grid of vertices (e.g., 32x24 quads)
         *    - Calculate warped UV coordinates per vertex
         *    - Use smooth interpolation across triangles
         * 
         * Example advanced combiner setup:
         * rdpq_mode_combiner(RDPQ_COMBINER2(
         *     // Cycle 1: Warp FBM coordinates
         *     RDPQ_COMBINER1((TEX1, 0, TEX0, 0), (0, 0, 0, 1)),
         *     // Cycle 2: Apply marble function and palette lookup
         *     RDPQ_COMBINER1((COMBINED, 0, TEX2, 0), (0, 0, 0, 1))
         * ));
         */
        
    } else if (showFBM) {
        // Show FBM texture only
        rdpq_tex_upload(TILE0, &fbmTexture, &fbmTexParams);
        rdpq_mode_combiner(RDPQ_COMBINER_TEX);
        rdpq_mode_blender(0);
        rdpq_texture_rectangle(TILE0, 0, 0, 320, 240, 0, 0);
        
    } else if (showWarp) {
        // Show warp displacement map
        rdpq_tex_upload(TILE0, &warpTexture, &warpTexParams);
        rdpq_mode_combiner(RDPQ_COMBINER_TEX);
        rdpq_mode_blender(0);
        rdpq_texture_rectangle(TILE0, 0, 0, 320, 240, 0, 0);
    }
    
    t3d_matrix_pop(1);
}

void SceneBalatro::draw2D(float deltaTime)
{
    // Display scene info
    Debug::printf(10, 10, "Balatro Background Effect");
    Debug::printf(10, 20, "Time: %.2f", timeCounter);
    Debug::printf(10, 30, "Scroll: %.3f", scrollSpeed);
    Debug::printf(10, 40, "Rotate: %.3f", rotateSpeed);
    
    // Show current mode
    if (showCombined) Debug::printf(10, 60, "Mode: Combined Effect");
    else if (showFBM) Debug::printf(10, 60, "Mode: FBM Noise Only");
    else if (showWarp) Debug::printf(10, 60, "Mode: Warp Map Only");
    
    // Texture info
    Debug::printf(200, 10, "FBM: 32x32 RGB555 (2KB)");
    Debug::printf(200, 20, "Warp: 32x32 RGB555 (2KB)");
    Debug::printf(200, 30, "Palette: 64x1 RGB555 (128B)");
    
    // Animation status
    Debug::printf(200, 50, "Animation: %s", useAnimation ? "ON" : "OFF");
}