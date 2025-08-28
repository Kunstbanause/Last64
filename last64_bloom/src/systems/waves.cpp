/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "waves.h"
#include "../main.h"

namespace Waves {
    // Define all waves in an array for easier counting
    static const struct {
        float spawnInterval;
        int spawnMaximum;
        float speedMultiplier;
        int healthMultiplier;
        Actor::EnemySize enemySize;
        uint32_t enemyColor;
        int xpReward;
        int allowedSpawnEdges;
        bool isBossWave;
        int bossCount;
    } waveData[] = {
    //Interval, max, speed, hp, enemySize,               enemyColor, xpMod, allowedSpawnEdges,          boss?, bossCount
        {1.0f,  -1,  0.8f, 1,    Actor::EnemySize::SMALL,  0xFF0000FF, 2, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Small, weak enemies - infinite
        {0.4f,  -1,  1.4f, 1,    Actor::EnemySize::SMALL,  0xFFFF00FF, 1, SpawnManager::SPAWN_EDGE_LEFT | SpawnManager::SPAWN_EDGE_RIGHT, false, 0},// Swarm - Only spawn from left and right edges - infinite
        {1.2f,  -1,  1.8f, 4,    Actor::EnemySize::MEDIUM, 0xFF00FFFF, 2, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // More intense version of wave 1 - infinite
        {1.0f,   1,  1.0f, 100,  Actor::EnemySize::LARGE,  0xFF0000FF, 10, SpawnManager::SPAWN_EDGE_ALL, true, 1},  // Single large boss enemy
        {0.3f,  -1,  2.0f, 2,    Actor::EnemySize::SMALL,  0x00FF00FF, 1, SpawnManager::SPAWN_EDGE_TOP | SpawnManager::SPAWN_EDGE_BOTTOM, false, 0}, // More intense version of wave 2 - infinite
        {1.0f,   2,  1.2f, 150,  Actor::EnemySize::LARGE,  0x800080FF, 15, SpawnManager::SPAWN_EDGE_TOP | SpawnManager::SPAWN_EDGE_BOTTOM, true, 2}, // Two large boss enemies - spawn from top and bottom
        {0.12f, -1,  2.2f, 3,    Actor::EnemySize::SMALL,  0x00FF00FF, 1, SpawnManager::SPAWN_EDGE_ALL, false, 0} // Ender
    };
    
    // Get the number of waves (statically known)
    int getWaveCount() {
        return sizeof(waveData) / sizeof(waveData[0]);
    }
    
    // Initialize all wave configurations
    void initializeWaveConfigs(SpawnManager::WaveConfig* waveConfigs) {
        // Calculate wave count from the array size
        int waveCount = sizeof(waveData) / sizeof(waveData[0]);
        
        // Copy the wave data to the provided array
        for (int i = 0; i < waveCount; i++) {
            waveConfigs[i].spawnInterval = waveData[i].spawnInterval;
            waveConfigs[i].spawnMaximum = waveData[i].spawnMaximum;
            waveConfigs[i].speedMultiplier = waveData[i].speedMultiplier;
            waveConfigs[i].healthMultiplier = waveData[i].healthMultiplier;
            waveConfigs[i].enemySize = waveData[i].enemySize;
            waveConfigs[i].enemyColor = waveData[i].enemyColor;
            waveConfigs[i].xpReward = waveData[i].xpReward;
            waveConfigs[i].allowedSpawnEdges = waveData[i].allowedSpawnEdges;
            waveConfigs[i].isBossWave = waveData[i].isBossWave;
            waveConfigs[i].bossCount = waveData[i].bossCount;
        }
    }
}