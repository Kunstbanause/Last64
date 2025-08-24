/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "spawn_manager.h"
#include "../main.h"
#include <libdragon.h>
#include <vector>
#include <algorithm>

namespace SpawnManager {
    // Wave configurations
    static WaveConfig waveConfigs[4]; // 3 waves + 1 boss wave
    static int currentWave = 0;
    static float waveTimer = 0.0f;
    static float spawnTimer = 0.0f;
    static bool initialized = false;
    static bool bossSpawned = false;
    
    // Player references for targeting
    static Actor::Player* players[4] = {nullptr, nullptr, nullptr, nullptr};
    
    // Initialize wave configurations
    void initializeWaves() {
        // Wave 1: Small, weak enemies
        waveConfigs[0].waveNumber = 1;
        waveConfigs[0].spawnInterval = 2.0f;  // Spawn every 2 seconds
        waveConfigs[0].baseEnemyCount = 5;
        waveConfigs[0].speedMultiplier = 1.0f;
        waveConfigs[0].healthMultiplier = 1;
        waveConfigs[0].enemySize = Actor::EnemySize::SMALL;
        waveConfigs[0].enemyColor = 0xFF0000FF; // Red
        waveConfigs[0].xpReward = 1;
        
        // Wave 2: Medium enemies with more health
        waveConfigs[1].waveNumber = 2;
        waveConfigs[1].spawnInterval = 0.5f;
        waveConfigs[1].baseEnemyCount = 50;
        waveConfigs[1].speedMultiplier = 2.5f;
        waveConfigs[1].healthMultiplier = 5;
        waveConfigs[1].enemySize = Actor::EnemySize::MEDIUM;
        waveConfigs[1].enemyColor = 0xFFFF00FF; // Yellow
        waveConfigs[1].xpReward = 2;
        
        // Wave 3: Large, fast enemies with high health
        waveConfigs[2].waveNumber = 3;
        waveConfigs[2].spawnInterval = 1.0f;  // Spawn every 1 second
        waveConfigs[2].baseEnemyCount = 20;
        waveConfigs[2].speedMultiplier = 1.5f;
        waveConfigs[2].healthMultiplier = 10;
        waveConfigs[2].enemySize = Actor::EnemySize::MEDIUM;
        waveConfigs[2].enemyColor = 0x00FFFFFF; // Cyan
        waveConfigs[2].xpReward = 3;
        
        // Boss Wave: Single large boss enemy
        waveConfigs[3].waveNumber = 4;
        waveConfigs[3].spawnInterval = 5.0f;  // Not used for boss
        waveConfigs[3].baseEnemyCount = 1;
        waveConfigs[3].speedMultiplier = 1.0f;
        waveConfigs[3].healthMultiplier = 100;
        waveConfigs[3].enemySize = Actor::EnemySize::LARGE;
        waveConfigs[3].enemyColor = 0xFF0000FF; // Red (Boss color)
        waveConfigs[3].xpReward = 10;
    }
    
    void initialize() {
        if (initialized) return;
        
        initializeWaves();
        currentWave = 0;
        waveTimer = 0.0f;
        spawnTimer = 0.0f;
        bossSpawned = false;
        
        initialized = true;
    }
    
    void setPlayers(Actor::Player* player1, Actor::Player* player2, Actor::Player* player3, Actor::Player* player4) {
        players[0] = player1;
        players[1] = player2;
        players[2] = player3;
        players[3] = player4;
    }
    
    int getCurrentWave() {
        return currentWave;
    }
    
    float getWaveTime() {
        return waveTimer;
    }
    
    const WaveConfig& getCurrentWaveConfig() {
        // Return the boss wave config if boss is spawned
        if (bossSpawned && currentWave >= 3) {
            return waveConfigs[3];
        }
        // Return current wave config, or last wave if we've gone beyond
        return waveConfigs[std::min(currentWave, 2)];
    }
    
    void update(float deltaTime, float roundTimer) {
        if (!initialized) return;
        
        // Update timers
        waveTimer += deltaTime;
        
        // Determine current wave based on total time (1 minute per wave)
        int newWave = (int)(roundTimer / 60.0f);
        if (newWave > 3) newWave = 3; // Cap at boss wave
        
        // If we've moved to a new wave, reset wave timer
        if (newWave > currentWave) {
            currentWave = newWave;
            waveTimer = 0.0f;
            spawnTimer = 0.0f;
            bossSpawned = false;
            
            // For boss wave, we only spawn the boss once
            if (currentWave >= 3) {
                bossSpawned = false; // Reset boss spawn flag for the boss wave
            }
        }
        
        
        // Get current wave config
        const WaveConfig& config = getCurrentWaveConfig();
        
        // Handle boss wave specially
        if (currentWave >= 3 && !bossSpawned) {
            // Spawn the boss
            Actor::Player* targetPlayer = nullptr;
            std::vector<Actor::Player*> alivePlayers;
            for (int i = 0; i < 4; i++) {
                if (players[i] && !players[i]->getIsDead()) {
                    alivePlayers.push_back(players[i]);
                }
            }
            
            if (!alivePlayers.empty()) {
                targetPlayer = alivePlayers[rand() % alivePlayers.size()];
                
                // Spawn boss at a random edge
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
                float speed = 15.0f * config.speedMultiplier;
                
                // Spawn enemy with the selected target player and parameters
                Actor::Enemy* boss = Actor::Enemy::spawn(pos, speed, targetPlayer, config.enemySize, config.enemyColor, config.xpReward);
                
                bossSpawned = true;
            }
            return; // Don't spawn regular enemies during boss wave
        }
        
        // Regular enemy spawning for waves 1-3
        spawnTimer += deltaTime;
        if (spawnTimer > config.spawnInterval) {
            spawnTimer = 0.0f;
            
            // Randomly select a target player from alive players
            Actor::Player* targetPlayer = nullptr;
            std::vector<Actor::Player*> alivePlayers;
            for (int i = 0; i < 4; i++) {
                if (players[i] && !players[i]->getIsDead()) {
                    alivePlayers.push_back(players[i]);
                }
            }
            
            if (!alivePlayers.empty()) {
                targetPlayer = alivePlayers[rand() % alivePlayers.size()];
                
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
                float speed = 20.0f * config.speedMultiplier;
                
                // Spawn enemy with the selected target player and parameters
                Actor::Enemy* enemy =
                        Actor::Enemy::spawn(pos, speed, targetPlayer, config.enemySize, config.enemyColor, config.xpReward);
            }
        }
    }
}