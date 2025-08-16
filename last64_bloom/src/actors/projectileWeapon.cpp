/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "projectileWeapon.h"
#include <libdragon.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Actor {
    ProjectileWeapon::ProjectileWeapon() : Weapon() {
        // Initialize projectile pool
        Projectile::initialize();
        
        // Set weapon-specific properties
        fireRate = 0.9f;
        projectileSpeed = 200.0f;
        projectileSlowdown = 5.0f;

        // Default Parameters
        maxUpgradeLevel = 5;
        spawnOffset = {0, 0, 0};
    }
    
    ProjectileWeapon::~ProjectileWeapon() {
        // Cleanup projectile pool
        Projectile::cleanup();
    }
    
    void ProjectileWeapon::update(float deltaTime) {
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
    
    void ProjectileWeapon::draw3D(float deltaTime) {
        // Draw all projectiles
        Projectile::drawAll(deltaTime);
    }
    
    void ProjectileWeapon::drawPTX(float deltaTime) {
        // No particle effects for this weapon
    }
    
    void ProjectileWeapon::fire(const T3DVec3& position, const T3DVec3& direction) {
        // Calculate spawn position with offset
        T3DVec3 spawnPos = {{
            position.x + spawnOffset.x,
            position.y + spawnOffset.y,
            position.z + spawnOffset.z
        }};
        
        // Spawn projectile
        Projectile::spawn(spawnPos, direction, projectileSpeed, projectileSlowdown);
    }

    void ProjectileWeapon::fireManual() {
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

    void ProjectileWeapon::upgrade() {
        if (upgradeLevel < maxUpgradeLevel) {
            upgradeLevel++;
            // Increase fire rate for each upgrade
            fireRate *= 0.5f; //% increase in fire rate (}
        }
    };
}