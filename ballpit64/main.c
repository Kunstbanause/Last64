#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>
#include <t3d/t3ddebug.h>

#include "src/core/game_state.h"
#include "src/input/input_manager.h"
#include "src/systems/collision.h"
#include "src/resources/resource_manager.h"
#include "src/entities/player.h"
#include "src/entities/ball.h"
#include "src/entities/enemy.h"
#include "src/entities/reticle.h"

#define FONT_MAIN 2

// Create a reusable textbox popup. Returns an rspq block that can be run
// or freed. `title` and `body` may contain formatting codes used by rdpq_text.
static rspq_block_t *create_textbox_popup(sprite_t *spriteBox, int fontId, const char *title, const char *body)
{
  float posCenter = display_get_width() / 2;
  float posY = display_get_height() - 90;
  float bxWidth = 220.0f;
  float bxHeight = 72.0f;
  float posX = posCenter - bxWidth / 2;

  rspq_block_begin();

  rdpq_sync_pipe();
  rdpq_sync_tile();
  rdpq_set_mode_standard();
  rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM), (PRIM,0,TEX0,0)));
  rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
  rdpq_set_prim_color((color_t){33, 33, 33, 0x99});

  rdpq_sprite_upload(TILE0, spriteBox, NULL);

  // texture is only the corner, draw 4 times for each corner and extend the clamped texture
  rdpq_texture_rectangle(TILE0, posX,           posY,            posX + bxWidth/2,    posY + bxHeight/2,     0, 0);
  rdpq_texture_rectangle(TILE0, posX,           posY + bxHeight, posX + bxWidth/2,    posY + bxHeight/2 - 1, 0, 0);
  rdpq_texture_rectangle(TILE0, posX + bxWidth, posY,            posX + bxWidth/2 -1, posY + bxHeight/2,     0, 0);
  rdpq_texture_rectangle(TILE0, posX + bxWidth, posY + bxHeight, posX + bxWidth/2 -1, posY + bxHeight/2 - 1, 0, 0);

  // Draw text-box background
  posY += 18;

  if(title && title[0] != '\0') {
    rdpq_text_printf(&(rdpq_textparms_t){
      .align = ALIGN_CENTER, .width = bxWidth, .wrap = WRAP_WORD,
    }, fontId, posX, posY, "%s\n", title);
  }

  if(body && body[0] != '\0') {
    rdpq_text_printf(&(rdpq_textparms_t){
      .align = ALIGN_LEFT, .width = bxWidth, .wrap = WRAP_WORD,
      .line_spacing = -4
    }, fontId, posX+22, posY + 16, "%s", body);
  }

  return rspq_block_end();
}


