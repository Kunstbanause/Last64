/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "../actors/base.h"
#include "../actors/projectile.h"
#include "../systems/base_weapon.h"
#include "../audio.h"
#include <t3d/t3d.h>
#include <libdragon.h>

namespace Actor {
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
        
        // Weapons reference
        Weapon* weapon1;  // Standard projectile weapon
        Weapon* weapon2;  // Homing projectile weapon
        Weapon* activeWeapon;  // Currently active weapon
        int activeWeaponIndex; // 0 for weapon1, 1 for weapon2
        
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
        Weapon* getActiveWeapon() const { return activeWeapon; }
        void switchWeapon();
        
        static void initializePlayer() { initialize(); }
        static void cleanupPlayer() { cleanup(); }
    };
}