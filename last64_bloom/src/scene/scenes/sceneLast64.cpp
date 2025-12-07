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
#include <libdragon.h>
#include <t3d/t3d.h>
#include <cmath>

namespace {
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

    // Set up camera
    camera.fov = T3D_DEG_TO_RAD(80.0f);
    camera.near = 5.0f;
    camera.far = 500.0f; // Increased to accommodate larger scene
    // Position camera to look at the center of the screen from a reasonable distance
    camera.pos = {(float)(SCREEN_RIGHT/2.0f), (float)(SCREEN_BOTTOM/2.0f), 200.0f};
    camera.target = {(float)(SCREEN_RIGHT/2.0f), (float)(SCREEN_BOTTOM/2.0f), 0.0f};

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
    if (updateCounter % 2500 == 0) {
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
                    if (inputs.btn.a || inputs.btn.z) {
                        playerJoined[i] = true;
                        // Create player instance
                        T3DVec3 startPos;
                        if (isForceAllPlayers) { // Debug spawn all players
                            startPos = {{SCREEN_RIGHT/2.0f - 20.0f, SCREEN_BOTTOM/2.0f, 0.0f}}; player1 = new Actor::Player(startPos, JOYPAD_PORT_1);
                            startPos = {{SCREEN_RIGHT/2.0f        , SCREEN_BOTTOM/2.0f, 0.0f}}; player2 = new Actor::Player(startPos, JOYPAD_PORT_2);
                            startPos = {{SCREEN_RIGHT/2.0f + 20.0f, SCREEN_BOTTOM/2.0f, 0.0f}}; player3 = new Actor::Player(startPos, JOYPAD_PORT_3);
                            startPos = {{SCREEN_RIGHT/2.0f + 40.0f, SCREEN_BOTTOM/2.0f, 0.0f}}; player4 = new Actor::Player(startPos, JOYPAD_PORT_4);
                            activePlayerCount = 4; // All players joined
                        }
                        else
                        {
                            switch (i) {
                                case 0: 
                                    startPos.x = (float)(SCREEN_RIGHT/2.0f - 20.0f);
                                    startPos.y = (float)(SCREEN_BOTTOM/2.0f);
                                    startPos.z = 0.0f;
                                    player1 = new Actor::Player(startPos, JOYPAD_PORT_1); 
                                    break;
                                case 1: 
                                    startPos.x = (float)(SCREEN_RIGHT/2.0f);
                                    startPos.y = (float)(SCREEN_BOTTOM/2.0f);
                                    startPos.z = 0.0f;
                                    player2 = new Actor::Player(startPos, JOYPAD_PORT_2); 
                                    break;
                                case 2: 
                                    startPos.x = (float)(SCREEN_RIGHT/2.0f + 20.0f);
                                    startPos.y = (float)(SCREEN_BOTTOM/2.0f);
                                    startPos.z = 0.0f;
                                    player3 = new Actor::Player(startPos, JOYPAD_PORT_3); 
                                    break;
                                case 3: 
                                    startPos.x = (float)(SCREEN_RIGHT/2.0f + 40.0f);
                                    startPos.y = (float)(SCREEN_BOTTOM/2.0f);
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
                                startPos.x = (float)(SCREEN_RIGHT/2.0f - 20.0f);
                                startPos.y = (float)(SCREEN_BOTTOM/2.0f);
                                startPos.z = 0.0f;
                                player1 = new Actor::Player(startPos, JOYPAD_PORT_1); 
                                newPlayer = player1; 
                                break;
                            case 1: 
                                startPos.x = (float)(SCREEN_RIGHT/2.0f);
                                startPos.y = (float)(SCREEN_BOTTOM/2.0f);
                                startPos.z = 0.0f;
                                player2 = new Actor::Player(startPos, JOYPAD_PORT_2); 
                                newPlayer = player2; 
                                break;
                            case 2: 
                                startPos.x = (float)(SCREEN_RIGHT/2.0f + 20.0f);
                                startPos.y = (float)(SCREEN_BOTTOM/2.0f);
                                startPos.z = 0.0f;
                                player3 = new Actor::Player(startPos, JOYPAD_PORT_3); 
                                newPlayer = player3; 
                                break;
                            case 3: 
                                startPos.x = (float)(SCREEN_RIGHT/2.0f + 40.0f);
                                startPos.y = (float)(SCREEN_BOTTOM/2.0f);
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

            // Check for level complete: final wave survived and no active enemies
            if (SpawnManager::isFinalWaveCleared()) {
                currentGameState = LEVEL_COMPLETE;
                // Save best time and mark level complete
                SaveGame::maybe_update_best_time((uint32_t)roundTimer);
                SaveGame::set_level_complete(0); // level index 0 for now
                // Play a level complete sound or effect
                gSFXManager.setVolume_Music(1.0f, 0.34f); // restore normal music volume
            }
            
            // Update all enemies
            Actor::Enemy::updateAll(deltaTime);
            
            // Update all projectiles
            Actor::Projectile::updateAll(deltaTime);
            
            // Update all shapes
            Actor::Shape::updateAll(deltaTime);

            // Update all XP shards
            Actor::XPShard::updateAll(deltaTime);

            // Update all enemy death VFX
            Actor::EnemyDeathVFX::updateAll(deltaTime);

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
            // Check for START button press to restart
            bool restartPressed = false;
            for (int i = 0; i < 4; ++i) {
                joypad_inputs_t inputs = joypad_get_inputs((joypad_port_t)(JOYPAD_PORT_1 + i));
                if (inputs.btn.a || inputs.btn.z) {
                    restartPressed = true;
                    break;
                }
            }

            if (restartPressed) {
                restartRequested = true; // Signal restart Scene
                // Restart background music when game restarts
                gSFXManager.setVolume_Music(1.0f, 0.34f); // Set Volume to normal
                // All cleanup and reset logic will be handled by SceneManager::loadScene(0)
                // and the SceneLast64 destructor/constructor.
            }
            // If not restarting, just stay in GAME_OVER state
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

void SceneLast64::draw3D(float deltaTime)
{
    camera.attach();
    
    t3d_screen_clear_color(RGBA32(32, 32, 32, 0xFF)); // Dark grey background
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
            // Display "Game Over" message
            Debug::printf(120, 100, "Game Over");
            Debug::printf(100, 120, "Press A to restart");
            break;
        }
        case LEVEL_COMPLETE: {
            Debug::printf(120, 100, "Level Complete!");
            Debug::printf(100, 120, "Press A to continue");
            break;
        }
    }
}