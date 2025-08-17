/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "../actors/base.h"
#include <t3d/t3d.h>

#define MAX_PROJECTILES 100

namespace Actor {
    class Projectile : public Base {
    private:
        // Static data for the projectile pool
        static T3DVertPacked* sharedVertices;
        static T3DMat4FP** sharedMatrices;
        static bool* activeFlags;
        static uint32_t activeCount;
        static bool initialized;
        static Projectile projectilePool[MAX_PROJECTILES];

        // Per-projectile data
        T3DVec3 position;
        T3DVec3 velocity;
        float speed;
        float slowdown; // Slowdown factor per second
        float lifetime;
        float maxLifetime;
        uint32_t poolIndex;

        static void initializePool();

    public:
        Projectile();
        ~Projectile();

        // Static methods for managing the pool
        static void initialize();
        static void cleanup();
        static Projectile* spawn(const T3DVec3& position, const T3DVec3& velocity, float speed, float slowdown);
        static void updateAll(float deltaTime);
        static void drawAll(float deltaTime);
        static uint32_t getActiveCount() { return activeCount; }
        static Projectile* getProjectile(uint32_t index) { return &projectilePool[index]; }

        // Overrides from Base
        void update(float deltaTime) override;
        void draw3D(float deltaTime) override;
        void drawPTX(float deltaTime) override;

        void deactivate();
        bool isActive() const;

        T3DVec3 getPosition() const override { return position; }
        float getRadius() const override { return 2.0f; } // Projectiles are 2x2 quads
    };
}