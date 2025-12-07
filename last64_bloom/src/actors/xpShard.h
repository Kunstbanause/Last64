/**
 * XP shard collectible - spawns when enemies die and can be picked up by players
 */
#pragma once
#include "../actors/base.h"
#include <t3d/t3d.h>

#define MAX_XP_SHARDS 64

namespace Actor {
    class XPShard : public Base {
    private:
        static T3DVertPacked* sharedVertices;
        static T3DMat4FP** sharedMatrices;
        static bool* activeFlags;
        static uint32_t activeCount;
        static bool initialized;
        static XPShard shardPool[MAX_XP_SHARDS];

        T3DVec3 position;
        T3DVec3 velocity;
        uint32_t poolIndex;
        int xpValue;
        uint32_t color;
        bool attracted;
        Base* targetPlayer;
        float spawnTime;
        float attractionTime;

        static void initializePool();

    public:
        XPShard();
        ~XPShard();

        static void initialize();
        static void cleanup();

        static XPShard* spawn(const T3DVec3& pos, int xpValue, uint32_t color = 0xFF00FFFF);
        static void updateAll(float deltaTime);
        static void drawAll(float deltaTime);
        static XPShard* getShard(uint32_t index) { return &shardPool[index]; }
        static bool isActive(uint32_t index) { return activeFlags[index]; }

        void update(float deltaTime) override;
        void draw3D(float deltaTime) override;
        void drawPTX(float deltaTime) override;

        void deactivate();
        bool isActive() const;

        T3DVec3 getPosition() const override { return position; }
    };
}
