/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "shape.h"
#include "enemy.h"  // Include enemy header for collision detection
#include "../main.h"
#include "../systems/experience.h"
#include "../systems/roundStats.h"
#include <t3d/t3d.h>
#include <libdragon.h>
#include <malloc.h>

namespace Actor {
    // Static member definitions
    T3DVertPacked* Shape::sharedVertices = nullptr;
    T3DMat4FP** Shape::sharedMatrices = nullptr;
    bool* Shape::activeFlags = nullptr;
    uint32_t Shape::activeCount = 0;
    bool Shape::initialized = false;
    Shape Shape::shapePool[MAX_SHAPES];

    Shape::Shape() : Base() {
        if (!initialized) {
            initializePool();
        }
        poolIndex = MAX_SHAPES;
        position = {0, 0, 0};
        offset = {0, 0, 0};
        attachedTo = nullptr;
        lifetime = 0.0f;
        maxLifetime = 1.0f;
        attackFrequency = 1.0f;
        damage = 4; // Default damage
        color = 0xFF00FFFF; // Default color (cyan)
        width = 4.0f; // Default width
        height = 4.0f; // Default height
        ownerIndex = -1;
        weaponType = WeaponType::SHAPE;
        flags |= FLAG_DISABLED;
    }

    Shape::~Shape()
    {
        // We don't actually delete from the pool here
        // The pool is managed statically
    }
    
    void Shape::getAABBSize(float& aabbWidth, float& aabbHeight) const {
        // Use the actual dimensions of the shape
        aabbWidth = width;
        aabbHeight = height;
    }

    void Shape::initialize() {
        if (!initialized) {
            initializePool();
        }
    }

