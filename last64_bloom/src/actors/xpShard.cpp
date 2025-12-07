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
        spawnTime = 0.0f;
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

        // We'll store 4 vertices per shard for a small diamond (square on corner).
        // Each T3DVertPacked contains 2 vertices, so we allocate 2 structs per shard.
        T3DVec3 normalVec = {{0.0f,0.0f,1.0f}};
        uint16_t norm = t3d_vert_pack_normal(&normalVec);
        sharedVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * MAX_XP_SHARDS * 2);

        for (int i = 0; i < MAX_XP_SHARDS; ++i) {
            int base = i * 2;
            // Define a small diamond (square rotated 45°) centered at origin.
            // Vertices form a diamond: top, right, bottom, left
            // v0 (top), v1 (right)
            sharedVertices[base+0] = (T3DVertPacked){};
            sharedVertices[base+0].posA[0] = 0; sharedVertices[base+0].posA[1] = 1; sharedVertices[base+0].posA[2] = 0;
            sharedVertices[base+0].normA = norm;
            sharedVertices[base+0].posB[0] = 1; sharedVertices[base+0].posB[1] = 0; sharedVertices[base+0].posB[2] = 0;
            sharedVertices[base+0].normB = norm;
            sharedVertices[base+0].rgbaA = 0xFF00FFFF; sharedVertices[base+0].rgbaB = 0xFF00FFFF;

            // v2 (bottom), v3 (left)
            sharedVertices[base+1] = (T3DVertPacked){};
            sharedVertices[base+1].posA[0] = 0; sharedVertices[base+1].posA[1] = -1; sharedVertices[base+1].posA[2] = 0;
            sharedVertices[base+1].normA = norm;
            sharedVertices[base+1].posB[0] = -1; sharedVertices[base+1].posB[1] = 0; sharedVertices[base+1].posB[2] = 0;
            sharedVertices[base+1].normB = norm;
            sharedVertices[base+1].rgbaA = 0xFF00FFFF; sharedVertices[base+1].rgbaB = 0xFF00FFFF;
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
                s->spawnTime = 0.0f;
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
                float speed = 80.0f; // fast homing speed
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
            // Just track time for sparkle effect, no movement
            spawnTime += deltaTime;
        }

        // Update matrix
        if (poolIndex < MAX_XP_SHARDS) {
            // Scale: double size for better visibility
            t3d_mat4fp_from_srt_euler(
                sharedMatrices[poolIndex],
                (T3DVec3){{1.0f, 1.0f, 1.0f}},
                (T3DVec3){{0.0f, 0.0f, 0.0f}},
                position
            );
        }
    }

    void XPShard::draw3D(float deltaTime) {
        if (flags & FLAG_DISABLED) return;
        if (poolIndex >= MAX_XP_SHARDS) return;

        // Rainbow sparkle effect with bright colors
        int base = poolIndex * 2;
        
        // Cycle through rainbow colors
        float rainbowPhase = spawnTime * 2.0f;  // Rotate through rainbow
        float hue = fmodf(rainbowPhase, 6.0f);
        
        uint8_t r, g, b;
        // Simple rainbow: Red -> Yellow -> Green -> Cyan -> Blue -> Magenta -> Red
        if (hue < 1.0f) {
            // Red to Yellow
            r = 255; g = (uint8_t)(255 * hue); b = 0;
        } else if (hue < 2.0f) {
            // Yellow to Green
            r = (uint8_t)(255 * (2.0f - hue)); g = 255; b = 0;
        } else if (hue < 3.0f) {
            // Green to Cyan
            r = 0; g = 255; b = (uint8_t)(255 * (hue - 2.0f));
        } else if (hue < 4.0f) {
            // Cyan to Blue
            r = 0; g = (uint8_t)(255 * (4.0f - hue)); b = 255;
        } else if (hue < 5.0f) {
            // Blue to Magenta
            r = (uint8_t)(255 * (hue - 4.0f)); g = 0; b = 255;
        } else {
            // Magenta to Red
            r = 255; g = 0; b = (uint8_t)(255 * (6.0f - hue));
        }
        
        // Add sparkle pulsing effect on top of rainbow
        float sparkle = 0.6f + 0.4f * sinf(spawnTime * 8.0f);
        uint8_t rs = (uint8_t)(r * sparkle);
        uint8_t gs = (uint8_t)(g * sparkle);
        uint8_t bs = (uint8_t)(b * sparkle);
        uint32_t rainbowColor = (rs << 24) | (gs << 16) | (bs << 8) | 0xFF;
        sharedVertices[base + 0].rgbaA = rainbowColor;
        sharedVertices[base + 0].rgbaB = rainbowColor;
        sharedVertices[base + 1].rgbaA = rainbowColor;
        sharedVertices[base + 1].rgbaB = rainbowColor;

        t3d_matrix_push(sharedMatrices[poolIndex]);
        t3d_vert_load(&sharedVertices[base], 0, 4);
        // Triangulate diamond: v0=top, v1=right, v2=bottom, v3=left
        t3d_tri_draw(0, 1, 2);  // top-right-bottom
        t3d_tri_draw(0, 2, 3);  // top-bottom-left
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
