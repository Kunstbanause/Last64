/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "../actors/base.h"
#include "../actors/projectile.h"
#include "../systems/weapon_base.h"
#include "../audio.h"
#include <t3d/t3d.h>
#include <libdragon.h>

namespace Actor {
    // Forward declarations
    class WeaponBase;
    class Player : public Base {
    private:
        static T3DVertPacked* sharedVertices;
        static T3DMat4FP* sharedMatrix;
        static bool initialized;
        
        T3DVertPacked* playerVertices; // Per-player vertices
        T3DMat4FP* playerMatrix; // Per-player matrix
        
        T3DVec3 position;
        T3DVec3 velocity;
        float speed;
        float rotation;
        joypad_port_t playerPort; // To identify which controller this player uses
        uint32_t playerColor; // Store player-specific color
        bool isDead;
        int health;
        int maxHealth;
        
        // Weapons reference - all weapons active simultaneously
        WeaponBase* weapon1;  // Standard projectile weapon
        WeaponBase* weapon2;  // Homing projectile weapon
        WeaponBase* weapon3;  // Third weapon (we'll add a new one)
        
        static void initialize();
        static void cleanup();
        
    public:
        Player(T3DVec3 startPos, joypad_port_t port);
        ~Player();
        void update(float deltaTime) override;
        void draw3D(float deltaTime) override;
        void drawPTX(float deltaTime) override;
        
        T3DVec3 getPosition() const { return position; }
        void setPosition(T3DVec3 newPos) { position = newPos; }
        float getRotation() const { return rotation; }
        
        void takeDamage(int amount);
        void kill() { isDead = true; playerColor = 0xFF0000FF; gSFXManager.play(SFXManager::SFX_DEATH);}
        bool getIsDead() const { return isDead; }
        bool collidesWith(Base* other);
        
        // Weapon methods
        WeaponBase* getWeapon1() const { return weapon1; }
        WeaponBase* getWeapon2() const { return weapon2; }
        WeaponBase* getWeapon3() const { return weapon3; }
        
        static void initializePlayer() { initialize(); }
        static void cleanupPlayer() { cleanup(); }
    };
}