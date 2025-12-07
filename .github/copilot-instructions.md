# Tiny3D + Last64_Bloom Development Guide

## Project Overview
**Tiny3D** is a custom 3D rendering engine and RSP microcode for Nintendo 64 using libdragon. The main project in this repo is **Last64_Bloom**, a 4-player Vampire Survivors-style game with abstract visuals, HDR, and bloom effects.

### Key Architecture Components
- **Tiny3D Library** (`src/t3d/`): Core 3D engine with RSP microcode for N64 hardware
- **Last64_Bloom Game** (`last64_bloom/`): Main game implementation using Tiny3D
- **Examples** (`examples/`): 24+ progressive examples demonstrating Tiny3D features
- **Custom RSP Microcode** (`src/t3d/rsp/`): Low-level GPU code for N64's Reality Signal Processor

## Critical Build System Knowledge

### Building Tiny3D Library
```bash
./build.sh              # Builds library, tools, and all examples
make -j4                # Build library only
make install            # Install to N64_INST (may need sudo)
```

### Building Last64_Bloom
```bash
cd last64_bloom
make                    # Build the game ROM
make clean              # Clean build artifacts
make sc64               # Deploy to SC64 flashcart via network
```

### Asset Pipeline
- **GLTF → T3DM**: `gltf_to_t3d` tool converts 3D models (Blender 4.0 + Fast64)
- **PNG → Sprite**: `mksprite` converts textures
- **WAV → WAV64**: `audioconv64` converts audio (forced mono)
- All assets auto-convert via Makefile rules: `filesystem/%.t3dm: assets/%.glb`

### Task System
Use VS Code tasks for common operations:
- `make` - Build Last64_Bloom
- `make-run-last64` - Build and run in ares emulator (background)
- `kill-ares` - Stop emulator

## Tiny3D Rendering Model (Critical Differences from OpenGL)

### Memory Model: DMA Everything
**Matrices and vertices are DMA'd to RSP on every frame** - they live in RDRAM, not display lists:
```cpp
// Allocate uncached memory for RSP DMA
T3DMat4FP* matrix = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
T3DVertPacked* verts = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * count);

// Modify anytime - changes are picked up on next frame
matrix->m[0][0] = newValue; // No need to rebuild display lists
```

### Fixed Vertex Layout
All vertices use `T3DVertPacked` - interleaved data for TWO vertices per struct:
```cpp
typedef struct {
  int16_t posA[3];    // Position A (s16 fixed point)
  uint16_t normA;     // Normal A (5.6.5 packed)
  int16_t posB[3];    // Position B
  uint16_t normB;     // Normal B
  uint32_t rgbaA;     // Color A (RGBA8)
  uint32_t rgbaB;     // Color B
  int16_t stA[2];     // UV A (10.5 fixed point, pixel coords!)
  int16_t stB[2];     // UV B
} T3DVertPacked; // 32 bytes, MUST be 8-byte aligned
```

**UVs are in pixel coordinates (10.5 fixed point), NOT normalized [0,1]!**

### Direct RDP Interop
Tiny3D doesn't abstract materials or textures - use RDPQ API directly:
```cpp
t3d_frame_start();  // Begin Tiny3D rendering

// Set RDP modes manually
rdpq_mode_combiner(RDPQ_COMBINER_TEX);
rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
rdpq_sprite_blit(sprite, x, y, NULL);  // Load textures via sprite_load

// Draw with Tiny3D
t3d_viewport_attach(viewport);
t3d_matrix_push(modelMatrix);
t3d_model_draw(model);
```

### Normal Packing
Use `t3d_vert_pack_normal()` to convert vec3 normals to 16-bit packed format (5.6.5).

## Last64_Bloom Architecture

### Scene System
All gameplay lives in scenes (`src/scene/scenes/`):
- `sceneLast64.cpp` - Main game scene (Vampire Survivors mechanics)
- `sceneBunker.cpp` - Alternative environment
- Scenes own actors, camera, and game state

Base pattern:
```cpp
class Scene {
  std::vector<Actor::Base*> actors;
  Camera camera;
  virtual void updateScene(float deltaTime) = 0;
  virtual void draw3D(float deltaTime) = 0;
  virtual void draw2D(float deltaTime) = 0; // HUD/UI
};
```

### Actor System (Entity Pattern)
All game objects inherit from `Actor::Base` (`src/actors/base.h`):
- **Player** (`player.cpp`) - 4-player local co-op, controller input
- **Enemy** (`enemy.cpp`) - Shared vertex buffer pattern (MAX_ENEMIES pool)
- **Projectile** (`projectile.cpp`) - Pooled projectiles, reused vertices
- **XPShard** (`xpShard.cpp`) - Collectible drops with magnetic attraction

**Object Pooling Pattern** (performance critical for N64):
```cpp
// Shared static buffers across all instances
static T3DVertPacked* sharedVertices;  // Pre-allocated uncached memory
static T3DMat4FP* sharedMatrices[MAX_COUNT];

// Per-instance pointers into shared pool
T3DVertPacked* myVertices = &sharedVertices[myIndex * 2];
```

