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
    const int maxWaves = 4; // 3 waves + 1 boss wave
    static WaveConfig waveConfigs[maxWaves];
    static int currentWave = 0;
    static float waveTimer = 0.0f;
    static float spawnTimer = 0.0f;
    static bool initialized = false;
    static bool bossSpawned = false;
    
    // Player references for targeting
    static Actor::Player* players[4] = {nullptr, nullptr, nullptr, nullptr};
    
    // Helper function to get a random spawn position at screen edge based on allowed edges
    static void getRandomEdgeSpawnPosition(float& spawnX, float& spawnY, int allowedEdges) {
        // Build a list of allowed edges
        std::vector<int> edges;
        if (allowedEdges & SPAWN_EDGE_TOP) edges.push_back(0);
        if (allowedEdges & SPAWN_EDGE_RIGHT) edges.push_back(1);
        if (allowedEdges & SPAWN_EDGE_BOTTOM) edges.push_back(2);
        if (allowedEdges & SPAWN_EDGE_LEFT) edges.push_back(3);
        
        // If no edges are allowed, default to all
        if (edges.empty()) {
            edges.push_back(0); // top
            edges.push_back(1); // right
            edges.push_back(2); // bottom
            edges.push_back(3); // left
        }
        
        // Select a random edge from allowed edges
        int edge = edges[rand() % edges.size()];
        
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
    }
    
    // Helper function to get a random alive player
    static Actor::Player* getRandomAlivePlayer() {
        std::vector<Actor::Player*> alivePlayers;
        for (int i = 0; i < 4; i++) {
            if (players[i] && !players[i]->getIsDead()) {
                alivePlayers.push_back(players[i]);
            }
        }
        
        if (!alivePlayers.empty()) {
            return alivePlayers[rand() % alivePlayers.size()];
        }
        return nullptr;
    }
    
    // Initialize wave configurations
    void initializeWaves() {
        // Wave 1: Small, weak enemies
        waveConfigs[0].waveNumber = 1;
        waveConfigs[0].spawnInterval = 1.6f;
        waveConfigs[0].baseEnemyCount = 5;
        waveConfigs[0].speedMultiplier = 1.0f;
        waveConfigs[0].healthMultiplier = 1;
        waveConfigs[0].enemySize = Actor::EnemySize::SMALL;
        waveConfigs[0].enemyColor = 0xFF0000FF; // Red
        waveConfigs[0].xpReward = 2;
        waveConfigs[0].allowedSpawnEdges = SPAWN_EDGE_ALL;
        
        // Wave 2: Swarm - Only spawn from left and right edges
        waveConfigs[1].waveNumber = 2;
        waveConfigs[1].spawnInterval = 0.4f;
        waveConfigs[1].baseEnemyCount = 55;
        waveConfigs[1].speedMultiplier = 2.0f;
        waveConfigs[1].healthMultiplier = 1;
        waveConfigs[1].enemySize = Actor::EnemySize::SMALL;
        waveConfigs[1].enemyColor = 0xFFFF00FF; // Yellow
        waveConfigs[1].xpReward = 1;
        waveConfigs[1].allowedSpawnEdges = SPAWN_EDGE_LEFT | SPAWN_EDGE_RIGHT;
        
        // Wave 3: Large, fast enemies with high health
        waveConfigs[2].waveNumber = 3;
        waveConfigs[2].spawnInterval = 1.2f;
        waveConfigs[2].baseEnemyCount = 15;
        waveConfigs[2].speedMultiplier = 1.5f;
        waveConfigs[2].healthMultiplier = 10;
        waveConfigs[2].enemySize = Actor::EnemySize::MEDIUM;
        waveConfigs[2].enemyColor = 0x00FFFFFF; // Cyan
        waveConfigs[2].xpReward = 3;
        waveConfigs[2].allowedSpawnEdges = SPAWN_EDGE_ALL;
        
        // Boss Wave: Single large boss enemy (Wave 4, but triggered at 3 minutes)
        waveConfigs[3].waveNumber = 4;
        waveConfigs[3].spawnInterval = 1.0f;  // Not used for boss
        waveConfigs[3].baseEnemyCount = 1;
        waveConfigs[3].speedMultiplier = 1.0f;
        waveConfigs[3].healthMultiplier = 100;
        waveConfigs[3].enemySize = Actor::EnemySize::LARGE;
        waveConfigs[3].enemyColor = 0xFF0000FF; // Red (Boss color)
        waveConfigs[3].xpReward = 10;
        waveConfigs[3].allowedSpawnEdges = SPAWN_EDGE_ALL;
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
        // Return current wave config, or last wave if we've gone beyond
        return waveConfigs[std::min(currentWave, maxWaves-1)];
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
            
            // Only reset bossSpawned flag if we're not entering the boss wave
            // or if we're moving between boss waves (which shouldn't happen with the current cap)
            if (currentWave < 3) {
                bossSpawned = false;
            }
        }
        
        
        // Get current wave config
        const WaveConfig& config = getCurrentWaveConfig();
        
        // Handle boss wave specially (spawn at exactly 3 minutes)
        if (roundTimer >= 180.0f) {
            if (!bossSpawned) {
                // Use boss wave config specifically for boss
                const WaveConfig& bossConfig = waveConfigs[3];
                
                // Spawn the boss
                Actor::Player* targetPlayer = getRandomAlivePlayer();
                
                if (targetPlayer) {
                    // Spawn boss at a random edge
                    float spawnX, spawnY;
                    getRandomEdgeSpawnPosition(spawnX, spawnY, bossConfig.allowedSpawnEdges);
                    
                    T3DVec3 pos = {{spawnX, spawnY, 0.0f}};
                    float speed = 15.0f * bossConfig.speedMultiplier;
                    
                    // Spawn enemy with the selected target player and parameters
                    Actor::Enemy::spawn(pos, speed, targetPlayer, bossConfig.enemySize, bossConfig.enemyColor, bossConfig.xpReward, 8 * bossConfig.healthMultiplier);
                    
                    bossSpawned = true;
                }
            }
            // Don't spawn regular enemies during boss wave
            return;
        } else {
            // Regular enemy spawning for waves 1-3
            spawnTimer += deltaTime;
            if (spawnTimer > config.spawnInterval) {
                spawnTimer = 0.0f;
                
                // Randomly select a target player from alive players
                Actor::Player* targetPlayer = getRandomAlivePlayer();
                
                if (targetPlayer) {
                    // Spawn a new enemy at a random edge of the screen
                    float spawnX, spawnY;
                    getRandomEdgeSpawnPosition(spawnX, spawnY, config.allowedSpawnEdges);
                    
                    T3DVec3 pos = {{spawnX, spawnY, 0.0f}};
                    float speed = 20.0f * config.speedMultiplier;
                    
                    // Spawn enemy with the selected target player and parameters
                    Actor::Enemy::spawn(pos, speed, targetPlayer, config.enemySize, config.enemyColor, config.xpReward, 8 * config.healthMultiplier);
                }
            }
        }
    }
    
    void deinitialize() {
        if (!initialized) return;

        // Reset all static variables to their initial state
        currentWave = 0;
        waveTimer = 0.0f;
        spawnTimer = 0.0f;
        initialized = false;
        bossSpawned = false;

        // Clear player references
        for (int i = 0; i < 4; ++i) {
            players[i] = nullptr;
        }

        // Note: waveConfigs data is not dynamically allocated, so it does not need explicit clearing.
        // If waveConfigs held pointers or other complex types that required cleanup, those would be handled here.
    }
}