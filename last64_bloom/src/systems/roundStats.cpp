/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "roundStats.h"
#include <algorithm>
#include <climits>

namespace {
    using DamageTable = std::array<std::array<int, RoundStats::MAX_WEAPON_TYPES>, RoundStats::MAX_PLAYERS>;
    DamageTable damageTable{};
    std::array<bool, RoundStats::MAX_PLAYERS> playerActive{};

    bool isValidPlayer(int index) {
        return index >= 0 && index < RoundStats::MAX_PLAYERS;
    }

    bool isValidWeapon(Actor::WeaponType weaponType) {
        int weaponIndex = static_cast<int>(weaponType);
        return weaponIndex >= 0 && weaponIndex < RoundStats::MAX_WEAPON_TYPES;
    }
}

void RoundStats::reset() {
    for (auto &playerRow : damageTable) {
        playerRow.fill(0);
    }
    playerActive.fill(false);
}

void RoundStats::markPlayerActive(int playerIndex, bool active) {
    if (!isValidPlayer(playerIndex)) return;
    playerActive[playerIndex] = active;
}

void RoundStats::addDamage(int playerIndex, Actor::WeaponType weaponType, int damage) {
    if (damage <= 0) return;
    if (!isValidPlayer(playerIndex) || !isValidWeapon(weaponType)) return;
    int weaponIndex = static_cast<int>(weaponType);
    int newValue = damageTable[playerIndex][weaponIndex] + damage;
    damageTable[playerIndex][weaponIndex] = newValue;
    playerActive[playerIndex] = true; // Mark as participated when dealing damage
}

int RoundStats::getDamage(int playerIndex, Actor::WeaponType weaponType) {
    if (!isValidPlayer(playerIndex) || !isValidWeapon(weaponType)) return 0;
    return damageTable[playerIndex][static_cast<int>(weaponType)];
}

int RoundStats::getTotalDamageForPlayer(int playerIndex) {
    if (!isValidPlayer(playerIndex)) return 0;
    int total = 0;
    for (int weaponIndex = 0; weaponIndex < MAX_WEAPON_TYPES; ++weaponIndex) {
        total += damageTable[playerIndex][weaponIndex];
    }
    return total;
}

bool RoundStats::isPlayerActive(int playerIndex) {
    if (!isValidPlayer(playerIndex)) return false;
    return playerActive[playerIndex];
}
