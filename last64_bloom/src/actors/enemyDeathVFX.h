/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "../actors/base.h"
#include <t3d/t3d.h>

#define MAX_ENEMY_DEATH_VFX 100

namespace Actor {
    class EnemyDeathVFX : public Base {
    private:
        static T3DVertPacked* sharedVertices;
        static T3DMat4FP** sharedMatrices;
        static bool* activeFlags;
        static uint32_t activeCount;
        static bool initialized;
        static EnemyDeathVFX vfxPool[MAX_ENEMY_DEATH_VFX]; // Static pool of VFX objects

        T3DVec3 position;
        float lifetime;
        float maxLifetime;
        uint32_t poolIndex;
        uint32_t color;
        float size;

        static void initializePool();

    public:
        EnemyDeathVFX();
        ~EnemyDeathVFX();

        static void initialize();
        static void cleanup();
        static EnemyDeathVFX* spawn(const T3DVec3& position, float size, uint32_t color);
        static void updateAll(float deltaTime);
        static void drawAll(float deltaTime);
        static uint32_t getActiveCount() { return activeCount; }

        void update(float deltaTime) override;
        void draw3D(float deltaTime) override;
        void drawPTX(float deltaTime) override;

        void deactivate();
        bool isActive() const;
    };
}