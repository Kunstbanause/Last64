/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "sceneLast64.h"
#include "../../actors/player.h"
#include "../../actors/enemy.h"
#include "../../actors/projectile.h"
#include "../../actors/xpShard.h"
#include "../../systems/experience.h"
#include "../../systems/spawn_manager.h"
#include "../../systems/waves.h"
#include "../../memory/savegame.h"
#include "../../systems/weapon_registry.h"
#include "../../main.h"
#include "../../render/colors.h"
#include "../../render/colorTest.h"
#include "../../debugMenu.h"
#include "../../ui/mainMenu.h"
#include "../../utils/profiler.h"
#include <libdragon.h>
#include <t3d/t3d.h>
#include <cmath>

namespace {
  // Static matrix for scene
  T3DMat4FP* sceneMatFP = nullptr;
  
  // Arena border rendering
  T3DVertPacked* borderVertices = nullptr;
  T3DMat4FP* borderMatrix = nullptr;
}

// Debug menu flag for marble background (extern so debugMenu can access)
bool showMarbleBackground = true;
bool marbleBackgroundChanged = false;

// Flag to track if a round is active (for debug features like XP spawn)
bool isRoundCurrentlyActive = false;

SceneLast64::SceneLast64()
{
    // Check if this is a restart (players were already playing) vs first launch
    // If we're restarting from game over, skip main menu and go to waiting for players
    static bool isFirstLaunch = true;
    
    if (isFirstLaunch) {
        currentGameState = MAIN_MENU;
        isFirstLaunch = false;
    } else {
        // This is a restart - skip main menu
        currentGameState = WAITING_FOR_PLAYERS;
    }
    
    isRoundCurrentlyActive = false; // Reset round state on scene init
    MainMenu::initialize();  // Initialize the main menu
    for (int i = 0; i < 4; ++i) {
        playerJoined[i] = false;
    }
    activePlayerCount = 0;
    roundTimer = 0.0f;
    exposure = 30.0f; // Set exposure for HDR effect
    marbleTime = 0.0f;
    restartRequested = false; // Scene restart flag for game over

    // Set up camera
    camera.fov = T3D_DEG_TO_RAD(80.0f);
    camera.near = 5.0f;
    camera.far = 500.0f; // Increased to accommodate larger scene
    // Position camera to look at the center of the screen from a reasonable distance
    camera.pos = {(float)(ARENA_RIGHT/2.0f), (float)(ARENA_BOTTOM/2.0f), 200.0f};
    camera.target = {(float)(ARENA_RIGHT/2.0f), (float)(ARENA_BOTTOM/2.0f), 0.0f};

    // Initialize scene matrix
    sceneMatFP = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(sceneMatFP);

    // Players are initialized to nullptr and created when they join
    player1 = nullptr;
    player2 = nullptr;
    player3 = nullptr;
    player4 = nullptr;
    
    // Initialize systems
    Actor::Enemy::initialize();
    Actor::Projectile::initialize();
    Actor::Shape::initialize();
    Actor::XPShard::initialize();
    Actor::EnemyDeathVFX::initialize();
    SpawnManager::initialize();
    
    // Initialize weapon icons
    WeaponIcons::init();
    
    // Initialize marble background from save game
    showMarbleBackground = SaveGame::is_marble_enabled();
    
    // Register debug menu entries
    DebugMenu::addEntry({"Marble ", DebugMenu::EntryType::BOOL, &showMarbleBackground}, &marbleBackgroundChanged);
    
    // Initialize arena border (draw as 4 thin rectangles in 3D space)
    // We need 8 vertices (4 rectangles, each needs 2 T3DVertPacked structs for 4 verts)
    borderVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * 8);
    borderMatrix = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(borderMatrix);
    
    // Very dark grey color for border (doesn't blow out in HDR)
    uint32_t borderColor = 0x303030FF;  // Very dark grey to avoid HDR bloom
    T3DVec3 normalVec = {{0.0f, 0.0f, 1.0f}};
    uint16_t norm = t3d_vert_pack_normal(&normalVec);
    
    constexpr float BORDER_THICKNESS = 2.0f;
    
    // Top border (horizontal rectangle)
    // Vertex 0: top-left outer
    borderVertices[0].posA[0] = (int16_t)ARENA_LEFT;
    borderVertices[0].posA[1] = (int16_t)ARENA_TOP;
    borderVertices[0].posA[2] = 0;
    borderVertices[0].normA = norm;
    borderVertices[0].rgbaA = borderColor;
    // Vertex 1: top-right outer
    borderVertices[0].posB[0] = (int16_t)ARENA_RIGHT;
    borderVertices[0].posB[1] = (int16_t)ARENA_TOP;
    borderVertices[0].posB[2] = 0;
    borderVertices[0].normB = norm;
    borderVertices[0].rgbaB = borderColor;
    
    // Vertex 2: top-left inner
    borderVertices[1].posA[0] = (int16_t)ARENA_LEFT;
    borderVertices[1].posA[1] = (int16_t)(ARENA_TOP + BORDER_THICKNESS);
    borderVertices[1].posA[2] = 0;
    borderVertices[1].normA = norm;
    borderVertices[1].rgbaA = borderColor;
    // Vertex 3: top-right inner
    borderVertices[1].posB[0] = (int16_t)ARENA_RIGHT;
    borderVertices[1].posB[1] = (int16_t)(ARENA_TOP + BORDER_THICKNESS);
    borderVertices[1].posB[2] = 0;
    borderVertices[1].normB = norm;
    borderVertices[1].rgbaB = borderColor;
    
    // Bottom border (horizontal rectangle)
    // Vertex 4: bottom-left inner
    borderVertices[2].posA[0] = (int16_t)ARENA_LEFT;
    borderVertices[2].posA[1] = (int16_t)(ARENA_BOTTOM - BORDER_THICKNESS);
    borderVertices[2].posA[2] = 0;
    borderVertices[2].normA = norm;
    borderVertices[2].rgbaA = borderColor;
    // Vertex 5: bottom-right inner
    borderVertices[2].posB[0] = (int16_t)ARENA_RIGHT;
    borderVertices[2].posB[1] = (int16_t)(ARENA_BOTTOM - BORDER_THICKNESS);
    borderVertices[2].posB[2] = 0;
    borderVertices[2].normB = norm;
    borderVertices[2].rgbaB = borderColor;
    
    // Vertex 6: bottom-left outer
    borderVertices[3].posA[0] = (int16_t)ARENA_LEFT;
    borderVertices[3].posA[1] = (int16_t)ARENA_BOTTOM;
    borderVertices[3].posA[2] = 0;
    borderVertices[3].normA = norm;
    borderVertices[3].rgbaA = borderColor;
    // Vertex 7: bottom-right outer
    borderVertices[3].posB[0] = (int16_t)ARENA_RIGHT;
    borderVertices[3].posB[1] = (int16_t)ARENA_BOTTOM;
    borderVertices[3].posB[2] = 0;
    borderVertices[3].normB = norm;
    borderVertices[3].rgbaB = borderColor;
    
    // Left border (vertical rectangle)
    // Vertex 8: top-left outer
    borderVertices[4].posA[0] = (int16_t)ARENA_LEFT;
    borderVertices[4].posA[1] = (int16_t)ARENA_TOP;
    borderVertices[4].posA[2] = 0;
    borderVertices[4].normA = norm;
    borderVertices[4].rgbaA = borderColor;
    // Vertex 9: top-left inner
    borderVertices[4].posB[0] = (int16_t)(ARENA_LEFT + BORDER_THICKNESS);
    borderVertices[4].posB[1] = (int16_t)ARENA_TOP;
    borderVertices[4].posB[2] = 0;
    borderVertices[4].normB = norm;
    borderVertices[4].rgbaB = borderColor;
    
    // Vertex 10: bottom-left outer
    borderVertices[5].posA[0] = (int16_t)ARENA_LEFT;
    borderVertices[5].posA[1] = (int16_t)ARENA_BOTTOM;
    borderVertices[5].posA[2] = 0;
    borderVertices[5].normA = norm;
    borderVertices[5].rgbaA = borderColor;
    // Vertex 11: bottom-left inner
    borderVertices[5].posB[0] = (int16_t)(ARENA_LEFT + BORDER_THICKNESS);
    borderVertices[5].posB[1] = (int16_t)ARENA_BOTTOM;
    borderVertices[5].posB[2] = 0;
    borderVertices[5].normB = norm;
    borderVertices[5].rgbaB = borderColor;
    
    // Right border (vertical rectangle)
    // Vertex 12: top-right inner
    borderVertices[6].posA[0] = (int16_t)(ARENA_RIGHT - BORDER_THICKNESS);
    borderVertices[6].posA[1] = (int16_t)ARENA_TOP;
    borderVertices[6].posA[2] = 0;
    borderVertices[6].normA = norm;
    borderVertices[6].rgbaA = borderColor;
    // Vertex 13: top-right outer
    borderVertices[6].posB[0] = (int16_t)ARENA_RIGHT;
    borderVertices[6].posB[1] = (int16_t)ARENA_TOP;
    borderVertices[6].posB[2] = 0;
    borderVertices[6].normB = norm;
    borderVertices[6].rgbaB = borderColor;
    
    // Vertex 14: bottom-right inner
    borderVertices[7].posA[0] = (int16_t)(ARENA_RIGHT - BORDER_THICKNESS);
    borderVertices[7].posA[1] = (int16_t)ARENA_BOTTOM;
    borderVertices[7].posA[2] = 0;
    borderVertices[7].normA = norm;
    borderVertices[7].rgbaA = borderColor;
    // Vertex 15: bottom-right outer
    borderVertices[7].posB[0] = (int16_t)ARENA_RIGHT;
    borderVertices[7].posB[1] = (int16_t)ARENA_BOTTOM;
    borderVertices[7].posB[2] = 0;
    borderVertices[7].normB = norm;
    borderVertices[7].rgbaB = borderColor;
}

