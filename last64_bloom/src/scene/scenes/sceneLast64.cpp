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
#include "../../systems/roundStats.h"
#include "../../memory/savegame.h"
#include "../../systems/weapon_registry.h"
#include "../../main.h"
#include "../../render/colors.h"
#include "../../render/colorTest.h"
#include "../../render/backgroundMarble.h"
#include "../../debugMenu.h"
#include "../../ui/mainMenu.h"
#include "../../utils/profiler.h"
#include <libdragon.h>
#include <t3d/t3d.h>
#include <rdpq_mode.h>
#include <cmath>

namespace {
  // Static matrix for scene
  T3DMat4FP* sceneMatFP = nullptr;
  
  // Arena border rendering
  T3DVertPacked* borderVertices = nullptr;
  T3DMat4FP* borderMatrix = nullptr;
}

static BackgroundMarble::PaletteTheme themeForLevel(int levelIdx) {
        switch (levelIdx) {
                case 0: return BackgroundMarble::PaletteTheme::RED;   // existing burgundy
                case 1: return BackgroundMarble::PaletteTheme::GREEN; // deep green
                case 2: return BackgroundMarble::PaletteTheme::PINK;  // pink
                default: return BackgroundMarble::PaletteTheme::RED;
        }
}

// Debug menu flag for marble background (extern so debugMenu can access)
bool showMarbleBackground = true;

// Flag to track if a round is active (for debug features like XP spawn)
bool isRoundCurrentlyActive = false;

