/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weapon_spiral.h"
#include "../actors/player.h"
#include <libdragon.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define a new color for spiral projectiles
#define SPIRAL_PROJECTILE_COLOR 0x08040000

namespace Actor {
    WeaponSpiral::WeaponSpiral() : WeaponBase(WeaponType::SPIRAL) {
        // Set weapon-specific properties
        fireRate = 1.5f;              // Moderate fire rate
        projectileSpeed = 120.0f;     // Moderate projectile speed
        projectileSlowdown = 30.0f;   // Some slowdown
        projectileLifetime = 1.2f;    // Longer lifetime for spiral projectiles
        baseProjectileCount = 1;      // Start with 1 projectile
        spiralTightness = 0.5f;       // How tightly the spiral is wound
        rotationSpeed = 2.0f;         // Speed of spiral rotation
        currentRotation = 0.0f;       // Current rotation

        // Default Parameters
        maxUpgradeLevel = 3;          // 3 upgrade levels
        spawnOffset = {0, 0, 0};
        damage = 4;
    }
    
    WeaponSpiral::~WeaponSpiral() {
        // No cleanup needed - projectile system is managed by the scene
    }
    
    void WeaponSpiral::update(float deltaTime) {
        // Update fire cooldown
        if (fireCooldown > 0) {
            fireCooldown -= deltaTime;
        }
        
        // Update spiral rotation
        currentRotation += rotationSpeed * deltaTime;
        
        // Auto-fire logic
        if (fireCooldown <= 0 && player) {
            fireCooldown = fireRate;
            
            T3DVec3 playerPos = player->getPosition();
            float playerRotation = player->getRotation();
            
            T3DVec3 direction = {{
                sinf(playerRotation),
                cosf(playerRotation),
                0.0f
            }};
            fire(playerPos, direction);
        }
    }
    
    void WeaponSpiral::draw3D(float deltaTime) {
        // No longer drawing projectiles here - handled by scene
    }
    
    void WeaponSpiral::drawPTX(float deltaTime) {
        // No particle effects for this weapon
    }
    
    void WeaponSpiral::fire(const T3DVec3& position, const T3DVec3& direction) {
        // Calculate spawn position with offset
        T3DVec3 spawnPos = {{
            position.x + spawnOffset.x,
            position.y + spawnOffset.y,
            position.z + spawnOffset.z
        }};
        
        // Fire projectiles in a spiral pattern
        int projectileCount = baseProjectileCount + upgradeLevel;
        for (int i = 0; i < projectileCount; i++) {
            // Calculate angle for this projectile based on spiral pattern
            float angle = currentRotation + (2.0f * M_PI * i) / projectileCount;
            
            // Add spiral offset based on time and tightness
            float spiralOffset = spiralTightness * currentRotation;
            float finalAngle = angle + spiralOffset;
            
            // Calculate direction vector
            T3DVec3 fireDirection = {{
                cosf(finalAngle),
                sinf(finalAngle),
                0.0f
            }};
            
            // Spawn projectile with spiral direction and special color
            Projectile::spawn(spawnPos, fireDirection, projectileSpeed, projectileSlowdown, projectileLifetime, damage, SPIRAL_PROJECTILE_COLOR);
        }
    }

    void WeaponSpiral::fireManual() {
        if(!player) return;

        // Fire in a spiral pattern around the player
        T3DVec3 playerPos = player->getPosition();
        fire(playerPos, {{0, 0, 0}});
    }

    void WeaponSpiral::upgrade() {
        if (upgradeLevel < maxUpgradeLevel) {
            upgradeLevel++;
            // Increase fire rate and number of projectiles for each upgrade
            fireRate *= 0.85f;     // Increase fire rate
            // Number of projectiles increases with upgrade level in the fire() method
        }
    }
}