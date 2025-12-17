#ifndef PLAYER_H
#define PLAYER_H

#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>
#include <libdragon.h>

typedef struct {
  T3DVec3 pos;
  T3DVec3 move_dir;
  float rot_y;
  float curr_speed;
  float anim_blend;
  bool is_attack;
  
  // Animation data
  T3DSkeleton skel;
  T3DSkeleton skel_blend;
  T3DAnim anim_idle;
  T3DAnim anim_walk;
  T3DAnim anim_attack;
  
  // Rendering
  T3DMat4FP* matrix;
} Player;

// Initialize player with model and starting position
void player_init(Player* player, T3DModel* model, T3DVec3 start_pos);

// Update player movement, animation, and matrix
void player_update(Player* player, joypad_inputs_t joypad, joypad_buttons_t btn, float deltaTime, float box_size);

// Render the player (must be called after skeleton_update)
void player_render(Player* player, rspq_block_t* dpl_block);

// Update skeleton matrices (call after all animation updates)
void player_update_skeleton(Player* player);

// Cleanup player resources
void player_destroy(Player* player);

#endif // PLAYER_H