int main()
{
  debug_init_isviewer();
  debug_init_usblog();
  asset_init_compression(2);

  dfs_init(DFS_DEFAULT_LOCATION);

  display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS);

  rdpq_init();

  // Load all game resources
  Resources resources;
  resources_load(&resources);
  rdpq_text_register_font(FONT_MAIN, resources.font_main);

  joypad_init();

  t3d_init((T3DInitParams){});
  rdpq_text_register_font(FONT_BUILTIN_DEBUG_MONO, rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO));
  T3DViewport viewport = t3d_viewport_create();

  T3DMat4FP* mapMatFP = malloc_uncached(sizeof(T3DMat4FP));
  t3d_mat4fp_from_srt_euler(mapMatFP, (float[3]){0.3f, 0.3f, 0.3f}, (float[3]){0, 0, 0}, (float[3]){0, 0, -10});

  // Static top-down camera location (centered in arena)
  T3DVec3 camPos = {{0, 188.0f, 36.0f}};   // high above, pulled back slightly
  T3DVec3 camTarget = {{0, 0, 0.0f}};    // look slightly forward of origin

  T3DVec3 lightDirVec = {{1.0f, 1.0f, 1.0f}};
  t3d_vec3_norm(&lightDirVec);

  uint8_t colorAmbient[4] = {0xAA, 0xAA, 0xAA, 0xFF};
  uint8_t colorDir[4]     = {0xFF, 0xAA, 0xAA, 0xFF};

  // Initialize game entities
  const float BOX_SIZE = 140.0f;
  
  Player player;
  player_init(&player, resources.model_snake, (T3DVec3){{0, 0.15f, 120.0f}});
  
  Ball ball;
  ball_init(&ball, player.pos);
  
  Reticle reticle;
  reticle_init(&reticle, player.pos);
  
  EnemySystem enemy_system;
  enemy_system_init(&enemy_system, BOX_SIZE);

  rspq_block_begin();
    t3d_matrix_push(player.matrix);
    rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
    t3d_model_draw_skinned(resources.model_snake, &player.skel);

    // Shadow uses texture - ensure wrap mode is set
    rdpq_set_prim_color(RGBA32(0, 0, 0, 120));
    rdpq_mode_filter(FILTER_BILINEAR);
    t3d_model_draw(resources.model_shadow);
    t3d_matrix_pop(1);
  rspq_block_t *dplSnake = rspq_block_end();

  rspq_block_begin();
    t3d_matrix_push(mapMatFP);
    rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
    t3d_model_draw(resources.model_map);
    t3d_matrix_pop(1);
  rspq_block_t *dplMap = rspq_block_end();

  rspq_block_t *dplTextbox = NULL;
  bool showPopup = false;
  bool startMenuOpen = false;

  // Camera tuning variables
  float camY = camPos.v[1];
  float camZ = camPos.v[2];
  float targetZ = camTarget.v[2];
  int camEditMode = 0;

  float lastTime = get_time_s() - (1.0f / 60.0f);
  rspq_syncpoint_t syncPoint = 0;

  for(;;)
  {
    // ======== Update ======== //
    joypad_poll();

    float newTime = get_time_s();
    float deltaTime = newTime - lastTime;
    lastTime = newTime;

    joypad_inputs_t joypad = joypad_get_all_inputs(); // Get combined inputs from all ports
    joypad_buttons_t btn = joypad_get_all_pressed();
    joypad_buttons_t btnHeld = joypad_get_all_held();

    // If a popup is showing and the A button is pressed, close the popup
    if(dplTextbox && btn.a) {
      rspq_block_free(dplTextbox);
      dplTextbox = NULL;
      // prevent it from being re-created immediately
      showPopup = false;
      // consume the A press so it doesn't trigger other actions (like attack)
      btn.a = 0;
    }

    // Toggle start menu
    if(btn.start) {
      startMenuOpen = !startMenuOpen;
    }

    // Camera tuning only active when menu is open
    if(startMenuOpen) {
      // use D-Pad left/right to cycle edit mode, C-up/C-down to change value
      if(btn.d_left) {
        // previous mode
        camEditMode = (camEditMode + 2) % 3;
      }
      if(btn.d_right) {
        // next mode
        camEditMode = (camEditMode + 1) % 3;
      }

      // continuous adjustment while C-buttons are held; scale by deltaTime for smoothness
      float camAdjustSpeed = 40.0f; // units per second
      float adjust = 0.0f;
      if(btnHeld.c_up) adjust += camAdjustSpeed * deltaTime;
      if(btnHeld.c_down) adjust -= camAdjustSpeed * deltaTime;

      if(adjust != 0.0f) {
        if(camEditMode == 0) camY += adjust;
        else if(camEditMode == 1) camZ += adjust;
        else if(camEditMode == 2) targetZ += adjust;
      }
    }

    // apply tuned values to camera (camera uses last tuned values regardless of menu state)
    camPos.v[1] = camY;
    camPos.v[2] = camZ;
    camTarget.v[2] = targetZ;

    // Update reticle
    reticle_update(&reticle, btnHeld, deltaTime, BOX_SIZE);

    // Update enemy system
    enemy_system_update(&enemy_system, deltaTime, BOX_SIZE);
    
    // Update player
    player_update(&player, joypad, btn, deltaTime, BOX_SIZE);

    // Camera is static (configured before the main loop)
    t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(85.0f), 10.0f, 150.0f);
    t3d_viewport_look_at(&viewport, &camPos, &camTarget, &(T3DVec3){{0,1,0}});

    if(syncPoint)rspq_syncpoint_wait(syncPoint);
    
    // Update player skeleton matrices
    player_update_skeleton(&player);

    // Update ball physics
    ball_update(&ball, deltaTime, BOX_SIZE);
    
    // Check ball collision with enemies
    enemy_system_check_ball_collision(&enemy_system, ball.pos, 6.0f, &ball.vz);
    
    // Check if ball should be caught
    ball_check_catch(&ball, player.pos, reticle.pos, BOX_SIZE);

    // ===== Draw ball =====
    // (drawing moved to the 3D draw section after attaching the viewport)

    // ======== Draw (3D) ======== //
    rdpq_attach(display_get(), display_get_zbuf());
    t3d_frame_start();
    t3d_viewport_attach(&viewport);

    t3d_screen_clear_color(RGBA32(224, 180, 96, 0xFF));
    t3d_screen_clear_depth();

    t3d_light_set_ambient(colorAmbient);
    t3d_light_set_directional(0, colorDir, &lightDirVec);
    t3d_light_set_count(1);

    rspq_block_run(dplMap);
    player_render(&player, dplSnake);
    
    // Render game entities
    ball_render(&ball, resources.model_snake);
    enemy_system_render(&enemy_system, resources.model_enemy);
    reticle_render(&reticle, resources.model_shadow);

    syncPoint = rspq_syncpoint_new();

    // ======== Draw (UI) ======== //
    if(showPopup && !dplTextbox)
    {
      dplTextbox = create_textbox_popup(resources.sprite_textbox, FONT_MAIN,
        "^01~ Test Title ~",
        "^02[A Button]^00 Test color text\n"
        "^02[C]^00 C buttons\n"
        "^02[Z]^03 Z Button\n"
      );
    }

    if(dplTextbox) rspq_block_run(dplTextbox);

    //rdpq_text_printf(NULL, FONT_MAIN, 24, 24, "FPS: %.2f", display_get_fps());
    int enemyCount = enemy_system_get_count(&enemy_system);
    rdpq_text_printf(NULL, FONT_MAIN, 24, 24, "Enemies: %d", enemyCount);
    //rdpq_text_printf(NULL, FONT_MAIN, 24, 56, "Reticle: (%.0f, %.0f)", reticle.pos.v[0], reticle.pos.v[2]);
    // Camera debug overlay: color the active value with ^02...^00 (shown only when menu open)
    if(startMenuOpen) {
      const char *camFmt0 = "CamY: ^02%.1f^00  CamZ: %.1f  TargZ: %.1f";
      const char *camFmt1 = "CamY: %.1f  CamZ: ^02%.1f^00  TargZ: %.1f";
      const char *camFmt2 = "CamY: %.1f  CamZ: %.1f  TargZ: ^02%.1f^00";
      const char *camFmt = camFmt0;
      if(camEditMode == 1) camFmt = camFmt1;
      else if(camEditMode == 2) camFmt = camFmt2;
      rdpq_text_printf(NULL, FONT_MAIN, 24, 40, camFmt, camY, camZ, targetZ);
    }
    rdpq_detach_show();
  }

  player_destroy(&player);
  ball_destroy(&ball);
  reticle_destroy(&reticle);
  enemy_system_destroy(&enemy_system);

  resources_free(&resources);

  t3d_destroy();
  return 0;
}

