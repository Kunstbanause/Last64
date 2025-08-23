/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "sceneLast64.h"
#include "../../main.h"
#include "../../debugMenu.h"
#include "../../render/debugDraw.h"
#include "../../systems/experience.h"
#include "../../systems/upgrade_system.h"
#include <t3d/t3d.h>
#include <t3d/tpx.h>
#include <t3d/t3dmath.h>
#include <libdragon.h>
#include <vector>
#include "../../audio.h"

namespace {
  // Ambient lighting
  constexpr uint8_t colorAmbient[4] = {0xC0, 0xB0, 0xA0, 0xFF};

  // Static matrix for scene
  T3DMat4FP* sceneMatFP = nullptr;
}

SceneLast64::SceneLast64()
{
    currentGameState = WAITING_FOR_PLAYERS;
    for (int i = 0; i < 4; ++i) {
        playerJoined[i] = false;
    }
    activePlayerCount = 0;
    roundTimer = 0.0f;
    exposure = 30.0f; // Set exposure for HDR effect
    restartRequested = false; // Scene restart flag for game over

    // Set up camera - match SceneBunker more closely
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
    delete player1;
    delete player2;
    delete player3;
    delete player4;
    Actor::Enemy::cleanup();
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
    // Add debug output to show the scene is updating
    static int updateCounter = 0;
    updateCounter++;
    if (updateCounter % 60 == 0) {
        debugf("SceneLast64 updating... Counter: %d, State: %d\n", updateCounter, currentGameState);
    }
    
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
                        bool DEBUG_SPAWN_ALL = false; // Debug spawn all players
                        if (DEBUG_SPAWN_ALL) {
                            startPos = {{120.0f, 100.0f, 0.0f}}; player1 = new Actor::Player(startPos, JOYPAD_PORT_1);
                            startPos = {{140.0f, 100.0f, 0.0f}}; player2 = new Actor::Player(startPos, JOYPAD_PORT_2);
                            startPos = {{160.0f, 100.0f, 0.0f}}; player3 = new Actor::Player(startPos, JOYPAD_PORT_3);
                            startPos = {{180.0f, 100.0f, 0.0f}}; player4 = new Actor::Player(startPos, JOYPAD_PORT_4);
                            activePlayerCount = 4; // All players joined
                        }
                        else
                        {
                            switch (i) {
                                case 0: startPos = {{120.0f, 100.0f, 0.0f}}; player1 = new Actor::Player(startPos, JOYPAD_PORT_1); break;
                                case 1: startPos = {{140.0f, 100.0f, 0.0f}}; player2 = new Actor::Player(startPos, JOYPAD_PORT_2); break;
                                case 2: startPos = {{160.0f, 100.0f, 0.0f}}; player3 = new Actor::Player(startPos, JOYPAD_PORT_3); break;
                                case 3: startPos = {{180.0f, 100.0f, 0.0f}}; player4 = new Actor::Player(startPos, JOYPAD_PORT_4); break;
                            }
                            activePlayerCount++;
                        }
                        
                        gSFXManager.play(SFXManager::SFX_START);

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
                            // Re-initialize Enemy and Projectile systems for a new round
                            Actor::Enemy::initialize();
                            Actor::Projectile::initialize();
                            // Initialize Experience system
                            Experience::initialize();
                            // Add all currently joined players to the Experience system
                            if (player1) Experience::addPlayer(player1);
                            if (player2) Experience::addPlayer(player2);
                            if (player3) Experience::addPlayer(player3);
                            if (player4) Experience::addPlayer(player4);
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
                        Actor::Player* newPlayer = nullptr;
                        switch (i) {
                            case 0: startPos = {{140.0f, 100.0f, 0.0f}}; player1 = new Actor::Player(startPos, JOYPAD_PORT_1); newPlayer = player1; break;
                            case 1: startPos = {{160.0f, 100.0f, 0.0f}}; player2 = new Actor::Player(startPos, JOYPAD_PORT_2); newPlayer = player2; break;
                            case 2: startPos = {{120.0f, 100.0f, 0.0f}}; player3 = new Actor::Player(startPos, JOYPAD_PORT_3); newPlayer = player3; break;
                            case 3: startPos = {{180.0f, 100.0f, 0.0f}}; player4 = new Actor::Player(startPos, JOYPAD_PORT_4); newPlayer = player4; break;
                        }
                        activePlayerCount++;
                        gSFXManager.play(SFXManager::SFX_JOIN);
                        if (newPlayer) {
                            Experience::addPlayer(newPlayer);
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
            // Enemy-Projectile Collision
            for (uint32_t i = 0; i < MAX_ENEMIES; ++i) {
                if (!Actor::Enemy::isActive(i)) continue;
                Actor::Enemy* enemy = Actor::Enemy::getEnemy(i);
                if (!enemy || !enemy->isActive()) continue;

                for (uint32_t j = 0; j < MAX_PROJECTILES; ++j) {
                    Actor::Projectile* proj = Actor::Projectile::getProjectile(j);
                    if (!proj || !proj->isActive()) continue;

                    if (enemy->collidesWith(proj)) {
                        enemy->takeDamage(proj->getDamage()); // Use projectile's damage value
                        proj->deactivate(); // Projectile disappears on hit
                        // Play hit sound effect
                        gSFXManager.play(SFXManager::SFX_HIT);
                    }
                }
            }

            // Player-Enemy Collision
            Actor::Player* players[4] = {player1, player2, player3, player4};
            for (int p = 0; p < 4; ++p) {
                Actor::Player* currentPlayer = players[p];
                if (!currentPlayer || currentPlayer->getIsDead()) continue; // Only check active, alive players

                for (uint32_t i = 0; i < MAX_ENEMIES; ++i) {
                    Actor::Enemy* enemy = Actor::Enemy::getEnemy(i);
                    if (!enemy || !enemy->isActive()) continue;

                    if (currentPlayer->collidesWith(enemy)) {
                        currentPlayer->takeDamage(1);
                        // gSFXManager.play(SFXManager::SFX_PLAYER_HIT); // Assuming a player hit sound effect
                    }
                }
            }

            // Recalculate active players for game over check
            int alivePlayers = 0;
            if (player1 && !player1->getIsDead()) alivePlayers++;
            if (player2 && !player2->getIsDead()) alivePlayers++;
            if (player3 && !player3->getIsDead()) alivePlayers++;
            if (player4 && !player4->getIsDead()) alivePlayers++;

            if (alivePlayers == 0 && activePlayerCount > 0) { // Ensure at least one player was active before game over
                currentGameState = GAME_OVER;
                // gSFXManager.play(SFXManager::SFX_GAME_OVER); // Assuming a game over sound effect
            }
            
            // Spawn new enemies occasionally
            const float enemySpawnInterval = 0.3f; // Spawn every 10 seconds
            static float enemySpawnTimer = enemySpawnInterval;
            enemySpawnTimer += deltaTime;
            if (enemySpawnTimer > enemySpawnInterval) { // Spawn an enemy every x seconds
                enemySpawnTimer = 0.0f;
                
                // Randomly select a target player from active players
                Actor::Player* targetPlayer = nullptr;
                if (activePlayerCount > 0) {
                    std::vector<Actor::Player*> activePlayersList;
                    if (player1) activePlayersList.push_back(player1);
                    if (player2) activePlayersList.push_back(player2);
                    if (player3) activePlayersList.push_back(player3);
                    if (player4) activePlayersList.push_back(player4);
                    targetPlayer = activePlayersList[rand() % activePlayersList.size()];
                }
                
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
                
                // Spawn enemy with the selected target player
                Actor::Enemy::spawn(pos, 45.0f, targetPlayer);
            }
            break;
        }

        case GAME_OVER: {
            // Check for START button press to restart
            bool restartPressed = false;
            for (int i = 0; i < 4; ++i) {
                joypad_inputs_t inputs = joypad_get_inputs((joypad_port_t)(JOYPAD_PORT_1 + i));
                if (inputs.btn.a) {
                    restartPressed = true;
                    break;
                }
            }

            if (restartPressed) {
                restartRequested = true; // Signal restart Scene
                // All cleanup and reset logic will be handled by SceneManager::loadScene(0)
                // and the SceneLast64 destructor/constructor.
            }
            // If not restarting, just stay in GAME_OVER state
        }
    }
}

