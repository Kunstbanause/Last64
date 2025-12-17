#include "enemy.h"
#include "../systems/collision.h"
#include <libdragon.h>
#include <stdlib.h>

void enemy_system_init(EnemySystem* sys, float box_size) {
  // Allocate uncached matrices for DMA
  sys->matrices = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP) * MAX_ENEMIES);
  for(int i = 0; i < MAX_ENEMIES; i++) {
    t3d_mat4fp_identity(&sys->matrices[i]);
    sys->enemies[i].active = false;
    sys->enemies[i].health = 2;
  }
  
  // Wave configuration
  sys->wave_timer = 0.0f; // spawn first wave immediately
  sys->wave_timer_max = 12.0f;
  sys->wave_spawned = false;
  
  // Grid configuration
  sys->grid_cell_size_x = (box_size * 2.0f) / GRID_COLS;
  sys->spawn_z = -box_size + 10.0f;
  sys->speed = 10.0f;
}

void enemy_system_update(EnemySystem* sys, float deltaTime, float box_size) {
  // Update wave timer
  sys->wave_timer -= deltaTime;
  
  // Spawn wave when timer reaches zero
  if(sys->wave_timer <= 0.0f && !sys->wave_spawned) {
    sys->wave_spawned = true;
    
    // Spawn enemies in grid pattern
    for(int col = 0; col < GRID_COLS; col++) {
      // Top row always spawns
      for(int i = 0; i < MAX_ENEMIES; i++) {
        if(!sys->enemies[i].active) {
          sys->enemies[i].active = true;
          sys->enemies[i].health = 2;
          float grid_x = -box_size + (col + 0.5f) * sys->grid_cell_size_x;
          float grid_z = sys->spawn_z;
          sys->enemies[i].pos = (T3DVec3){{grid_x, 1.15f, grid_z}};
          break;
        }
      }
      
      // Lower rows with probabilistic spawning
      for(int row = 1; row < 4; row++) {
        float chance = 1.0f / (1.0f + row * 1.5f);
        if(rand() % 100 < (int)(chance * 100.0f)) {
          for(int i = 0; i < MAX_ENEMIES; i++) {
            if(!sys->enemies[i].active) {
              sys->enemies[i].active = true;
              sys->enemies[i].health = 2;
              float grid_x = -box_size + (col + 0.5f) * sys->grid_cell_size_x;
              float grid_z = sys->spawn_z - row * sys->grid_cell_size_x * 0.8f;
              sys->enemies[i].pos = (T3DVec3){{grid_x, 1.15f, grid_z}};
              break;
            }
          }
        }
      }
    }
  }
  
  // Reset wave timer
  if(sys->wave_timer <= 0.0f && sys->wave_spawned) {
    sys->wave_timer = sys->wave_timer_max;
    sys->wave_spawned = false;
  }
  
  // Update enemy positions
  for(int i = 0; i < MAX_ENEMIES; i++) {
    if(!sys->enemies[i].active) continue;
    
    // Move downward
    sys->enemies[i].pos.v[2] += sys->speed * deltaTime;
    
    // Remove if off screen
    if(sys->enemies[i].pos.v[2] > box_size) {
      sys->enemies[i].active = false;
    }
  }
}

void enemy_system_check_ball_collision(EnemySystem* sys, T3DVec3 ball_pos, float ball_radius, float* ball_vz) {
  for(int i = 0; i < MAX_ENEMIES; i++) {
    if(!sys->enemies[i].active) continue;
    
    if(collision_circle_circle_xz(ball_pos, ball_radius, sys->enemies[i].pos, 6.0f)) {
      sys->enemies[i].health--;
      if(sys->enemies[i].health <= 0) {
        sys->enemies[i].active = false;
      }
      // Bounce ball back
      *ball_vz = -(*ball_vz);
    }
  }
}

void enemy_system_render(EnemySystem* sys, T3DModel* model) {
  for(int i = 0; i < MAX_ENEMIES; i++) {
    if(!sys->enemies[i].active) continue;
    
    t3d_mat4fp_from_srt_euler(&sys->matrices[i], 
      (float[3]){0.1f, 0.1f, 0.1f}, 
      (float[3]){0, 0, 0}, 
      sys->enemies[i].pos.v
    );
    t3d_matrix_push(&sys->matrices[i]);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
    rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
    t3d_model_draw(model);
    t3d_matrix_pop(1);
  }
}

int enemy_system_get_count(EnemySystem* sys) {
  int count = 0;
  for(int i = 0; i < MAX_ENEMIES; i++) {
    if(sys->enemies[i].active) count++;
  }
  return count;
}

void enemy_system_destroy(EnemySystem* sys) {
  if(sys->matrices) {
    free(sys->matrices);
    sys->matrices = NULL;
  }
}
