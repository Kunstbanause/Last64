/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include <t3d/t3d.h>
#include "../actors/player.h"
#include "../actors/enemy.h"

namespace SpawnManager {
    // Wave configuration structure
    struct WaveConfig {
        int waveNumber;
        float spawnInterval;        // Time between enemy spawns
        int baseEnemyCount;         // Base number of enemies to spawn
        float speedMultiplier;      // Multiplier for enemy speed
        int healthMultiplier;       // Multiplier for enemy health
        Actor::EnemySize enemySize; // Size of enemies in this wave
        uint32_t enemyColor;        // Color of enemies in this wave
        int xpReward;              // XP reward for killing enemies in this wave
    };

    // Initialize the spawn manager
    void initialize();
    
    // Update the spawn manager (call every frame)
    void update(float deltaTime, float roundTimer);
    
    // Get the current wave number
    int getCurrentWave();
    
    // Get the time elapsed in the current wave
    float getWaveTime();
    
    // Get the total game time
    float getTotalTime();
    
    // Set the players for targeting
    void setPlayers(Actor::Player* player1, Actor::Player* player2, Actor::Player* player3, Actor::Player* player4);
    
    // Get the current wave configuration
    const WaveConfig& getCurrentWaveConfig();
    
    // Deinitialize the spawn manager
    void deinitialize();
}