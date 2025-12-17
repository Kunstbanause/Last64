#include "player.h"
#include <math.h>

void player_init(Player* player, T3DModel* model, T3DVec3 start_pos) {
  player->pos = start_pos;
  player->move_dir = (T3DVec3){{0, 0, 0}};
  player->rot_y = 0.0f;
  player->curr_speed = 0.0f;
  player->anim_blend = 0.0f;
  player->is_attack = false;
  
  // Allocate matrix
  player->matrix = malloc_uncached(sizeof(T3DMat4FP));
  
  // Create skeletons
  player->skel = t3d_skeleton_create(model);
  player->skel_blend = t3d_skeleton_clone(&player->skel, false);
  
  // Create animations
  player->anim_idle = t3d_anim_create(model, "Snake_Idle");
  t3d_anim_attach(&player->anim_idle, &player->skel);
  
  player->anim_walk = t3d_anim_create(model, "Snake_Walk");
  t3d_anim_attach(&player->anim_walk, &player->skel_blend);
  
  player->anim_attack = t3d_anim_create(model, "Snake_Attack");
  t3d_anim_set_looping(&player->anim_attack, false);
  t3d_anim_set_playing(&player->anim_attack, false);
  t3d_anim_attach(&player->anim_attack, &player->skel);

  // Initialize matrix
  t3d_mat4fp_identity(player->matrix);
}

void player_update(Player* player, joypad_inputs_t joypad, joypad_buttons_t btn, float deltaTime, float box_size) {
  // Calculate movement direction
  T3DVec3 new_dir = {{
    (float)joypad.stick_x * 0.05f, 0,
    -(float)joypad.stick_y * 0.05f
  }};
  float speed = sqrtf(t3d_vec3_len2(&new_dir));
  
  // Handle attack input
  if((btn.a || btn.b) && !player->anim_attack.isPlaying) {
    t3d_anim_set_playing(&player->anim_attack, true);
    t3d_anim_set_time(&player->anim_attack, 0.0f);
    player->is_attack = true;
  }
  
  // Movement logic
  if(speed > 0.15f && !player->is_attack) {
    new_dir.v[0] /= speed;
    new_dir.v[2] /= speed;
    player->move_dir = new_dir;
    
    float new_angle = atan2f(player->move_dir.v[0], player->move_dir.v[2]);
    player->rot_y = t3d_lerp_angle(player->rot_y, new_angle, 0.25f);
    player->curr_speed = t3d_lerp(player->curr_speed, speed * 0.15f, 0.15f);
  } else {
    player->curr_speed *= 0.8f;
  }
  
  // Calculate animation blend
  player->anim_blend = player->curr_speed / 0.51f;
  if(player->anim_blend > 1.0f) player->anim_blend = 1.0f;
  
  // Move player
  player->pos.v[0] += player->move_dir.v[0] * player->curr_speed;
  player->pos.v[2] += player->move_dir.v[2] * player->curr_speed;
  
  // Clamp to arena bounds
  if(player->pos.v[0] < -box_size) player->pos.v[0] = -box_size;
  if(player->pos.v[0] > box_size) player->pos.v[0] = box_size;
  if(player->pos.v[2] < -box_size) player->pos.v[2] = -box_size;
  if(player->pos.v[2] > box_size) player->pos.v[2] = box_size;
  
  // Update animations
  t3d_anim_update(&player->anim_idle, deltaTime);
  t3d_anim_set_speed(&player->anim_walk, player->anim_blend + 0.15f);
  t3d_anim_update(&player->anim_walk, deltaTime);
  
  if(player->is_attack) {
    t3d_anim_update(&player->anim_attack, deltaTime);
    if(!player->anim_attack.isPlaying) player->is_attack = false;
  }
  
  // Blend animations
  t3d_skeleton_blend(&player->skel, &player->skel, &player->skel_blend, player->anim_blend);
}

void player_update_skeleton(Player* player) {
  t3d_skeleton_update(&player->skel);
}

void player_render(Player* player, rspq_block_t* dpl_block) {
  // Update matrix with fixed yaw and roll based on movement
  float fixed_yaw = T3D_DEG_TO_RAD(180.0f);
  float max_lean = 0.45f;
  float roll = -player->move_dir.v[0] * max_lean * (player->curr_speed / 0.51f);
  if(roll > max_lean) roll = max_lean;
  if(roll < -max_lean) roll = -max_lean;
  
  t3d_mat4fp_from_srt_euler(player->matrix,
    (float[3]){0.125f, 0.125f, 0.125f},
    (float[3]){0.0f, fixed_yaw, roll},
    player->pos.v
  );
  
  // Run the display list (if provided)
  if(dpl_block) {
    rspq_block_run(dpl_block);
  }
}

void player_destroy(Player* player) {
  t3d_skeleton_destroy(&player->skel);
  t3d_skeleton_destroy(&player->skel_blend);
  
  t3d_anim_destroy(&player->anim_idle);
  t3d_anim_destroy(&player->anim_walk);
  t3d_anim_destroy(&player->anim_attack);
  
  if(player->matrix) {
    free(player->matrix);
    player->matrix = NULL;
  }
}
