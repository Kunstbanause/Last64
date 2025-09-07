/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "enemyDeathVFX.h"
#include "../main.h"
#include <t3d/t3d.h>
#include <t3d/tpx.h>
#include <libdragon.h>
#include <malloc.h>

namespace Actor {
    // Static member definitions
    T3DVertPacked* EnemyDeathVFX::sharedVertices = nullptr;
    T3DMat4FP** EnemyDeathVFX::sharedMatrices = nullptr;
    bool* EnemyDeathVFX::activeFlags = nullptr;
    uint32_t EnemyDeathVFX::activeCount = 0;
    bool EnemyDeathVFX::initialized = false;
    EnemyDeathVFX EnemyDeathVFX::vfxPool[MAX_ENEMY_DEATH_VFX];

    EnemyDeathVFX::EnemyDeathVFX() : Base() {
        if (!initialized) {
            initializePool();
        }
        
        poolIndex = MAX_ENEMY_DEATH_VFX; // Invalid index until spawned
        position = {0, 0, 0};
        lifetime = 0.0f;
        maxLifetime = 0.2f; // Half a second lifetime
        size = 1.0f;
        color = 0xFFFFFFFF; // Default white color
        flags |= FLAG_DISABLED; // Start as disabled
    }

    EnemyDeathVFX::~EnemyDeathVFX() {
        // We don't actually delete from the pool here
        // The pool is managed statically
    }

    void EnemyDeathVFX::initialize() {
        if (!initialized) {
            initializePool();
        }
    }

    void EnemyDeathVFX::cleanup() {
        if (sharedVertices) {
            free_uncached(sharedVertices);
            sharedVertices = nullptr;
        }
        
        if (sharedMatrices) {
            for (int i = 0; i < MAX_ENEMY_DEATH_VFX; i++) {
                if (sharedMatrices[i]) {
                    free_uncached(sharedMatrices[i]);
                }
            }
            free(sharedMatrices);
            sharedMatrices = nullptr;
        }
        
        if (activeFlags) {
            free(activeFlags);
            activeFlags = nullptr;
        }
        
        activeCount = 0;
        initialized = false;
    }

    void EnemyDeathVFX::initializePool() {
        if (initialized) return;

        // Allocate vertices for all VFX objects (quads)
        T3DVec3 normalVec = {{0.0f, 0.0f, 1.0f}};
        uint16_t norm = t3d_vert_pack_normal(&normalVec);
        sharedVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * MAX_ENEMY_DEATH_VFX * 2);
        
        for (int i = 0; i < MAX_ENEMY_DEATH_VFX; i++) {
            int idx = i * 2;
            // First structure: vertices 0 and 1
            sharedVertices[idx] = (T3DVertPacked){};
            sharedVertices[idx].posA[0] = -3; sharedVertices[idx].posA[1] = -3; sharedVertices[idx].posA[2] = 0;
            sharedVertices[idx].normA = norm;
            sharedVertices[idx].posB[0] = 3; sharedVertices[idx].posB[1] = -3; sharedVertices[idx].posB[2] = 0;
            sharedVertices[idx].normB = norm;
            sharedVertices[idx].rgbaA = 0xFFFFFFFF; // White
            sharedVertices[idx].rgbaB = 0xFFFFFFFF; // White
            sharedVertices[idx].stA[0] = 0; sharedVertices[idx].stA[1] = 0;
            sharedVertices[idx].stB[0] = 0; sharedVertices[idx].stB[1] = 0;
            
            // Second structure: vertices 2 and 3
            sharedVertices[idx+1] = (T3DVertPacked){};
            sharedVertices[idx+1].posA[0] = 3; sharedVertices[idx+1].posA[1] = 3; sharedVertices[idx+1].posA[2] = 0;
            sharedVertices[idx+1].normA = norm;
            sharedVertices[idx+1].posB[0] = -3; sharedVertices[idx+1].posB[1] = 3; sharedVertices[idx+1].posB[2] = 0;
            sharedVertices[idx+1].normB = norm;
            sharedVertices[idx+1].rgbaA = 0xFFFFFFFF; // White
            sharedVertices[idx+1].rgbaB = 0xFFFFFFFF; // White
            sharedVertices[idx+1].stA[0] = 0; sharedVertices[idx+1].stA[1] = 0;
            sharedVertices[idx+1].stB[0] = 0; sharedVertices[idx+1].stB[1] = 0;
        }
        
        // Allocate matrices for all VFX objects
        sharedMatrices = (T3DMat4FP**)malloc(sizeof(T3DMat4FP*) * MAX_ENEMY_DEATH_VFX);
        for (int i = 0; i < MAX_ENEMY_DEATH_VFX; i++) {
            sharedMatrices[i] = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
            t3d_mat4fp_identity(sharedMatrices[i]);
        }
        
