/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "upgrade_system.h"
#include "weapon_projectile.h"
#include "weapon_homing.h"
#include "weapon_circular.h"
#include <cstdlib>
#include <algorithm>
#include <typeinfo>

namespace UpgradeSystem {
    std::vector<UpgradeOption> generateUpgradeOptions(Actor::Player* player) {
        std::vector<UpgradeOption> options;
        
        // Get player's weapons
        auto& weapons = player->getWeapons();
        
        // Find all weapons that can be upgraded
        std::vector<Actor::WeaponBase*> upgradableWeapons;
        for (auto& weapon : weapons) {
            if (weapon && canUpgradeWeapon(weapon)) {
                upgradableWeapons.push_back(weapon);
            }
        }
        
        // If there are upgradable weapons, randomly select one
        if (!upgradableWeapons.empty()) {
            int randomIndex = rand() % upgradableWeapons.size();
            UpgradeOption upgradeOption;
            upgradeOption.type = UpgradeType::WEAPON_UPGRADE;
            upgradeOption.weapon = upgradableWeapons[randomIndex];
            options.push_back(upgradeOption);
        }
        
        // Check if player can get a new weapon (different from current)
        // Try up to 10 times to find a valid new weapon (increased from 3)
        for (int i = 0; i < 10; i++) {
            int weaponType = rand() % 3;
            Actor::WeaponBase* newWeapon = createWeapon(weaponType);
            
            if (newWeapon && canAddWeapon(player, newWeapon)) {
                UpgradeOption newWeaponOption;
                newWeaponOption.type = UpgradeType::NEW_WEAPON;
                newWeaponOption.weapon = newWeapon;
                options.push_back(newWeaponOption);
                break; // Only add one new weapon option
            }
            
            // Clean up the temporary weapon
            delete newWeapon;
        }
        
        return options;
    }
    
    void applyUpgrade(Actor::Player* player, const UpgradeOption& option) {
        if (option.type == UpgradeType::WEAPON_UPGRADE) {
            // Upgrade the weapon
            if (option.weapon) {
                option.weapon->upgrade();
            }
        } else if (option.type == UpgradeType::NEW_WEAPON) {
            // Add the new weapon to the player
            if (option.weapon && player) {
                option.weapon->setPlayer(player);
                player->addWeapon(option.weapon);
            }
        }
    }
    
    bool canUpgradeWeapon(Actor::WeaponBase* weapon) {
        if (!weapon) return false;
        return weapon->getUpgradeLevel() < weapon->getMaxUpgradeLevel();
    }
    
    bool canAddWeapon(Actor::Player* player, Actor::WeaponBase* newWeapon) {
        if (!player || !newWeapon) return false;
        
        // Check if player already has this type of weapon
        auto& weapons = player->getWeapons();
        for (auto& weapon : weapons) {
            if (weapon) {
                // Compare types - this is a simplified approach
                // In a real implementation, you might want a better way to identify weapon types
                if (typeid(*weapon) == typeid(*newWeapon)) {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    Actor::WeaponBase* createWeapon(int weaponType) {
        switch (weaponType) {
            case 0:
                return new Actor::WeaponProjectile();
            case 1:
                return new Actor::WeaponHoming();
            case 2:
                return new Actor::WeaponCircular();
            default:
                return nullptr;
        }
    }
}