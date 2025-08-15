#include "experience.h"
#include <libdragon.h>
#include <cmath>
#include <rdpq.h>
#include "../render/debugDraw.h"

namespace {
    int currentXP = 0;
    int xpToNextLevel = 10;
    int currentLevel = 1;
    Actor::Player* player1 = nullptr;
    Actor::Player* player2 = nullptr;
    Actor::Player* player3 = nullptr;
    Actor::Player* player4 = nullptr;

    // Exponential growth factor for XP required for next level
    const float xpGrowthFactor = 1.5f;
}

void Experience::initialize(Actor::Player* p1, Actor::Player* p2, Actor::Player* p3, Actor::Player* p4) {
    currentXP = 0;
    xpToNextLevel = 10;
    currentLevel = 1;
    player1 = p1;
    player2 = p2;
    player3 = p3;
    player4 = p4;
}

void Experience::shutdown() {
    player1 = nullptr;
    player2 = nullptr;
    player3 = nullptr;
    player4 = nullptr;
}

void Experience::addXP(int amount) {
    currentXP += amount;
    if (currentXP >= xpToNextLevel) {
        currentLevel++;
        currentXP -= xpToNextLevel;
        xpToNextLevel = static_cast<int>(xpToNextLevel * xpGrowthFactor);

        if (player1 && player1->getWeapon()) {
            player1->getWeapon()->upgrade();
        }
        if (player2 && player2->getWeapon()) {
            player2->getWeapon()->upgrade();
        }
        if (player3 && player3->getWeapon()) {
            player3->getWeapon()->upgrade();
        }
        if (player4 && player4->getWeapon()) {
            player4->getWeapon()->upgrade();
        }
    }
}



int Experience::getLevel() {
    return currentLevel;
}

float Experience::getXPPercentage() {
    return (float)currentXP / (float)xpToNextLevel;
}