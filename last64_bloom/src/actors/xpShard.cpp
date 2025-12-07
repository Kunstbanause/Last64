/**
 * XP shard collectible implementation
 */
#include "xpShard.h"
#include "player.h"
#include "../systems/experience.h"
#include "../main.h"
#include <t3d/t3d.h>
#include <libdragon.h>
#include <malloc.h>
#include <cmath>

namespace Actor {
    T3DVertPacked* XPShard::sharedVertices = nullptr;
    T3DMat4FP** XPShard::sharedMatrices = nullptr;
    bool* XPShard::activeFlags = nullptr;
    uint32_t XPShard::activeCount = 0;
    bool XPShard::initialized = false;
    XPShard XPShard::shardPool[MAX_XP_SHARDS];

    XPShard::XPShard() : Base() {
        if (!initialized) initializePool();
        poolIndex = MAX_XP_SHARDS;
        position = {0,0,0};
        velocity = {0,0,0};
        xpValue = 0;
        color = 0xFF00FFFF;
        attracted = false;
        targetPlayer = nullptr;
        flags |= FLAG_DISABLED;
    }

    XPShard::~XPShard() {}

    void XPShard::initialize() { if (!initialized) initializePool(); }

    void XPShard::cleanup() {
        if (sharedVertices) { free_uncached(sharedVertices); sharedVertices = nullptr; }
        if (sharedMatrices) {
            for (int i = 0; i < MAX_XP_SHARDS; ++i) if (sharedMatrices[i]) free_uncached(sharedMatrices[i]);
            free(sharedMatrices);
            sharedMatrices = nullptr;
        }
        if (activeFlags) { free(activeFlags); activeFlags = nullptr; }
        activeCount = 0;
        initialized = false;
    }

    void XPShard::initializePool() {
        if (initialized) return;

        // We'll store 6 vertices per shard. Each T3DVertPacked contains 2 vertices,
        // so we allocate 3 structs per shard (6 total vertices).
        T3DVec3 normalVec = {{0.0f,0.0f,1.0f}};
        uint16_t norm = t3d_vert_pack_normal(&normalVec);
        sharedVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * MAX_XP_SHARDS * 3);

        for (int i = 0; i < MAX_XP_SHARDS; ++i) {
            int base = i * 3;
            // Define a vertical-elongated hexagon centered at origin.
            // Vertex order (clockwise): v0 top-left, v1 top-right, v2 mid-right,
            // v3 bottom-right, v4 bottom-left, v5 mid-left
            // We'll fill pairs: [v0,v1], [v2,v3], [v4,v5]
            // v0
            sharedVertices[base+0] = (T3DVertPacked){};
            sharedVertices[base+0].posA[0] = -3; sharedVertices[base+0].posA[1] = 6; sharedVertices[base+0].posA[2] = 0;
            sharedVertices[base+0].normA = norm;
            // v1
            sharedVertices[base+0].posB[0] = 3; sharedVertices[base+0].posB[1] = 6; sharedVertices[base+0].posB[2] = 0;
            sharedVertices[base+0].normB = norm;
            sharedVertices[base+0].rgbaA = 0xFF00FFFF; sharedVertices[base+0].rgbaB = 0xFF00FFFF;

            // v2
            sharedVertices[base+1] = (T3DVertPacked){};
            sharedVertices[base+1].posA[0] = 6; sharedVertices[base+1].posA[1] = 0; sharedVertices[base+1].posA[2] = 0;
            sharedVertices[base+1].normA = norm;
            // v3
            sharedVertices[base+1].posB[0] = 3; sharedVertices[base+1].posB[1] = -6; sharedVertices[base+1].posB[2] = 0;
            sharedVertices[base+1].normB = norm;
            sharedVertices[base+1].rgbaA = 0xFF00FFFF; sharedVertices[base+1].rgbaB = 0xFF00FFFF;

            // v4
            sharedVertices[base+2] = (T3DVertPacked){};
            sharedVertices[base+2].posA[0] = -3; sharedVertices[base+2].posA[1] = -6; sharedVertices[base+2].posA[2] = 0;
            sharedVertices[base+2].normA = norm;
            // v5
            sharedVertices[base+2].posB[0] = -6; sharedVertices[base+2].posB[1] = 0; sharedVertices[base+2].posB[2] = 0;
            sharedVertices[base+2].normB = norm;
            sharedVertices[base+2].rgbaA = 0xFF00FFFF; sharedVertices[base+2].rgbaB = 0xFF00FFFF;
        }

        sharedMatrices = (T3DMat4FP**)malloc(sizeof(T3DMat4FP*) * MAX_XP_SHARDS);
        for (int i = 0; i < MAX_XP_SHARDS; ++i) {
            sharedMatrices[i] = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
            t3d_mat4fp_identity(sharedMatrices[i]);
        }

