/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weapon_base.h"

namespace Actor {
    WeaponBase::WeaponBase(WeaponType type) : Base() {
        player = nullptr;
        fireCooldown = 0.0f;
        upgradeLevel = 0;
        maxUpgradeLevel = 3;
        spawnOffset = {0, 0, 0};
        damage = 4; // Default damage
        weaponType = type;
        flags &= ~FLAG_DISABLED; // Clear the disabled flag to enable the actor
    }
    
    WeaponBase::~WeaponBase() {
        // Nothing to do here
    }
    
    void WeaponBase::fire(const T3DVec3& position, const T3DVec3& direction) {
        // Default implementation does nothing
        // This should be overridden by subclasses
    }

    void WeaponBase::fireManual() {
        // Default implementation does nothing
        // This should be overridden by subclasses
    }

    void WeaponBase::upgrade() {
        if (upgradeLevel < maxUpgradeLevel) {
            upgradeLevel++;
            // Default implementation does nothing
            // This should be overridden by subclasses to implement specific upgrade behavior
        }
    }
}