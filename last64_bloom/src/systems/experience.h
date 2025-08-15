#pragma once
#include "../actors/player.h"

namespace Experience {
    void initialize(Actor::Player* p1, Actor::Player* p2, Actor::Player* p3, Actor::Player* p4);
    void shutdown();
    void addXP(int amount);
    
    int getLevel();
    float getXPPercentage();
};