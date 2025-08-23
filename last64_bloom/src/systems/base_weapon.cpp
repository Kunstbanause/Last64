/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "base_weapon.h"

namespace Actor {
    Weapon::Weapon() : Base() {
        player = nullptr;
        fireCooldown = 0.0f;
        upgradeLevel = 0;
        maxUpgradeLevel = 3;
        spawnOffset = {0, 0, 0};
        flags &= ~FLAG_DISABLED; // Clear the disabled flag to enable the actor
    }
    
    Weapon::~Weapon() {
        // Nothing to do here
    }
    
    void Weapon::fire(const T3DVec3& position, const T3DVec3& direction) {
        // Default implementation does nothing
        // This should be overridden by subclasses
    }

    void Weapon::fireManual() {
        // Default implementation does nothing
        // This should be overridden by subclasses
    }

    void Weapon::upgrade() {
        if (upgradeLevel < maxUpgradeLevel) {
            upgradeLevel++;
            // Default implementation does nothing
            // This should be overridden by subclasses to implement specific upgrade behavior
        }
    }
}