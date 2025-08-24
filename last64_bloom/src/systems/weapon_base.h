/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "../actors/base.h"
#include "../actors/player.h"
#include <t3d/t3d.h>

namespace Actor {
    // Forward declarations
    class Projectile;
    class Player;
    
    enum class WeaponType {
        PROJECTILE,
        HOMING,
        CIRCULAR,
        SPIRAL
    };
    
    class WeaponBase : public Base {
    protected:
        Player* player;              // Reference to the player owning this weapon
        float fireCooldown;          // Current cooldown timer
        float fireRate;              // Time between shots (seconds)
        int upgradeLevel;            // Current upgrade level
        int maxUpgradeLevel;         // Maximum upgrade level
        int damage;                  // Damage per projectile
        T3DVec3 spawnOffset;         // Offset from firing position
        WeaponType weaponType;       // Type of weapon for identification
        
    public:
        WeaponBase(WeaponType type);
        ~WeaponBase();
        
        virtual void update(float deltaTime) override = 0;
        virtual void draw3D(float deltaTime) override = 0;
        virtual void drawPTX(float deltaTime) override = 0;
        
        virtual void fire(const T3DVec3& position, const T3DVec3& direction);
        virtual void fireManual();
        virtual void upgrade();
        
        // Getters and setters
        void setPlayer(Player* p) { player = p; }
        Player* getPlayer() const { return player; }
        
        int getUpgradeLevel() const { return upgradeLevel; }
        int getMaxUpgradeLevel() const { return maxUpgradeLevel; }
        WeaponType getWeaponType() const { return weaponType; }
        
        T3DVec3 getSpawnOffset() const { return spawnOffset; }
        void setSpawnOffset(const T3DVec3& offset) { spawnOffset = offset; }
    };
}