void SceneLast64::draw3D(float deltaTime)
{
    camera.attach();
    
    t3d_screen_clear_color(RGBA32(32, 32, 32, 0xFF)); // Dark grey background
    t3d_screen_clear_depth();
    // rdpq_set_env_color({0xFF, 0xAA, 0xEE, 0xAA}); //slightly see-through soft magenta

    t3d_light_set_ambient(colorAmbient);
    t3d_light_set_count(0); // No directional lights, just ambient
    
    // Set exposure for HDR effect
    t3d_light_set_exposure(exposure);

    t3d_matrix_push(sceneMatFP);

    // Set up rendering state
    //t3d_state_set_drawflags((enum T3DDrawFlags)(T3D_FLAG_SHADED | T3D_FLAG_DEPTH));
    
    // Set combiner mode to use vertex colors (SHADE) instead of textures
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);

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
            // Draw player weapons overview
            for (int i = 0; i < 4; ++i) {
                Actor::Player* currentPlayer = nullptr;
                switch (i) {
                    case 0: currentPlayer = player1; break;
                    case 1: currentPlayer = player2; break;
                    case 2: currentPlayer = player3; break;
                    case 3: currentPlayer = player4; break;
                }
                if (currentPlayer) {
                    // Display player weapons in a compact format
                    auto& weapons = currentPlayer->getWeapons();
                    if (!weapons.empty()) {
                        // Show first weapon type and level
                        char weaponChar = '?';
                        int level = 0;
                        if (weapons[0]) {
                            level = weapons[0]->getUpgradeLevel();
                            
                            // Use first letter of weapon type as identifier
                            // P=Projectile, H=Homing, C=Circular
                            switch (weapons[0]->getWeaponType()) {
                                case Actor::WeaponType::PROJECTILE:
                                    weaponChar = 'P';
                                    break;
                                case Actor::WeaponType::HOMING:
                                    weaponChar = 'H';
                                    break;
                                case Actor::WeaponType::CIRCULAR:
                                    weaponChar = 'C';
                                    break;
                                default:
                                    weaponChar = 'W';
                                    break;
                            }
                        }
                        Debug::printf(10, 10 + (i * 10), "P%d:%c%d", i + 1, weaponChar, level);
                    } else {
                        Debug::printf(10, 10 + (i * 10), "P%d:None", i + 1);
                    }
                }
            }
            
            // Draw enemy and projectile counts
            Debug::printf(230, 10, "E:%d P:%d", Actor::Enemy::getActiveCount(), Actor::Projectile::getActiveCount());
            
            // Draw Level
            Debug::printf(10, 230-20, "(L): %d", Experience::getLevel());

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
        case GAME_OVER: {
            // Display "Game Over" message
            Debug::printf(120, 100, "Game Over");
            Debug::printf(100, 120, "Press A to restart");
            break;
        }
    }
}