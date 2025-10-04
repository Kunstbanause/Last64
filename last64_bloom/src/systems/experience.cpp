#include "experience.h"
#include "upgrade_system.h"
#include "../systems/weapon_base.h"
#include "../systems/weapon_registry.h"
#include "../render/hdrBoost.h"
#include <libdragon.h>
#include <cmath>
#include <rdpq.h>
#include "../render/debugDraw.h"
#include "../audio.h"

using UpgradeOption = UpgradeSystem::UpgradeOption;

namespace {
    constexpr int MAX_PLAYERS = 4;
    int currentXP = 0;
    int xpToNextLevel = 10;
    int currentLevel = 1;
    Actor::Player* activePlayers[MAX_PLAYERS];
    int activePlayerCount = 0;

    struct PendingChoice {
        bool active = false;
        std::vector<UpgradeOption> options;
    };

    PendingChoice pendingChoices[MAX_PLAYERS];

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

        // Trigger HDR boost on level up
        HDRBoost::triggerBoost();

        // Generate upgrade options for each player
        for (int i = 0; i < activePlayerCount; ++i) {
            if (activePlayers[i] && !activePlayers[i]->getIsDead()) {
                // Generate upgrade options
                auto upgradeOptions = UpgradeSystem::generateUpgradeOptions(activePlayers[i]);

                // If there are no options, skip
                if (upgradeOptions.empty()) continue;

                // Choose two options to present (try to pick distinct ones)
                std::vector<UpgradeOption> chosen;
                if (upgradeOptions.size() == 1) {
                    // Duplicate the single option so there are two choices.
                    // If it's a NEW_WEAPON option we must create a separate weapon instance
                    // so both choices own different weapon objects (to avoid double-delete).
                    chosen.push_back(upgradeOptions[0]);
                    if (upgradeOptions[0].type == UpgradeSystem::UpgradeType::NEW_WEAPON && upgradeOptions[0].weapon) {
                        Actor::WeaponType wt = upgradeOptions[0].weapon->getWeaponType();
                        Actor::WeaponBase* newW = WeaponRegistry::createWeapon(wt);
                        if (newW) {
                            UpgradeOption opt;
                            opt.type = UpgradeSystem::UpgradeType::NEW_WEAPON;
                            opt.weapon = newW;
                            chosen.push_back(opt);
                        } else {
                            // Fallback: duplicate pointer (rare). This may risk double-delete,
                            // but WeaponRegistry::createWeapon should normally succeed.
                            chosen.push_back(upgradeOptions[0]);
                        }
                    } else {
                        chosen.push_back(upgradeOptions[0]);
                    }
                } else {
                    // Pick two distinct random indices
                    int idxA = rand() % upgradeOptions.size();
                    int idxB = rand() % upgradeOptions.size();
                    while (idxB == idxA && upgradeOptions.size() > 1) {
                        idxB = rand() % upgradeOptions.size();
                    }
                    chosen.push_back(upgradeOptions[idxA]);
                    chosen.push_back(upgradeOptions[idxB]);
                }

                // Store in pendingChoices for this player index
                pendingChoices[i].active = true;
                pendingChoices[i].options = std::move(chosen);

                // Fire weapons for visual feedback
                for (int p = 0; p < activePlayerCount; ++p) {
                    if (activePlayers[p] && !activePlayers[p]->getIsDead()) {
                        auto& weapons = activePlayers[p]->getWeapons();
                        for (auto& weapon : weapons) {
                            if (weapon) {
                                weapon->fireManual();
                            }
                        }
                    }
                }
                // Note: do not delete unselected NEW_WEAPON pointers here; they'll be
                // cleaned up when the player makes a choice (selectPendingChoice).
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

int Experience::getXToNextLevel() {
    return xpToNextLevel;
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

// Helper: find index of player in activePlayers or -1
static int findPlayerIndex(Actor::Player* player) {
    for (int i = 0; i < activePlayerCount; ++i) {
        if (activePlayers[i] == player) return i;
    }
    return -1;
}

bool Experience::hasPendingChoice(Actor::Player* player) {
    int idx = findPlayerIndex(player);
    if (idx < 0) return false;
    return pendingChoices[idx].active;
}

const std::vector<UpgradeOption>& Experience::getPendingOptions(Actor::Player* player) {
    static std::vector<UpgradeOption> empty;
    int idx = findPlayerIndex(player);
    if (idx < 0) return empty;
    return pendingChoices[idx].options;
}

void Experience::selectPendingChoice(Actor::Player* player, int choiceIndex) {
    int idx = findPlayerIndex(player);
    if (idx < 0) return;
    if (!pendingChoices[idx].active) return;

    auto& opts = pendingChoices[idx].options;
    if (choiceIndex < 0 || choiceIndex >= (int)opts.size()) {
        // invalid index, ignore
        return;
    }

    // Apply selected option
    UpgradeSystem::applyUpgrade(player, opts[choiceIndex]);

    // Cleanup any NEW_WEAPON allocations that weren't selected
    for (int i = 0; i < (int)opts.size(); ++i) {
        if (i == choiceIndex) continue;
        if (opts[i].type == UpgradeSystem::UpgradeType::NEW_WEAPON && opts[i].weapon) {
            delete opts[i].weapon;
        }
    }

    // Clear pending
    pendingChoices[idx].options.clear();
    pendingChoices[idx].active = false;
}