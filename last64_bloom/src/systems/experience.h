#pragma once
#include "../actors/player.h"
#include "upgrade_system.h"
#include <vector>

namespace Experience {
    void initialize();
    void shutdown();
    void addXP(int amount);
    void addPlayer(Actor::Player* player);
    void removePlayer(Actor::Player* player);
    
    int getLevel();
    int getXToNextLevel();
    int getCurrentXP();
    float getXPPercentage();
    int getActivePlayerCount();
    Actor::Player* getPlayer(int index);

    // Level-up tracking for current run
    int getLevelUpsThisRun();
    int getCreditsEarnedThisRun(); // level-ups * 10
    
    // Get number of alive players
    int getAlivePlayerCount();
    
    // Get a random alive player (returns nullptr if none are alive)
    Actor::Player* getRandomAlivePlayer();

    // Slow motion helpers: start a realtime slow motion timer (seconds), tick with real delta
    void startSlowMotion(float seconds);
    void tickSlowMotionRealtime(float realDelta);
    float getSlowMotionRemaining();
    float getSlowMotionScale();
    // Per-frame tick for experience system (call from main loop)
    void tick(float deltaTime);

    // XP bar flash intensity (0..1) and gain amount for scaling
    float getXPBarFlash();
    int getXPGainAmount();

    // Pending upgrade choice helpers (two choices shown to player)
    bool hasPendingChoice(Actor::Player* player);
    const std::vector<UpgradeSystem::UpgradeOption>& getPendingOptions(Actor::Player* player);
    // Player selects 0 (A) or 1 (B)
    void selectPendingChoice(Actor::Player* player, int choiceIndex);
};