        activeFlags = (bool*)calloc(MAX_XP_SHARDS, sizeof(bool));
        initialized = true;
    }

    XPShard* XPShard::spawn(const T3DVec3& pos, int xpValue_, uint32_t color_) {
        if (!initialized) initializePool();
        for (uint32_t i = 0; i < MAX_XP_SHARDS; ++i) {
            if (!activeFlags[i]) {
                activeFlags[i] = true;
                activeCount++;
                XPShard* s = &shardPool[i];
                s->poolIndex = i;
                s->position = pos;
                s->velocity = {0,0,0};
                s->xpValue = xpValue_;
                s->color = color_;
                s->attracted = false;
                s->targetPlayer = nullptr;
                s->flags &= ~FLAG_DISABLED;
                return s;
            }
        }
        return nullptr;
    }

    void XPShard::updateAll(float deltaTime) {
        if (!initialized) return;
        for (uint32_t i = 0; i < MAX_XP_SHARDS; ++i) {
            if (activeFlags[i]) shardPool[i].update(deltaTime);
        }
    }

    void XPShard::drawAll(float deltaTime) {
        if (!initialized || activeCount == 0) return;

        t3d_state_set_drawflags((enum T3DDrawFlags)(T3D_FLAG_SHADED | T3D_FLAG_DEPTH));
        for (uint32_t i = 0; i < MAX_XP_SHARDS; ++i) {
            if (activeFlags[i]) shardPool[i].draw3D(deltaTime);
        }
    }

    void XPShard::update(float deltaTime) {
        if (flags & FLAG_DISABLED) return;

        // If not attracted, check nearby players
        if (!attracted) {
            int active = Experience::getActivePlayerCount();
            for (int i = 0; i < active; ++i) {
                Actor::Player* p = Experience::getPlayer(i);
                if (!p || p->getIsDead()) continue;
                // compute distance
                T3DVec3 pp = p->getPosition();
                float dx = pp.x - position.x;
                float dy = pp.y - position.y;
                float distSq = dx*dx + dy*dy;
                float pickupR = p->getRadius();
                // Use player's getRadius as baseline; allow a larger pickup range
                float effectiveRange = pickupR * 6.0f; // heuristic pickup range multiplier
                if (distSq <= effectiveRange * effectiveRange) {
                    attracted = true;
                    targetPlayer = p;
                    break;
                }
            }
        }

        // Movement: if attracted, home towards the target player quickly
        if (attracted && targetPlayer) {
            T3DVec3 tp = targetPlayer->getPosition();
            float dx = tp.x - position.x;
            float dy = tp.y - position.y;
            float dz = tp.z - position.z;
            float dist = sqrtf(dx*dx + dy*dy + dz*dz);
            if (dist > 0.0f) {
                // Move faster when attracted
                float speed = 220.0f; // fast homing speed
                position.x += dx / dist * speed * deltaTime;
                position.y += dy / dist * speed * deltaTime;
                position.z += dz / dist * speed * deltaTime;
            }

            // Check collision with player
            float radius = targetPlayer->getRadius();
            if (dist <= radius) {
                // Award XP and deactivate
                Experience::addXP(xpValue);
                gSFXManager.play(SFXManager::SFX_PICKUP);
                deactivate();
                return;
            }
        } else {
            // idle float/bob effect
            static const float bobSpeed = 1.8f;
            static const float bobAmount = 2.0f;
            float bob = sinf((float)flags + (float)bobSpeed) * bobAmount * deltaTime; // simple variation
            position.z += bob;
        }

        // Update matrix
        if (poolIndex < MAX_XP_SHARDS) {
            // Scale: make shard taller than wide
            t3d_mat4fp_from_srt_euler(
                sharedMatrices[poolIndex],
                (T3DVec3){{1.0f, 1.6f, 1.0f}},
                (T3DVec3){{0.0f, 0.0f, 0.0f}},
                position
            );
        }
    }

    void XPShard::draw3D(float deltaTime) {
        if (flags & FLAG_DISABLED) return;
        if (poolIndex >= MAX_XP_SHARDS) return;

        // Update vertex colors for this shard
        int base = poolIndex * 3;
        for (int i = 0; i < 3; ++i) {
            sharedVertices[base + i].rgbaA = color;
            sharedVertices[base + i].rgbaB = color;
        }

        t3d_matrix_push(sharedMatrices[poolIndex]);
        t3d_vert_load(&sharedVertices[base], 0, 6);
        // Triangulate hexagon: fan from vertex 0
        t3d_tri_draw(0, 1, 2);
        t3d_tri_draw(0, 2, 3);
        t3d_tri_draw(0, 3, 4);
        t3d_tri_draw(0, 4, 5);
        t3d_tri_sync();
        t3d_matrix_pop(1);
    }

    void XPShard::drawPTX(float deltaTime) {}

    void XPShard::deactivate() {
        if (poolIndex < MAX_XP_SHARDS) {
            activeFlags[poolIndex] = false;
            flags |= FLAG_DISABLED;
            if (activeCount > 0) activeCount--;
        }
    }

    bool XPShard::isActive() const {
        if (poolIndex < MAX_XP_SHARDS) {
            return activeFlags[poolIndex] && !(flags & FLAG_DISABLED);
        }
        return false;
    }
}
