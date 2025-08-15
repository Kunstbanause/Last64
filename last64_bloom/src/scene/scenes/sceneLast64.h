/**                                                                                                                                                                                                                                                     
* @copyright 2025 - Max Bebök                                                                                                                                                                                                                            
* @license MIT                                                                                                                                                                                                                                          
*/                                                                                                                                                                                                                                                      
#pragma once                                                                                                                                                                                                                                            
#include "../scene.h"                                                                                                                                                                                                                                   
#include "../../camera/camera.h"                                                                                                                                                                                                                        
#include "../../camera/staticCam.h"                                                                                                                                                                                                                     
#include "../../render/debugDraw.h"                                                                                                                                                                                                                     
#include "../../actors/player.h" // Include Player class                                                                                                                                                                                                
#include "../../actors/enemy.h"  // Include Enemy class                                                                                                                                                                                                 
#include "../../actors/projectile.h"                                                                                                                                                                                                                    
#include <t3d/t3d.h>                                                                                                                                                                                                                                    
#include <t3d/t3dmath.h>                                                                                                                                                                                                                                

class SceneLast64 : public Scene                                                                                                                                                                                                                        
{                                                                                                                                                                                                                                                       
  private:                                                                                                                                                                                                                                              
    enum GameState {
        WAITING_FOR_PLAYERS,
        ROUND_ACTIVE
    };

    GameState currentGameState;
    bool playerJoined[4];
    float roundTimer;
    float exposure; // HDR exposure value

    // Player instances                                                                                                                                                                                                                                
    Actor::Player* player1;                                                                                                                                                                                                                             
    Actor::Player* player2;
    Actor::Player* player3;
    Actor::Player* player4;
    int activePlayerCount;                                                                                                                                                                                                                             
                                                                                                                                                                                                                                                        
    StaticCam staticCam{camera};                                                                                                                                                                                                                        
                                                                                                                                                                                                                                                        
    void updateScene(float deltaTime) final;                                                                                                                                                                                                            
    void draw3D(float deltaTime) final;                                                                                                                                                                                                                 

  public:                                                                                                                                                                                                                                               
    void draw2D(float deltaTime) final;                                                                                                                                                                                                                 

    SceneLast64();                                                                                                                                                                                                                                      
    ~SceneLast64();                                                                                                                                                                                                                                     
};