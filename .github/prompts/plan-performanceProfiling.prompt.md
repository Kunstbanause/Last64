# Performance Profiling Strategy for Last64_Bloom

## Overview
This document outlines a comprehensive strategy for measuring and optimizing performance in Last64_Bloom, focusing on CPU-bound operations while leveraging libdragon's built-in RSP/RDP profiling capabilities.

## Phase 1: Enable Existing Profiling Infrastructure

### RSP/RDP Profiling (Already Partially Implemented)
The game already displays `lastUcodeTime` in main.cpp (line 286), but we can expand this:

**Current State:**
```cpp
debugf("LastUcode: %d us\n", (int)lastUcodeTime);
```

**Expand to Full Profile Data:**
- Enable `RSPQ_PROFILE` in libdragon build flags (if not already enabled)
- Display additional metrics: frame_count, RSP slots breakdown, RDP usage
- Reference: `examples/23_hdr/main.c` and `examples/99_testscene/main.c` show full profiling overlay

**Key Metrics to Track:**
- Total frame time (target: 16.67ms for 60fps)
- RSP microcode time
- RDP sync time
- CPU time (frame_time - RSP - RDP)

## Phase 2: Add CPU-Side Timing Infrastructure

### Create Profiler Helper Class
```cpp
// src/utils/profiler.h
#pragma once
#include <libdragon.h>

struct ProfileSection {
    const char* name;
    uint64_t start_ticks;
    uint64_t total_ticks;
    uint32_t call_count;
};

class Profiler {
public:
    static void begin(const char* section);
    static void end(const char* section);
    static void reset();
    static void display();
    
private:
    static constexpr int MAX_SECTIONS = 16;
    static ProfileSection sections[MAX_SECTIONS];
    static int section_count;
};

// RAII helper for automatic timing
class ProfileScope {
public:
    ProfileScope(const char* name) : name(name) { Profiler::begin(name); }
    ~ProfileScope() { Profiler::end(name); }
private:
    const char* name;
};
```

### Usage Pattern
```cpp
void SceneLast64::updateScene(float deltaTime) {
    ProfileScope profile("SceneUpdate");
    
    {
        ProfileScope profile("EnemyUpdate");
        for (auto& enemy : enemies) {
            enemy->update(deltaTime);
        }
    }
    
    {
        ProfileScope profile("ProjectileUpdate");
        for (auto& proj : projectiles) {
            proj->update(deltaTime);
        }
    }
    
    {
        ProfileScope profile("Collision");
        checkCollisions();
    }
}
```

## Phase 3: Identify Hot Spots (Suspected Performance Bottlenecks)

### 1. Collision Detection (HIGH PRIORITY)
**Location:** `sceneLast64.cpp` - checkCollisions() and related methods

**Current Complexity:**
- Projectile-Enemy: O(n*m) where n=projectiles (up to 500), m=enemies (up to 250+)
- Enemy-Player: O(n*4) where n=enemies
- XPShard-Player: O(n*4) where n=shards (up to 100)

**Worst Case:** 500 × 250 = 125,000 distance checks per frame!

**Optimization Strategies:**
- Spatial partitioning (grid-based or quadtree)
- Broad-phase culling (AABB checks before distance)
- Early rejection based on actor state (dead, inactive)
- Distance-squared comparisons (avoid sqrt)

### 2. Enemy AI/Pathfinding
**Location:** `src/actors/enemy.cpp` - update() method

**Current State:** Each enemy updates individually (250+ per frame)

**Potential Issues:**
- Player distance calculation per enemy
- Vector normalization per enemy
- Movement calculations

