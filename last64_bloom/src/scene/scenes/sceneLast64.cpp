/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "sceneLast64.h"
#include "../../main.h"
#include "../../debugMenu.h"
#include "../../render/debugDraw.h"
#include "../../systems/experience.h"
#include <t3d/t3d.h>
#include <t3d/tpx.h>
#include <t3d/t3dmath.h>
#include <libdragon.h>

namespace {
  // Screen boundaries
  constexpr float SCREEN_LEFT = 0.0f;
  constexpr float SCREEN_RIGHT = 312.0f;
  constexpr float SCREEN_TOP = 0.0f;
  constexpr float SCREEN_BOTTOM = 232.0f;
  constexpr float SCREEN_WIDTH = SCREEN_RIGHT - SCREEN_LEFT;
  constexpr float SCREEN_HEIGHT = SCREEN_BOTTOM - SCREEN_TOP;
  
  // Ambient lighting
  constexpr uint8_t colorAmbient[4] = {0xB0, 0xB0, 0xB0, 0xFF};

  // Static matrix for scene
  T3DMat4FP* sceneMatFP = nullptr;
}

SceneLast64::SceneLast64()
{
    currentGameState = WAITING_FOR_PLAYERS;
    for (int i = 0; i < 4; ++i) {
        playerJoined[i] = false;
    }
    roundTimer = 0.0f;

    // Set up camera - match SceneMain more closely
    camera.fov = T3D_DEG_TO_RAD(80.0f);
    camera.near = 5.0f;
    camera.far = 295.0f;
    camera.pos = {140.0f, 100.0f, 160.0f};
    camera.target = {140.0f, 100.0f, 0.0f};

    // Initialize scene matrix
    sceneMatFP = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(sceneMatFP);

    // Players are initialized to nullptr and created when they join
    player1 = nullptr;
    player2 = nullptr;
    player3 = nullptr;
    player4 = nullptr;
    
    // Initialize systems (without players for now)
    Actor::Enemy::initialize();
    Actor::Projectile::initialize();
}

SceneLast64::~SceneLast64()
{
    delete player1; // Clean up player1 instance
    delete player2; // Clean up player2 instance
    delete player3;
    delete player4;
    Actor::Enemy::cleanup(); // Clean up enemy pool
    Actor::Projectile::cleanup();
    Experience::shutdown();
    
    // Clean up scene matrix
    if (sceneMatFP) {
        free_uncached(sceneMatFP);
        sceneMatFP = nullptr;
    }
}

void SceneLast64::updateScene(float deltaTime)
{
    // Update camera
    camera.update(deltaTime);
    camera.attach();
    
    switch (currentGameState) {
        case WAITING_FOR_PLAYERS: {
            // Check for player input to join
            for (int i = 0; i < 4; ++i) {
                if (!playerJoined[i]) {
                    joypad_inputs_t inputs = joypad_get_inputs((joypad_port_t)(JOYPAD_PORT_1 + i));
                    if (inputs.btn.a) {
                        playerJoined[i] = true;
                        // Create player instance
                        T3DVec3 startPos;
                        switch (i) {
                            case 0: startPos = {{140.0f, 100.0f, 0.0f}}; player1 = new Actor::Player(startPos, JOYPAD_PORT_1); break;
                            case 1: startPos = {{160.0f, 100.0f, 0.0f}}; player2 = new Actor::Player(startPos, JOYPAD_PORT_2); break;
                            case 2: startPos = {{120.0f, 100.0f, 0.0f}}; player3 = new Actor::Player(startPos, JOYPAD_PORT_3); break;
                            case 3: startPos = {{180.0f, 100.0f, 0.0f}}; player4 = new Actor::Player(startPos, JOYPAD_PORT_4); break;
                        }

                        // If this is the first player to join, start the round
                        bool anyPlayerJoined = false;
                        for (int j = 0; j < 4; ++j) {
                            if (playerJoined[j]) {
                                anyPlayerJoined = true;
                                break;
                            }
                        }
                        if (anyPlayerJoined && currentGameState == WAITING_FOR_PLAYERS) {
                            currentGameState = ROUND_ACTIVE;
                            // Initialize Experience system with all players now that at least one has joined
                            Experience::initialize(player1, player2, player3, player4);
                        }
                    }
                }
            }
            break;
        }

                case ROUND_ACTIVE: {
            // Check for player input to join (even during active round)
            for (int i = 0; i < 4; ++i) {
                if (!playerJoined[i]) {
                    joypad_inputs_t inputs = joypad_get_inputs((joypad_port_t)(JOYPAD_PORT_1 + i));
                    if (inputs.btn.a) {
                        playerJoined[i] = true;
                        // Create player instance
                        T3DVec3 startPos;
                        switch (i) {
                            case 0: startPos = {{140.0f, 100.0f, 0.0f}}; player1 = new Actor::Player(startPos, JOYPAD_PORT_1); break;
                            case 1: startPos = {{160.0f, 100.0f, 0.0f}}; player2 = new Actor::Player(startPos, JOYPAD_PORT_2); break;
                            case 2: startPos = {{120.0f, 100.0f, 0.0f}}; player3 = new Actor::Player(startPos, JOYPAD_PORT_3); break;
                            case 3: startPos = {{180.0f, 100.0f, 0.0f}}; player4 = new Actor::Player(startPos, JOYPAD_PORT_4); break;
                        }
                    }
                }
            }

            roundTimer += deltaTime;

            // Update players (this will also update their weapons)
            if (player1) player1->update(deltaTime);
            if (player2) player2->update(deltaTime);
            if (player3) player3->update(deltaTime);
            if (player4) player4->update(deltaTime);
            
            // Update all enemies
            Actor::Enemy::updateAll(deltaTime);
            
            // Update all projectiles
            Actor::Projectile::updateAll(deltaTime);

            // --- Collision Detection ---
            for (uint32_t i = 0; i < MAX_ENEMIES; ++i) {
                Actor::Enemy* enemy = Actor::Enemy::getEnemy(i);
                if (!enemy || !enemy->isActive()) continue;

                for (uint32_t j = 0; j < MAX_PROJECTILES; ++j) {
                    Actor::Projectile* proj = Actor::Projectile::getProjectile(j);
                    if (!proj || !proj->isActive()) continue;

                    if (enemy->collidesWith(proj)) {
                        enemy->takeDamage(1);
                        proj->deactivate(); // Projectile disappears on hit
                    }
                }
            }
            
            // Get player positions for enemy positioning
            // T3DVec3 player1Pos = player1->getPosition();
            // T3DVec3 player2Pos = player2->getPosition();
            
            // Spawn new enemies occasionally
            static float enemySpawnTimer = 0.0f;
            enemySpawnTimer += deltaTime;
            if (enemySpawnTimer > 0.3f) { // Spawn an enemy every x seconds
                enemySpawnTimer = 0.0f;
                
                // Spawn a new enemy at a random edge of the screen
                float spawnX, spawnY;
                int edge = rand() % 4; // 0=top, 1=right, 2=bottom, 3=left
                
                switch (edge) {
                    case 0: // Top
                        spawnX = SCREEN_LEFT + (rand() % (int)SCREEN_WIDTH);
                        spawnY = SCREEN_TOP;
                        break;
                    case 1: // Right
                        spawnX = SCREEN_RIGHT;
                        spawnY = SCREEN_TOP + (rand() % (int)SCREEN_HEIGHT);
                        break;
                    case 2: // Bottom
                        spawnX = SCREEN_LEFT + (rand() % (int)SCREEN_WIDTH);
                        spawnY = SCREEN_BOTTOM;
                        break;
                    case 3: // Left
                        spawnX = SCREEN_LEFT;
                        spawnY = SCREEN_TOP + (rand() % (int)SCREEN_HEIGHT);
                        break;
                    default:
                        spawnX = 0;
                        spawnY = 0;
                        break;
                }
                
                T3DVec3 pos = {{spawnX, spawnY, 0.0f}};
                
                // Spawn enemy with zero initial velocity (will be calculated by enemy itself)
                // All actors exist in the same 3D space with Z=0 for the playing field
                // Randomly select a target player for this enemy
                Actor::Enemy::spawn(pos, 45.0f, player1, player2, player3, player4);
            }
            break;
        }
    }
}

