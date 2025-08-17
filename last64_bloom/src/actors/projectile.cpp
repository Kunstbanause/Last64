/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "projectile.h"
#include "../main.h"
#include <t3d/t3d.h>
#include <libdragon.h>
#include <malloc.h>



namespace Actor {
    // Static member definitions
    T3DVertPacked* Projectile::sharedVertices = nullptr;
    T3DMat4FP** Projectile::sharedMatrices = nullptr;
    bool* Projectile::activeFlags = nullptr;
    uint32_t Projectile::activeCount = 0;
    bool Projectile::initialized = false;
    Projectile Projectile::projectilePool[MAX_PROJECTILES];

    Projectile::Projectile() : Base() {
        if (!initialized) {
            initializePool();
        }
        poolIndex = MAX_PROJECTILES;
        position = {0, 0, 0};
        velocity = {0, 0, 0};
        speed = 0.0f;
        lifetime = 0.0f;
        maxLifetime = 5.0f; // 5 second lifetime
        flags |= FLAG_DISABLED;
    }

    Projectile::~Projectile() {}

    void Projectile::initialize() {
        if (!initialized) {
            initializePool();
        }
    }

    void Projectile::cleanup() {
        if (sharedVertices) {
            free_uncached(sharedVertices);
            sharedVertices = nullptr;
        }
        if (sharedMatrices) {
            for (int i = 0; i < MAX_PROJECTILES; i++) {
                if (sharedMatrices[i]) free_uncached(sharedMatrices[i]);
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

    void Projectile::initializePool() {
        if (initialized) return;

        T3DVec3 normalVec = {{0.0f, 0.0f, 1.0f}};
        uint16_t norm = t3d_vert_pack_normal(&normalVec);
        // Allocate vertices for all projectiles, just like enemies
        sharedVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * MAX_PROJECTILES * 2);

        for (int i = 0; i < MAX_PROJECTILES; i++) {
            int idx = i * 2;
            // A simple 2x2 quad
            sharedVertices[idx] = (T3DVertPacked){};
            sharedVertices[idx].posA[0] = -2; sharedVertices[idx].posA[1] = -2; sharedVertices[idx].posA[2] = 0;
            sharedVertices[idx].posB[0] =  2; sharedVertices[idx].posB[1] = -2; sharedVertices[idx].posB[2] = 0;
            sharedVertices[idx].rgbaA = 0xFF00FFFF;
            sharedVertices[idx].rgbaB = 0xFF00FFFF;
            sharedVertices[idx].normA = norm;
            sharedVertices[idx].normB = norm;

            sharedVertices[idx+1] = (T3DVertPacked){};
            sharedVertices[idx+1].posA[0] =  2; sharedVertices[idx+1].posA[1] =  2; sharedVertices[idx+1].posA[2] = 0;
            sharedVertices[idx+1].posB[0] = -2; sharedVertices[idx+1].posB[1] =  2; sharedVertices[idx+1].posB[2] = 0;
            sharedVertices[idx+1].rgbaA = 0xFF00FFFF;
            sharedVertices[idx+1].rgbaB = 0xFF00FFFF;
            sharedVertices[idx+1].normA = norm;
            sharedVertices[idx+1].normB = norm;
        }

        sharedMatrices = (T3DMat4FP**)malloc(sizeof(T3DMat4FP*) * MAX_PROJECTILES);
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            sharedMatrices[i] = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
            t3d_mat4fp_identity(sharedMatrices[i]);
        }

        activeFlags = (bool*)calloc(MAX_PROJECTILES, sizeof(bool));
        initialized = true;
    }

    Projectile* Projectile::spawn(const T3DVec3& pos, const T3DVec3& vel, float spd, float slowdown) {
        if (!initialized) initializePool();

        for (uint32_t i = 0; i < MAX_PROJECTILES; i++) {
            if (!activeFlags[i]) {
                activeFlags[i] = true;
                activeCount++;
                Projectile* p = &projectilePool[i];
                p->poolIndex = i;
                p->position = pos;
                p->velocity = vel;
                p->speed = spd;
                p->lifetime = 0.0f;
                p->flags &= ~FLAG_DISABLED;
                return p;
            }
        }
        return nullptr;
    }

    void Projectile::updateAll(float deltaTime) {
        if (!initialized) return;
        for (uint32_t i = 0; i < MAX_PROJECTILES; i++) {
            if (activeFlags[i]) {
                projectilePool[i].update(deltaTime);
            }
        }
    }

    void Projectile::drawAll(float deltaTime) {
        if (!initialized || activeCount == 0) return;

        t3d_state_set_drawflags((enum T3DDrawFlags)(T3D_FLAG_SHADED | T3D_FLAG_DEPTH));

        for (uint32_t i = 0; i < MAX_PROJECTILES; i++) {
            if (activeFlags[i]) {
                projectilePool[i].draw3D(deltaTime);
            }
        }
    }

    void Projectile::update(float deltaTime) {
        if (flags & FLAG_DISABLED) return;

        lifetime += deltaTime;
        if (lifetime >= maxLifetime) {
            deactivate();
            return;
        }

        position.x += velocity.x * speed * deltaTime;
        position.y += velocity.y * speed * deltaTime;
        position.z += velocity.z * speed * deltaTime;

        if (position.x < SCREEN_LEFT || position.x > SCREEN_RIGHT ||
            position.y < SCREEN_TOP || position.y > SCREEN_BOTTOM) {
            deactivate();
            return;
        }

        if (poolIndex < MAX_PROJECTILES) {
            t3d_mat4fp_from_srt_euler(
                sharedMatrices[poolIndex],
                (T3DVec3){{1.0f, 1.0f, 1.0f}},
                (T3DVec3){{0.0f, 0.0f, 0.0f}},
                position
            );
        }
    }

    void Projectile::draw3D(float deltaTime) {
        if (flags & FLAG_DISABLED) return;

        if (poolIndex < MAX_PROJECTILES) {
            t3d_matrix_push(sharedMatrices[poolIndex]);
            t3d_vert_load(&sharedVertices[poolIndex * 2], 0, 4);
            t3d_tri_draw(0, 1, 2);
            t3d_tri_draw(2, 3, 0);
            t3d_tri_sync();
            t3d_matrix_pop(1);
        }
    }

    void Projectile::drawPTX(float deltaTime) {}

    void Projectile::deactivate() {
        if (poolIndex < MAX_PROJECTILES) {
            activeFlags[poolIndex] = false;
            flags |= FLAG_DISABLED;
            activeCount--;
        }
    }

    bool Projectile::isActive() const {
        if (poolIndex < MAX_PROJECTILES) {
            return activeFlags[poolIndex] && !(flags & FLAG_DISABLED);
        }
        return false;
    }
}