**Optimization Strategies:**
- Batch enemy updates (process in chunks)
- Stagger AI updates (update 1/4 of enemies per frame)
- Cache player positions (don't recalculate 250 times)
- Distance-based LOD (far enemies update less frequently)

### 3. XP Shard Physics
**Location:** `src/actors/xpShard.cpp` - update() method

**Current State:** Magnetic attraction with distance checks for each player

**Optimization Strategies:**
- Spatial partitioning (only check nearby shards)
- Reduce attraction update frequency for distant shards
- Batch position updates

### 4. Weapon System
**Location:** `src/systems/weapon_*.cpp` - Various weapon updates

**Potential Issues:**
- Multiple weapons per player (up to 8 weapons × 4 players)
- Per-frame cooldown checks
- Projectile spawning

**Optimization Strategies:**
- Early exit if weapon on cooldown
- Pool projectile spawn requests
- Reduce unnecessary calculations in fire() methods

### 5. Drawing Overhead
**Location:** `sceneLast64.cpp` - draw3D() and draw2D()

**Current State:**
- Individual t3d_model_draw() calls per actor
- Sprite drawing for each UI element
- Text rendering per HUD element

**Optimization Strategies:**
- Batch similar draw calls
- Frustum culling (don't draw off-screen objects)
- Distance-based LOD (reduce detail for far objects)
- Reduce UI redraw frequency (only update on change)

### 6. Actor Pool Management
**Location:** Various actor files with static pools

**Current State:**
- Linear search for inactive actors when spawning
- Full iteration over max pool size

**Optimization Strategies:**
- Free list for inactive actors
- Compact active actors to front of array
- Track active count to avoid iterating empty slots

## Phase 4: Quick Wins (Low-Hanging Fruit)

### 1. Distance-Squared Comparisons
**Impact:** Eliminate sqrt() calls in collision detection

**Before:**
```cpp
float dist = sqrtf(dx*dx + dy*dy + dz*dz);
if (dist < radius) { /* collision */ }
```

**After:**
```cpp
float distSq = dx*dx + dy*dy + dz*dz;
if (distSq < radius*radius) { /* collision */ }
```

### 2. Early Exit for Dead/Inactive Actors
**Impact:** Skip update/collision logic for inactive actors

```cpp
if (!actor->isActive || actor->health <= 0) continue;
```

### 3. Cache Frequently Used Values
**Examples:**
- Player positions (recalculated 250+ times per frame in enemy AI)
- Pickup range squared (used in every XP shard update)
- Weapon multipliers (calculated repeatedly)

### 4. Reduce Redundant Calculations
**Example in XP Shards:**
```cpp
// Instead of calculating distance for attraction AND collision
float distSq = calculateDistanceSquared(player, shard);
if (distSq < attractionRadiusSq) {
    applyAttraction(distSq);
    if (distSq < collisionRadiusSq) {
        collect();
    }
}
```

## Phase 5: Measurement Strategy

### Baseline Metrics to Capture
1. **FPS:** Target 60fps (16.67ms frame budget)
2. **Frame Time Breakdown:**
   - Scene update time
   - Collision detection time
   - Drawing time (CPU-side setup)
   - RSP/RDP time (from RSPQ_PROFILE)

3. **Per-System Timing:**
   - Enemy update (individual + total)
   - Projectile update
   - XP shard update
   - Weapon system update
   - Player update

4. **Worst-Case Scenarios:**
   - Max enemies (250) + max projectiles (500) + max XP (100)
   - 4 players active with multiple weapons each
   - Dense combat scenarios

### Test Scenarios
1. **Early Game:** Few enemies, few projectiles (baseline)
2. **Mid Game:** 50-100 enemies, 100-200 projectiles
3. **Late Game:** 200+ enemies, 400+ projectiles
4. **Stress Test:** Max spawns with debug menu

## Phase 6: Implementation Plan

### Step 1: Add Profiler Infrastructure (1-2 hours)
- Create profiler.h/cpp
- Add ProfileScope to key systems
- Implement display overlay

### Step 2: Gather Baseline Data (30 mins)
- Run test scenarios
- Record frame times and bottlenecks
- Identify worst offenders with data

### Step 3: Implement Quick Wins (1-2 hours)
- Distance-squared optimizations
- Early exits for inactive actors
- Value caching

### Step 4: Optimize Worst Offender (2-4 hours)
- Likely collision detection
- Implement spatial partitioning or broad-phase culling

### Step 5: Measure Again (30 mins)
- Compare before/after metrics
- Validate improvements

### Step 6: Iterate (as needed)
- Move to next bottleneck
- Repeat optimize-measure cycle

## Phase 7: Expected Outcomes

### Conservative Estimates
- **Collision Detection:** 50-75% reduction with spatial partitioning
- **Enemy AI:** 25-50% reduction with batching/caching
- **XP Shards:** 20-30% reduction with distance checks optimization
- **Overall:** 30-50% frame time improvement in worst-case scenarios

### Success Criteria
- Maintain 60fps with max enemies + max projectiles
- Frame time < 14ms in typical scenarios (leave 2ms buffer)
- No frame drops during intense combat

## Ignoring: Background Marble Pattern
As requested, we're not optimizing the background marble rendering at this time. This is already handled by the GPU and typically not a bottleneck.

## Tools & References

### libdragon Profiling
- `RSPQ_PROFILE` flag enables profiling
- `rspq_profile_get_data()` retrieves profile data
- `RCP_TICKS_TO_USECS()` macro converts ticks to microseconds

### Timing Functions
- `get_ticks()` - High-resolution CPU timer
- `TICKS_TO_MS()` / `TICKS_TO_US()` - Conversion macros
- `display_get_fps()` - Current FPS

### Example References
- `examples/99_testscene/main.c` - Full profiling overlay
- `examples/23_hdr/main.c` - Performance measurement patterns
- `examples/24_hdr_bloom/main.c` - HDR + bloom performance

## Next Steps

**Option A: Implement Profiling Infrastructure First**
- Build profiler.h/cpp
- Add timing to all major systems
- Gather data before optimizing
- Make decisions based on measurements

**Option B: Optimize Known Bottlenecks Immediately**
- Start with collision detection (high confidence bottleneck)
- Implement spatial partitioning
- Add quick wins (distance-squared, early exits)
- Measure improvements afterward

**Recommendation:** Option A (measure first) is safer, but Option B could yield faster results if time-constrained. Given the clear complexity issues in collision detection (O(n²) with large n), starting with Option B might be justified.
