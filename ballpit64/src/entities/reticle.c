#include "reticle.h"
#include <stdlib.h>

void reticle_init(Reticle* reticle, T3DVec3 player_pos) {
  // Start at player position
  reticle->pos = player_pos;
  reticle->speed = 80.0f;
  // CRITICAL: Allocate uncached matrix like ball does
  reticle->matrix = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
  t3d_mat4fp_identity(reticle->matrix);
}

void reticle_update(Reticle* reticle, joypad_buttons_t btn_held, float deltaTime, float box_size) {
  // Move with C-buttons
  if(btn_held.c_left) reticle->pos.v[0] -= reticle->speed * deltaTime;
  if(btn_held.c_right) reticle->pos.v[0] += reticle->speed * deltaTime;
  if(btn_held.c_up) reticle->pos.v[2] -= reticle->speed * deltaTime;
  if(btn_held.c_down) reticle->pos.v[2] += reticle->speed * deltaTime;

  // Clamp to arena bounds
  if(reticle->pos.v[0] < -box_size) reticle->pos.v[0] = -box_size;
  if(reticle->pos.v[0] > box_size) reticle->pos.v[0] = box_size;
  if(reticle->pos.v[2] < -box_size) reticle->pos.v[2] = -box_size;
  if(reticle->pos.v[2] > box_size) reticle->pos.v[2] = box_size;
}

void reticle_render(Reticle* reticle, T3DModel* shadow_model) {
  // Render shadow underneath (same pattern as example 08_animation)
  t3d_mat4fp_from_srt_euler(reticle->matrix, 
    (float[3]){0.12f, 0.12f, 0.12f}, 
    (float[3]){0, 0, 0}, 
    reticle->pos.v
  );
  t3d_matrix_push(reticle->matrix);
  rdpq_set_prim_color(RGBA32(255, 255, 0, 120)); // Semi-transparent yellow for reticle
  t3d_model_draw(shadow_model);
  t3d_matrix_pop(1);
}

void reticle_destroy(Reticle* reticle) {
  if(reticle->matrix) {
    free(reticle->matrix);
    reticle->matrix = NULL;
  }
}
