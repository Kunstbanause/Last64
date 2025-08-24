/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weapon_spiral.h"
#include "../actors/player.h"
#include "../actors/enemy.h"
#include "../actors/projectile.h"
#include <libdragon.h>
#include <cmath>
#include <vector>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define a new color for spiral projectiles (bright orange)
#define SPIRAL_PROJECTILE_COLOR 0xFFFF8000

namespace Actor {
    // Store orbiting projectile data
    struct OrbitingProjectile {
        Projectile* projectile;
        float angleOffset;
        float distance;
        float rotationSpeed;
        float lifetime;
        float maxLifetime;
        int slotIndex;
    };
    
    // Static storage for orbiting projectiles
    static std::vector<OrbitingProjectile> orbitingProjectiles;
    static const int MAX_ORBITING_PROJECTILES = 20;
    
    WeaponSpiral::WeaponSpiral() : WeaponBase(WeaponType::SPIRAL) {
        // Set weapon-specific properties
        fireRate = 3.0f;              // Rate at which new projectiles are added
        projectileSpeed = 0.0f;       // Not used for orbiting projectiles
        projectileSlowdown = 0.0f;    // No slowdown
        projectileLifetime = 10.0f;   // Lifetime of orbiting projectiles
        baseProjectileCount = 1;      // Start with 1 projectile
        spiralTightness = 1.5f;       // How fast projectiles orbit
        rotationSpeed = 2.5f;         // Base rotation speed
        currentRotation = 0.0f;       // Current rotation

        // Default Parameters
        maxUpgradeLevel = 3;          // 3 upgrade levels
        spawnOffset = {0, 0, 0};
        damage = 4;
    }
    
    WeaponSpiral::~WeaponSpiral() {
        // Clean up any remaining orbiting projectiles
        orbitingProjectiles.clear();
    }
    
    void WeaponSpiral::update(float deltaTime) {
        // Update fire cooldown
        if (fireCooldown > 0) {
            fireCooldown -= deltaTime;
        }
        
        // Auto-fire logic - create new orbiting projectiles
        if (fireCooldown <= 0 && player && orbitingProjectiles.size() < MAX_ORBITING_PROJECTILES) {
            fireCooldown = fireRate;
            
            T3DVec3 playerPos = player->getPosition();
            fire(playerPos, {{0, 0, 0}});
        }
        
        // Update all orbiting projectiles
        if (!player) return;
        T3DVec3 playerPos = player->getPosition();
        
        for (auto it = orbitingProjectiles.begin(); it != orbitingProjectiles.end();) {
            OrbitingProjectile& orbit = *it;
            
            // Check if projectile is still active
            if (!orbit.projectile || !orbit.projectile->isActive()) {
                it = orbitingProjectiles.erase(it);
                continue;
            }
            
            // Update lifetime
            orbit.lifetime -= deltaTime;
            if (orbit.lifetime <= 0) {
                orbit.projectile->deactivate();
                it = orbitingProjectiles.erase(it);
                continue;
            }
            
            // Update angle
            orbit.angleOffset += orbit.rotationSpeed * deltaTime;
            
            // Calculate new position around player
            T3DVec3 newPos = {{
                playerPos.x + cosf(orbit.angleOffset) * orbit.distance,
                playerPos.y + sinf(orbit.angleOffset) * orbit.distance,
                playerPos.z
            }};
            
            // Update projectile position directly
            orbit.projectile->setPosition(newPos);
            
            // Update projectile matrix for rendering
            if (orbit.projectile->isActive()) {
                // This would require access to the projectile's matrix, which we don't have
                // We'll need to work within the existing system
            }
            
            ++it;
        }
    }
    
    void WeaponSpiral::draw3D(float deltaTime) {
        // No special drawing needed - projectiles are drawn by the scene
    }
    
    void WeaponSpiral::drawPTX(float deltaTime) {
        // No particle effects for this weapon
    }
    
    void WeaponSpiral::fire(const T3DVec3& position, const T3DVec3& direction) {
        // Create orbiting projectiles around the player
        int projectileCount = std::min(baseProjectileCount + upgradeLevel, 4);
        
        for (int i = 0; i < projectileCount; i++) {
            // Skip if we've reached the maximum
            if (orbitingProjectiles.size() >= MAX_ORBITING_PROJECTILES) {
                break;
            }
            
            // Calculate initial angle for this projectile
            float angle = (2.0f * M_PI * i) / projectileCount;
            
            // Set distance from player
            float distance = 25.0f;
            
            // Calculate spawn position around player
            T3DVec3 spawnPos = {{
                position.x + cosf(angle) * distance,
                position.y + sinf(angle) * distance,
                position.z
            }};
            
            // Spawn a stationary projectile (velocity = 0)
            Projectile* projectile = Projectile::spawn(
                spawnPos, 
                {{0, 0, 0}}, 
                0.0f, 
                0.0f, 
                projectileLifetime, 
                damage, 
                SPIRAL_PROJECTILE_COLOR
            );
            
            // Store projectile for orbiting
            if (projectile) {
                OrbitingProjectile orbitProj;
                orbitProj.projectile = projectile;
                orbitProj.angleOffset = angle;
                orbitProj.distance = distance;
                orbitProj.rotationSpeed = spiralTightness * rotationSpeed;
                orbitProj.lifetime = projectileLifetime;
                orbitProj.maxLifetime = projectileLifetime;
                orbitProj.slotIndex = orbitingProjectiles.size();
                orbitingProjectiles.push_back(orbitProj);
            }
        }
    }

    void WeaponSpiral::fireManual() {
        if(!player) return;

        // Fire orbiting projectiles around the player
        T3DVec3 playerPos = player->getPosition();
        fire(playerPos, {{0, 0, 0}});
    }

    void WeaponSpiral::upgrade() {
        if (upgradeLevel < maxUpgradeLevel) {
            upgradeLevel++;
            // Decrease fire rate (more frequent spawning) and increase rotation speed
            fireRate *= 0.8f;
            rotationSpeed *= 1.2f;
        }
    }
}