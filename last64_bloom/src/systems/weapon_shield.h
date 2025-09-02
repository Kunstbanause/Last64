/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "weapon_base.h"
#include "../actors/projectile.h"
#include <t3d/t3d.h>

namespace Actor {
    class WeaponShield : public WeaponBase {
    public:
        WeaponShield();
        ~WeaponShield();
        
        void update(float deltaTime) override;
        void draw3D(float deltaTime) override;
        void drawPTX(float deltaTime) override;
        
        void fire(const T3DVec3& position, const T3DVec3& direction) override;
        void fireManual() override;
        void upgrade() override;

    private:
        float shieldSize;             // Size of the shield
        float shieldLifetime;         // How long the shield lasts
        float shieldCooldown;         // Cooldown between shield uses
        bool shieldActive;            // Whether the shield is currently active
        float shieldRotation;         // Rotation of the shield for visual effect
        Projectile* shieldProjectile; // Projectile used for the shield visual
    };
}