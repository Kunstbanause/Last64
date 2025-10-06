#include "experience.h"
#include "upgrade_system.h"
#include "../systems/weapon_base.h"
#include "../systems/weapon_registry.h"
#include "../memory/savegame.h"
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

    // Per-player queue of pending upgrade option sets. Each element is a set of choices presented at once.
    std::vector<std::vector<UpgradeOption>> pendingQueues[MAX_PLAYERS];

    // Exponential growth factor for XP required for next level
    const float xpGrowthFactor = 1.25f;
    
        // Slow motion timer in real seconds. When >0, main loop should scale deltaTime accordingly.
        float slowMotionRemaining = 0.0f;
        const float slowMotionScale = 0.25f; // 25% speed in slow motion
}

void Experience::initialize() {
    currentXP = 0;
    xpToNextLevel = 10;
    currentLevel = 1;
    activePlayerCount = 0;
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        activePlayers[i] = nullptr;
        pendingQueues[i].clear();
    }
}

void Experience::shutdown() {
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        // cleanup any queued NEW_WEAPON allocations
        for (auto &queueEntry : pendingQueues[i]) {
            for (auto &opt : queueEntry) {
                if (opt.type == UpgradeSystem::UpgradeType::NEW_WEAPON && opt.weapon) {
                    delete opt.weapon;
                    opt.weapon = nullptr;
                }
            }
        }
        pendingQueues[i].clear();
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
                // move pending queue as well
                pendingQueues[j] = std::move(pendingQueues[j+1]);
            }
            // cleanup last slot
            // delete any queued weapons in the moved-from last queue
            for (auto &queueEntry : pendingQueues[activePlayerCount-1]) {
                for (auto &opt : queueEntry) {
                    if (opt.type == UpgradeSystem::UpgradeType::NEW_WEAPON && opt.weapon) {
                        delete opt.weapon;
                        opt.weapon = nullptr;
                    }
                }
            }
            pendingQueues[activePlayerCount-1].clear();
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
                    // Single available option. If it's a NEW_WEAPON we duplicate/clone so
                    // the player sees two selectable items (both valid). If it's a
                    // WEAPON_UPGRADE, present only the single choice (A selects it).
                    if (upgradeOptions[0].type == UpgradeSystem::UpgradeType::NEW_WEAPON && upgradeOptions[0].weapon) {
                        // Duplicate the NEW_WEAPON option by creating a separate instance
                        chosen.push_back(upgradeOptions[0]);
                        Actor::WeaponType wt = upgradeOptions[0].weapon->getWeaponType();
                        Actor::WeaponBase* newW = WeaponRegistry::createWeapon(wt);
                        if (newW) {
                            UpgradeOption opt;
                            opt.type = UpgradeSystem::UpgradeType::NEW_WEAPON;
                            opt.weapon = newW;
                            chosen.push_back(opt);
                        } else {
                            // Fallback: if create failed, present only the original
                            // (do not duplicate to avoid double-delete risk)
                        }
                    } else {
                        // Single WEAPON_UPGRADE -> present only that choice (A will select it)
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

                // Append this set of choices to the player's pending queue
                pendingQueues[i].push_back(std::move(chosen));
                // Start slow motion for a few seconds (real time)
                slowMotionRemaining = 2.2f;

                // Track total level-ups in persistent save
                SaveGame::accum_level_up();

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
    return !pendingQueues[idx].empty();
}

const std::vector<UpgradeOption>& Experience::getPendingOptions(Actor::Player* player) {
    static std::vector<UpgradeOption> empty;
    int idx = findPlayerIndex(player);
    if (idx < 0) return empty;
    if (pendingQueues[idx].empty()) return empty;
    return pendingQueues[idx].front();
}

void Experience::selectPendingChoice(Actor::Player* player, int choiceIndex) {
    int idx = findPlayerIndex(player);
    if (idx < 0) return;
    if (pendingQueues[idx].empty()) return;

    auto opts = std::move(pendingQueues[idx].front());
    // validate index
    if (choiceIndex < 0 || choiceIndex >= (int)opts.size()) {
        // invalid index, drop this queued entry to be safe
        // cleanup any allocated NEW_WEAPONs
        for (auto &opt : opts) {
            if (opt.type == UpgradeSystem::UpgradeType::NEW_WEAPON && opt.weapon) {
                delete opt.weapon;
            }
        }
        // pop front
        pendingQueues[idx].erase(pendingQueues[idx].begin());
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

    // Remove the processed front entry
    pendingQueues[idx].erase(pendingQueues[idx].begin());
    // If no more queued choices for any player, or if this player's queue is empty, stop slow motion when appropriate
    bool anyPending = false;
    for (int i = 0; i < activePlayerCount; ++i) {
        if (!pendingQueues[i].empty()) { anyPending = true; break; }
    }
    if (!anyPending) {
        slowMotionRemaining = 0.0f;
    }
}

void Experience::startSlowMotion(float seconds) {
    slowMotionRemaining = seconds;
}

void Experience::tickSlowMotionRealtime(float realDelta) {
    if (slowMotionRemaining <= 0.0f) return;
    slowMotionRemaining -= realDelta;
    if (slowMotionRemaining < 0.0f) slowMotionRemaining = 0.0f;
}

float Experience::getSlowMotionRemaining() {
    return slowMotionRemaining;
}

float Experience::getSlowMotionScale() {
    return slowMotionScale;
}