void SceneLast64::draw3D(float deltaTime)
{
    // Attach camera
    camera.attach();
    
    // Clear screen with a dark grey color (similar to original but darker)
    t3d_screen_clear_color(RGBA32(32, 32, 32, 0xFF)); // Dark grey background
    // Clear depth buffer
    t3d_screen_clear_depth();
    
    // Set up environment color for bloom effect
    rdpq_set_env_color({0xFF, 0xAA, 0xEE, 0xAA});

    // Set up lighting to match SceneMain
    t3d_light_set_ambient(colorAmbient);
    t3d_light_set_count(0); // No directional lights, just ambient

    // Push scene matrix
    t3d_matrix_push(sceneMatFP);

    // Set up rendering state
    t3d_state_set_drawflags((enum T3DDrawFlags)(T3D_FLAG_SHADED | T3D_FLAG_DEPTH));

    // Draw players using the Player class (this will also draw their weapons)
    if (player1) player1->draw3D(deltaTime);
    if (player2) player2->draw3D(deltaTime);
    if (player3) player3->draw3D(deltaTime);
    if (player4) player4->draw3D(deltaTime);
    
    // Draw all enemies
    Actor::Enemy::drawAll(deltaTime);

    // Draw all projectiles
    Actor::Projectile::drawAll(deltaTime);

    // Pop scene matrix
    t3d_matrix_pop(1);
}

void SceneLast64::draw2D(float deltaTime)
{
    switch (currentGameState) {
        case WAITING_FOR_PLAYERS: {
            // Display "Press A to join" for each player
            for (int i = 0; i < 4; ++i) {
                if (!playerJoined[i]) {
                    Debug::printf(10, 10 + (i * 10), "P%d: Press A to join", i + 1);
                }
            }
            break;
        }
        case ROUND_ACTIVE: {
            // Draw player positions
            for (int i = 0; i < 4; ++i) {
                Actor::Player* currentPlayer = nullptr;
                switch (i) {
                    case 0: currentPlayer = player1; break;
                    case 1: currentPlayer = player2; break;
                    case 2: currentPlayer = player3; break;
                    case 3: currentPlayer = player4; break;
                }
                if (currentPlayer) {
                    T3DVec3 playerPos = currentPlayer->getPosition();
                    Debug::printf(10, 10 + (i * 10), "P%d:%.0f/%.0f", i + 1, playerPos.x, playerPos.y);
                }
            }
            
            // Draw enemy and projectile counts
            Debug::printf(230, 10, "E:%d P:%d", Actor::Enemy::getActiveCount(), Actor::Projectile::getActiveCount());

            // Draw XP Bar
            Experience::draw();

            // Draw round timer
            int minutes = (int)roundTimer / 60;
            int seconds = (int)roundTimer % 60;
            if (minutes > 0) {
                Debug::printf(150, 10, "%02d:%02d", minutes, seconds);
            } else {
                Debug::printf(150, 10, "%02d", seconds);
            }

            break;
        }
    }
}