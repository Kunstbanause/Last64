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
constexpr float ARENA_TOP = 0.0f;
constexpr float ARENA_LEFT = 0.0f;
constexpr float ARENA_BOTTOM = (float)SCREEN_HEIGHT;
constexpr float ARENA_RIGHT = (float)SCREEN_WIDTH;
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