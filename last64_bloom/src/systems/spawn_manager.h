/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include <t3d/t3d.h>
#include "../actors/player.h"
#include "../actors/enemy.h"

namespace SpawnManager {

    // Enum for allowed spawn edges
    enum SpawnEdges {
        SPAWN_EDGE_TOP    = 1 << 0,  // 1
        SPAWN_EDGE_RIGHT  = 1 << 1,  // 2
        SPAWN_EDGE_BOTTOM = 1 << 2,  // 4
        SPAWN_EDGE_LEFT   = 1 << 3,  // 8
        SPAWN_EDGE_ALL    = SPAWN_EDGE_TOP | SPAWN_EDGE_RIGHT | SPAWN_EDGE_BOTTOM | SPAWN_EDGE_LEFT  // 15
    };

    // Enum for spawn method types
    enum SpawnMethod {
        SPAWN_RATE_BASED,    // Original: spawn one enemy every spawnInterval seconds
        SPAWN_FILL_SCREEN    // VS-style: maintain minimum enemy count, cap at MAX_ENEMIES
    };

    // Wave configuration structure
    struct WaveConfig {
        int waveNumber;
        float spawnInterval;        // Time between enemy spawns (RATE_BASED) or spawn attempt checks (FILL_SCREEN)
        int spawnMaximum;           // Maximum number of enemies to spawn (-1 for unlimited/infinite)
        float speedMultiplier;      // Multiplier for enemy speed
        int healthMultiplier;       // Multiplier for enemy health
        Actor::EnemySize enemySize; // Size of enemies in this wave
        uint32_t enemyColor;        // Color of enemies in this wave
        int xpReward;              // XP reward for killing enemies in this wave
        int allowedSpawnEdges;     // Bitmask of allowed spawn edges
        bool isBossWave;           // Whether this is a boss wave
        int bossCount;             // Number of bosses to spawn (for boss waves)
        bool linearMovement;       // If true, enemies from this wave will move in a fixed direction instead of chasing players
        SpawnMethod spawnMethod;   // Spawn method: SPAWN_RATE_BASED or SPAWN_FILL_SCREEN
        int minimumEnemies;        // For FILL_SCREEN: minimum enemy count to maintain on screen
    };

    // Initialize the spawn manager with level-specific waves
    void initialize(int levelIndex = 0);
    
    // Update the spawn manager (call every frame)
    void update(float deltaTime, float roundTimer);
    
    // Get the current wave number
    int getCurrentWave();
    
    // Get the time elapsed in the current wave
    float getWaveTime();

    // get Wave Time max
    float getWaveTimeMax();

    // Get the total game time
    float getTotalTime();
    
    // Set the players for targeting
    void setPlayers(Actor::Player* player1, Actor::Player* player2, Actor::Player* player3, Actor::Player* player4);
    
    // Get the current wave configuration
    const WaveConfig& getCurrentWaveConfig();

    // Returns true when the final wave has finished spawning and there are no active enemies
    bool isFinalWaveCleared();
    
    // Deinitialize the spawn manager
    void deinitialize();
}