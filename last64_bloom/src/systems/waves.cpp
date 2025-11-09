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
        bool linearMovement;
    } waveData[] = {
    //Interval, max, speed, hp, enemySize,               enemyColor, xpMod, allowedSpawnEdges,          boss?, bossCount LinearMovement
        {1.0f,  -1,  0.8f, 1,    Actor::EnemySize::SMALL,  0xFF0000FF, 2, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Small, weak enemies - infinite
        {0.4f,  -1,  1.4f, 1,    Actor::EnemySize::SMALL,  0xFFFF00FF, 1, SpawnManager::SPAWN_EDGE_LEFT | SpawnManager::SPAWN_EDGE_RIGHT, false, 0},// Swarm - Only spawn from left and right edges - infinite
        {1.2f,  -1,  1.8f, 4,    Actor::EnemySize::MEDIUM, 0xFF00FFFF, 2, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // More intense version of wave 1 - infinite
        {1.0f,   1,  1.0f, 100,  Actor::EnemySize::LARGE,  0xFF0000FF, 10,SpawnManager::SPAWN_EDGE_ALL, true, 1},  // Single large boss enemy
        {0.1f,  -1,  4.0f, 2,    Actor::EnemySize::SMALL,  0x00FF00FF, 1, SpawnManager::SPAWN_EDGE_TOP, false, 0, true}, // More intense version of wave 2 - infinite (linear movement - comet style)
        {1.0f,   2,  1.2f, 150,  Actor::EnemySize::LARGE,  0x800080FF, 15,SpawnManager::SPAWN_EDGE_TOP | SpawnManager::SPAWN_EDGE_BOTTOM, true, 2}, // Two large boss enemies - spawn from top and bottom
        {0.12f, -1,  2.2f, 3,    Actor::EnemySize::SMALL,  0x00FF00FF, 1, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Ender
        {0.8f,  -1,  2.5f, 2,    Actor::EnemySize::SMALL,  0x0000FFFF, 2, SpawnManager::SPAWN_EDGE_LEFT, false, 0}, // Meter wave - enemies come from left side only
        {0.6f,  -1,  3.0f, 1,    Actor::EnemySize::SMALL,  0x0080FFFF, 1, SpawnManager::SPAWN_EDGE_RIGHT, false, 0}, // Fast meter wave - right side
        {0.2f,  -1,  4.0f, 1,    Actor::EnemySize::SMALL,  0x800000FF, 1, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Ultra fast swarm
        {0.5f,   1,  1.5f, 200,  Actor::EnemySize::LARGE,  0x004080FF, 20,SpawnManager::SPAWN_EDGE_ALL, true, 1},  // Fast boss
        {0.25f, 10,  3.5f, 3,    Actor::EnemySize::SMALL,  0x002040FF, 2, SpawnManager::SPAWN_EDGE_TOP | SpawnManager::SPAWN_EDGE_BOTTOM, false, 0}, // Intense swarm - top and bottom
        {0.25f, 10,  3.5f, 3,    Actor::EnemySize::SMALL,  0x002040FF, 2, SpawnManager::SPAWN_EDGE_BOTTOM, false, 0, true}, // Intense swarm - top and bottom (linear movement)
        {0.15f, -1,  5.0f, 1,    Actor::EnemySize::SMALL,  0x200040FF, 1, SpawnManager::SPAWN_EDGE_LEFT, false, 0}, // Ultra meter - left only
        {0.7f,   3,  1.8f, 180,  Actor::EnemySize::LARGE,  0x400080FF, 25,SpawnManager::SPAWN_EDGE_LEFT | SpawnManager::SPAWN_EDGE_RIGHT, true, 3}, // Triple fast boss
        {0.1f,  20,  6.0f, 1,    Actor::EnemySize::SMALL,  0x100020FF, 1, SpawnManager::SPAWN_EDGE_ALL, false, 0}  // Chaos wave - ultra fast with many enemies
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
            waveConfigs[i].linearMovement = waveData[i].linearMovement;
        }
    }
}