/**                                                                                                                                                                                                                                                     
* @copyright 2025 - Max Bebök                                                                                                                                                                                                                           
* @license MIT                                                                                                                                                                                                                                          
*/                                                                                                                                                                                                                                                      
#pragma once                                                                                                                                                                                                                                            
#include <libdragon.h>                                                                                                                                                                                                                                  
#include "postProcess.h"                                                                                                                                                                                                                                

// Screen parameters
constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 240;
constexpr float SCREEN_TOP = 0.0f;
constexpr float SCREEN_LEFT = 0.0f;
constexpr float SCREEN_BOTTOM = (float)SCREEN_HEIGHT;
constexpr float SCREEN_RIGHT = (float)SCREEN_WIDTH;

// Arena/Map boundaries (gameplay area)
// These define the actual playable area where players can move
// Camera is at Z=200 looking down with 80° FOV
// At Z=0, the visible area is larger due to perspective projection
// Calculation: visible_height = 2 * tan(FOV/2) * distance
// visible_height = 2 * tan(40°) * 200 ≈ 335 units
// visible_width = visible_height * aspect_ratio = 335 * (4/3) ≈ 447 units

// Safe zone margins for each side (can be adjusted to fine-tune the arena)
constexpr float ARENA_MARGIN_TOP = 4.0f;
constexpr float ARENA_MARGIN_LEFT = 4.0f;
constexpr float ARENA_MARGIN_RIGHT = 40.0f;
constexpr float ARENA_MARGIN_BOTTOM = 30.0f;

// Base arena boundaries (calculated from screen size with perspective projection and margins)
constexpr float ARENA_TOP = 0.0f + ARENA_MARGIN_TOP;
constexpr float ARENA_LEFT = 0.0f + ARENA_MARGIN_LEFT;
constexpr float ARENA_BOTTOM = 335.0f - ARENA_MARGIN_BOTTOM;
constexpr float ARENA_RIGHT = 447.0f - ARENA_MARGIN_RIGHT;
constexpr float ARENA_WIDTH = ARENA_RIGHT - ARENA_LEFT;
constexpr float ARENA_HEIGHT = ARENA_BOTTOM - ARENA_TOP;

class Scene;

struct State
{
  PostProcessConf ppConf{};
  bool showOffscreen{};
  bool autoExposure{};

  Scene* activeScene{};
};

extern State state;

namespace Debug {
    bool isMenuVisible();
    void setMenuVisible(bool visible);
}