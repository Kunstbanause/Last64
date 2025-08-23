/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weapon_homing.h"
#include "../actors/player.h"
#include <libdragon.h>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define a new color for homing projectiles (bright orange)
#define HOMING_PROJECTILE_COLOR 0xFFFF8040

namespace Actor {
    WeaponHoming::WeaponHoming() : WeaponBase(WeaponType::HOMING) {
        // Initialize projectile pool
        Projectile::initialize();
        
        // Set weapon-specific properties
        fireRate = 1.5f;              // Slower fire rate
        projectileSpeed = 150.0f;     // Slower projectile speed
        projectileSlowdown = 100.0f;  // Less slowdown
        detectionRange = 100.0f;      // Detection range for enemies

        // Default Parameters
        maxUpgradeLevel = 3;          // Fewer upgrade levels
        spawnOffset = {0, 0, 0};
    }
    
    WeaponHoming::~WeaponHoming() {
        // Cleanup projectile pool
        Projectile::cleanup();
    }
    
    void WeaponHoming::update(float deltaTime) {
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
    
    void WeaponHoming::draw3D(float deltaTime) {
        // No longer drawing projectiles here - handled by scene
    }
    
    void WeaponHoming::drawPTX(float deltaTime) {
        // No particle effects for this weapon
    }
    
    void WeaponHoming::fire(const T3DVec3& position, const T3DVec3& direction) {
        // Calculate spawn position with offset
        T3DVec3 spawnPos = {{
            position.x + spawnOffset.x,
            position.y + spawnOffset.y,
            position.z + spawnOffset.z
        }};
        
        // Find the closest enemy
        Actor::Enemy* closestEnemy = nullptr;
        float closestDistanceSq = detectionRange * detectionRange;
        
        // Iterate through all enemies to find the closest one
        for (uint32_t i = 0; i < MAX_ENEMIES; i++) {
            if (Actor::Enemy::isActive(i)) {
                Actor::Enemy* enemy = Actor::Enemy::getEnemy(i);
                if (enemy && enemy->isActive()) {
                    T3DVec3 enemyPos = enemy->getPosition();
                    
                    // Calculate squared distance to avoid sqrt
                    float dx = enemyPos.x - spawnPos.x;
                    float dy = enemyPos.y - spawnPos.y;
                    float distanceSq = dx * dx + dy * dy;
                    
                    // Check if this enemy is closer
                    if (distanceSq < closestDistanceSq) {
                        closestDistanceSq = distanceSq;
                        closestEnemy = enemy;
                    }
                }
            }
        }
        
        // Determine firing direction
        T3DVec3 fireDirection = direction;
        
        // If we found a close enemy, aim toward it
        if (closestEnemy != nullptr) {
            T3DVec3 enemyPos = closestEnemy->getPosition();
            
            // Calculate direction to enemy
            fireDirection.x = enemyPos.x - spawnPos.x;
            fireDirection.y = enemyPos.y - spawnPos.y;
            fireDirection.z = enemyPos.z - spawnPos.z;
            
            // Normalize direction
            float length = sqrtf(fireDirection.x * fireDirection.x + 
                                fireDirection.y * fireDirection.y + 
                                fireDirection.z * fireDirection.z);
            if (length > 0.0f) {
                fireDirection.x /= length;
                fireDirection.y /= length;
                fireDirection.z /= length;
            }
        }
        
        // Spawn a projectile with double damage and special color
        Projectile::spawn(spawnPos, fireDirection, projectileSpeed, projectileSlowdown, 8, HOMING_PROJECTILE_COLOR);
    }

    void WeaponHoming::fireManual() {
        if(!player) return;

        // Fire in the player's forward direction
        float playerRotation = player->getRotation();
        T3DVec3 direction = {{
            sinf(playerRotation),
            cosf(playerRotation),
            0.0f
        }};

        fire(player->getPosition(), direction);
    }

    void WeaponHoming::upgrade() {
        if (upgradeLevel < maxUpgradeLevel) {
            upgradeLevel++;
            // Increase fire rate and projectile speed for each upgrade
            fireRate *= 0.9f;     // Increase fire rate
            projectileSpeed *= 1.1f; // Increase projectile speed
            detectionRange *= 1.1f;  // Increase detection range
        }
    }
}