/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "../actors/base.h"
#include <t3d/t3d.h>
#include <unordered_map>

#define MAX_SHAPES 50

namespace Actor {
    class Shape : public Base {
    private:
        // Static data for the shape pool
        static T3DVertPacked* sharedVertices;
        static T3DMat4FP** sharedMatrices;
        static bool* activeFlags;
        static uint32_t activeCount;
        static bool initialized;
        static Shape shapePool[MAX_SHAPES];

        // Per-shape data
        T3DVec3 position;
        T3DVec3 offset; // Offset from the attached object
        Base* attachedTo; // Object this shape is attached to (nullptr if not attached)
        float lifetime;
        float maxLifetime;
        float attackFrequency; // Time between attacks on the same enemy
        uint32_t poolIndex;
        int damage; // Damage dealt by this shape
        uint32_t color; // Color of this shape
        float size; // Size of this shape (scale factor)
        std::unordered_map<uint32_t, float> enemyAttackTimers; // Track attack timers per enemy

        static void initializePool();

    public:
        Shape();
        ~Shape();

        // Static methods for managing the pool
        static void initialize();
        static void cleanup();
        static Shape* spawn(const T3DVec3& position, float maxLifetime, float attackFrequency, int damage, uint32_t color = 0xFF00FFFF, float size = 1.0f);
        static Shape* spawnAttached(Base* attachTo, const T3DVec3& offset, float maxLifetime, float attackFrequency, int damage, uint32_t color = 0xFF00FFFF, float size = 1.0f);
        static void updateAll(float deltaTime);
        static void drawAll(float deltaTime);
        static uint32_t getActiveCount() { return activeCount; }
        static Shape* getShape(uint32_t index) { return &shapePool[index]; }

        // Overrides from Base
        void update(float deltaTime) override;
        void draw3D(float deltaTime) override;
        void drawPTX(float deltaTime) override;

        void deactivate();
        bool isActive() const;

        T3DVec3 getPosition() const override { 
            if (attachedTo) {
                T3DVec3 attachPos = attachedTo->getPosition();
                return {attachPos.x + offset.x, attachPos.y + offset.y, attachPos.z + offset.z};
            }
            return position; 
        }
        void setPosition(const T3DVec3& newPosition) { position = newPosition; }
        float getRadius() const override { return 2.0f * size; } // Shapes are 2x2 quads, scaled by size
        void getAABBSize(float& width, float& height) const override; // Shapes are 4x4 quads
        int getDamage() const { return damage; }
        uint32_t getColor() const { return color; }
        void setColor(uint32_t newColor) { color = newColor; }
        
        // Shape-specific methods
        bool canDamageEnemy(uint32_t enemyId) const;
        void registerEnemyHit(uint32_t enemyId);
        void setAttachedTo(Base* attachTo, const T3DVec3& attachOffset) { 
            attachedTo = attachTo; 
            offset = attachOffset;
        }
    };
}