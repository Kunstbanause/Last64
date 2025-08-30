/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weapon_shield.h"
#include "../actors/player.h"
#include "../actors/enemy.h"
#include <libdragon.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define a new color for shield projectiles (bright blue)
#define SHIELD_PROJECTILE_COLOR 0xFF4080FF

namespace Actor {
    WeaponShield::WeaponShield() : WeaponBase(WeaponType::SHIELD) {
        // Set weapon-specific properties
        fireRate = 0.0f;              // Not used for shield
        shieldRadius = 30.0f;         // Radius of the shield
        shieldLifetime = 5.0f;        // Shield lasts 5 seconds
        shieldCooldown = 10.0f;       // 10 second cooldown
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
        // Update shield rotation for visual effect
        shieldRotation += 1.0f * deltaTime;
        if (shieldRotation > 2.0f * M_PI) {
            shieldRotation -= 2.0f * M_PI;
        }
        
        // Update fire cooldown
        if (fireCooldown > 0) {
            fireCooldown -= deltaTime;
        }
        
        // Deactivate shield when duration is over
        if (shieldActive && fireCooldown <= 0) {
            shieldActive = false;
            // Don't set cooldown here - it's set when shield is activated
        }
        
        // Update shield projectile position if active
        if (shieldActive && shieldProjectile && player) {
            T3DVec3 playerPos = player->getPosition();
            playerPos.z += 1.0f; // Slightly in front of player
            shieldProjectile->setPosition(playerPos);
            
            // Update projectile matrix for rotation
            // This would require access to the projectile's matrix, which is not directly available
            // We'll handle rotation in the drawing code instead
        }
        
        // Check for enemy collisions if shield is active
        if (shieldActive && player) {
            T3DVec3 playerPos = player->getPosition();
            
            // Check collisions with all enemies
            for (uint32_t i = 0; i < MAX_ENEMIES; i++) {
                if (Actor::Enemy::isActive(i)) {
                    Actor::Enemy* enemy = Actor::Enemy::getEnemy(i);
                    if (enemy && enemy->isActive()) {
                        T3DVec3 enemyPos = enemy->getPosition();
                        
                        // Calculate distance between player and enemy
                        float dx = enemyPos.x - playerPos.x;
                        float dy = enemyPos.y - playerPos.y;
                        float distance = sqrtf(dx * dx + dy * dy);
                        
                        // Check if enemy is within shield radius
                        if (distance <= shieldRadius) {
                            // Deal damage to enemy
                            enemy->takeDamage(damage);
                        }
                    }
                }
            }
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
        // Manually activate shield if available
        if (!shieldActive && fireCooldown <= 0 && player) {
            // Activate shield
            shieldActive = true;
            fireCooldown = shieldLifetime; // Use fireCooldown to track shield duration
            
            // Create shield projectile
            T3DVec3 playerPos = player->getPosition();
            playerPos.z += 1.0f; // Slightly in front of player
            
            // Create a stationary projectile for the shield
            shieldProjectile = Projectile::spawn(
                playerPos,
                {{0, 0, 0}}, // No velocity
                0.0f, // No speed
                0.0f, // No slowdown
                shieldLifetime, // Lifetime matches shield duration
                damage,
                SHIELD_PROJECTILE_COLOR,
                shieldRadius / 2.0f // Scale based on radius (projectiles are 2 units by default)
            );
            
            // Play shield activation sound
            // gSFXManager.play(SFXManager::SFX_SHIELD); // Uncomment when sound is available
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