#pragma once
#include "../actors/player.h"

namespace Experience {
    void initialize();
    void shutdown();
    void addXP(int amount);
    void addPlayer(Actor::Player* player);
    void removePlayer(Actor::Player* player);
    
    int getLevel();
    int getXToNextLevel();
    float getXPPercentage();
    int getActivePlayerCount();
    
    // Get number of alive players
    int getAlivePlayerCount();
    
    // Get a random alive player (returns nullptr if none are alive)
    Actor::Player* getRandomAlivePlayer();
    
    // HDR boost functions
    void updateHDRBoost(float deltaTime);
    float getHDRBoostFactor();
    void setDefaultHDRFactor(float defaultFactor);
};