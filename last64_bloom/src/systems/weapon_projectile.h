/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "weapon_base.h"
#include "../actors/projectile.h"
#include <t3d/t3d.h>

namespace Actor {
    class WeaponProjectile : public WeaponBase {
    public:
        WeaponProjectile();
        ~WeaponProjectile();
        
        void update(float deltaTime) override;
        void draw3D(float deltaTime) override;
        void drawPTX(float deltaTime) override;
        
        void fire(const T3DVec3& position, const T3DVec3& direction) override;
        void fireManual() override;
        void upgrade() override;

    private:
        float fireRate;
        float projectileSpeed;
        float projectileSlowdown;
        float projectileLifetime;    // Lifetime of projectiles
        float spreadAngle;           // Angle between projectiles in a burst
        int projectilesPerBurst;     // Number of projectiles per burst
    };
}