/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "enemy.h"
#include "player.h"
#include "../systems/experience.h"
#include "xpShard.h"
#include "../render/hdrBoost.h"
#include "../main.h"
#include "../utils/profiler.h"
#include <t3d/t3d.h>
#include <t3d/tpx.h>
#include <libdragon.h>
#include <malloc.h>



namespace Actor {
    static float g_lastSeparationMS = 0.0f;
    // Static member definitions
    T3DVertPacked* Enemy::sharedVertices = nullptr;
    T3DMat4FP** Enemy::sharedMatrices = nullptr;
    bool* Enemy::activeFlags = nullptr;
    uint32_t Enemy::activeCount = 0;
    uint32_t Enemy::totalDeathCount = 0;
    bool Enemy::initialized = false;
    Enemy Enemy::enemyPool[MAX_ENEMIES];

    Enemy::Enemy() : Base() {
        if (!initialized) {
            initializePool();
        }
        
        poolIndex = MAX_ENEMIES; // Invalid index until spawned
        position = {0, 0, 0};
        speed = 0.0f;
        health = 8;
        maxHealth = 8;
        targetPlayer = nullptr; // Initialize individual target player
        useFixedDirection = false;
        fixedDirection = {{0.0f, 0.0f, 0.0f}};
        size = EnemySize::SMALL;
        color = 0xFF0000FF; // Default red color
        xpReward = 1;
        flags |= FLAG_DISABLED; // Start as disabled
    }

    Enemy::~Enemy() {
        // We don't actually delete from the pool here
        // The pool is managed statically
    }

    void Enemy::initialize() {
        if (!initialized) {
            initializePool();
        }
        totalDeathCount = 0; // Reset death count when initializing a new round
    }

    void Enemy::cleanup() {
        if (sharedVertices) {
            free_uncached(sharedVertices);
            sharedVertices = nullptr;
        }
        
        if (sharedMatrices) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
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

    void Enemy::initializePool() {
        if (initialized) return;

        // Allocate vertices for all enemies (quads)
        T3DVec3 normalVec = {{0.0f, 0.0f, 1.0f}};
        uint16_t norm = t3d_vert_pack_normal(&normalVec);
        sharedVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * MAX_ENEMIES * 2);
        
        for (int i = 0; i < MAX_ENEMIES; i++) {
            int idx = i * 2;
            // First structure: vertices 0 and 1
            sharedVertices[idx] = (T3DVertPacked){};
            sharedVertices[idx].posA[0] = -3; sharedVertices[idx].posA[1] = -3; sharedVertices[idx].posA[2] = 0;
            sharedVertices[idx].normA = norm;
            sharedVertices[idx].posB[0] = 3; sharedVertices[idx].posB[1] = -3; sharedVertices[idx].posB[2] = 0;
            sharedVertices[idx].normB = norm;
            // Ensure enemy colors are also bright for bloom effect
            sharedVertices[idx].rgbaA = 0xFF0000FF; // Bright red
            sharedVertices[idx].rgbaB = 0xFF0000FF; // Bright red
            sharedVertices[idx].stA[0] = 0; sharedVertices[idx].stA[1] = 0;
            sharedVertices[idx].stB[0] = 0; sharedVertices[idx].stB[1] = 0;
            
            // Second structure: vertices 2 and 3
            sharedVertices[idx+1] = (T3DVertPacked){};
            sharedVertices[idx+1].posA[0] = 3; sharedVertices[idx+1].posA[1] = 3; sharedVertices[idx+1].posA[2] = 0;
            sharedVertices[idx+1].normA = norm;
            sharedVertices[idx+1].posB[0] = -3; sharedVertices[idx+1].posB[1] = 3; sharedVertices[idx+1].posB[2] = 0;
            sharedVertices[idx+1].normB = norm;
            sharedVertices[idx+1].rgbaA = 0xFF0000FF; // Bright red
            sharedVertices[idx+1].rgbaB = 0xFF0000FF; // Bright red
            sharedVertices[idx+1].stA[0] = 0; sharedVertices[idx+1].stA[1] = 0;
            sharedVertices[idx+1].stB[0] = 0; sharedVertices[idx+1].stB[1] = 0;
        }
        
        // Allocate matrices for all enemies
        sharedMatrices = (T3DMat4FP**)malloc(sizeof(T3DMat4FP*) * MAX_ENEMIES);
        for (int i = 0; i < MAX_ENEMIES; i++) {
            sharedMatrices[i] = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
            t3d_mat4fp_identity(sharedMatrices[i]);
        }
        
        // Allocate active flags
        activeFlags = (bool*)calloc(MAX_ENEMIES, sizeof(bool));
        
        initialized = true;
    }

