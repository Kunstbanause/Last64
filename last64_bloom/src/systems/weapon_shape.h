/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "weapon_base.h"
#include "../actors/shape.h"
#include <t3d/t3d.h>

namespace Actor {
    class WeaponShape : public WeaponBase {
    public:
        WeaponShape();
        ~WeaponShape();
        
        void update(float deltaTime) override;
        void draw3D(float deltaTime) override;
        void drawPTX(float deltaTime) override;
        
        void fire();
        void fireManual() override;
        void upgrade() override;

    private:
        float shapeLifetime;         // How long shapes last
        float attackFrequency;       // Time between attacks on the same enemy
        float weaponCooldown;        // Cooldown between placing shapes
        float shapeWidth;            // Width of the shapes
        float shapeHeight;           // Height of the shapes
        int shapeDamage;             // Damage per attack
        Shape* currentShape;         // Currently active shape
    };
}