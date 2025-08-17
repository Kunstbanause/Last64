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

class Scene;

struct State
{
  PostProcessConf ppConf{};
  bool showOffscreen{};
  bool autoExposure{};

  Scene* activeScene{};
};

extern State state;