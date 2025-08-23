/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "../actors/player.h"
#include "weapon_base.h"
#include <vector>

namespace UpgradeSystem {
    enum class UpgradeType {
        WEAPON_UPGRADE,
        NEW_WEAPON
    };
    
    struct UpgradeOption {
        UpgradeType type;
        Actor::WeaponBase* weapon;  // For weapon upgrades, this is the weapon to upgrade
                                    // For new weapons, this is the new weapon type
    };
    
    // Generate upgrade options for a player
    std::vector<UpgradeOption> generateUpgradeOptions(Actor::Player* player);
    
    // Apply an upgrade option to a player
    void applyUpgrade(Actor::Player* player, const UpgradeOption& option);
    
    // Check if a player can upgrade a specific weapon
    bool canUpgradeWeapon(Actor::WeaponBase* weapon);
    
    // Check if a player can receive a new weapon (doesn't already have it)
    bool canAddWeapon(Actor::Player* player, Actor::WeaponBase* newWeapon);
    
    // Create a new weapon of a specific type
    Actor::WeaponBase* createWeapon(int weaponType);
}