    Enemy* Enemy::spawn(const T3DVec3& position, float speed, Player* targetPlayer, EnemySize size, uint32_t color, int xpReward, int health, bool useFixedDirection, T3DVec3 fixedDirection) {
        if (!initialized) {
            initializePool();
        }
        
        // Find an inactive enemy slot
        for (uint32_t i = 0; i < MAX_ENEMIES; i++) {
            if (!activeFlags[i]) {
                // Found an inactive slot, activate it
                activeFlags[i] = true;
                activeCount++;
                
                // Get an enemy from the pool
                Enemy* enemy = &enemyPool[i];
                
                enemy->poolIndex = i;
                enemy->position = position;
                enemy->speed = speed;
                enemy->size = size;
                enemy->color = color;
                enemy->xpReward = xpReward;
                enemy->maxHealth = health;
                enemy->health = health;
                
                // Set movement mode and target player accordingly
                enemy->useFixedDirection = useFixedDirection;
                enemy->fixedDirection = fixedDirection;
                if (useFixedDirection) {
                    enemy->targetPlayer = nullptr;
                } else {
                    enemy->targetPlayer = targetPlayer;
                }
                
                enemy->flags &= ~FLAG_DISABLED; // Enable the enemy
                
                return enemy;
            }
        }
        
        // No inactive slots available
        return nullptr;
    }

    void Enemy::updateAll(float deltaTime) {
        if (!initialized) return;
        
        for (uint32_t i = 0; i < MAX_ENEMIES; i++) {
            if (activeFlags[i]) {
                Enemy* enemy = &enemyPool[i];
                enemy->update(deltaTime);
            }
        }

        // Profiling start
        int profId = Profiler::begin("EnemySep");
        uint64_t ticksStart = get_ticks();

        // Lightweight enemy-enemy separation using a uniform grid
        // Tunables
        const float CELL_SIZE = 40.0f;          // pixels
        const float MIN_DIST = 9.0f;            // desired minimum spacing (larger for clearer separation)
        const float MAX_DISP = 1.5f;            // clamp per-frame displacement
        const int   MAX_NEIGHBORS = 6;          // limit neighbor checks per enemy

        // Arena extents (2D only)
        const float AX = ARENA_LEFT;
        const float AY = ARENA_TOP;
        const float BX = ARENA_RIGHT;
        const float BY = ARENA_BOTTOM;

        const int GRID_W = (int)((BX - AX) / CELL_SIZE) + 2;
        const int GRID_H = (int)((BY - AY) / CELL_SIZE) + 2;
        const int GRID_SIZE = GRID_W * GRID_H;

        // Fixed-size buckets: store indices of enemies per cell
        static int cellCounts[256] = {0};
        static uint16_t cellIndices[256][16]; // up to 16 enemies per cell

        // Resize guards for extreme arena sizes
        int capCells = GRID_SIZE;
        if (capCells > 256) capCells = 256;
        for (int c = 0; c < capCells; ++c) cellCounts[c] = 0;

        auto cellIndexOf = [&](float x, float y) -> int {
            int cx = (int)((x - AX) / CELL_SIZE) + 1;
            int cy = (int)((y - AY) / CELL_SIZE) + 1;
            if (cx < 0) cx = 0;
            if (cx >= GRID_W) cx = GRID_W-1;
            if (cy < 0) cy = 0;
            if (cy >= GRID_H) cy = GRID_H-1;
            int idx = cy * GRID_W + cx;
            if (idx < 0) idx = 0;
            if (idx >= GRID_SIZE) idx = GRID_SIZE-1;
            if (idx >= 256) idx = 255;
            return idx;
        };

        // Build grid
        for (uint32_t i = 0; i < MAX_ENEMIES; i++) {
            if (!activeFlags[i]) continue;
            Enemy* e = &enemyPool[i];
            T3DVec3 p = e->getPosition();
            int ci = cellIndexOf(p.x, p.y);
            int cnt = cellCounts[ci];
            if (cnt < 16) {
                cellIndices[ci][cnt] = (uint16_t)i;
                cellCounts[ci] = cnt + 1;
            }
        }

        // Separation pass
        const float MIN_DIST_SQ = MIN_DIST * MIN_DIST;
        for (uint32_t i = 0; i < MAX_ENEMIES; i++) {
            if (!activeFlags[i]) continue;
            Enemy* ei = &enemyPool[i];
            T3DVec3 pi = ei->getPosition();
            int ci = cellIndexOf(pi.x, pi.y);

            // Neighbors: current cell only (cheap) — can expand to 8-neighborhood if needed
            int checks = 0;
            int cnt = cellCounts[ci];
            for (int k = 0; k < cnt && checks < MAX_NEIGHBORS; ++k) {
                uint16_t idxj = cellIndices[ci][k];
                if (idxj == i) continue;
                if (!activeFlags[idxj]) continue;
                Enemy* ej = &enemyPool[idxj];
                T3DVec3 pj = ej->getPosition();

                float dx = pi.x - pj.x;
                float dy = pi.y - pj.y;
                float d2 = dx*dx + dy*dy;
                if (d2 > 0.0001f && d2 < MIN_DIST_SQ) {
                    float inv = 1.0f / sqrtf(d2);
                    float nx = dx * inv;
                    float ny = dy * inv;
                    float overlap = MIN_DIST - sqrtf(d2);
                    float push = overlap * 0.5f; // split the correction roughly
                    if (push > MAX_DISP) push = MAX_DISP;
                    // Apply to both (half-half). Keep z unchanged.
                    pi.x += nx * push;
                    pi.y += ny * push;
                    pj.x -= nx * push;
                    pj.y -= ny * push;
                    // Write back
                    ei->setPosition(pi);
                    ej->setPosition(pj);
                }
                checks++;
            }
        }
        uint64_t ticksEnd = get_ticks();
        Profiler::end(profId);
        uint64_t delta = ticksEnd - ticksStart;
        g_lastSeparationMS = (float)((double)delta * 1000.0 / (double)RCP_FREQUENCY);
    }

