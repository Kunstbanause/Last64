/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "spawn_manager.h"

namespace Waves {
    // Get the number of waves for a specific level
    int getWaveCount(int levelIndex);
    
    // Initialize wave configurations for a specific level
    void initializeWaveConfigs(SpawnManager::WaveConfig* waveConfigs, int levelIndex);
}