### Weapon System
Weapons are actors owned by players (`src/systems/weapon_*.cpp`):
- `WeaponBase` - Base class with upgrade levels, fire rate, cooldowns
- `WeaponProjectile`, `WeaponHoming`, `WeaponCircular`, `WeaponSpiral`, `WeaponShield`, `WeaponShape`
- Players can have multiple weapons simultaneously
- Weapons fire automatically based on cooldowns

### HDR + Bloom Pipeline
Custom RSP microcode (`src/rsp/rsp_fx.rspl`) implements HDR downscaling:
1. Render scene to RGBA32 with reduced brightness (1/8th intensity)
2. Downscale to 1/4 resolution blur buffer
3. Multi-pass Gaussian blur with threshold
4. Composite blurred bloom onto main buffer during final RGBA32→RGBA16 conversion

**Integration requires**: 
- Custom ucode compilation: `$(BUILD_DIR)/src/rsp/rsp_fx.o`
- `PostProcess` class wraps framebuffer redirects
- Render darker colors to preserve HDR headroom

### Color System
See `src/render/colors.h` for color definitions:
- Colors are RGBA8 in HDR space (10% intensity = full brightness after exposure)
- Neutral ambient light: `{0x80, 0x80, 0x80, 0xFF}`
- Player colors: Red, Yellow, Blue, Green (very dark base values)

**Use `colorConverter.html` to preview HDR colors outside the engine!**

### Audio System
Global `SFXManager gSFXManager` lives in `main.cpp`:
- Survives scene reloads for continuous music
- WAV files forced to mono: `audioconv64 --wav-mono`
- Mixer initialized with 16 channels

### Save System
EEPROM via `src/memory/savegame.h`:
```cpp
SaveGame::init();
SaveGame::set_music_enabled(true);
SaveGame::set_level_complete(levelIndex);
SaveGame::get_total_level_ups();
```
- Saves to EEPROM 4k blocks
- Use `purge_save()` for testing

## Common Patterns & Conventions

### C++ in N64 Environment
- C++20 with `-fno-exceptions` (exceptions disabled)
- STL containers OK: `std::vector`, `std::array`
- Avoid excessive allocations - prefer object pools
- Use `debugf()` for USB logging

### Include Structure
```cpp
#include <libdragon.h>     // N64 SDK
#include <t3d/t3d.h>       // Tiny3D core
#include <t3d/t3dmodel.h>  // Model loading
#include <t3d/tpx.h>       // Particle system
```

### Math Conventions
- Use `T3DVec3`, `T3DMat4` for float math
- Use `T3DMat4FP` for fixed-point matrices (RSP)
- Helper: `t3d_vec3_norm()`, `t3d_mat4_from_srt()`

### Viewport & Camera
```cpp
T3DViewport vp = t3d_viewport_create();
t3d_viewport_set_projection(vp, fov, near, far);
t3d_viewport_look_at(vp, pos, target, {{0, 1, 0}});
t3d_viewport_attach(vp);  // Make active for rendering
```

### Debug Features
- **Debug Menu**: Toggle with Start button (`debugMenu.cpp`)
- **USB Logging**: `debugf()` works with libdragon USB
- **L Button**: Level up all players (debug cheat)

## Dependencies & Environment
- **libdragon**: Preview branch required (not trunk)
- **Docker Support**: `libdragon exec ./build.sh`
- **Blender 4.0**: GLTF export (4.1+ has issues)
- **Fast64**: For optimal material/texture settings
- **RSPL**: For modifying RSP microcode (optional, ASM included)

## Documentation References
- `docs/modelFormat.md` - T3DM binary format details
- `docs/fast64Settings.md` - Supported Blender export settings
- `docs/modelOpt.md` - Vertex cache optimization details
- `last64_bloom/DESIGN.md` - Game design philosophy
- `last64_bloom/Readme.md` - HDR/Bloom integration guide
- `last64_bloom/TASKS.md` - Feature implementation tracker

## Common Gotchas
1. **Always use `malloc_uncached()` for RSP data** (matrices, vertices)
2. **UV coordinates are pixel-based**, not [0,1] normalized
3. **Vertices are packed in pairs** - struct holds 2 vertices
4. **Include t3d.mk in project Makefiles**: `include $(T3D_INST)/t3d.mk`
5. **Scene restarts reset actors but not audio** - design decision for continuous music
6. **Blender 4.1+ GLTF export broken** - stick to 4.0
7. **`data_cache_hit_writeback()` needed** if modifying uncached memory from cached code

## Testing & Deployment
- **Emulator**: `flatpak run dev.ares.ares ./Last64_Bloom.z64`
- **Real Hardware**: SC64 via `make sc64` target (network deploy)
- **ROM Header**: `N64_ROM_TITLE`, `N64_ED64ROMCONFIGFLAGS=-w eeprom4k`
