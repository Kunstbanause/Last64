/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weapon_registry.h"
#include "weapon_projectile.h"
#include "weapon_homing.h"
#include "weapon_circular.h"
#include "weapon_spiral.h"
#include "weapon_shield.h"
#include "weapon_shape.h"
#include <cassert>

namespace WeaponRegistry {
    // Static weapon registry
    static const std::vector<WeaponMetadata> weaponRegistry = {
        {
            Actor::WeaponType::PROJECTILE,
            "Projectile Spread",
            "P",
            "Fires projectiles in the direction of movement",
            []() -> Actor::WeaponBase* { return new Actor::WeaponProjectile(); }
        },
        {
            Actor::WeaponType::HOMING,
            "Homing Projectiles",
            "H",
            "Fires projectiles that home in on nearby enemies",
            []() -> Actor::WeaponBase* { return new Actor::WeaponHoming(); }
        },
        {
            Actor::WeaponType::CIRCULAR,
            "Circular Projectiles",
            "C",
            "Fires projectiles in a circular pattern aways from the player",
            []() -> Actor::WeaponBase* { return new Actor::WeaponCircular(); }
        },
        {
            Actor::WeaponType::SPIRAL,
            "Spiral",
            "S",
            "Projectiles in a spiral around the player",
            []() -> Actor::WeaponBase* { return new Actor::WeaponSpiral(); }
        },
        {
            Actor::WeaponType::SHIELD,
            "Defene",
            "D",
            "Generates a shield around the player",
            []() -> Actor::WeaponBase* { return new Actor::WeaponShield(); }
        },
        {
            Actor::WeaponType::SHAPE,
            "Whip",
            "W",
            "Spawns Whip Shape",
            []() -> Actor::WeaponBase* { return new Actor::WeaponShape(); }
        }
    };

    // Get all registered weapon types
    const std::vector<Actor::WeaponType>& getAllWeaponTypes() {
        static std::vector<Actor::WeaponType> weaponTypes;
        static bool initialized = false;
        
        if (!initialized) {
            weaponTypes.reserve(weaponRegistry.size());
            for (const auto& metadata : weaponRegistry) {
                weaponTypes.push_back(metadata.type);
            }
            initialized = true;
        }
        
        return weaponTypes;
    }

    // Get metadata for a specific weapon type
    const WeaponMetadata* getWeaponMetadata(Actor::WeaponType type) {
        for (const auto& metadata : weaponRegistry) {
            if (metadata.type == type) {
                return &metadata;
            }
        }
        return nullptr;
    }

    // Get metadata by index
    const WeaponMetadata* getWeaponMetadata(size_t index) {
        if (index < weaponRegistry.size()) {
            return &weaponRegistry[index];
        }
        return nullptr;
    }

    // Get weapon count
    size_t getWeaponCount() {
        return weaponRegistry.size();
    }

    // Create weapon instance by type
    Actor::WeaponBase* createWeapon(Actor::WeaponType type) {
        const WeaponMetadata* metadata = getWeaponMetadata(type);
        if (metadata && metadata->factoryFunction) {
            return metadata->factoryFunction();
        }
        return nullptr;
    }

    // Create weapon instance by index
    Actor::WeaponBase* createWeapon(size_t index) {
        const WeaponMetadata* metadata = getWeaponMetadata(index);
        if (metadata && metadata->factoryFunction) {
            return metadata->factoryFunction();
        }
        return nullptr;
    }

    // Get weapon type from index
    Actor::WeaponType getWeaponTypeFromIndex(size_t index) {
        if (index < weaponRegistry.size()) {
            return weaponRegistry[index].type;
        }
        // Return first weapon type as fallback
        return weaponRegistry[0].type;
    }

    // Get index from weapon type
    size_t getIndexFromWeaponType(Actor::WeaponType type) {
        for (size_t i = 0; i < weaponRegistry.size(); ++i) {
            if (weaponRegistry[i].type == type) {
                return i;
            }
        }
        // Return 0 as fallback
        return 0;
    }

    // Check if weapon type is valid
    bool isValidWeaponType(Actor::WeaponType type) {
        for (const auto& metadata : weaponRegistry) {
            if (metadata.type == type) {
                return true;
            }
        }
        return false;
    }
}