    float Enemy::getLastSeparationMS() { return g_lastSeparationMS; }

    void Enemy::drawAll(float deltaTime) {
        if (!initialized) return;
        
        // Set up rendering state once for all enemies
        t3d_state_set_drawflags((enum T3DDrawFlags)(T3D_FLAG_SHADED | T3D_FLAG_DEPTH));
        
        for (uint32_t i = 0; i < MAX_ENEMIES; i++) {
            if (activeFlags[i]) {
                Enemy* enemy = &enemyPool[i];
                enemy->draw3D(deltaTime);
            }
        }
    }

    void Enemy::update(float deltaTime) {
        if (flags & FLAG_DISABLED) return;

        if (hitTimer > 0.0f) {
            hitTimer -= deltaTime;
        }
        
        // If this enemy uses a fixed direction, move along it
        if (useFixedDirection) {
            // Ensure direction is normalized
            float lx = fixedDirection.x;
            float ly = fixedDirection.y;
            float lz = fixedDirection.z;
            float lenSq = lx*lx + ly*ly + lz*lz;
            if (lenSq > 0.0001f) {
                float invLen = 1.0f / sqrtf(lenSq);  // Only one sqrt
                lx *= invLen; ly *= invLen; lz *= invLen;
                float moveDistance = speed * deltaTime;
                position.x += lx * moveDistance;
                position.y += ly * moveDistance;
                position.z += lz * moveDistance;
            }
        } else {
            // Check if our target player is still alive, if not, find a new one
            if (targetPlayer && targetPlayer->getIsDead()) {
                targetPlayer = Experience::getRandomAlivePlayer();
            }
            
            // If we still don't have a target player, try to get one
            if (!targetPlayer) {
                targetPlayer = Experience::getRandomAlivePlayer();
            }
            
            // Get player position from the individual target player reference
            if (targetPlayer) {
                T3DVec3 playerPos = targetPlayer->getPosition();
                
                // Calculate direction to player
                float dx = playerPos.x - position.x;
                float dy = playerPos.y - position.y;
                float dz = playerPos.z - position.z;
                
                // Normalize direction (optimized with single sqrt)
                float lengthSq = dx*dx + dy*dy + dz*dz;
                if (lengthSq > 0.0001f) {
                    float invLength = 1.0f / sqrtf(lengthSq);
                    dx *= invLength;
                    dy *= invLength;
                    dz *= invLength;
                    
                    // Move enemy directly towards player
                    float moveDistance = speed * deltaTime;
                    position.x += dx * moveDistance;
                    position.y += dy * moveDistance;
                    position.z += dz * moveDistance;
                }
            }
        }
        
        // Deactivate enemies that go too far off-screen
        // Allow some buffer zone so enemies can spawn and enter from outside the arena
        constexpr float OFF_SCREEN_BUFFER = 30.0f;
        if (position.x < ARENA_LEFT - OFF_SCREEN_BUFFER || position.x > ARENA_RIGHT + OFF_SCREEN_BUFFER || 
            position.y < ARENA_TOP - OFF_SCREEN_BUFFER || position.y > ARENA_BOTTOM + OFF_SCREEN_BUFFER) {
            deactivate();
            return;
        }
        
        // Matrix is now only updated in draw3D() to prevent flickering between sizes
    }

