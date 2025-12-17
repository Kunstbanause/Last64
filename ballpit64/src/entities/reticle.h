#ifndef RETICLE_H
#define RETICLE_H

#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <libdragon.h>

typedef struct {
  T3DVec3 pos;
  float speed; // units per second
  T3DMat4FP* matrix;  // Add matrix pointer like ball has
} Reticle;

// Initialize reticle in front of the player
void reticle_init(Reticle* reticle, T3DVec3 player_pos);

// Update reticle position based on C-button input
void reticle_update(Reticle* reticle, joypad_buttons_t btn_held, float deltaTime, float box_size);

// Render the reticle
void reticle_render(Reticle* reticle, T3DModel* model);

// Destroy reticle and free memory
void reticle_destroy(Reticle* reticle);

#endif // RETICLE_H
