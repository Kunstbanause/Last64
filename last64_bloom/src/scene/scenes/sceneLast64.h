/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "../scene.h"
#include "../../camera/camera.h"
#include "../../camera/staticCam.h"
#include "../../render/debugDraw.h"
#include "../../render/backgroundMarble.h"
#include "../../actors/player.h" // Include Player class
#include "../../actors/enemy.h"  // Include Enemy class
#include "../../actors/projectile.h"
#include "../../actors/shape.h"
#include "../../render/weaponIcons.h"
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>

class SceneLast64 : public Scene
{
  private:                                                                                                                                                                                                                                              
  enum GameState {
    MAIN_MENU,
    WAITING_FOR_PLAYERS,
    ROUND_ACTIVE,
    LEVEL_COMPLETE,
    GAME_OVER
  };

    GameState currentGameState;
    bool playerJoined[4];
    float roundTimer;
    float exposure; // HDR exposure value
    
    // Background effects
    BackgroundMarble* backgroundMarble;
    int currentLevelIndex;

    // Player instances                                                                                                                                                                                                                                
    Actor::Player* player1;                                                                                                                                                                                                                             
    Actor::Player* player2;
    Actor::Player* player3;
    Actor::Player* player4;
    int activePlayerCount;
    bool restartRequested; // Flag to signal restart to main loop
                                                                                                                                                                                                                                                        
    StaticCam staticCam{camera};
    
    void updateScene(float deltaTime) final;
    void draw3D(float deltaTime) final;
    void drawArenaBorder();  public:                                                                                                                                                                                                                                               
    void draw2D(float deltaTime) final;                                                                                                                                                                                                                 
    bool isRestartRequested() const { return restartRequested; }
    bool isRoundActive() const { return currentGameState == ROUND_ACTIVE; }
    void setLevelIndex(int idx) { currentLevelIndex = idx; }
    int getLevelIndex() const { return currentLevelIndex; }

    SceneLast64();                                                                                                                                                                                                                                      
    ~SceneLast64();                                                                                                                                                                                                                                     
};