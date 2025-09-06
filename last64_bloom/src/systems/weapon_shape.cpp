/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weapon_shape.h"
#include "../actors/player.h"
#include "../audio.h"
#include <libdragon.h>

namespace Actor {
    WeaponShape::WeaponShape() : WeaponBase(WeaponType::PROJECTILE) {
        fireRate = 0.5f;        // Time between shots
        shapeLifetime = 5.0f;   // Shapes last 5 seconds
        attackFrequency = 0.5f; // Attack every 0.5 seconds
        weaponCooldown = 0.0f;  // Current cooldown
        shapeSize = 1.0f;       // Default size
        shapeDamage = 4;        // Default damage
        currentShape = nullptr; // No active shape initially
        maxUpgradeLevel = 5;    // Max upgrade level
    }

    WeaponShape::~WeaponShape() {
        // Cleanup if needed
    }

    void WeaponShape::update(float deltaTime) {
        // Update weapon cooldown
        if (fireCooldown > 0.0f) {
            fireCooldown -= deltaTime;
        }
        
        // Update weapon cooldown
        if (weaponCooldown > 0.0f) {
            weaponCooldown -= deltaTime;
        }
        
        // Update the current shape if it exists
        if (currentShape && !currentShape->isActive()) {
            currentShape = nullptr;
        }
    }

    void WeaponShape::draw3D(float deltaTime) {
        // The shape itself handles its drawing
    }

    void WeaponShape::drawPTX(float deltaTime) {
        // No particle effects for this weapon
    }

    void WeaponShape::fire(const T3DVec3& position, const T3DVec3& direction) {
        // This weapon doesn't fire in a direction, it places a shape at the player's position
        // Only fire if we're off cooldown and don't already have an active shape
        if (fireCooldown <= 0.0f && weaponCooldown <= 0.0f && !currentShape) {
            // Spawn a shape at the player's position
            currentShape = Shape::spawn(position, shapeLifetime, attackFrequency, shapeDamage, player->getColor(), shapeSize);
            
            // Set cooldowns
            fireCooldown = fireRate;
            weaponCooldown = 2.0f; // 2 second cooldown before we can place another
            
            // Play fire sound
            gSFXManager.play(SFXManager::SFX_HIT);
        }
    }

    void WeaponShape::fireManual() {
        // Manual fire uses the player's position
        if (player) {
            T3DVec3 playerPos = player->getPosition();
            fire(playerPos, {0, 0, 0}); // Direction doesn't matter for this weapon
        }
    }

    void WeaponShape::upgrade() {
        if (upgradeLevel < maxUpgradeLevel) {
            upgradeLevel++;
            
            switch (upgradeLevel) {
                case 1:
                    shapeDamage = 6;
                    break;
                case 2:
                    shapeLifetime = 6.0f;
                    break;
                case 3:
                    attackFrequency = 0.4f; // Faster attacks
                    break;
                case 4:
                    shapeSize = 1.2f; // Larger shape
                    break;
                case 5:
                    shapeDamage = 8; // More damage
                    break;
                default:
                    break;
            }
        }
    }
}