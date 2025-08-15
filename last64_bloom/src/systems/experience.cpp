#include "experience.h"
#include <libdragon.h>
#include <cmath>
#include <rdpq.h>
#include "../render/debugDraw.h"

namespace {
    constexpr int MAX_PLAYERS = 4;
    int currentXP = 0;
    int xpToNextLevel = 10;
    int currentLevel = 1;
    Actor::Player* activePlayers[MAX_PLAYERS];
    int activePlayerCount = 0;

    // Exponential growth factor for XP required for next level
    const float xpGrowthFactor = 1.5f;
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

        for (int i = 0; i < activePlayerCount; ++i) {
            if (activePlayers[i] && activePlayers[i]->getWeapon()) {
                activePlayers[i]->getWeapon()->upgrade();
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