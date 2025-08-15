#include "experience.h"
#include <libdragon.h>
#include <cmath>
#include <rdpq.h>

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
    }
}

void Experience::draw() {
    // Screen dimensions
    const int screenWidth = 320;
    const int screenHeight = 240;
    const int barHeight = 10;

    // Calculate bar width
    float xpPercentage = (float)currentXP / (float)xpToNextLevel;
    int barWidth = static_cast<int>(xpPercentage * screenWidth);

    // Draw the bar background
    rdpq_set_fill_color(RGBA32(50, 50, 50, 255));
    rdpq_fill_rectangle(0, screenHeight - barHeight, screenWidth, screenHeight);

    // Draw the bar foreground
    rdpq_set_fill_color(RGBA32(100, 200, 255, 255)); // Light blue color
    rdpq_fill_rectangle(0, screenHeight - barHeight, barWidth, screenHeight);
}

int Experience::getLevel() {
    return currentLevel;
}

float Experience::getXPPercentage() {
    return (float)currentXP / (float)xpToNextLevel;
}