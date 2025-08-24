#include "experience.h"
#include "upgrade_system.h"
#include "../systems/weapon_base.h"
#include <libdragon.h>
#include <cmath>
#include <rdpq.h>
#include "../render/debugDraw.h"
#include "../audio.h"

namespace {
    constexpr int MAX_PLAYERS = 4;
    int currentXP = 0;
    int xpToNextLevel = 10;
    int currentLevel = 1;
    Actor::Player* activePlayers[MAX_PLAYERS];
    int activePlayerCount = 0;

    // Exponential growth factor for XP required for next level
    const float xpGrowthFactor = 1.25f;
}

void Experience::initialize() {
    currentXP = 0;
    xpToNextLevel = 10;
    currentLevel = 1;
    activePlayerCount = 0;
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        activePlayers[i] = nullptr;
    }
}

void Experience::shutdown() {
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        activePlayers[i] = nullptr;
    }
    activePlayerCount = 0;
}

void Experience::addPlayer(Actor::Player* player) {
    if (activePlayerCount < MAX_PLAYERS) {
        activePlayers[activePlayerCount++] = player;
    }
}

void Experience::removePlayer(Actor::Player* player) {
    for (int i = 0; i < activePlayerCount; ++i) {
        if (activePlayers[i] == player) {
            // Shift elements to fill the gap
            for (int j = i; j < activePlayerCount - 1; ++j) {
                activePlayers[j] = activePlayers[j+1];
            }
            activePlayers[activePlayerCount-1] = nullptr; // Clear the last element
            activePlayerCount--;
            break;
        }
    }
}

void Experience::addXP(int amount) {
    currentXP += amount;
    if (currentXP >= xpToNextLevel) {
        currentLevel++;
        currentXP -= xpToNextLevel;
        xpToNextLevel = static_cast<int>(xpToNextLevel * xpGrowthFactor);
        gSFXManager.play(SFXManager::SFX_LEVEL_UP);

        // Generate upgrade options for each player
        for (int i = 0; i < activePlayerCount; ++i) {
            if (activePlayers[i] && !activePlayers[i]->getIsDead()) {
                // Generate upgrade options
                auto upgradeOptions = UpgradeSystem::generateUpgradeOptions(activePlayers[i]);
                
                // If we have options, apply a random one
                if (!upgradeOptions.empty()) {
                    int randomIndex = rand() % upgradeOptions.size();
                    UpgradeSystem::applyUpgrade(activePlayers[i], upgradeOptions[randomIndex]);
                    
                    // Clean up any new weapon options that weren't selected
                    for (auto& option : upgradeOptions) {
                        if (option.type == UpgradeSystem::UpgradeType::NEW_WEAPON && 
                            option.weapon != upgradeOptions[randomIndex].weapon) {
                            delete option.weapon;
                        }
                    }
                }
            }
        }
    }
}

int Experience::getLevel() {
    return currentLevel;
}

float Experience::getXPPercentage() {
    return (float)currentXP / (float)xpToNextLevel;
}

int Experience::getActivePlayerCount() {
    return activePlayerCount;
}

int Experience::getAlivePlayerCount() {
    int count = 0;
    for (int i = 0; i < activePlayerCount; ++i) {
        if (activePlayers[i] && !activePlayers[i]->getIsDead()) {
            count++;
        }
    }
    return count;
}

Actor::Player* Experience::getRandomAlivePlayer() {
    // Create a temporary array of alive players
    Actor::Player* alivePlayers[MAX_PLAYERS];
    int aliveCount = 0;
    
    for (int i = 0; i < activePlayerCount; ++i) {
        if (activePlayers[i] && !activePlayers[i]->getIsDead()) {
            alivePlayers[aliveCount++] = activePlayers[i];
        }
    }
    
    // Return a random alive player or nullptr if none are alive
    if (aliveCount > 0) {
        return alivePlayers[rand() % aliveCount];
    }
    
    return nullptr;
}