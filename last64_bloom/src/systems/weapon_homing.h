/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "weapon_base.h"
#include "../actors/projectile.h"
#include "../actors/enemy.h"
#include <t3d/t3d.h>

namespace Actor {
    class WeaponHoming : public WeaponBase {
    public:
        WeaponHoming();
        ~WeaponHoming();
        
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
        float projectileLifetime;     // Lifetime of projectiles
        float detectionRange;        // Range to detect enemies
    };
}