    void Shape::cleanup() {
        if (sharedVertices) {
            free_uncached(sharedVertices);
            sharedVertices = nullptr;
        }
        if (sharedMatrices) {
            for (int i = 0; i < MAX_SHAPES; i++) {
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

    void Shape::initializePool() {
        if (initialized) return;

        T3DVec3 normalVec = {{0.0f, 0.0f, 1.0f}};
        uint16_t norm = t3d_vert_pack_normal(&normalVec);
        // Allocate vertices for all shapes, just like enemies
        sharedVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * MAX_SHAPES * 2);

        for (int i = 0; i < MAX_SHAPES; i++) {
            int idx = i * 2;
            // A simple rectangle (default 4x4)
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

        sharedMatrices = (T3DMat4FP**)malloc(sizeof(T3DMat4FP*) * MAX_SHAPES);
        for (int i = 0; i < MAX_SHAPES; i++) {
            sharedMatrices[i] = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
            t3d_mat4fp_identity(sharedMatrices[i]);
        }

        activeFlags = (bool*)calloc(MAX_SHAPES, sizeof(bool));
        initialized = true;
    }

    Shape* Shape::spawn(const T3DVec3& pos, float width, float height, float maxLifetime, float attackFrequency, int damage, uint32_t color, int ownerPlayerIndex, WeaponType weaponTypeParam) {
        if (!initialized) initializePool();

        for (uint32_t i = 0; i < MAX_SHAPES; i++) {
            if (!activeFlags[i]) {
                activeFlags[i] = true;
                activeCount++;
                Shape* s = &shapePool[i];
                s->poolIndex = i;
                s->position = pos;
                s->attachedTo = nullptr; // Not attached
                s->lifetime = 0.0f;
                s->maxLifetime = maxLifetime;
                s->attackFrequency = attackFrequency;
                s->damage = damage;
                s->color = color;
                s->width = width;
                s->height = height;
                s->enemyAttackTimers.clear(); // Reset enemy attack timers
                s->ownerIndex = ownerPlayerIndex;
                s->weaponType = weaponTypeParam;
                s->flags &= ~FLAG_DISABLED;
                return s;
            }
        }
        return nullptr;
    }
    
    Shape* Shape::spawnAttached(Base* attachTo, const T3DVec3& offset, float width, float height, float maxLifetime, float attackFrequency, int damage, uint32_t color, int ownerPlayerIndex, WeaponType weaponTypeParam) {
        if (!initialized) initializePool();

        for (uint32_t i = 0; i < MAX_SHAPES; i++) {
            if (!activeFlags[i]) {
                activeFlags[i] = true;
                activeCount++;
                Shape* s = &shapePool[i];
                s->poolIndex = i;
                s->position = {0, 0, 0}; // Position will be calculated from attachment
                s->attachedTo = attachTo;
                s->offset = offset;
                s->lifetime = 0.0f;
                s->maxLifetime = maxLifetime;
                s->attackFrequency = attackFrequency;
                s->damage = damage;
                s->color = color;
                s->width = width;
                s->height = height;
                s->enemyAttackTimers.clear(); // Reset enemy attack timers
                s->ownerIndex = ownerPlayerIndex;
                s->weaponType = weaponTypeParam;
                s->flags &= ~FLAG_DISABLED;
                return s;
            }
        }
        return nullptr;
    }

    void Shape::updateAll(float deltaTime) {
        if (!initialized) return;
        
        for (uint32_t i = 0; i < MAX_SHAPES; i++) {
            if (activeFlags[i]) {
                shapePool[i].update(deltaTime);
            }
        }
    }

    void Shape::drawAll(float deltaTime) {
        if (!initialized || activeCount == 0) return;

        t3d_state_set_drawflags((enum T3DDrawFlags)(T3D_FLAG_SHADED | T3D_FLAG_DEPTH));

        for (uint32_t i = 0; i < MAX_SHAPES; i++) {
            if (activeFlags[i]) {
                shapePool[i].draw3D(deltaTime);
            }
        }
    }

    void Shape::update(float deltaTime) {
        if (flags & FLAG_DISABLED) return;

        lifetime += deltaTime;
        if (lifetime >= maxLifetime) {
            deactivate();
            return;
        }

        // Update enemy attack timers
        for (auto it = enemyAttackTimers.begin(); it != enemyAttackTimers.end();) {
            it->second -= deltaTime;
            if (it->second <= 0.0f) {
                // Timer expired, remove from map
                it = enemyAttackTimers.erase(it);
            } else {
                ++it;
            }
        }

        // Check for collisions with enemies
        for (uint32_t i = 0; i < MAX_ENEMIES; i++) {
            if (Actor::Enemy::isActive(i)) {
                Actor::Enemy* enemy = Actor::Enemy::getEnemy(i);
                if (enemy && enemy->isActive() && enemy->collidesWith(this)) {
                    // Check if we can damage this enemy
                    if (canDamageEnemy(i)) {
                        enemy->takeDamage(damage);
                        RoundStats::addDamage(ownerIndex, weaponType, damage);
                        registerEnemyHit(i);
                        // Play hit sound effect
                        gSFXManager.play(SFXManager::SFX_HIT);
                    }
                }
            }
        }

        // Update matrix position and scale
        T3DVec3 currentPosition = getPosition(); // This will use attachment if applicable
        if (poolIndex < MAX_SHAPES) {
            // Calculate scale based on width and height
            T3DVec3 scale = {{width / 4.0f, height / 4.0f, 1.0f}}; // Default quad is 4x4
            t3d_mat4fp_from_srt_euler(
                sharedMatrices[poolIndex],
                scale,
                (T3DVec3){{0.0f, 0.0f, 0.0f}},
                currentPosition
            );
        }
    }

    void Shape::draw3D(float deltaTime) {
        if (flags & FLAG_DISABLED) return;

        if (poolIndex < MAX_SHAPES) {
            // Update vertex colors for this specific shape
            sharedVertices[poolIndex * 2].rgbaA = color;
            sharedVertices[poolIndex * 2].rgbaB = color;
            sharedVertices[poolIndex * 2 + 1].rgbaA = color;
            sharedVertices[poolIndex * 2 + 1].rgbaB = color;

            t3d_matrix_push(sharedMatrices[poolIndex]);
            t3d_vert_load(&sharedVertices[poolIndex * 2], 0, 4);
            t3d_tri_draw(0, 1, 2);
            t3d_tri_draw(2, 3, 0);
            t3d_tri_sync();
            t3d_matrix_pop(1);
        }
    }

    void Shape::drawPTX(float deltaTime) {}

    void Shape::deactivate() {
        if (poolIndex < MAX_SHAPES) {
            activeFlags[poolIndex] = false;
            flags |= FLAG_DISABLED;
            if (activeCount > 0) {
                activeCount--;
            }
            enemyAttackTimers.clear();
        }
    }

    bool Shape::isActive() const {
        if (poolIndex < MAX_SHAPES) {
            return activeFlags[poolIndex] && !(flags & FLAG_DISABLED);
        }
        return false;
    }
    
    bool Shape::canDamageEnemy(uint32_t enemyId) const {
        // Check if we have a timer for this enemy
        auto it = enemyAttackTimers.find(enemyId);
        if (it == enemyAttackTimers.end()) {
            // No timer means we can damage
            return true;
        }
        // Timer exists, check if it's expired
        return it->second <= 0.0f;
    }
    
    void Shape::registerEnemyHit(uint32_t enemyId) {
        // Set the attack timer for this enemy
        enemyAttackTimers[enemyId] = attackFrequency;
    }
}