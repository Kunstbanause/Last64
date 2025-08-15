#pragma once
#include "../actors/player.h"

namespace Experience {
    void initialize();
    void shutdown();
    void addXP(int amount);
    void addPlayer(Actor::Player* player);
    void removePlayer(Actor::Player* player);
    
    int getLevel();
    float getXPPercentage();
};