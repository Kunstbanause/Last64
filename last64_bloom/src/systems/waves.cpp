/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "waves.h"
#include "../main.h"

namespace Waves {
    
    // Level 0: Crimson Bloom - Classic wave progression with mixed enemy types
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
    } level0_waves[] = {
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
    
    // Level 1: Verdant Bloom - Focus on swarms and speed, more aggressive
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
    } level1_waves[] = {
    //Interval, max, speed, hp, enemySize,               enemyColor, xpMod, allowedSpawnEdges,          boss?, bossCount LinearMovement
        {0.6f,  -1,  1.2f, 1,    Actor::EnemySize::SMALL,  0x00FF00FF, 1, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Green swarm - faster start
        {0.3f,  -1,  2.0f, 2,    Actor::EnemySize::SMALL,  0x80FF80FF, 2, SpawnManager::SPAWN_EDGE_LEFT | SpawnManager::SPAWN_EDGE_RIGHT, false, 0}, // Side swarms
        {0.8f,   1,  1.5f, 120,  Actor::EnemySize::LARGE,  0x00FF00FF, 12,SpawnManager::SPAWN_EDGE_ALL, true, 1},  // Early boss
        {0.2f,  -1,  2.5f, 3,    Actor::EnemySize::SMALL,  0x40FF40FF, 2, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Fast green chaos
        {0.15f, -1,  3.5f, 2,    Actor::EnemySize::SMALL,  0x00FF80FF, 1, SpawnManager::SPAWN_EDGE_TOP, false, 0, true}, // Comet wave from top
        {0.4f,  -1,  3.0f, 4,    Actor::EnemySize::MEDIUM, 0x008040FF, 3, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Medium green
        {0.7f,   2,  2.0f, 180,  Actor::EnemySize::LARGE,  0x00C000FF, 18,SpawnManager::SPAWN_EDGE_LEFT | SpawnManager::SPAWN_EDGE_RIGHT, true, 2}, // Twin bosses
        {0.1f,  -1,  4.5f, 1,    Actor::EnemySize::SMALL,  0x60FF60FF, 1, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Ultra swarm
        {0.15f, -1,  5.5f, 2,    Actor::EnemySize::SMALL,  0x00FF00FF, 2, SpawnManager::SPAWN_EDGE_LEFT | SpawnManager::SPAWN_EDGE_RIGHT, false, 0}, // Speed walls
        {0.3f,  -1,  4.0f, 5,    Actor::EnemySize::MEDIUM, 0x40C040FF, 3, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Tough medium
        {0.6f,   3,  2.5f, 220,  Actor::EnemySize::LARGE,  0x00A000FF, 25,SpawnManager::SPAWN_EDGE_ALL, true, 3}, // Triple boss finale
        {0.08f, -1,  6.5f, 1,    Actor::EnemySize::SMALL,  0x20FF20FF, 1, SpawnManager::SPAWN_EDGE_ALL, false, 0}  // Endless green chaos
    };
    
    // Level 2: Rose Bloom - Mixed colors, alternating patterns, tactical challenge
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
    } level2_waves[] = {
    //Interval, max, speed, hp, enemySize,               enemyColor, xpMod, allowedSpawnEdges,          boss?, bossCount LinearMovement
        {0.5f,  -1,  1.5f, 2,    Actor::EnemySize::SMALL,  0xFF00FFFF, 2, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Pink swarm
        {0.25f, -1,  2.5f, 1,    Actor::EnemySize::SMALL,  0xFF80FFFF, 1, SpawnManager::SPAWN_EDGE_TOP | SpawnManager::SPAWN_EDGE_BOTTOM, false, 0, true}, // Vertical comets
        {0.4f,  -1,  2.0f, 3,    Actor::EnemySize::SMALL,  0xFF00AAFF, 2, SpawnManager::SPAWN_EDGE_LEFT | SpawnManager::SPAWN_EDGE_RIGHT, false, 0}, // Horizontal waves
        {0.8f,   2,  1.8f, 150,  Actor::EnemySize::LARGE,  0xFF0080FF, 15,SpawnManager::SPAWN_EDGE_ALL, true, 2},  // Early double boss
        {0.2f,  -1,  3.5f, 2,    Actor::EnemySize::SMALL,  0xFF60FFFF, 2, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Fast pink chaos
        {0.15f, 15,  4.0f, 1,    Actor::EnemySize::SMALL,  0xFFAAFFFF, 1, SpawnManager::SPAWN_EDGE_LEFT, false, 0, true}, // Left comet burst
        {0.35f, -1,  3.0f, 5,    Actor::EnemySize::MEDIUM, 0xC000C0FF, 3, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Purple medium
        {0.15f, 15,  4.0f, 1,    Actor::EnemySize::SMALL,  0xFFAAFFFF, 1, SpawnManager::SPAWN_EDGE_RIGHT, false, 0, true}, // Right comet burst
        {0.2f,  -1,  4.5f, 3,    Actor::EnemySize::SMALL,  0xFF40FFFF, 2, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Ultra pink swarm
        {0.6f,   3,  2.2f, 200,  Actor::EnemySize::LARGE,  0xFF00C0FF, 20,SpawnManager::SPAWN_EDGE_TOP | SpawnManager::SPAWN_EDGE_BOTTOM, true, 3}, // Triple vertical bosses
        {0.12f, -1,  5.5f, 2,    Actor::EnemySize::SMALL,  0xFF80C0FF, 2, SpawnManager::SPAWN_EDGE_ALL, false, 0}, // Speed chaos
        {0.3f,  -1,  4.0f, 6,    Actor::EnemySize::MEDIUM, 0xA000A0FF, 4, SpawnManager::SPAWN_EDGE_LEFT | SpawnManager::SPAWN_EDGE_RIGHT, false, 0}, // Tough horizontal
        {0.5f,   4,  2.8f, 250,  Actor::EnemySize::LARGE,  0xFF0090FF, 30,SpawnManager::SPAWN_EDGE_ALL, true, 4}, // Quad boss finale
        {0.07f, -1,  7.0f, 1,    Actor::EnemySize::SMALL,  0xFF60C0FF, 1, SpawnManager::SPAWN_EDGE_ALL, false, 0}  // Ultimate pink chaos
    };
    
    // Get the number of waves for a specific level
    int getWaveCount(int levelIndex) {
        switch (levelIndex) {
            case 0: return sizeof(level0_waves) / sizeof(level0_waves[0]);
            case 1: return sizeof(level1_waves) / sizeof(level1_waves[0]);
            case 2: return sizeof(level2_waves) / sizeof(level2_waves[0]);
            default: return sizeof(level0_waves) / sizeof(level0_waves[0]); // Default to level 0
        }
    }
    
    // Initialize wave configurations for a specific level
    void initializeWaveConfigs(SpawnManager::WaveConfig* waveConfigs, int levelIndex) {
        int waveCount = getWaveCount(levelIndex);
        
        // Copy the wave data to the provided array based on level
        if (levelIndex == 1) {
            for (int i = 0; i < waveCount; i++) {
                waveConfigs[i].spawnInterval = level1_waves[i].spawnInterval;
                waveConfigs[i].spawnMaximum = level1_waves[i].spawnMaximum;
                waveConfigs[i].speedMultiplier = level1_waves[i].speedMultiplier;
                waveConfigs[i].healthMultiplier = level1_waves[i].healthMultiplier;
                waveConfigs[i].enemySize = level1_waves[i].enemySize;
                waveConfigs[i].enemyColor = level1_waves[i].enemyColor;
                waveConfigs[i].xpReward = level1_waves[i].xpReward;
                waveConfigs[i].allowedSpawnEdges = level1_waves[i].allowedSpawnEdges;
                waveConfigs[i].isBossWave = level1_waves[i].isBossWave;
                waveConfigs[i].bossCount = level1_waves[i].bossCount;
                waveConfigs[i].linearMovement = level1_waves[i].linearMovement;
            }
        } else if (levelIndex == 2) {
            for (int i = 0; i < waveCount; i++) {
                waveConfigs[i].spawnInterval = level2_waves[i].spawnInterval;
                waveConfigs[i].spawnMaximum = level2_waves[i].spawnMaximum;
                waveConfigs[i].speedMultiplier = level2_waves[i].speedMultiplier;
                waveConfigs[i].healthMultiplier = level2_waves[i].healthMultiplier;
                waveConfigs[i].enemySize = level2_waves[i].enemySize;
                waveConfigs[i].enemyColor = level2_waves[i].enemyColor;
                waveConfigs[i].xpReward = level2_waves[i].xpReward;
                waveConfigs[i].allowedSpawnEdges = level2_waves[i].allowedSpawnEdges;
                waveConfigs[i].isBossWave = level2_waves[i].isBossWave;
                waveConfigs[i].bossCount = level2_waves[i].bossCount;
                waveConfigs[i].linearMovement = level2_waves[i].linearMovement;
            }
        } else {
            // Default to level 0
            for (int i = 0; i < waveCount; i++) {
                waveConfigs[i].spawnInterval = level0_waves[i].spawnInterval;
                waveConfigs[i].spawnMaximum = level0_waves[i].spawnMaximum;
                waveConfigs[i].speedMultiplier = level0_waves[i].speedMultiplier;
                waveConfigs[i].healthMultiplier = level0_waves[i].healthMultiplier;
                waveConfigs[i].enemySize = level0_waves[i].enemySize;
                waveConfigs[i].enemyColor = level0_waves[i].enemyColor;
                waveConfigs[i].xpReward = level0_waves[i].xpReward;
                waveConfigs[i].allowedSpawnEdges = level0_waves[i].allowedSpawnEdges;
                waveConfigs[i].isBossWave = level0_waves[i].isBossWave;
                waveConfigs[i].bossCount = level0_waves[i].bossCount;
                waveConfigs[i].linearMovement = level0_waves[i].linearMovement;
            }
        }
    }
}