SceneLast64::~SceneLast64()
{
    delete player1;
    delete player2;
    delete player3;
    delete player4;
    Actor::Enemy::cleanup();
    Actor::Projectile::cleanup();
    Actor::Shape::cleanup();
    Actor::XPShard::cleanup();
    Actor::EnemyDeathVFX::cleanup();
    Experience::shutdown();
    SpawnManager::deinitialize();
    
    // Clean up weapon icons
    WeaponIcons::destroy();
    
    // Clean up main menu
    MainMenu::cleanup();
    
    // Clean up scene matrix
    if (sceneMatFP) {
        free_uncached(sceneMatFP);
        sceneMatFP = nullptr;
    }
    
    // Clean up border geometry
    if (borderVertices) {
        free_uncached(borderVertices);
        borderVertices = nullptr;
    }
    if (borderMatrix) {
        free_uncached(borderMatrix);
        borderMatrix = nullptr;
    }
}

void SceneLast64::updateScene(float deltaTime)
{
    // Add debug output to show the scene is updating
    static int updateCounter = 0;
    updateCounter++;
    if (updateCounter % 2500 == 0) {
        debugf("SceneLast64 updating... Counter: %d, State: %d\n", updateCounter, currentGameState);
    }
    
    // Update camera
    camera.update(deltaTime);
    camera.attach();
    
    switch (currentGameState) {
        case MAIN_MENU: {
            // Update main menu
            MainMenu::update(deltaTime);
            
            // Check if player wants to start the game
            if (MainMenu::shouldStartGame()) {
                currentGameState = WAITING_FOR_PLAYERS;
                MainMenu::reset();
            }
            break;
        }
        case WAITING_FOR_PLAYERS: {
            // Check for player input to join
            for (int i = 0; i < 4; ++i) {
                if (!playerJoined[i]) {
                    joypad_inputs_t inputs = joypad_get_inputs((joypad_port_t)(JOYPAD_PORT_1 + i));
                    if (inputs.btn.a || inputs.btn.z) {
                        playerJoined[i] = true;
                        // Create player instance
                        T3DVec3 startPos;
                        if (isForceAllPlayers) { // Debug spawn all players
                            startPos = {{ARENA_RIGHT/2.0f - 20.0f, ARENA_BOTTOM/2.0f, 0.0f}}; player1 = new Actor::Player(startPos, JOYPAD_PORT_1);
                            startPos = {{ARENA_RIGHT/2.0f        , ARENA_BOTTOM/2.0f, 0.0f}}; player2 = new Actor::Player(startPos, JOYPAD_PORT_2);
                            startPos = {{ARENA_RIGHT/2.0f + 20.0f, ARENA_BOTTOM/2.0f, 0.0f}}; player3 = new Actor::Player(startPos, JOYPAD_PORT_3);
                            startPos = {{ARENA_RIGHT/2.0f + 40.0f, ARENA_BOTTOM/2.0f, 0.0f}}; player4 = new Actor::Player(startPos, JOYPAD_PORT_4);
                            activePlayerCount = 4; // All players joined
                        }
                        else
                        {
                            switch (i) {
                                case 0: 
                                    startPos.x = (float)(ARENA_RIGHT/2.0f - 20.0f);
                                    startPos.y = (float)(ARENA_BOTTOM/2.0f);
                                    startPos.z = 0.0f;
                                    player1 = new Actor::Player(startPos, JOYPAD_PORT_1); 
                                    break;
                                case 1: 
                                    startPos.x = (float)(ARENA_RIGHT/2.0f);
                                    startPos.y = (float)(ARENA_BOTTOM/2.0f);
                                    startPos.z = 0.0f;
                                    player2 = new Actor::Player(startPos, JOYPAD_PORT_2); 
                                    break;
                                case 2: 
                                    startPos.x = (float)(ARENA_RIGHT/2.0f + 20.0f);
                                    startPos.y = (float)(ARENA_BOTTOM/2.0f);
                                    startPos.z = 0.0f;
                                    player3 = new Actor::Player(startPos, JOYPAD_PORT_3); 
                                    break;
                                case 3: 
                                    startPos.x = (float)(ARENA_RIGHT/2.0f + 40.0f);
                                    startPos.y = (float)(ARENA_BOTTOM/2.0f);
                                    startPos.z = 0.0f;
                                    player4 = new Actor::Player(startPos, JOYPAD_PORT_4); 
                                    break;
                            }
                            activePlayerCount++;
                            gSFXManager.play(SFXManager::SFX_START);
                        }
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
                isRoundCurrentlyActive = true;
                // Re-initialize Enemy and Projectile systems for a new round
                Actor::Enemy::initialize();
                Actor::Projectile::initialize();
                Actor::Shape::initialize();
                Actor::XPShard::initialize();
                // Initialize Experience system
                Experience::initialize();
                // Add all currently joined players to the Experience system
                if (player1) Experience::addPlayer(player1);
                if (player2) Experience::addPlayer(player2);
                if (player3) Experience::addPlayer(player3);
                if (player4) Experience::addPlayer(player4);
                // Restart background music when round starts
                gSFXManager.setVolume_Music(1.0f, 0.34f); // Set Volume to normal
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
                    if (inputs.btn.a || inputs.btn.z) {
                        playerJoined[i] = true;
                        // Create player instance
                        T3DVec3 startPos;
                        Actor::Player* newPlayer = nullptr;
                        switch (i) {
                            case 0: 
                                startPos.x = (float)(ARENA_RIGHT/2.0f - 20.0f);
                                startPos.y = (float)(ARENA_BOTTOM/2.0f);
                                startPos.z = 0.0f;
                                player1 = new Actor::Player(startPos, JOYPAD_PORT_1); 
                                newPlayer = player1; 
                                break;
                            case 1: 
                                startPos.x = (float)(ARENA_RIGHT/2.0f);
                                startPos.y = (float)(ARENA_BOTTOM/2.0f);
                                startPos.z = 0.0f;
                                player2 = new Actor::Player(startPos, JOYPAD_PORT_2); 
                                newPlayer = player2; 
                                break;
                            case 2: 
                                startPos.x = (float)(ARENA_RIGHT/2.0f + 20.0f);
                                startPos.y = (float)(ARENA_BOTTOM/2.0f);
                                startPos.z = 0.0f;
                                player3 = new Actor::Player(startPos, JOYPAD_PORT_3); 
                                newPlayer = player3; 
                                break;
                            case 3: 
                                startPos.x = (float)(ARENA_RIGHT/2.0f + 40.0f);
                                startPos.y = (float)(ARENA_BOTTOM/2.0f);
                                startPos.z = 0.0f;
                                player4 = new Actor::Player(startPos, JOYPAD_PORT_4); 
                                newPlayer = player4; 
                                break;
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

            // Debug input: Press L button to level up all players (for testing)
            for (int i = 0; i < 4; ++i) {
                joypad_buttons_t pressed = joypad_get_buttons_pressed((joypad_port_t)(JOYPAD_PORT_1 + i));
                if (pressed.l) {
                    // Add XP to trigger level up
                    Experience::addXP(Experience::getXToNextLevel());
                    break; // Only trigger once per frame
                }
                // Debug input: Press R button to skip to next wave
                if (pressed.r) {
                    // Skip to next wave by adjusting the round timer
                    int currentWave = SpawnManager::getCurrentWave();
                    roundTimer = (currentWave + 1) * SpawnManager::getWaveTimeMax() + 1.0f; // Jump to just after the next wave starts
                    break; // Only trigger once per frame
                }
                // Debug input: DPad down to spawn XP shards (only when debug menu is not visible)
                if (pressed.d_down && !Debug::isMenuVisible()) {
                    const int numShards = 20;
                    // Get first active player position as spawn center
                    Actor::Player* player = Experience::getPlayer(0);
                    T3DVec3 centerPos = player ? player->getPosition() : T3DVec3{{0.0f, 0.0f, 0.0f}};
                    
                    for (int j = 0; j < numShards; ++j) {
                        // Spawn in a radius around player/center
                        float randomX = centerPos.x + ((float)(rand() % 200) - 100.0f);  // ±100 units
                        float randomY = centerPos.y + ((float)(rand() % 200) - 100.0f);  // ±100 units
                        float randomZ = 5.0f + (float)(rand() % 20);  // Height: 5-25 units
                        T3DVec3 randomPos = {{randomX, randomY, randomZ}};
                        uint32_t randomColor = 0xFF000000 | (rand() & 0x00FFFFFF);  // Random color
                        Actor::XPShard::spawn(randomPos, 1, randomColor, 1.0f);
                    }
                    debugf("Spawned %d XP shards around player\n", numShards);
                    break; // Only trigger once per frame
                }
            }

            // Update players (this will also update their weapons)
            if (player1) player1->update(deltaTime);
            if (player2) player2->update(deltaTime);
            if (player3) player3->update(deltaTime);
            if (player4) player4->update(deltaTime);
            
            // Update spawn manager with player references
            SpawnManager::setPlayers(player1, player2, player3, player4);
            
            // Update spawn manager
            SpawnManager::update(deltaTime, roundTimer);

            // Check if return to main menu was requested from debug menu
            if (DebugMenu::isReturnToMainMenuRequested()) {
                // Close the debug menu immediately
                Debug::setMenuVisible(false);
                
                // Transition to main menu
                currentGameState = MAIN_MENU;
                MainMenu::reset();
                
                // Reset game state and end round immediately
                for (int i = 0; i < 4; ++i) {
                    playerJoined[i] = false;
                }
                isRoundCurrentlyActive = false;
                roundTimer = 0.0f; // Reset round timer
                Experience::initialize(); // Reset XP bar and level
                
                // Clean up all actors immediately
                Actor::Enemy::cleanup();
                Actor::Projectile::cleanup();
                Actor::Shape::cleanup();
                Actor::XPShard::cleanup();
                Actor::EnemyDeathVFX::cleanup();
                
                if (player1) { delete player1; player1 = nullptr; }
                if (player2) { delete player2; player2 = nullptr; }
                if (player3) { delete player3; player3 = nullptr; }
                if (player4) { delete player4; player4 = nullptr; }
                
                // Don't continue processing this frame
                break;
            }

            // Check for level complete: final wave survived and no active enemies
            if (SpawnManager::isFinalWaveCleared()) {
                currentGameState = LEVEL_COMPLETE;
                isRoundCurrentlyActive = false;
                // Save best time and mark level complete
                SaveGame::maybe_update_best_time((uint32_t)roundTimer);
                SaveGame::set_level_complete(0); // level index 0 for now
                // Play a level complete sound or effect
                gSFXManager.setVolume_Music(1.0f, 0.34f); // restore normal music volume
            }
            
            // Update all enemies
            { ProfileScope profile("Enemy"); Actor::Enemy::updateAll(deltaTime); }
            
            // Update all projectiles
            { ProfileScope profile("Proj"); Actor::Projectile::updateAll(deltaTime); }
            
            // Update all shapes
            { ProfileScope profile("Shape"); Actor::Shape::updateAll(deltaTime); }

            // Update all XP shards
            { ProfileScope profile("XP"); Actor::XPShard::updateAll(deltaTime); }

            // Update all enemy death VFX
            { ProfileScope profile("VFX"); Actor::EnemyDeathVFX::updateAll(deltaTime); }

            // --- Collision Detection ---
            {
                ProfileScope profile("Collision");
                
                // Enemy-Projectile Collision (optimized with early exits)
                {
                    ProfileScope profile("EnemyProj");
                    for (uint32_t i = 0; i < MAX_ENEMIES; ++i) {
                        if (!Actor::Enemy::isActive(i)) continue;
                        Actor::Enemy* enemy = Actor::Enemy::getEnemy(i);
                        if (!enemy || !enemy->isActive()) continue;

                        T3DVec3 enemyPos = enemy->getPosition();
                        float enemyRadius = enemy->getRadius();

                        for (uint32_t j = 0; j < MAX_PROJECTILES; ++j) {
                            Actor::Projectile* proj = Actor::Projectile::getProjectile(j);
                            if (!proj || !proj->isActive()) continue;

                            // Quick distance-squared check before full collision
                            T3DVec3 projPos = proj->getPosition();
                            float dx = enemyPos.x - projPos.x;
                            float dy = enemyPos.y - projPos.y;
                            float dz = enemyPos.z - projPos.z;
                            float distSq = dx*dx + dy*dy + dz*dz;
                            float sumRadius = enemyRadius + proj->getRadius();
                            
                            if (distSq <= sumRadius * sumRadius) {
                                enemy->takeDamage(proj->getDamage());
                                proj->deactivate();
                                gSFXManager.play(SFXManager::SFX_HIT);
                            }
                        }
                    }
                }

                // Player-Enemy Collision (optimized)
                {
                    ProfileScope profile("PlyrEnemy");
                    Actor::Player* players[4] = {player1, player2, player3, player4};
                    for (int p = 0; p < 4; ++p) {
                        Actor::Player* currentPlayer = players[p];
                        if (!currentPlayer || currentPlayer->getIsDead()) continue;

                        T3DVec3 playerPos = currentPlayer->getPosition();
                        float playerRadius = currentPlayer->getRadius();

                        for (uint32_t i = 0; i < MAX_ENEMIES; ++i) {
                            Actor::Enemy* enemy = Actor::Enemy::getEnemy(i);
                            if (!enemy || !enemy->isActive()) continue;

                            T3DVec3 enemyPos = enemy->getPosition();
                            float dx = playerPos.x - enemyPos.x;
                            float dy = playerPos.y - enemyPos.y;
                            float dz = playerPos.z - enemyPos.z;
                            float distSq = dx*dx + dy*dy + dz*dz;
                            float sumRadius = playerRadius + enemy->getRadius();
                            
                            if (distSq <= sumRadius * sumRadius) {
                                currentPlayer->takeDamage(1);
                            }
                        }
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
                isRoundCurrentlyActive = false;
                // Stop background music when game is over
                gSFXManager.setVolume_Music(0.45f, 0.1f); // Lower volume
                // gSFXManager.play(SFXManager::SFX_GAME_OVER); // Assuming a game over sound effect
                // Save best time for this run (roundTimer is in seconds)
                SaveGame::maybe_update_best_time((uint32_t)roundTimer);
                // If we reached the final wave (level complete) mark level as complete
                int currentWave = SpawnManager::getCurrentWave();
                int maxWaves = Waves::getWaveCount();
                if (currentWave >= maxWaves - 1) {
                    // For now level index 0
                    SaveGame::set_level_complete(0);
                }
            }
            break;
        }

        case GAME_OVER: {
            // Check for A to restart or B to return to main menu
            bool restartPressed = false;
            bool menuPressed = false;
            for (int i = 0; i < 4; ++i) {
                joypad_buttons_t pressed = joypad_get_buttons_pressed((joypad_port_t)(JOYPAD_PORT_1 + i));
                if (pressed.a || pressed.z) {
                    restartPressed = true;
                    break;
                }
                if (pressed.b) {
                    menuPressed = true;
                    break;
                }
            }

            if (restartPressed) {
                restartRequested = true; // Signal restart Scene
                // Restart background music when game restarts
                gSFXManager.setVolume_Music(1.0f, 0.34f); // Set Volume to normal
                // All cleanup and reset logic will be handled by SceneManager::loadScene(0)
                // and the SceneLast64 destructor/constructor.
            } else if (menuPressed) {
                // Return to main menu with proper cleanup
                currentGameState = MAIN_MENU;
                MainMenu::reset();
                
                // Reset game state
                for (int i = 0; i < 4; ++i) {
                    playerJoined[i] = false;
                }
                isRoundCurrentlyActive = false;
                roundTimer = 0.0f; // Reset round timer
                Experience::initialize(); // Reset XP bar and level
                
                // Clean up all actors
                Actor::Enemy::cleanup();
                Actor::Projectile::cleanup();
                Actor::Shape::cleanup();
                Actor::XPShard::cleanup();
                Actor::EnemyDeathVFX::cleanup();
                
                // Delete players
                if (player1) { delete player1; player1 = nullptr; }
                if (player2) { delete player2; player2 = nullptr; }
                if (player3) { delete player3; player3 = nullptr; }
                if (player4) { delete player4; player4 = nullptr; }
                
                // Restore normal music volume
                gSFXManager.setVolume_Music(1.0f, 0.34f);
            }
            // If neither, just stay in GAME_OVER state
            break;
        }
        case LEVEL_COMPLETE: {
            // Simple display state; wait for player to press A to continue/restart
            bool restartPressed = false;
            for (int i = 0; i < 4; ++i) {
                joypad_inputs_t inputs = joypad_get_inputs((joypad_port_t)(JOYPAD_PORT_1 + i));
                if (inputs.btn.a || inputs.btn.z) {
                    restartPressed = true;
                    break;
                }
            }
            if (restartPressed) {
                restartRequested = true;
            }
            break;
        }
    }
}

void SceneLast64::drawMarbleBackground(float deltaTime)
{
    marbleTime += deltaTime;

    rdpq_set_scissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    rdpq_set_mode_standard();
    rdpq_mode_zbuf(false, false);
    rdpq_mode_persp(false);
    rdpq_mode_filter(FILTER_BILINEAR);

    // Simplified grid with single-iteration distortion, similar-hue palette
    const int cellW = 12;
    const int cellH = 8;
    float phase = marbleTime * 0.45f;

    for (int y = 0; y < SCREEN_HEIGHT; y += cellH) {
        for (int x = 0; x < SCREEN_WIDTH; x += cellW) {
            float px = (float)x / (float)SCREEN_WIDTH;
            float py = (float)y / (float)SCREEN_HEIGHT;

            // Single distortion pass (minimal math)
            float angle = phase * 0.2f + sinf(py * 3.0f) * 0.3f;
            float c = cosf(angle);
            float s = sinf(angle);
            float px_new = px * c - py * s;
            float py_new = px * s + py * c;

            // Simple turbulence
            px_new += sinf(py_new * 2.5f + phase * 0.6f) * 0.08f;
            py_new += cosf(px_new * 2.0f + phase * 0.5f) * 0.08f;

            // Pattern: single oscillation
            float pattern = 0.5f + 0.5f * sinf(px_new * 3.5f + py_new * 2.8f + phase * 0.7f);

            // Balanced palette (dark gray with moderate reddish tint)
            uint8_t base_r = 55;
            uint8_t base_g = 25;
            uint8_t base_b = 28;
            uint8_t accent_r = 110;
            uint8_t accent_g = 45;
            uint8_t accent_b = 55;

            uint8_t r = base_r + (uint8_t)((accent_r - base_r) * pattern);
            uint8_t g = base_g + (uint8_t)((accent_g - base_g) * pattern);
            uint8_t b = base_b + (uint8_t)((accent_b - base_b) * pattern);

            rdpq_set_mode_fill(RGBA32(r, g, b, 0xFF));
            rdpq_fill_rectangle(x, y, x + cellW, y + cellH);
        }
    }

    rdpq_set_mode_standard();
}

void SceneLast64::drawArenaBorder()
{
    // Draw arena border in 3D space at Z=0 (where players and enemies are)
    // This will match the actual movement boundaries
    
    if (!borderVertices || !borderMatrix) return;
    
    // Draw border as 4 thin rectangles forming the outline
    t3d_matrix_push(borderMatrix);
    t3d_vert_load(borderVertices, 0, 16); // Load all 16 vertices (8 structs × 2 verts each)
    
    // Top border (vertices 0-3)
    t3d_tri_draw(0, 1, 2);
    t3d_tri_draw(2, 1, 3);
    
    // Bottom border (vertices 4-7)
    t3d_tri_draw(4, 5, 6);
    t3d_tri_draw(6, 5, 7);
    
    // Left border (vertices 8-11)
    t3d_tri_draw(8, 9, 10);
    t3d_tri_draw(10, 9, 11);
    
    // Right border (vertices 12-15)
    t3d_tri_draw(12, 13, 14);
    t3d_tri_draw(14, 13, 15);
    
    t3d_tri_sync();
    
    t3d_matrix_pop(1);
}

void SceneLast64::draw3D(float deltaTime)
{
    camera.attach();

    if (showMarbleBackground) {
        drawMarbleBackground(deltaTime);
    } else {
        // Clear screen to black when no background
        rdpq_set_mode_fill(RGBA32(32, 32, 32, 0xFF));
        rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        rdpq_set_mode_standard();
    }
    
    t3d_screen_clear_depth();
    // rdpq_set_env_color({0xFF, 0xAA, 0xEE, 0xAA}); //slightly see-through soft magenta

    t3d_light_set_ambient(Colors::colorAmbient);
    t3d_light_set_count(0); // No directional lights, just ambient
    
    // Set exposure for HDR effect
    t3d_light_set_exposure(exposure);

    t3d_matrix_push(sceneMatFP);

    // Set up rendering state
    //t3d_state_set_drawflags((enum T3DDrawFlags)(T3D_FLAG_SHADED | T3D_FLAG_DEPTH));
    
    // Set combiner mode to use vertex colors (SHADE) instead of textures
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    
    // Draw arena border in 3D space
    drawArenaBorder();
    
    // Draw all shapes
    Actor::Shape::drawAll(deltaTime);
    
    // Draw all XP shards
    Actor::XPShard::drawAll(deltaTime);
    
    // Draw all enemy death VFX
    Actor::EnemyDeathVFX::drawAll(deltaTime);

    // Draw all projectiles
    t3d_screen_clear_depth();
    Actor::Projectile::drawAll(deltaTime);

    // Draw all enemies
    t3d_screen_clear_depth();
    Actor::Enemy::drawAll(deltaTime);
    
    // Draw all players
    t3d_screen_clear_depth(); // Clear so the players are on top.
    if (player1) player1->draw3D(deltaTime);
    if (player2) player2->draw3D(deltaTime);
    if (player3) player3->draw3D(deltaTime);
    if (player4) player4->draw3D(deltaTime);

    // Draw color test strip only when debug menu is visible
    if (Debug::isMenuVisible()) {
        color_test_draw();
    }

    // Pop scene matrix
    t3d_matrix_pop(1);
}

void SceneLast64::draw2D(float deltaTime)
{
    switch (currentGameState) {
        case MAIN_MENU: {
            // Draw main menu
            MainMenu::draw();
            break;
        }
        case WAITING_FOR_PLAYERS: {
            // Display "Press A to join" in two columns: players 1-2 on the left, 3-4 on the right
            Debug::printf(25, 10, "Player 1 to 4: Press (A) to join");
            break;
        }
        case ROUND_ACTIVE: {
            // Draw player weapons overview
            // Draw player 1 and 2 weapons at top left
            Actor::Player* playersTopLeft[2] = {player1, player2};
            for (int i = 0; i < 2; ++i) {
                Actor::Player* currentPlayer = playersTopLeft[i];
                if (currentPlayer) {
                    // Display all player weapons using icons
                    auto& weapons = currentPlayer->getWeapons();
                    if (!weapons.empty()) {
                        // Draw player number
                        Debug::printf(10, 10 + (i * 20), "P%d", i + 1);
                        
                        // Draw weapon icons
                        float iconX = 35; // Start position for icons
                        float iconY = 10 + (i * 20) - 4; // Adjust Y position to center icons
                        for (size_t j = 0; j < weapons.size() && j < 3; ++j) {
                            if (weapons[j]) {
                                int level = weapons[j]->getUpgradeLevel();
                                Actor::WeaponType weaponType = weapons[j]->getWeaponType();
                                
                                // Draw the weapon icon
                                WeaponIcons::drawIcon(iconX, iconY, weaponType, level);
                                
                                // Move to next icon position
                                iconX += WeaponIcons::getIconWidth() + 2; // Add spacing between icons
                            }
                        }
                    } else {
                        Debug::printf(10, 10 + (i * 20), "P%d:None", i + 1);
                    }
                }
            }
            
            // Draw player 3 and 4 weapons at top right
            Actor::Player* playersTopRight[2] = {player3, player4};
            for (int i = 0; i < 2; ++i) {
                Actor::Player* currentPlayer = playersTopRight[i];
                if (currentPlayer) {
                    // Display all player weapons using icons
                    auto& weapons = currentPlayer->getWeapons();
                    if (!weapons.empty()) {
                        // Calculate position for top right corner
                        // Start from right side and work backwards
                        float baseX = SCREEN_WIDTH - 10; // Right edge padding
                        float yPosition = 10 + (i * 20); // Same vertical spacing as top left
                        
                        // Draw player number at the rightmost position
                        int playerNum = i + 3; // Player 3 or 4
                        int numWidth = 20; // Approximate width for "P3:" or "P4:"
                        Debug::printf(baseX - numWidth, yPosition, "P%d", playerNum);
                        
                        // Draw weapon icons to the left of the player number
                        float iconX = baseX - numWidth - 5; // Start position for icons (some padding from player number)
                        float iconY = yPosition - 4; // Adjust Y position to center icons
                        
                        // Draw up to 3 weapon icons (in reverse order to maintain visual order)
                        int iconsToDraw = (weapons.size() < 3) ? weapons.size() : 3;
                        for (int j = iconsToDraw - 1; j >= 0; --j) {
                            if (weapons[j]) {
                                int level = weapons[j]->getUpgradeLevel();
                                Actor::WeaponType weaponType = weapons[j]->getWeaponType();
                                
                                // Move icon position to the left for each icon
                                iconX -= WeaponIcons::getIconWidth();
                                
                                // Draw the weapon icon
                                WeaponIcons::drawIcon(iconX, iconY, weaponType, level);
                                
                                // Add spacing between icons (move further left)
                                iconX -= 2; // Spacing between icons
                            }
                        }
                    } else {
                        // Calculate position for "None" text
                        float yPosition = 10 + (i * 10);
                        Debug::printf(SCREEN_WIDTH - 50, yPosition, "P%d:None", i + 3);
                    }
                }
            }
            
            // Draw enemy and projectile counts
            if (Actor::Projectile::getActiveCount() >= 75 || Actor::Enemy::getActiveCount() >= 75 || Debug::isMenuVisible()) {
                Debug::printf(230, 200, "E:%d P:%d", Actor::Enemy::getActiveCount(), Actor::Projectile::getActiveCount());
            }

            // Draw current wave
            Debug::printf(SCREEN_WIDTH/2-20, 10, "Wave:%d", SpawnManager::getCurrentWave() + 1);

            // Draw round timer
            int minutes = (int)roundTimer / 60;
            int seconds = (int)roundTimer % 60;
            if (minutes > 0) {
                Debug::printf(142, 20, "%02d:%02d", minutes, seconds);
            } else {
                Debug::printf(150, 20, "%02d", seconds);
            }

            // Draw Level
            Debug::printf(10, SCREEN_HEIGHT-30, "Level:%d", Experience::getLevel());

            // Draw pending upgrade choices for players (so they render on top in 2D)
            Actor::Player* playersArr[4] = {player1, player2, player3, player4};
            for (int p = 0; p < 4; ++p) {
                Actor::Player* pl = playersArr[p];
                if (!pl) continue;
                if (!Experience::hasPendingChoice(pl)) continue;

                const auto& opts = Experience::getPendingOptions(pl);
                // Project player world position into screen space so labels follow camera
                T3DVec3 worldPos = pl->getPosition();
                T3DViewport* vp = t3d_viewport_get();
                T3DVec3 screenV = {{0,0,0}};
                t3d_viewport_calc_viewspace_pos(vp, &screenV, &worldPos);
                float sx = screenV.v[0];
                float sy = screenV.v[1] - 12.0f; // slightly above player

                // Draw pending choice icons. If there's only one choice, show only the A prompt.
                float iconW = WeaponIcons::getIconWidth();
                float iconH = WeaponIcons::getIconHeight();
                if (opts.size() == 1) {
                    const auto& o = opts[0];
                    Actor::WeaponType wt = Actor::WeaponType::PROJECTILE;
                    if (o.weapon) wt = o.weapon->getWeaponType();
                    // Center the single icon over the player
                    float ix = sx - (iconW/2.0f);
                    float iy = sy - (iconH/2.0f);
                    WeaponIcons::drawIcon(ix, iy, wt, (o.type == UpgradeSystem::UpgradeType::WEAPON_UPGRADE && o.weapon) ? o.weapon->getUpgradeLevel()+1 : 0);
                    // Draw A prompt to the left of the icon
                    Debug::printf(ix - 10.0f, iy + (iconH/2.0f) - 4.0f, "A");
                } else {
                    // Two choices: draw left and right icons. Mapping:
                    // opts[0] = left choice (selected by B), opts[1] = right choice (selected by A)
                    if (opts.size() > 0) {
                        const auto& o = opts[0];
                        Actor::WeaponType wt = Actor::WeaponType::PROJECTILE;
                        if (o.weapon) wt = o.weapon->getWeaponType();
                        float ix = sx - (iconW/2.0f) - 12.0f; // left offset
                        float iy = sy - (iconH/2.0f);
                        WeaponIcons::drawIcon(ix, iy, wt, (o.type == UpgradeSystem::UpgradeType::WEAPON_UPGRADE && o.weapon) ? o.weapon->getUpgradeLevel()+1 : 0);
                        // Draw B prompt to the left of the left icon
                        Debug::printf(ix - 10.0f, iy + (iconH/2.0f) - 4.0f, "B");
                    }
                    if (opts.size() > 1) {
                        const auto& o = opts[1];
                        Actor::WeaponType wt = Actor::WeaponType::PROJECTILE;
                        if (o.weapon) wt = o.weapon->getWeaponType();
                        float ix = sx - (iconW/2.0f) + 12.0f; // right offset
                        float iy = sy - (iconH/2.0f);
                        WeaponIcons::drawIcon(ix, iy, wt, (o.type == UpgradeSystem::UpgradeType::WEAPON_UPGRADE && o.weapon) ? o.weapon->getUpgradeLevel()+1 : 0);
                        // Draw A prompt to the right of the right icon
                        Debug::printf(ix + iconW + 2.0f, iy + (iconH/2.0f) - 4.0f, "A");
                    }
                }
            }

            break;
        }
        case GAME_OVER: {
            // Display "Game Over" message with restart and menu options
            Debug::printf(120, 100, "Game Over");
            Debug::printf(80, 130, "Press A to restart");
            Debug::printf(80, 150, "Press B for main menu");
            break;
        }
        case LEVEL_COMPLETE: {
            Debug::printf(120, 100, "Level Complete!");
            Debug::printf(100, 120, "Press A to continue");
            break;
        }
    }
}