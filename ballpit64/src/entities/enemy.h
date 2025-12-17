#ifndef ENEMY_H
#define ENEMY_H

#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <stdbool.h>

#define MAX_ENEMIES 64
#define GRID_COLS 11

typedef struct {
  T3DVec3 pos;
  int health;
  bool active;
} Enemy;

typedef struct {
  Enemy enemies[MAX_ENEMIES];
  T3DMat4FP* matrices; // Shared uncached matrices
  
  // Wave spawning
  float wave_timer;
  float wave_timer_max;
  bool wave_spawned;
  
  // Configuration
  float grid_cell_size_x;
  float spawn_z;
  float speed;
} EnemySystem;

// Initialize enemy system
void enemy_system_init(EnemySystem* sys, float box_size);

// Update all enemies (movement, wave spawning)
void enemy_system_update(EnemySystem* sys, float deltaTime, float box_size);

// Check collision between ball and enemies, damage/destroy enemies on hit
void enemy_system_check_ball_collision(EnemySystem* sys, T3DVec3 ball_pos, float ball_radius, float* ball_vz);

// Render all active enemies
void enemy_system_render(EnemySystem* sys, T3DModel* model);

// Get count of active enemies
int enemy_system_get_count(EnemySystem* sys);

// Cleanup enemy system
void enemy_system_destroy(EnemySystem* sys);

#endif // ENEMY_H
