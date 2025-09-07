/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "spawn_manager.h"
#include "waves.h"
#include "../main.h"
#include <libdragon.h>
#include <vector>
#include <algorithm>

namespace SpawnManager {
    // Wave configurations
    static WaveConfig* waveConfigs = nullptr; // Dynamic array for wave configs
    static int maxWaves = 0; // Will be set from Waves::getMaxWaves()
    static int currentWave = 0;
    static float waveTimer = 0.0f;
    static float waveTimerMax = 30.0f; // Each wave lasts 60 seconds
    static float spawnTimer = 0.0f;
    static bool initialized = false;
    static bool bossSpawned = false;
    static int enemiesSpawned = 0;  // Track number of enemies spawned in current wave
    
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
        Waves::initializeWaveConfigs(waveConfigs);
    }
    
    void initialize() {
        if (initialized) return;
        
        // Get the number of waves from the Waves module (statically known)
        maxWaves = Waves::getWaveCount();
        
        // Allocate memory for wave configs
        waveConfigs = new WaveConfig[maxWaves];
        
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

    float getWaveTimeMax() {
        return waveTimerMax;
    }
    
    const WaveConfig& getCurrentWaveConfig() {
        // Return current wave config, or last wave if we've gone beyond
        if (waveConfigs && maxWaves > 0) {
            return waveConfigs[std::min(currentWave, maxWaves-1)];
        }
        // This should never happen if properly initialized, but provide a safe fallback
        static WaveConfig defaultConfig = {};
        return defaultConfig;
    }
    
    void update(float deltaTime, float roundTimer) {
        if (!initialized) return;
        
        // Update timers
        waveTimer += deltaTime;
        
        // Determine current wave based on total time (1 minute per wave)
        int newWave = (int)(roundTimer / waveTimerMax);
        if (maxWaves > 0 && newWave > maxWaves - 1) newWave = maxWaves - 1; // Cap at final wave
        
        // If we've moved to a new wave, reset wave timer and counters
        if (newWave > currentWave) {
            currentWave = newWave;
            waveTimer = 0.0f;
            spawnTimer = 0.0f;
            enemiesSpawned = 0;
            bossSpawned = false;
        }
        
        // Get current wave config
        const WaveConfig& config = getCurrentWaveConfig();
        
        // Handle boss waves specially
        if (config.isBossWave) {
            if (!bossSpawned) {
                // Spawn the boss(es)
                Actor::Player* targetPlayer = getRandomAlivePlayer();
                
                if (targetPlayer) {
                    // Spawn multiple bosses if needed
                    for (int i = 0; i < config.bossCount; i++) {
                        // Spawn boss at a random edge (respecting allowed edges)
                        float spawnX, spawnY;
                        getRandomEdgeSpawnPosition(spawnX, spawnY, config.allowedSpawnEdges);
                        
                        T3DVec3 pos = {{spawnX, spawnY, 0.0f}};
                        float speed = 15.0f * config.speedMultiplier;
                        
                        // Spawn enemy with the selected target player and parameters
                        Actor::Enemy::spawn(pos, speed, targetPlayer, config.enemySize, config.enemyColor, config.xpReward, 8 * config.healthMultiplier);
                    }
                    
                    bossSpawned = true;
                }
            }
            // Don't spawn regular enemies during boss wave
            return;
        } else {
            // Regular enemy spawning for normal waves
            // Check if we've reached the maximum number of enemies for this wave
            // If spawnMaximum is -1, there's no limit (infinite enemies)
            if (config.spawnMaximum >= 0 && enemiesSpawned >= config.spawnMaximum) {
                return; // Don't spawn more enemies
            }
            
            spawnTimer += deltaTime;
            if (spawnTimer > config.spawnInterval) {
                spawnTimer = 0.0f;
                enemiesSpawned++; // Increment the enemy counter
                
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
        enemiesSpawned = 0;
        maxWaves = 0;

        // Clean up allocated wave configs
        if (waveConfigs) {
            delete[] waveConfigs;
            waveConfigs = nullptr;
        }

        // Clear player references
        for (int i = 0; i < 4; ++i) {
            players[i] = nullptr;
        }

        // Note: waveConfigs data is not dynamically allocated, so it does not need explicit clearing.
        // If waveConfigs held pointers or other complex types that required cleanup, those would be handled here.
    }
}