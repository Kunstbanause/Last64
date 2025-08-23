/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "base_weapon.h"
#include "../actors/projectile.h"
#include <t3d/t3d.h>

namespace Actor {
    class ProjectileWeapon : public Weapon {
    public:
        ProjectileWeapon();
        ~ProjectileWeapon();
        
        void update(float deltaTime) override;
        void draw3D(float deltaTime) override;
        void drawPTX(float deltaTime) override;
        
        void fire(const T3DVec3& position, const T3DVec3& direction) override;
        void fireManual() override;
        void upgrade();

    private:
        float fireRate;
        float projectileSpeed;
        float projectileSlowdown;
    };
}