        // Allocate active flags
        activeFlags = (bool*)calloc(MAX_ENEMY_DEATH_VFX, sizeof(bool));
        
        initialized = true;
    }

    EnemyDeathVFX* EnemyDeathVFX::spawn(const T3DVec3& position, float size, uint32_t color) {
        if (!initialized) {
            initializePool();
        }
        
        // Find an inactive VFX slot
        for (uint32_t i = 0; i < MAX_ENEMY_DEATH_VFX; i++) {
            if (!activeFlags[i]) {
                // Found an inactive slot, activate it
                activeFlags[i] = true;
                activeCount++;
                
                // Get a VFX object from the pool
                EnemyDeathVFX* vfx = &vfxPool[i];
                
                vfx->poolIndex = i;
                vfx->position = position;
                vfx->size = size;
                vfx->color = color;
                vfx->lifetime = 0.0f;
                
                vfx->flags &= ~FLAG_DISABLED; // Enable the VFX
                
                return vfx;
            }
        }
        
        // No inactive slots available
        return nullptr;
    }

    void EnemyDeathVFX::updateAll(float deltaTime) {
        if (!initialized) return;
        
        for (uint32_t i = 0; i < MAX_ENEMY_DEATH_VFX; i++) {
            if (activeFlags[i]) {
                EnemyDeathVFX* vfx = &vfxPool[i];
                vfx->update(deltaTime);
            }
        }
    }

    void EnemyDeathVFX::drawAll(float deltaTime) {
        if (!initialized) return;
        
        // Set up rendering state once for all VFX objects
        t3d_state_set_drawflags((enum T3DDrawFlags)(T3D_FLAG_SHADED | T3D_FLAG_DEPTH));
        
        for (uint32_t i = 0; i < MAX_ENEMY_DEATH_VFX; i++) {
            if (activeFlags[i]) {
                EnemyDeathVFX* vfx = &vfxPool[i];
                vfx->draw3D(deltaTime);
            }
        }
    }

    void EnemyDeathVFX::update(float deltaTime) {
        if (flags & FLAG_DISABLED) return;

        lifetime += deltaTime;
        
        // Deactivate when lifetime is exceeded
        if (lifetime >= maxLifetime) {
            deactivate();
            return;
        }
    }

    void EnemyDeathVFX::draw3D(float deltaTime) {
        if (flags & FLAG_DISABLED) return;
        
        if (poolIndex < MAX_ENEMY_DEATH_VFX) {
            // Calculate alpha based on remaining lifetime (fade out effect)
            float fadePercentage = lifetime / maxLifetime;
            float sizeOverTime = size +(size * fadePercentage);
            float alpha = 1.0f - fadePercentage;
            uint8_t alphaByte = (uint8_t)(alpha * 255.0f);
            
            // Apply alpha to color
            uint32_t colorWithAlpha = (color & 0x00FFFFFF) | (alphaByte << 24);

            // Update vertex colors for this specific VFX object
            sharedVertices[poolIndex * 2].rgbaA = colorWithAlpha;
            sharedVertices[poolIndex * 2].rgbaB = colorWithAlpha;
            sharedVertices[poolIndex * 2 + 1].rgbaA = colorWithAlpha;
            sharedVertices[poolIndex * 2 + 1].rgbaB = colorWithAlpha;

            // Update matrix with scale
            if (poolIndex < MAX_ENEMY_DEATH_VFX) {
                t3d_mat4fp_from_srt_euler(
                    sharedMatrices[poolIndex],
                    (T3DVec3){{size * sizeOverTime, size * sizeOverTime, size * sizeOverTime}},  // scale
                    (T3DVec3){{0.0f, 0.0f, 0.0f}},  // rotation
                    position                         // translation
                );
            }

            t3d_matrix_push(sharedMatrices[poolIndex]);
            t3d_vert_load(&sharedVertices[poolIndex * 2], 0, 4);
            t3d_tri_draw(0, 1, 2);
            t3d_tri_draw(2, 3, 0);
            t3d_tri_sync();
            t3d_matrix_pop(1);
        }
    }

    void EnemyDeathVFX::drawPTX(float deltaTime) {
        // No particle effects for enemy death VFX
    }

    void EnemyDeathVFX::deactivate() {
        if (poolIndex < MAX_ENEMY_DEATH_VFX) {
            activeFlags[poolIndex] = false;
            flags |= FLAG_DISABLED;
            activeCount--;
        }
    }

    bool EnemyDeathVFX::isActive() const {
        if (poolIndex < MAX_ENEMY_DEATH_VFX) {
            return activeFlags[poolIndex] && !(flags & FLAG_DISABLED);
        }
        return false;
    }
}