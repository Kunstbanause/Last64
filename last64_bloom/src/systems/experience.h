#pragma once
#include "../actors/player.h"

namespace Experience {
    void initialize(Actor::Player* p1, Actor::Player* p2);
    void shutdown();
    void addXP(int amount);
    void draw();
    int getLevel();
    float getXPPercentage();
};