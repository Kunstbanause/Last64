/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "../actors/base.h"
#include "player.h"
#include <t3d/t3d.h>

#define MAX_ENEMIES 100

namespace Actor {
    // Enemy size types
    enum class EnemySize {
        SMALL,
        MEDIUM,
        LARGE
    };

    class Enemy : public Base {
    private:
        static T3DVertPacked* sharedVertices;
        static T3DMat4FP** sharedMatrices;
        static bool* activeFlags;
        static uint32_t activeCount;
        static bool initialized;
        static Enemy enemyPool[MAX_ENEMIES]; // Static pool of enemies

        T3DVec3 position;
        float speed;
        int health;
        int maxHealth;
        uint32_t poolIndex;
        Player* targetPlayer; // Individual target player for this enemy
        float hitTimer;
        EnemySize size;       // Size of this enemy
        uint32_t color;       // Color of this enemy
        int xpReward;         // XP reward for killing this enemy

        static void initializePool();
        void die();

    public:
        Enemy();
        ~Enemy();

        static void initialize();
        static void cleanup();
        static Enemy* spawn(const T3DVec3& position, float speed, Player* targetPlayer, EnemySize size = EnemySize::SMALL, uint32_t color = 0xFF0000FF, int xpReward = 1);
        static void updateAll(float deltaTime);
        static void drawAll(float deltaTime);
        static uint32_t getActiveCount() { return activeCount; }
        static Enemy* getEnemy(uint32_t index) { return &enemyPool[index]; }
        static bool isActive(uint32_t index) { return activeFlags[index]; }

        void update(float deltaTime) override;
        void draw3D(float deltaTime) override;
        void drawPTX(float deltaTime) override;

        void deactivate();
        bool isActive() const;

        void takeDamage(int amount);
        bool collidesWith(Base* other);

        T3DVec3 getPosition() const override { return position; }
        float getRadius() const override;
        EnemySize getSize() const { return size; }
        int getXPReward() const { return xpReward; }
    };
}