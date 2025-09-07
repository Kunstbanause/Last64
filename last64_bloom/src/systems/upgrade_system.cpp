/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "upgrade_system.h"
#include "weapon_projectile.h"
#include "weapon_homing.h"
#include "weapon_circular.h"
#include "weapon_spiral.h"
#include "weapon_shape.h"
#include "weapon_shield.h"
#include <cstdlib>
#include <algorithm>
#include <typeinfo>
#include <set>

namespace UpgradeSystem {
    // All possible weapon types
    const std::vector<Actor::WeaponType> ALL_WEAPON_TYPES = {
        Actor::WeaponType::PROJECTILE,
        Actor::WeaponType::HOMING,
        Actor::WeaponType::CIRCULAR,
        Actor::WeaponType::SPIRAL,
        Actor::WeaponType::SHIELD,
        Actor::WeaponType::SHAPE
    };
    
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
        
        // Instead of trying randomly, let's find which weapon types the player doesn't have yet
        std::set<Actor::WeaponType> existingWeaponTypes;
        for (const auto& weapon : weapons) {
            if (weapon) {
                existingWeaponTypes.insert(weapon->getWeaponType());
            }
        }
        
        // Find weapon types that the player doesn't have
        std::vector<Actor::WeaponType> availableWeaponTypes;
        for (const auto& weaponType : ALL_WEAPON_TYPES) {
            if (existingWeaponTypes.find(weaponType) == existingWeaponTypes.end()) {
                availableWeaponTypes.push_back(weaponType);
            }
        }
        
        // If there are available weapon types, randomly select one
        if (!availableWeaponTypes.empty()) {
            int randomIndex = rand() % availableWeaponTypes.size();
            Actor::WeaponBase* newWeapon = createWeapon(static_cast<int>(availableWeaponTypes[randomIndex]));
            
            if (newWeapon) {
                UpgradeOption newWeaponOption;
                newWeaponOption.type = UpgradeType::NEW_WEAPON;
                newWeaponOption.weapon = newWeapon;
                options.push_back(newWeaponOption);
            }
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
            case 3:
                return new Actor::WeaponSpiral();
            case 4:
                return new Actor::WeaponShield();
            case 5:
                return new Actor::WeaponShape();
            default:
                return nullptr;
        }
    }
    
    Actor::WeaponBase* createWeapon(Actor::WeaponType weaponType) {
        switch (weaponType) {
            case Actor::WeaponType::PROJECTILE:
                return new Actor::WeaponProjectile();
            case Actor::WeaponType::HOMING:
                return new Actor::WeaponHoming();
            case Actor::WeaponType::CIRCULAR:
                return new Actor::WeaponCircular();
            case Actor::WeaponType::SPIRAL:
                return new Actor::WeaponSpiral();
            case Actor::WeaponType::SHIELD:
                return new Actor::WeaponShield();
            case Actor::WeaponType::SHAPE:
                return new Actor::WeaponShape();
            default:
                return nullptr;
        }
    }
}