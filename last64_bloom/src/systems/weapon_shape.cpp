/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weapon_shape.h"
#include "../actors/player.h"
#include "../audio.h"
#include <libdragon.h>

namespace Actor {
    WeaponShape::WeaponShape() : WeaponBase(WeaponType::SHAPE) {
        fireRate = 3.0f;        // Time between shots
        weaponCooldown = 0.0f;  // Current cooldown
        shapeLifetime = 0.6f;   // Shapes last 1 second
        attackFrequency = 0.3f; // Attack every 0.3 seconds (faster for a melee-like weapon)
        shapeWidth = 24.0f;     // Default width
        shapeHeight = 5.0f;     // Default height (long rectangle)
        shapeDamage = 4;        // Default damage
        currentShape = nullptr; // No active shape initially
        maxUpgradeLevel = 5;    // Max upgrade level
        spawnOnRight = true;    // Start by spawning on the right
    }

    WeaponShape::~WeaponShape() {
        // Cleanup if needed
    }

    void WeaponShape::update(float deltaTime) {
        // Update weapon cooldown
        if (weaponCooldown > 0.0f) {
            weaponCooldown -= deltaTime;
        }
        
        // Update the current shape if it exists
        if (currentShape && !currentShape->isActive()) {
            currentShape = nullptr;
        }
        
        // Auto-fire if we're off cooldown and don't already have an active shape
        if (weaponCooldown <= 0.0f && player) {
            fire();
        }
    }

    void WeaponShape::draw3D(float deltaTime) {
        // The shape itself handles its drawing
    }

    void WeaponShape::drawPTX(float deltaTime) {
        // No particle effects for this weapon
    }

    void WeaponShape::fire() {
        if (!player || currentShape) return;
        
        // Alternate between left and right spawn positions
        T3DVec3 spawnOffset = spawnOnRight ? T3DVec3{shapeWidth/2+12, 0, 0} : T3DVec3{-(shapeWidth/2+12), 0, 0};
        currentShape = Shape::spawnAttached(player, spawnOffset, shapeWidth, shapeHeight, shapeLifetime, attackFrequency, getFinalDamage(), player->getColor(), getOwnerPlayerIndex(), weaponType);
        
        // Flip the spawn side for next time
        spawnOnRight = !spawnOnRight;
        
        // Set cooldowns
        weaponCooldown = fireRate;

        // Play fire sound
        // gSFXManager.play(SFXManager::SFX_HIT);
    }

    void WeaponShape::fireManual() {
        // Manual fire uses the player's position
        if (player) {
            fire();
        }
    }

    void WeaponShape::upgrade() {
        if (upgradeLevel < maxUpgradeLevel) {
            upgradeLevel++;
            
            switch (upgradeLevel) {
                case 1:
                    shapeDamage *= 1.25f;
                    shapeWidth *= 1.4f;
                    fireRate -= fireRate * 0.2f;
                    break;
                case 2:
                    shapeLifetime += 0.3f;
                    shapeHeight *= 1.5f;
                    shapeWidth *= 1.4f;
                    break;
                case 3:
                    shapeHeight *= 1.2f;
                    shapeWidth *= 1.4f;
                    fireRate -= fireRate * 0.2f;
                    break;
                case 4:
                    shapeHeight *= 2.0f;
                    shapeWidth *= 1.5f;
                    attackFrequency -= attackFrequency * 0.2f;
                    break;
                case 5:
                    shapeWidth *= 1.5f;
                    shapeDamage *= 1.25f;
                    break;
                default:
                    break;
            }
        }
    }
}