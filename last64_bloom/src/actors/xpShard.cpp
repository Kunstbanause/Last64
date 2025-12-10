/**
 * XP shard collectible implementation
 */
#include "xpShard.h"
#include "player.h"
#include "../systems/experience.h"
#include "../main.h"
#include "../render/hdrBoost.h"
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
        scale = 1.0f;
        attracted = false;
        isFleeing = false;
        fleeTimer = 0.0f;
        targetPlayer = nullptr;
        spawnTime = 0.0f;
        attractionTime = 0.0f;
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

    XPShard* XPShard::spawn(const T3DVec3& pos, int xpValue_, uint32_t color_, float scale_) {
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
                s->scale = scale_;
                s->attracted = false;
                s->isFleeing = false;
                s->fleeTimer = 0.0f;
                s->targetPlayer = nullptr;
                s->spawnTime = 0.0f;
                s->attractionTime = 0.0f;
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

        // Always update spawn time for sparkle effect
        spawnTime += deltaTime;

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
                float effectiveRange = p->getPickupRange(); // Use player's pickup range from upgrades
                if (distSq <= effectiveRange * effectiveRange) {
                    attracted = true;
                    isFleeing = true; // start with a short flee phase
                    fleeTimer = 0.0f;
                    targetPlayer = p;
                    break;
                }
            }
        }

        // Movement: if attracted, first flee briefly then home towards the target player
        if (attracted && targetPlayer) {
            T3DVec3 tp = targetPlayer->getPosition();
            // Use 2D distances only (x,y)
            float dx = tp.x - position.x;
            float dy = tp.y - position.y;
            float distSq = dx*dx + dy*dy;

            if (isFleeing) {
                // Flee for a short duration with a quick initial speed that eases out
                fleeTimer += deltaTime;
                const float fleeDuration = 0.22f; // seconds
                const float fleeSpeedStart = 80.0f;
                const float fleeSpeedEnd = 50.0f; // slow into homing
                float ft = fleeTimer / fleeDuration;
                if (ft > 1.0f) ft = 1.0f;
                // Interpolate speed down as fleeing time progresses
                float fleeSpeed = fleeSpeedStart * (1.0f - ft) + fleeSpeedEnd * ft;

                // Compute away direction in 2D (x,y). If zero-length, nudge on X.
                float ax = position.x - tp.x;
                float ay = position.y - tp.y;
                float adistSq = ax*ax + ay*ay;
                if (adistSq <= 0.000001f) { ax = 1.0f; ay = 0.0f; adistSq = 1.0f; }
                float invAdist = 1.0f / sqrtf(adistSq);  // Single sqrt
                position.x += (ax * invAdist) * fleeSpeed * deltaTime;
                position.y += (ay * invAdist) * fleeSpeed * deltaTime;

                if (fleeTimer >= fleeDuration) {
                    isFleeing = false;
                    attractionTime = 0.0f; // reset homing timer
                }
            } else {
                // Homing: relaxed ease-in curve for pickup
                attractionTime += deltaTime;
                const float homingMin = 120.0f;
                const float homingMax = 220.0f;
                const float accelTime = 1.4f; // time to reach max
                float tt = attractionTime / accelTime;
                if (tt > 1.0f) tt = 1.0f;
                // ease-in quadratic (slow start, accelerate)
                float ease = tt * tt;
                float speed = homingMin + (homingMax - homingMin) * ease;

                if (distSq > 0.000001f) {
                    float invDist = 1.0f / sqrtf(distSq);  // Single sqrt
                    position.x += dx * invDist * speed * deltaTime;
                    position.y += dy * invDist * speed * deltaTime;
                }
            }

            // After movement, check collision with ALL players (not just the target)
            // This ensures any player can pick up XP, even if they joined after the shard spawned
            int active = Experience::getActivePlayerCount();
            for (int i = 0; i < active; ++i) {
                Actor::Player* p = Experience::getPlayer(i);
                if (!p || p->getIsDead()) continue;
                
                T3DVec3 pp = p->getPosition();
                float n_dx = pp.x - position.x;
                float n_dy = pp.y - position.y;
                float n_distSq = n_dx*n_dx + n_dy*n_dy;
                float collisionRadius = p->getRadius() * 2.0f; // Slightly larger than visual for easier pickup
                
                if (n_distSq <= collisionRadius * collisionRadius) {
                    Experience::addXP(xpValue);
                    
                    // Select sound based on proximity to level up (0-100% maps to xp1-xp8)
                    float xpPercentage = Experience::getXPPercentage();  // 0.0 to 1.0
                    int soundIndex = (int)(xpPercentage * 7.999f);  // Maps 0.0-1.0 to 0-7
                    if (soundIndex < 0) soundIndex = 0;
                    if (soundIndex > 7) soundIndex = 7;
                    
                    SFXManager::SfxId sfxId = (SFXManager::SfxId)((int)SFXManager::SFX_XP1 + soundIndex);
                    gSFXManager.play(sfxId);
                    
                    // Trigger a small HDR boost to add a juicy glow on pickup
                    // peakValue short, duration ~ 0.45s, peakDuration short
                    HDRBoost::triggerBoost(1.6f, 0.2f, 0.06f);
                    deactivate();
                    return;
                }
            }
        }

        // Update matrix
        if (poolIndex < MAX_XP_SHARDS) {
            // Scale: base doubled for previous visual; multiply by per-shard scale
            float baseScale = 2.0f;
            float finalScale = baseScale * scale;
            t3d_mat4fp_from_srt_euler(
                sharedMatrices[poolIndex],
                (T3DVec3){{finalScale, finalScale, finalScale}},
                (T3DVec3){{0.0f, 0.0f, 0.0f}},
                position
            );
        }
    }

    void XPShard::draw3D(float deltaTime) {
        if (flags & FLAG_DISABLED) return;
        if (poolIndex >= MAX_XP_SHARDS) return;

        // Bright color sparkle - cycles through only vibrant colors
        int base = poolIndex * 2;
        
        // Discrete bright color palette (full primary/secondary colors)
        const uint32_t palette[6] = {
            0xFF0000FF, // Red
            0xFFFF00FF, // Yellow
            0x00FF00FF, // Green
            0x00FFFFFF, // Cyan
            0x40C0FFFF, // Light Sky Blue (brighter than the previous dark blue)
            0xFF00FFFF  // Magenta
        };

        // Smooth per-vertex color interpolation across the palette for a candy-like sparkle
        const int paletteCount = 6;
        float cycleSpeed = 1.6f; // revolutions per second across the palette
        float pos = spawnTime * cycleSpeed; // continuous position
        float idxf = fmodf(pos, (float)paletteCount);

        // shimmer and spike parameters
        float shimmer = 0.06f * (0.5f * (sinf(spawnTime * 16.0f) + 1.0f));
        const uint8_t dimWhite = 200;
        float spikeFreq = 0.35f; // ~2.86s period
        float spikeWidth = 0.06f; // proportion of period
        float phase = fmodf(spawnTime * spikeFreq, 1.0f);
        float spike = 0.0f;
        if (phase < spikeWidth) spike = 1.0f - (phase / spikeWidth);

        // Compute per-vertex colors (4 verts) by sampling slightly offset positions across the palette
        for (int v = 0; v < 4; ++v) {
            float samplePos = idxf + ((float)v) * 0.25f; // offset across vertices
            samplePos = fmodf(samplePos, (float)paletteCount);
            int aIdx = (int)floorf(samplePos);
            int bIdx = (aIdx + 1) % paletteCount;
            float t = samplePos - (float)aIdx;
            uint32_t cA = palette[aIdx];
            uint32_t cB = palette[bIdx];
            uint8_t rA = (cA >> 24) & 0xFF; uint8_t gA = (cA >> 16) & 0xFF; uint8_t bA = (cA >> 8) & 0xFF;
            uint8_t rB = (cB >> 24) & 0xFF; uint8_t gB = (cB >> 16) & 0xFF; uint8_t bB = (cB >> 8) & 0xFF;
            // linear interp between adjacent palette colors
            float rf = rA * (1.0f - t) + rB * t;
            float gf = gA * (1.0f - t) + gB * t;
            float bf = bA * (1.0f - t) + bB * t;

            // shimmer toward dim white
            uint8_t cr = (uint8_t)(rf * (1.0f - shimmer) + dimWhite * shimmer);
            uint8_t cg = (uint8_t)(gf * (1.0f - shimmer) + dimWhite * shimmer);
            uint8_t cb = (uint8_t)(bf * (1.0f - shimmer) + dimWhite * shimmer);

            // apply occasional full-white spike
            uint8_t vr = (uint8_t)(cr * (1.0f - spike) + 255 * spike);
            uint8_t vg = (uint8_t)(cg * (1.0f - spike) + 255 * spike);
            uint8_t vb = (uint8_t)(cb * (1.0f - spike) + 255 * spike);

            uint32_t vcolor = (vr << 24) | (vg << 16) | (vb << 8) | 0xFF;
            // Map vertices: base+0 contains posA (v0) and posB (v1)
            if (v == 0) { sharedVertices[base + 0].rgbaA = vcolor; }
            if (v == 1) { sharedVertices[base + 0].rgbaB = vcolor; }
            if (v == 2) { sharedVertices[base + 1].rgbaA = vcolor; }
            if (v == 3) { sharedVertices[base + 1].rgbaB = vcolor; }
        }

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
