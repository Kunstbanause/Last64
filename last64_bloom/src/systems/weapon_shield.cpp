/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weapon_shield.h"
#include "../actors/player.h"
#include "../actors/enemy.h"
#include <libdragon.h>
#include <cmath>

// Define a new color for shield projectiles
#define SHIELD_PROJECTILE_COLOR 0x0508051A

namespace Actor {
    WeaponShield::WeaponShield() : WeaponBase(WeaponType::SHIELD) {
        // Set weapon-specific properties
        fireRate = 0.0f;              // Not used for shield
        shieldSize = 12.0f;           // Size of the shield
        shieldLifetime = 3.0f;        // Shield lasts x seconds
        shieldCooldown = 2.0f;        // cooldown
        shieldActive = false;         // Shield starts inactive
        shieldRotation = 0.0f;        // Initial rotation
        shieldProjectile = nullptr;   // No projectile yet
        
        // Default Parameters
        maxUpgradeLevel = 5;          // 5 upgrade levels
        spawnOffset = {0, 0, 0};
        damage = 4;                   // Base damage
        fireCooldown = 0.0f;          // No initial cooldown
    }
    
    WeaponShield::~WeaponShield() {
        // No cleanup needed - projectile system is managed by the scene
    }
    
    void WeaponShield::update(float deltaTime) {
        // Update cooldown
        if (fireCooldown > 0) {
            fireCooldown -= deltaTime;
        }

        // Check if shield projectile is gone (due to damage or lifetime)
        if (shieldActive && shieldProjectile && !shieldProjectile->isActive()) {
            shieldActive = false;
            shieldProjectile = nullptr;
            fireCooldown = shieldCooldown; // Start the cooldown period
        }

        // Spawn new shield when cooldown is up
        if (!shieldActive && fireCooldown <= 0 && player) {
            T3DVec3 playerPos = player->getPosition();
            playerPos.z += 1.0f; // Slightly in front of player
            uint32_t projectileColor = SHIELD_PROJECTILE_COLOR;
            if (player) {
                projectileColor = player->getColor();
                // Reduce in color intensity for shield effect:
                int scale = 50;
                auto max = [](uint32_t a, uint32_t b) { return a > b ? a : b; };
                projectileColor = (max(1u, ((projectileColor >> 24) * scale) / 100) << 24) | (max(1u, ((projectileColor >> 16 & 0xFF) * scale) / 100) << 16) | (max(1u, ((projectileColor >> 8 & 0xFF) * scale) / 100) << 8) | (projectileColor & 0xFF);            }

            // Create a new shield projectile
            shieldProjectile = Projectile::spawn(
                playerPos,
                {{0, 0, 0}}, // No velocity
                0.0f, // No speed
                0.0f, // No slowdown
                shieldLifetime,
                damage,
                projectileColor,
                shieldSize
            );
            
            shieldActive = true;
            fireCooldown = shieldLifetime;
        }

        // Update projectile position if active
        if (shieldActive && shieldProjectile && player) {
            T3DVec3 playerPos = player->getPosition();
            playerPos.z += 1.0f; // Slightly in front of player
            shieldProjectile->setPosition(playerPos);
        }

        // Deactivate shield if lifetime is over
        if (shieldActive && fireCooldown <= 0) {
            shieldActive = false;
            if (shieldProjectile) {
                shieldProjectile->deactivate();
                shieldProjectile = nullptr;
            }
            fireCooldown = shieldCooldown; // Start the cooldown period
        }
    }
    
    void WeaponShield::draw3D(float deltaTime) {
        // The projectile is drawn by the scene, so we don't need to draw it here
    }
    
    void WeaponShield::drawPTX(float deltaTime) {
        // No particle effects for this weapon
    }
    
    void WeaponShield::fire(const T3DVec3& position, const T3DVec3& direction) {
        // Not used for shield weapon
    }

    void WeaponShield::fireManual() {
        // Force the cooldown to end if it's not active
        if (!shieldActive && fireCooldown > 0) {
            fireCooldown = 0;
        }
    }

    void WeaponShield::upgrade() {
        if (upgradeLevel < maxUpgradeLevel) {
            upgradeLevel++;
            
            // Improve shield stats with each upgrade
            damage += 2; // Increase damage dealt to enemies
            shieldCooldown *= 0.85f; // Reduce cooldown time
            shieldLifetime *= 1.1f; // Increase duration
        }
    }
}