SceneLast64::SceneLast64()
{
    // Check if this is a restart (players were already playing) vs first launch
    static bool isFirstLaunch = true;
    
    if (isFirstLaunch) {
        currentGameState = MAIN_MENU;
        isFirstLaunch = false;
    } else {
        // This is a restart - skip main menu and go directly to round
        currentGameState = ROUND_ACTIVE;
    }
    
    isRoundCurrentlyActive = false; // Reset round state on scene init
    MainMenu::initialize();  // Initialize the main menu
    for (int i = 0; i < 4; ++i) {
        playerJoined[i] = false;
    }
    activePlayerCount = 0;
    pauseMenuSelection = 0;
    roundTimer = 0.0f;
    roundEnemiesDefeated = 0;
    roundDamageDealt = 0;
    exposure = 30.0f; // Set exposure for HDR effect
    restartRequested = false; // Scene restart flag for game over
    currentLevelIndex = 0;
    firstPlayerSide = -1;  // Track which side the first player joins on (-1 = no players yet)
    
    // Initialize background marble
    backgroundMarble = new BackgroundMarble();
    backgroundMarble->setTheme(BackgroundMarble::PaletteTheme::GOLD); // Warm gold main menu look

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
    SpawnManager::initialize(currentLevelIndex);
    
    // Initialize weapon icons
    WeaponIcons::init();
    
    // Initialize marble background from save game
    showMarbleBackground = SaveGame::is_marble_enabled();
    
    // Initialize arena border (draw as 4 thin rectangles in 3D space)
    // We need 8 vertices (4 rectangles, each needs 2 T3DVertPacked structs for 4 verts)
    borderVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * 8);
    borderMatrix = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(borderMatrix);
    
    // Soft translucent white border (uses alpha; blender set when drawing)
    const uint8_t borderR = 255;
    const uint8_t borderG = 255;
    const uint8_t borderB = 255;
    const uint8_t borderA = 85;   // ~33% alpha
    uint32_t borderColor = (borderR << 24) | (borderG << 16) | (borderB << 8) | borderA;
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
    delete backgroundMarble;
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

    auto exitToMainMenu = [this]() {
        currentGameState = MAIN_MENU;
        backgroundMarble->setTheme(BackgroundMarble::PaletteTheme::GOLD);
        MainMenu::reset();

        for (int i = 0; i < 4; ++i) {
            playerJoined[i] = false;
        }
        firstPlayerSide = -1;
        activePlayerCount = 0;
        isRoundCurrentlyActive = false;
        roundTimer = 0.0f;
        Experience::initialize();
        RoundStats::reset();

        Actor::Enemy::cleanup();
        Actor::Projectile::cleanup();
        Actor::Shape::cleanup();
        Actor::XPShard::cleanup();
        Actor::EnemyDeathVFX::cleanup();
        SpawnManager::deinitialize();

        if (player1) { delete player1; player1 = nullptr; }
        if (player2) { delete player2; player2 = nullptr; }
        if (player3) { delete player3; player3 = nullptr; }
        if (player4) { delete player4; player4 = nullptr; }
    };
    
    switch (currentGameState) {
        case MAIN_MENU: {
            // Update main menu
            MainMenu::update(deltaTime);
            
            // Check if player wants to start the game
            if (MainMenu::shouldStartGame()) {
                currentLevelIndex = MainMenu::getSelectedLevel();
                backgroundMarble->setTheme(themeForLevel(currentLevelIndex));
                currentGameState = ROUND_ACTIVE;
                MainMenu::reset();
                
                // Reset player joining state for new level
                for (int i = 0; i < 4; ++i) {
                    playerJoined[i] = false;
                }
                firstPlayerSide = -1;
                activePlayerCount = 0;
                roundEnemiesDefeated = 0;
                roundDamageDealt = 0;
                RoundStats::reset();
            }
            break;
        }
        case ROUND_ACTIVE: {
            // Pause toggle (Start button on any controller)
            for (int i = 0; i < 4; ++i) {
                joypad_buttons_t pressed = joypad_get_buttons_pressed((joypad_port_t)(JOYPAD_PORT_1 + i));
                if (pressed.start) {
                    currentGameState = PAUSED;
                    pauseMenuSelection = 0; // Default to Continue
                    // Keep current round state; just halt gameplay updates
                    break;
                }
            }
            if (currentGameState == PAUSED) {
                break; // Skip all gameplay updates while paused
            }

            // Check for debug end round request
            if (DebugMenu::isEndRoundRequested()) {
                currentGameState = LEVEL_COMPLETE;
                isRoundCurrentlyActive = false;
                backgroundMarble->setTheme(BackgroundMarble::PaletteTheme::RAINBOW);
                SaveGame::maybe_update_best_time((uint32_t)roundTimer);
                SaveGame::set_level_complete(currentLevelIndex);
                gSFXManager.setVolume_Music(1.0f, 0.34f);
                break;
            }

            // Check for player input to join (even during active round)
            for (int i = 0; i < 4; ++i) {
                if (!playerJoined[i]) {
                    joypad_inputs_t inputs = joypad_get_inputs((joypad_port_t)(JOYPAD_PORT_1 + i));
                    if (inputs.btn.a || inputs.btn.z) {
                        playerJoined[i] = true;
                        
                        // Track which side the first player joined on
                        if (firstPlayerSide == -1) {
                            // i: 0,2 = left side (P1, P3), 1,3 = right side (P2, P4)
                            firstPlayerSide = (i == 0 || i == 2) ? 0 : 1;
                        }
                        
                        // Create player instance
                        T3DVec3 startPos;
                        Actor::Player* newPlayer = nullptr;
                        
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
                        }
                        
                        // If this is the first player to join, initialize the round systems
                        if (!isRoundCurrentlyActive) {
                            isRoundCurrentlyActive = true;
                            // Re-initialize Enemy and Projectile systems for a new round
                            Actor::Enemy::initialize();
                            Actor::Projectile::initialize();
                            Actor::Shape::initialize();
                            Actor::XPShard::initialize();
                            Actor::EnemyDeathVFX::initialize();
                            // Re-initialize SpawnManager for a new round with current level
                            SpawnManager::deinitialize();
                            SpawnManager::initialize(currentLevelIndex);
                            // Initialize Experience system
                            Experience::initialize();
                            // Initialize RoundStats for damage tracking
                            RoundStats::reset();
                            // Restart background music when round starts
                            gSFXManager.setVolume_Music(1.0f, 0.34f); // Set Volume to normal
                            gSFXManager.play(SFXManager::SFX_START);
                        } else {
                            // Late join sound
                            gSFXManager.play(SFXManager::SFX_JOIN);
                        }
                        
                        // Add player to Experience system (works for both first join and late joins)
                        if (newPlayer) {
                            Experience::addPlayer(newPlayer);
                            RoundStats::markPlayerActive(newPlayer->getPlayerIndex(), true);
                        } else if (isForceAllPlayers) {
                            // Debug mode: add all players
                            if (player1) {
                                Experience::addPlayer(player1);
                                RoundStats::markPlayerActive(player1->getPlayerIndex(), true);
                            }
                            if (player2) {
                                Experience::addPlayer(player2);
                                RoundStats::markPlayerActive(player2->getPlayerIndex(), true);
                            }
                            if (player3) {
                                Experience::addPlayer(player3);
                                RoundStats::markPlayerActive(player3->getPlayerIndex(), true);
                            }
                            if (player4) {
                                Experience::addPlayer(player4);
                                RoundStats::markPlayerActive(player4->getPlayerIndex(), true);
                            }
                        }
                    }
                }
            }

            roundTimer += deltaTime;

            // Debug input: Press L button to level up all players (for testing)
            for (int i = 0; i < 4; ++i) {
                joypad_buttons_t pressed = joypad_get_buttons_pressed((joypad_port_t)(JOYPAD_PORT_1 + i));
                joypad_inputs_t inputs = joypad_get_inputs((joypad_port_t)(JOYPAD_PORT_1 + i));
                
                // Debug input: L+R to open debug menu (only when not already visible)
                if (inputs.btn.l && inputs.btn.r && !Debug::isMenuVisible()) {
                    Debug::setMenuVisible(true);
                    break; // Only trigger once per frame
                }
                if (pressed.l && !inputs.btn.r) {
                    // Add XP to trigger level up (only if R is not also pressed)
                    Experience::addXP(Experience::getXToNextLevel());
                    break; // Only trigger once per frame
                }
                // Debug input: Press R button to skip to next wave
                if (pressed.r && !inputs.btn.l) {
                    // Skip to next wave by adjusting the round timer (only if L is not also pressed)
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
            
            // Update spawn manager with player references only if round is active
            if (isRoundCurrentlyActive) {
                SpawnManager::setPlayers(player1, player2, player3, player4);
                SpawnManager::update(deltaTime, roundTimer);
            }

            // Check for level complete: final wave survived and no active enemies
            if (SpawnManager::isFinalWaveCleared()) {
                currentGameState = LEVEL_COMPLETE;
                isRoundCurrentlyActive = false;
                backgroundMarble->setTheme(BackgroundMarble::PaletteTheme::RAINBOW);
                // Save best time and mark level complete
                SaveGame::maybe_update_best_time((uint32_t)roundTimer);
                SaveGame::set_level_complete(currentLevelIndex);
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
                                int damageAmount = proj->getDamage();
                                enemy->takeDamage(damageAmount);
                                roundDamageDealt += damageAmount;
                                // Check if enemy died from this hit
                                if (!enemy->isActive()) {
                                    roundEnemiesDefeated++;
                                }
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
                backgroundMarble->setTheme(BackgroundMarble::PaletteTheme::GREY); // Switch to muted background for game over
                // Stop background music when game is over
                gSFXManager.setVolume_Music(0.45f, 0.1f); // Lower volume
                // gSFXManager.play(SFXManager::SFX_GAME_OVER); // Assuming a game over sound effect
                // Save best time for this run (roundTimer is in seconds)
                SaveGame::maybe_update_best_time((uint32_t)roundTimer);
                // If we reached the final wave (level complete) mark level as complete
                int currentWave = SpawnManager::getCurrentWave();
                int maxWaves = Waves::getWaveCount(currentLevelIndex);
                if (currentWave >= maxWaves - 1) {
                    // For now level index 0
                    SaveGame::set_level_complete(currentLevelIndex);
                }
            }
            break;
        }

        case PAUSED: {
            bool debugVisible = Debug::isMenuVisible();
            bool resumeRequested = false;
            bool exitRequested = false;

            // Handle debug menu interactions while paused
            if (debugVisible) {
                // While debug menu is visible, skip pause menu input handling
                break;
            }

            for (int i = 0; i < 4; ++i) {
                joypad_buttons_t pressed = joypad_get_buttons_pressed((joypad_port_t)(JOYPAD_PORT_1 + i));
                joypad_inputs_t inputs = joypad_get_inputs((joypad_port_t)(JOYPAD_PORT_1 + i));

                // L+R opens debug menu
                if (inputs.btn.l && inputs.btn.r && !debugVisible) {
                    Debug::setMenuVisible(true);
                    break;
                }

                if (pressed.d_up) pauseMenuSelection = (pauseMenuSelection > 0) ? pauseMenuSelection - 1 : 1;
                if (pressed.d_down) pauseMenuSelection = (pauseMenuSelection < 1) ? pauseMenuSelection + 1 : 0;

                if ((pressed.a || pressed.z) && !debugVisible) {
                    if (pauseMenuSelection == 0) resumeRequested = true;
                    else if (pauseMenuSelection == 1) exitRequested = true;
                }

                if (pressed.b && !debugVisible) {
                    // B resumes the game
                    resumeRequested = true;
                }

                if (pressed.start && !debugVisible) {
                    // Start resumes only from Continue option
                    if (pauseMenuSelection == 0) resumeRequested = true;
                }
            }

            pauseMenuSelection = (pauseMenuSelection < 0 || pauseMenuSelection > 1) ? 0 : pauseMenuSelection; // Clamp to [0,1]

            if (exitRequested) {
                exitToMainMenu();
                break;
            }
            if (resumeRequested) {
                currentGameState = ROUND_ACTIVE;
                break;
            }
            // Do not advance timers or update gameplay systems while paused
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
                // Restart the round without reloading the entire scene
                currentGameState = ROUND_ACTIVE;
                backgroundMarble->setTheme(themeForLevel(currentLevelIndex)); // Restore level theme when restarting
                
                // Reset game state
                for (int i = 0; i < 4; ++i) {
                    playerJoined[i] = false;
                }
                firstPlayerSide = -1;  // Reset first player side tracker
                activePlayerCount = 0;
                isRoundCurrentlyActive = false;
                roundTimer = 0.0f; // Reset round timer
                roundEnemiesDefeated = 0; // Reset defeated enemies counter
                roundDamageDealt = 0; // Reset damage dealt counter
                Experience::initialize(); // Reset XP bar and level
                RoundStats::reset(); // Reset damage tracking stats
                
                // Clean up all actors
                Actor::Enemy::cleanup();
                Actor::Projectile::cleanup();
                Actor::Shape::cleanup();
                Actor::XPShard::cleanup();
                Actor::EnemyDeathVFX::cleanup();
                SpawnManager::deinitialize();
                
                // Delete players so they can rejoin
                if (player1) { delete player1; player1 = nullptr; }
                if (player2) { delete player2; player2 = nullptr; }
                if (player3) { delete player3; player3 = nullptr; }
                if (player4) { delete player4; player4 = nullptr; }
                
                // Restart background music when restarting
                gSFXManager.setVolume_Music(1.0f, 0.34f); // Set Volume to normal
            } else if (menuPressed) {
                // Return to main menu with proper cleanup
                currentGameState = MAIN_MENU;
                MainMenu::reset();
                backgroundMarble->setTheme(BackgroundMarble::PaletteTheme::GOLD);
                
                // Reset game state
                for (int i = 0; i < 4; ++i) {
                    playerJoined[i] = false;
                }
                firstPlayerSide = -1;  // Reset first player side tracker
                isRoundCurrentlyActive = false;
                roundTimer = 0.0f; // Reset round timer
                Experience::initialize(); // Reset XP bar and level
                RoundStats::reset(); // Reset damage tracking stats
                
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
            // Level completed state; wait for player to press A (not Z) to return to main menu
            bool continuePressed = false;
            for (int i = 0; i < 4; ++i) {
                joypad_buttons_t pressed = joypad_get_buttons_pressed((joypad_port_t)(JOYPAD_PORT_1 + i));
                if (pressed.a) {  // Only A button, not Z
                    continuePressed = true;
                    break;
                }
            }
            if (continuePressed) {
                // Return to main menu after level completion
                exitToMainMenu();
                break;
            }
            break;
        }
    }
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
        backgroundMarble->draw(deltaTime);
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
    
    // Draw arena border in 3D space with alpha blending (only while round is active)
    if (currentGameState == ROUND_ACTIVE) {
        rdpq_mode_push();
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        drawArenaBorder();
        rdpq_mode_pop();
    }
    
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

    // Draw color test strip only when debug menu is visible and feature is enabled
    if (Debug::isMenuVisible() && !SaveGame::are_color_test_strips_disabled()) {
        color_test_draw();
    }

    // Pop scene matrix
    t3d_matrix_pop(1);
}

