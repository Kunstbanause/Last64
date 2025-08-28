/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "spawn_manager.h"

namespace Waves {
    // Get the number of waves
    int getMaxWaves();
    
    // Initialize all wave configurations
    void initializeWaveConfigs(SpawnManager::WaveConfig* waveConfigs);
}