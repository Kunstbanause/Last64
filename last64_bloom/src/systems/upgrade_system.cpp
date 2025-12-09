/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "upgrade_system.h"
#include "weapon_registry.h"
#include "../memory/savegame.h"
#include <cstdlib>
#include <algorithm>
#include <typeinfo>
#include <set>

namespace UpgradeSystem {
    // Check if a weapon type is unlocked for offering
    static bool isWeaponTypeUnlocked(Actor::WeaponType type) {
        switch (type) {
            case Actor::WeaponType::PROJECTILE:
            case Actor::WeaponType::HOMING:
            case Actor::WeaponType::CIRCULAR:
            case Actor::WeaponType::SPIRAL:
                // First 4 weapons are always available
                return true;
            case Actor::WeaponType::SHIELD:
                // Shield is available only if unlocked via credits purchase
                return SaveGame::is_shield_weapon_unlocked();
            case Actor::WeaponType::SHAPE:
                // Shape is available only if unlocked via credits purchase
                return SaveGame::is_shape_weapon_unlocked();
            default:
                return false;
        }
    }
    
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
        
        // If there are upgradable weapons, select one or two distinct options
        if (!upgradableWeapons.empty()) {
            // Shuffle indices to pick random distinct choices if available
            std::vector<int> indices(upgradableWeapons.size());
            for (size_t i = 0; i < indices.size(); ++i) indices[i] = (int)i;
            std::random_shuffle(indices.begin(), indices.end());

            // Always add at least one upgrade option
            UpgradeOption firstOpt;
            firstOpt.type = UpgradeType::WEAPON_UPGRADE;
            firstOpt.weapon = upgradableWeapons[indices[0]];
            options.push_back(firstOpt);

            // If there are at least 2 distinct upgradable weapons, add a second distinct choice
            if (upgradableWeapons.size() > 1) {
                UpgradeOption secondOpt;
                secondOpt.type = UpgradeType::WEAPON_UPGRADE;
                secondOpt.weapon = upgradableWeapons[indices[1]];
                options.push_back(secondOpt);
            }
        }
        
        // Check if we can add a new weapon (limit to 3 weapons)
        if (weapons.size() < 3) {
            // Instead of trying randomly, let's find which weapon types the player doesn't have yet
            std::set<Actor::WeaponType> existingWeaponTypes;
            for (const auto& weapon : weapons) {
                if (weapon) {
                    existingWeaponTypes.insert(weapon->getWeaponType());
                }
            }
            
            // Find weapon types that the player doesn't have
            std::vector<Actor::WeaponType> availableWeaponTypes;
            const std::vector<Actor::WeaponType>& allWeaponTypes = WeaponRegistry::getAllWeaponTypes();
            for (const auto& weaponType : allWeaponTypes) {
                if (existingWeaponTypes.find(weaponType) == existingWeaponTypes.end()) {
                    // Only offer unlocked weapons
                    if (isWeaponTypeUnlocked(weaponType)) {
                        availableWeaponTypes.push_back(weaponType);
                    }
                }
            }
            
            // If there are available weapon types, randomly select one
            if (!availableWeaponTypes.empty()) {
                // If only one slot remaining, present one NEW_WEAPON option. If multiple different types
                // are available and there is no weapon-upgrade option already added, consider returning two
                // distinct NEW_WEAPON options by creating two separate instances of different types.
                if (availableWeaponTypes.size() == 1 || options.size() > 0) {
                    int randomIndex = rand() % availableWeaponTypes.size();
                    Actor::WeaponBase* newWeapon = WeaponRegistry::createWeapon(availableWeaponTypes[randomIndex]);
                    
                    if (newWeapon) {
                        UpgradeOption newWeaponOption;
                        newWeaponOption.type = UpgradeType::NEW_WEAPON;
                        newWeaponOption.weapon = newWeapon;
                        options.push_back(newWeaponOption);
                    }
                } else {
                    // Try to produce two distinct NEW_WEAPON options
                    std::vector<int> indices(availableWeaponTypes.size());
                    for (size_t i = 0; i < indices.size(); ++i) indices[i] = (int)i;
                    std::random_shuffle(indices.begin(), indices.end());
                    // Create up to two distinct weapons
                    for (size_t k = 0; k < 2 && k < indices.size(); ++k) {
                        Actor::WeaponBase* newWeapon = WeaponRegistry::createWeapon(availableWeaponTypes[indices[k]]);
                        if (newWeapon) {
                            UpgradeOption newWeaponOption;
                            newWeaponOption.type = UpgradeType::NEW_WEAPON;
                            newWeaponOption.weapon = newWeapon;
                            options.push_back(newWeaponOption);
                        }
                    }
                }
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
                // Auto-unlock the weapon when it's first offered
                Actor::WeaponType wt = option.weapon->getWeaponType();
                if (wt == Actor::WeaponType::SHIELD) {
                    SaveGame::set_shield_weapon_unlocked(true);
                } else if (wt == Actor::WeaponType::SHAPE) {
                    SaveGame::set_shape_weapon_unlocked(true);
                }
                
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
}