void SceneLast64::draw2D(float deltaTime)
{
    // If debug menu is visible, don't draw any UI elements - show clean 3D view only
    if (Debug::isMenuVisible()) {
        return;
    }
    
    switch (currentGameState) {
        case MAIN_MENU: {
            // Draw main menu
            MainMenu::draw();
            break;
        }
        case ROUND_ACTIVE: {
            // Show "Press A to join" on the opposite side from the first player
            if (activePlayerCount < 2 && firstPlayerSide != -1) {
                if (firstPlayerSide == 0) {
                    // First player on left (P1/P3), show prompt on right
                    Debug::printf(SCREEN_WIDTH - 120, 2, "Press A to join");
                } else {
                    // First player on right (P2/P4), show prompt on left
                    Debug::printf(10, 2, "Press A to join");
                }
            }
            // Draw player weapons overview
            // Draw player 1 and 3 weapons at top left
            Actor::Player* playersTopLeft[2] = {player1, player3};
            for (int i = 0; i < 2; ++i) {
                Actor::Player* currentPlayer = playersTopLeft[i];
                if (currentPlayer) {
                    // Display all player weapons using icons
                    auto& weapons = currentPlayer->getWeapons();
                    if (!weapons.empty()) {
                        // Draw player number
                        int playerNum = (i == 0) ? 1 : 3;  // P1 or P3
                        Debug::printf(10, 10 + (i * 20), "P%d", playerNum);
                        
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
                        int playerNum = (i == 0) ? 1 : 3;  // P1 or P3
                        Debug::printf(10, 10 + (i * 20), "P%d:None", playerNum);
                    }
                }
            }
            
            // Draw player 2 and 4 weapons at top right
            Actor::Player* playersTopRight[2] = {player2, player4};
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
                        int playerNum = (i == 0) ? 2 : 4;  // Player 2 or 4
                        int numWidth = 20; // Approximate width for "P2:" or "P4:"
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
                        int playerNum = (i == 0) ? 2 : 4;  // P2 or P4
                        Debug::printf(SCREEN_WIDTH - 50, yPosition, "P%d:None", playerNum);
                    }
                }
            }
            
            // Draw enemy and projectile counts
            if (Actor::Projectile::getActiveCount() >= 75 || Actor::Enemy::getActiveCount() >= 75 || Debug::isMenuVisible()) {
                Debug::printf(230, 200, "E:%d P:%d", Actor::Enemy::getActiveCount(), Actor::Projectile::getActiveCount());
            }

            // Draw current wave
            Debug::printf(SCREEN_WIDTH/2-20, 2, "Wave:%d", SpawnManager::getCurrentWave() + 1);

            // Draw round timer
            int minutes = (int)roundTimer / 60;
            int seconds = (int)roundTimer % 60;
            if (minutes > 0) {
                Debug::printf(142, 12, "%02d:%02d", minutes, seconds);
            } else {
                Debug::printf(150, 12, "%02d", seconds);
            }

            // Draw Level
            Debug::printf(SCREEN_WIDTH-60, SCREEN_HEIGHT-10, "Level:%d", Experience::getLevel());

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
        case PAUSED: {
            // Pause menu overlay with selectable entries
            int x = SCREEN_WIDTH/2 - 60;
            int y = SCREEN_HEIGHT/2 - 32;
            Debug::printf(x, y, "Paused");
            Debug::printf(x, y + 16, "%c Continue", pauseMenuSelection == 0 ? '>' : ' ');
            Debug::printf(x, y + 32, "%c Exit Round", pauseMenuSelection == 1 ? '>' : ' ');
            break;
        }
        case GAME_OVER: {
            // Display "Game Over" message with restart and menu options
            Debug::printf(120, 20, "Game Over");
            
            // Display round statistics
            int minutes = (int)roundTimer / 60;
            int seconds = (int)roundTimer % 60;
            
            Debug::printf(20, 40, "Time: %02d:%02d", minutes, seconds);
            Debug::printf(120, 40, "Kills: %d", Actor::Enemy::getTotalDeathCount());
            Debug::printf(200, 40, "Credits: %d", Experience::getCreditsEarnedThisRun());
            
            // DPS table with weapon icons for active players only
            float rowHeight = 40.0f;
            float firstRowY = 60.0f;
            float roundDuration = roundTimer > 0.0f ? roundTimer : 1.0f;
            
            int rowIdx = 0;
            for (int playerIdx = 0; playerIdx < 4; ++playerIdx) {
                if (!RoundStats::isPlayerActive(playerIdx)) continue;
                
                float rowY = firstRowY + rowIdx * rowHeight;
                
                // Player label (use color or index)
                Actor::Player* p = nullptr;
                switch (playerIdx) {
                    case 0: p = player1; break;
                    case 1: p = player2; break;
                    case 2: p = player3; break;
                    case 3: p = player4; break;
                }
                if (p) {
                    // Draw player indicator with color
                    Debug::printf(20, rowY, "P%d", playerIdx + 1);
                }
                
                // Draw weapon damage and DPS for each weapon type
                float colX = 80.0f;
                for (int wt = 0; wt < static_cast<int>(Actor::WeaponType::COUNT); ++wt) {
                    int dmg = RoundStats::getDamage(playerIdx, static_cast<Actor::WeaponType>(wt));
                    if (dmg > 0) {
                        // Draw weapon icon
                        WeaponIcons::drawIcon(colX, rowY - 5.0f, static_cast<Actor::WeaponType>(wt), 0);
                        // Draw damage value below icon
                        int dps = (int)(dmg / roundDuration);
                        Debug::printf(colX, rowY + 12.0f, "%d dmg", dmg);
                        Debug::printf(colX, rowY + 22.0f, "(%d dps)", dps);
                        colX += 70.0f;
                    }
                }
                
                rowIdx++;
            }
            
            Debug::printf(80, 220, "(A) to restart");
            // Debug::printf(80, 214, "(B) for main menu");
            break;
        }
        case LEVEL_COMPLETE: {
            Debug::printf(100, 20, "Level Complete!");
            
            // Display round statistics
            int minutes = (int)roundTimer / 60;
            int seconds = (int)roundTimer % 60;
            
            Debug::printf(20, 40, "Time: %02d:%02d", minutes, seconds);
            Debug::printf(120, 40, "Kills: %d", Actor::Enemy::getTotalDeathCount());
            Debug::printf(200, 40, "Credits: %d", Experience::getCreditsEarnedThisRun());
            
            // DPS table with weapon icons for active players only
            float rowHeight = 40.0f;
            float firstRowY = 60.0f;
            float roundDuration = roundTimer > 0.0f ? roundTimer : 1.0f;
            
            int rowIdx = 0;
            for (int playerIdx = 0; playerIdx < 4; ++playerIdx) {
                if (!RoundStats::isPlayerActive(playerIdx)) continue;
                
                float rowY = firstRowY + rowIdx * rowHeight;
                
                // Player label (use color or index)
                Actor::Player* p = nullptr;
                switch (playerIdx) {
                    case 0: p = player1; break;
                    case 1: p = player2; break;
                    case 2: p = player3; break;
                    case 3: p = player4; break;
                }
                if (p) {
                    // Draw player indicator with color
                    Debug::printf(20, rowY, "P%d", playerIdx + 1);
                }
                
                // Draw weapon damage and DPS for each weapon type
                float colX = 80.0f;
                for (int wt = 0; wt < static_cast<int>(Actor::WeaponType::COUNT); ++wt) {
                    int dmg = RoundStats::getDamage(playerIdx, static_cast<Actor::WeaponType>(wt));
                    if (dmg > 0) {
                        // Draw weapon icon
                        WeaponIcons::drawIcon(colX, rowY - 5.0f, static_cast<Actor::WeaponType>(wt), 0);
                        // Draw damage value below icon
                        int dps = (int)(dmg / roundDuration);
                        Debug::printf(colX, rowY + 12.0f, "%d", dmg);
                        Debug::printf(colX, rowY + 22.0f, "(%d dps)", dps);
                        colX += 70.0f;
                    }
                }
                
                rowIdx++;
            }
            
            Debug::printf(80, 220, "(A) to continue");
            break;
        }
    }
}