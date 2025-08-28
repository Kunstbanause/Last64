/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "waves.h"
#include "../main.h"

namespace Waves {
    void initializeWaveConfigs(SpawnManager::WaveConfig* waveConfigs, int maxWaves) {
        // Wave 1: Small, weak enemies - infinite
        waveConfigs[0].waveNumber = 1;
        waveConfigs[0].spawnInterval = 1.6f;
        waveConfigs[0].spawnMaximum = -1;  // Infinite enemies
        waveConfigs[0].speedMultiplier = 1.0f;
        waveConfigs[0].healthMultiplier = 1;
        waveConfigs[0].enemySize = Actor::EnemySize::SMALL;
        waveConfigs[0].enemyColor = 0xFF0000FF; // Red
        waveConfigs[0].xpReward = 2;
        waveConfigs[0].allowedSpawnEdges = SpawnManager::SPAWN_EDGE_ALL;
        waveConfigs[0].isBossWave = false;
        waveConfigs[0].bossCount = 0;
        
        // Wave 2: Swarm - Only spawn from left and right edges - infinite
        waveConfigs[1].waveNumber = 2;
        waveConfigs[1].spawnInterval = 0.4f;
        waveConfigs[1].spawnMaximum = -1;  // Infinite enemies
        waveConfigs[1].speedMultiplier = 2.0f;
        waveConfigs[1].healthMultiplier = 1;
        waveConfigs[1].enemySize = Actor::EnemySize::SMALL;
        waveConfigs[1].enemyColor = 0xFFFF00FF; // Yellow
        waveConfigs[1].xpReward = 1;
        waveConfigs[1].allowedSpawnEdges = SpawnManager::SPAWN_EDGE_LEFT | SpawnManager::SPAWN_EDGE_RIGHT;
        waveConfigs[1].isBossWave = false;
        waveConfigs[1].bossCount = 0;
        
        // Wave 3: More intense version of wave 1 - infinite
        waveConfigs[2].waveNumber = 3;
        waveConfigs[2].spawnInterval = 1.2f;
        waveConfigs[2].spawnMaximum = -1;  // Infinite enemies
        waveConfigs[2].speedMultiplier = 1.8f;
        waveConfigs[2].healthMultiplier = 3;
        waveConfigs[2].enemySize = Actor::EnemySize::MEDIUM;
        waveConfigs[2].enemyColor = 0xFF00FFFF; // Magenta
        waveConfigs[2].xpReward = 2;
        waveConfigs[2].allowedSpawnEdges = SpawnManager::SPAWN_EDGE_ALL;
        waveConfigs[2].isBossWave = false;
        waveConfigs[2].bossCount = 0;
        
        // Wave 4: More intense version of wave 2 - infinite
        waveConfigs[3].waveNumber = 4;
        waveConfigs[3].spawnInterval = 0.3f;
        waveConfigs[3].spawnMaximum = -1;  // Infinite enemies
        waveConfigs[3].speedMultiplier = 2.5f;
        waveConfigs[3].healthMultiplier = 2;
        waveConfigs[3].enemySize = Actor::EnemySize::SMALL;
        waveConfigs[3].enemyColor = 0x00FF00FF; // Green
        waveConfigs[3].xpReward = 1;
        waveConfigs[3].allowedSpawnEdges = SpawnManager::SPAWN_EDGE_TOP | SpawnManager::SPAWN_EDGE_BOTTOM;
        waveConfigs[3].isBossWave = false;
        waveConfigs[3].bossCount = 0;
        
        // Boss Wave 1: Single large boss enemy
        waveConfigs[4].waveNumber = 5;
        waveConfigs[4].spawnInterval = 1.0f;  // Not used for boss
        waveConfigs[4].spawnMaximum = 1;
        waveConfigs[4].speedMultiplier = 1.0f;
        waveConfigs[4].healthMultiplier = 100;
        waveConfigs[4].enemySize = Actor::EnemySize::LARGE;
        waveConfigs[4].enemyColor = 0xFF0000FF; // Red (Boss color)
        waveConfigs[4].xpReward = 10;
        waveConfigs[4].allowedSpawnEdges = SpawnManager::SPAWN_EDGE_ALL;
        waveConfigs[4].isBossWave = true;
        waveConfigs[4].bossCount = 1;
        
        // Boss Wave 2: Two large boss enemies - spawn from top and bottom
        waveConfigs[5].waveNumber = 6;
        waveConfigs[5].spawnInterval = 1.0f;  // Not used for boss
        waveConfigs[5].spawnMaximum = 2;
        waveConfigs[5].speedMultiplier = 1.2f;
        waveConfigs[5].healthMultiplier = 150;
        waveConfigs[5].enemySize = Actor::EnemySize::LARGE;
        waveConfigs[5].enemyColor = 0x800080FF; // Purple (Second boss color)
        waveConfigs[5].xpReward = 15;
        waveConfigs[5].allowedSpawnEdges = SpawnManager::SPAWN_EDGE_TOP | SpawnManager::SPAWN_EDGE_BOTTOM;
        waveConfigs[5].isBossWave = true;
        waveConfigs[5].bossCount = 2;
    }
}