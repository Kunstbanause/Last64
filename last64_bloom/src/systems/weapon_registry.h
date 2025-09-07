/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "weapon_base.h"
#include <vector>
#include <string>
#include <functional>

namespace WeaponRegistry {
    // Weapon metadata structure
    struct WeaponMetadata {
        Actor::WeaponType type;
        std::string name;
        std::string shortName;
        std::string description;
        std::function<Actor::WeaponBase*()> factoryFunction;
    };

    // Get all registered weapon types
    const std::vector<Actor::WeaponType>& getAllWeaponTypes();

    // Get metadata for a specific weapon type
    const WeaponMetadata* getWeaponMetadata(Actor::WeaponType type);

    // Get metadata by index
    const WeaponMetadata* getWeaponMetadata(size_t index);

    // Get weapon count
    size_t getWeaponCount();

    // Create weapon instance by type
    Actor::WeaponBase* createWeapon(Actor::WeaponType type);

    // Create weapon instance by index
    Actor::WeaponBase* createWeapon(size_t index);

    // Get weapon type from index
    Actor::WeaponType getWeaponTypeFromIndex(size_t index);

    // Get index from weapon type
    size_t getIndexFromWeaponType(Actor::WeaponType type);

    // Check if weapon type is valid
    bool isValidWeaponType(Actor::WeaponType type);
}