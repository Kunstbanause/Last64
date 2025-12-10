/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include <array>
#include <cstdint>
#include "weapon_types.h"

namespace RoundStats {
    constexpr int MAX_PLAYERS = 4;
    constexpr int MAX_WEAPON_TYPES = static_cast<int>(Actor::WeaponType::COUNT);

    void reset();
    void markPlayerActive(int playerIndex, bool active);
    void addDamage(int playerIndex, Actor::WeaponType weaponType, int damage);
    int getDamage(int playerIndex, Actor::WeaponType weaponType);
    int getTotalDamageForPlayer(int playerIndex);
    bool isPlayerActive(int playerIndex);
}
