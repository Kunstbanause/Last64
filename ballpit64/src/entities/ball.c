#include "ball.h"
#include "../systems/collision.h"
#include <libdragon.h>
#include <math.h>

void ball_init(Ball* ball, T3DVec3 start_pos) {
  ball->pos = start_pos;
  ball->speed = 120.0f;
  ball->vx = 0.0f;
  ball->vz = -ball->speed; // shoot upward (negative z)
  ball->released = false;
  ball->matrix = malloc_uncached(sizeof(T3DMat4FP));
  t3d_mat4fp_identity(ball->matrix);
}

void ball_update(Ball* ball, float deltaTime, float box_size) {
  // Integrate velocity
  ball->pos.v[0] += ball->vx * deltaTime;
  ball->pos.v[2] += ball->vz * deltaTime;

  // Bounce on arena walls (X) and top (Z min)
  if(ball->pos.v[0] < -box_size) {
    ball->pos.v[0] = -box_size;
    ball->vx = -ball->vx;
  }
  if(ball->pos.v[0] > box_size) {
    ball->pos.v[0] = box_size;
    ball->vx = -ball->vx;
  }
  if(ball->pos.v[2] < -box_size) {
    ball->pos.v[2] = -box_size;
    ball->vz = -ball->vz;
  }
}

bool ball_check_catch(Ball* ball, T3DVec3 player_pos, T3DVec3 reticle_pos, float box_size) {
  float ballCatchRadius = 8.0f;
  float distToBall = collision_distance_xz(ball->pos, player_pos);
  
  // Check if ball has been released (moved far enough from player)
  if(!ball->released && distToBall > 15.0f) {
    ball->released = true;
  }
  
  bool was_caught = false;
  
  // Caught by player
  if(ball->released && distToBall < ballCatchRadius) {
    was_caught = true;
  }
  // Hit bottom wall
  else if(ball->pos.v[2] > box_size) {
    was_caught = true;
  }
  
  if(was_caught) {
    // Reset to player and shoot toward reticle
    ball->pos = player_pos;
    ball->released = false;
    
    // Direction from player to reticle
    float dirx = reticle_pos.v[0] - player_pos.v[0];
    float dirz = reticle_pos.v[2] - player_pos.v[2];
    
    // Normalize
    float len = sqrtf(dirx*dirx + dirz*dirz);
    if(len < 0.1f) {
      // Reticle too close; default to straight up
      dirx = 0.0f;
      dirz = -1.0f;
      len = 1.0f;
    }
    
    ball->vx = (dirx / len) * ball->speed;
    ball->vz = (dirz / len) * ball->speed;
  }
  
  return was_caught;
}

void ball_render(Ball* ball, T3DModel* model) {
  t3d_mat4fp_from_srt_euler(ball->matrix, 
    (float[3]){0.12f, 0.12f, 0.12f}, 
    (float[3]){0, 0, 0}, 
    ball->pos.v
  );
  t3d_matrix_push(ball->matrix);
  rdpq_set_prim_color(RGBA32(255, 0, 0, 255));
  t3d_model_draw(model);
  t3d_matrix_pop(1);
}

void ball_destroy(Ball* ball) {
  if(ball->matrix) {
    free(ball->matrix);
    ball->matrix = NULL;
  }
}
