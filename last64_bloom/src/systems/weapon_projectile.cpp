/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weapon_projectile.h"
#include <libdragon.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Actor {
    WeaponProjectile::WeaponProjectile() : WeaponBase(WeaponType::PROJECTILE) {
        // Set weapon-specific properties
        fireRate = 0.9f;
        projectileSpeed = 200.0f;
        projectileSlowdown = 200.0f;
        spreadAngle = 0.15f;          // 0.15 radians spread (about 8.6 degrees) for tighter spread
        projectilesPerBurst = 3;     // Start with 3 projectiles per burst

        // Default Parameters
        maxUpgradeLevel = 5;
        spawnOffset = {0, 0, 0};
        damage = 4;
    }
    
    WeaponProjectile::~WeaponProjectile() {
        // No cleanup needed - projectile system is managed by the scene
    }
    
    void WeaponProjectile::update(float deltaTime) {
        // Update fire cooldown
        if (fireCooldown > 0) {
            fireCooldown -= deltaTime;
        }
        
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
    
    void WeaponProjectile::draw3D(float deltaTime) {
        // No longer drawing projectiles here - handled by scene
    }
    
    void WeaponProjectile::drawPTX(float deltaTime) {
        // No particle effects for this weapon
    }
    
    void WeaponProjectile::fire(const T3DVec3& position, const T3DVec3& direction) {
        // Calculate spawn position with offset
        T3DVec3 spawnPos = {{
            position.x + spawnOffset.x,
            position.y + spawnOffset.y,
            position.z + spawnOffset.z
        }};
        
        // Calculate the number of projectiles to fire based on upgrade level
        // Limit to a maximum of 6 projectiles to prevent memory issues
        int totalProjectiles = projectilesPerBurst + upgradeLevel;
        if (totalProjectiles > 6) {
            totalProjectiles = 6;
        }
        
        // Fire a burst of projectiles with slight spread
        for (int i = 0; i < totalProjectiles; i++) {
            // Calculate spread for this projectile
            // Create a spread that's centered around the main direction
            float spread = spreadAngle * (i - (totalProjectiles - 1) / 2.0f);
            
            // Apply spread to direction (rotate around Z-axis)
            T3DVec3 spreadDirection = {{
                direction.x * cosf(spread) - direction.y * sinf(spread),
                direction.x * sinf(spread) + direction.y * cosf(spread),
                direction.z
            }};
            
            // Spawn projectile with spread direction, default color)
            Projectile::spawn(spawnPos, spreadDirection, projectileSpeed, projectileSlowdown, damage, DEFAULT_PROJECTILE_COLOR);
        }
    }

    void WeaponProjectile::fireManual() {
        if(!player)return;

        // Fire in a random direction
        float randomAngle = (float)(rand() % 360) * (M_PI / 180.0f);
        T3DVec3 direction = {{
            sinf(randomAngle),
            cosf(randomAngle),
            0.0f
        }};

        fire(player->getPosition(), direction);
    }

    void WeaponProjectile::upgrade() {
        if (upgradeLevel < maxUpgradeLevel) {
            upgradeLevel++;
            // Increase fire rate for each upgrade
            fireRate *= 0.9f; // increase in fire rate
            // Note: Additional projectiles are handled in the fire() method
        }
    }
}