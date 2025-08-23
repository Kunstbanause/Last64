/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "circular_projectile_weapon.h"
#include "../actors/player.h"
#include <libdragon.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define a new color for circular projectiles (bright purple)
#define CIRCULAR_PROJECTILE_COLOR 0xFF00FFFF

namespace Actor {
    CircularProjectileWeapon::CircularProjectileWeapon() : Weapon() {
        // Initialize projectile pool
        Projectile::initialize();
        
        // Set weapon-specific properties
        fireRate = 2.0f;              // Slower fire rate
        projectileSpeed = 100.0f;     // Slower projectile speed
        projectileSlowdown = 50.0f;   // Less slowdown
        projectileCount = 8;          // Number of projectiles in circle

        // Default Parameters
        maxUpgradeLevel = 3;          // Fewer upgrade levels
        spawnOffset = {0, 0, 0};
    }
    
    CircularProjectileWeapon::~CircularProjectileWeapon() {
        // Cleanup projectile pool
        Projectile::cleanup();
    }
    
    void CircularProjectileWeapon::update(float deltaTime) {
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
    
    void CircularProjectileWeapon::draw3D(float deltaTime) {
        // No longer drawing projectiles here - handled by scene
    }
    
    void CircularProjectileWeapon::drawPTX(float deltaTime) {
        // No particle effects for this weapon
    }
    
    void CircularProjectileWeapon::fire(const T3DVec3& position, const T3DVec3& direction) {
        // Calculate spawn position with offset
        T3DVec3 spawnPos = {{
            position.x + spawnOffset.x,
            position.y + spawnOffset.y,
            position.z + spawnOffset.z
        }};
        
        // Fire projectiles in a circular pattern
        for (int i = 0; i < projectileCount + upgradeLevel; i++) {
            // Calculate angle for this projectile
            float angle = (2.0f * M_PI * i) / (projectileCount + upgradeLevel);
            
            // Calculate direction vector
            T3DVec3 fireDirection = {{
                cosf(angle),
                sinf(angle),
                0.0f
            }};
            
            // Spawn projectile with circular direction (damage = 1) and special color
            Projectile::spawn(spawnPos, fireDirection, projectileSpeed, projectileSlowdown, 1, CIRCULAR_PROJECTILE_COLOR);
        }
    }

    void CircularProjectileWeapon::fireManual() {
        if(!player) return;

        // Fire in a circular pattern around the player
        T3DVec3 playerPos = player->getPosition();
        fire(playerPos, {{0, 0, 0}});
    }

    void CircularProjectileWeapon::upgrade() {
        if (upgradeLevel < maxUpgradeLevel) {
            upgradeLevel++;
            // Increase fire rate and number of projectiles for each upgrade
            fireRate *= 0.9f;     // Increase fire rate
            // Number of projectiles increases with upgrade level in the fire() method
        }
    };
}