    void Enemy::draw3D(float deltaTime) {
        if (flags & FLAG_DISABLED) return;
        
        if (poolIndex < MAX_ENEMIES) {
            uint32_t new_color = color; // Use the enemy's color
            // Hit flash override
            if (hitTimer > 0.96f) {
                uint8_t flash_white = 64;
                new_color = (flash_white << 24) | (flash_white << 16) | (flash_white << 8) | 0xFF;
            }

            // Update vertex colors for this specific enemy
            sharedVertices[poolIndex * 2].rgbaA = new_color;
            sharedVertices[poolIndex * 2].rgbaB = new_color;
            sharedVertices[poolIndex * 2 + 1].rgbaA = new_color;
            sharedVertices[poolIndex * 2 + 1].rgbaB = new_color;

            // Calculate scale based on enemy size
            float scale = 1.0f;
            switch (size) {
                case EnemySize::SMALL:
                    scale = 1.0f;
                    break;
                case EnemySize::MEDIUM:
                    scale = 1.5f;
                    break;
                case EnemySize::LARGE:
                    scale = 4.0f;
                    break;
            }

            // Update matrix with scale
            if (poolIndex < MAX_ENEMIES) {
                t3d_mat4fp_from_srt_euler(
                    sharedMatrices[poolIndex],
                    (T3DVec3){{scale, scale, scale}},  // scale
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

    void Enemy::drawPTX(float deltaTime) {
        // No particle effects for enemies
    }

    void Enemy::deactivate() {
        if (poolIndex < MAX_ENEMIES) {
            activeFlags[poolIndex] = false;
            flags |= FLAG_DISABLED;
            activeCount--;
        }
    }

    bool Enemy::isActive() const {
        if (poolIndex < MAX_ENEMIES) {
            return activeFlags[poolIndex] && !(flags & FLAG_DISABLED);
        }
        return false;
    }

    void Enemy::takeDamage(int amount) {
        // Get the number of active players from the Experience system
        int activePlayers = Experience::getActivePlayerCount();
        
        // Scale damage inversely with the number of active players
        // More players = less effective damage per hit
        int scaledDamage = (activePlayers > 0) ? (amount + activePlayers - 1) / activePlayers : amount; // Integer division with rounding up
        health -= scaledDamage;
        hitTimer = 1.0f;
        if (health <= 0) {
            die();
        }
    }

    void Enemy::die() {
        // Increment death counter for stats tracking
        totalDeathCount++;
        
        // Spawn death VFX with the same position, size, and color as the enemy
        float vfxSize = 1.0f;
        switch (size) {
            case EnemySize::SMALL:
                vfxSize = 1.0f;
                break;
            case EnemySize::MEDIUM:
                vfxSize = 1.5f;
                break;
            case EnemySize::LARGE:
                vfxSize = 4.0f;
                // Trigger HDR boost when a boss dies
                HDRBoost::triggerBoost();
                break;
        }
        
        EnemyDeathVFX::spawn(position, vfxSize, color);

        // Spawn an XP shard holding this enemy's xp reward instead of awarding immediately
        // If this was a large enemy (boss), spawn a much larger shard (4x current size)
        float shardScale = 1.0f;
        if (size == EnemySize::LARGE) shardScale = 4.0f;
        Actor::XPShard::spawn(position, xpReward, color, shardScale);
        deactivate();
    }

    bool Enemy::collidesWith(Base* other) {
        // Use AABB collision detection for better performance and accuracy
        return collidesWithAABB(other);
    }
    
    float Enemy::getRadius() const {
        switch (size) {
            case EnemySize::SMALL:
                return 3.0f;
            case EnemySize::MEDIUM:
                return 5.0f;
            case EnemySize::LARGE:
                return 10.0f;
            default:
                return 3.0f;
        }
    }
    
    void Enemy::getAABBSize(float& width, float& height) const {
        // Return the actual dimensions of the enemy based on its size
        // These values match the sizes used in the rendering code
        switch (size) {
            case EnemySize::SMALL:
                width = 6.0f;   // 3 units in each direction
                height = 6.0f;
                break;
            case EnemySize::MEDIUM:
                width = 9.0f;   // 4.5 units in each direction
                height = 9.0f;
                break;
            case EnemySize::LARGE:
                width = 24.0f;  // 12 units in each direction
                height = 24.0f;
                break;
            default:
                width = 6.0f;
                height = 6.0f;
                break;
        }
    }
}