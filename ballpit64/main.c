#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>
#include <t3d/t3ddebug.h>

/**
 * Example project showcasing the usage of the animation system.
 * This includes instancing animations, blending animations, and controlling playback.
 */

float get_time_s() {
  return (float)((double)get_ticks_us() / 1000000.0);
}

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

  rdpq_font_t *fnt = rdpq_font_load("rom:/fibberish.font64");
  rdpq_font_style(fnt, 0, &(rdpq_fontstyle_t){.color = (color_t){0xFF, 0xFF, 0xFF, 0xFF}});
  rdpq_font_style(fnt, 1, &(rdpq_fontstyle_t){.color = (color_t){232, 101, 65, 0xFF}});
  rdpq_font_style(fnt, 2, &(rdpq_fontstyle_t){.color = (color_t){79, 209, 133, 0xFF}});
  rdpq_font_style(fnt, 3, &(rdpq_fontstyle_t){.color = (color_t){216, 220, 180, 0xFF}});
  rdpq_text_register_font(FONT_MAIN, fnt);

  sprite_t *spriteBox = sprite_load("rom:/textbox.i8.sprite");

  joypad_init();

  t3d_init((T3DInitParams){});
  rdpq_text_register_font(FONT_BUILTIN_DEBUG_MONO, rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO));
  T3DViewport viewport = t3d_viewport_create();

  T3DMat4FP* modelMatFP = malloc_uncached(sizeof(T3DMat4FP));
  T3DMat4FP* ballMatFP = malloc_uncached(sizeof(T3DMat4FP));
  T3DMat4FP* mapMatFP = malloc_uncached(sizeof(T3DMat4FP));
  t3d_mat4fp_from_srt_euler(mapMatFP, (float[3]){0.3f, 0.3f, 0.3f}, (float[3]){0, 0, 0}, (float[3]){0, 0, -10});

  // Static top-down camera location (centered in arena)
  T3DVec3 camPos = {{0, 188.0f, 36.0f}};   // high above, pulled back slightly
  T3DVec3 camTarget = {{0, 0, 0.0f}};    // look slightly forward of origin

  T3DVec3 lightDirVec = {{1.0f, 1.0f, 1.0f}};
  t3d_vec3_norm(&lightDirVec);

  uint8_t colorAmbient[4] = {0xAA, 0xAA, 0xAA, 0xFF};
  uint8_t colorDir[4]     = {0xFF, 0xAA, 0xAA, 0xFF};

  T3DModel *modelMap = t3d_model_load("rom:/map.t3dm");
  T3DModel *modelShadow = t3d_model_load("rom:/shadow.t3dm");

  // Model Credits: Quaternius (CC0) https://quaternius.com/packs/easyenemy.html
  T3DModel *model = t3d_model_load("rom:/snake.t3dm");

  // First instantiate skeletons, they will be used to draw models in a specific pose
  // And serve as the target for animations to modify
  T3DSkeleton skel = t3d_skeleton_create(model);
  T3DSkeleton skelBlend = t3d_skeleton_clone(&skel, false); // optimized for blending, has no matrices

  // Now create animation instances (by name), the data in 'model' is fixed,
  // whereas 'anim' contains all the runtime data.
  // Note that tiny3d internally keeps no track of animations, it's up to the user to manage and play them.
  T3DAnim animIdle = t3d_anim_create(model, "Snake_Idle");
  t3d_anim_attach(&animIdle, &skel); // tells the animation which skeleton to modify

  T3DAnim animWalk = t3d_anim_create(model, "Snake_Walk");
  t3d_anim_attach(&animWalk, &skelBlend);

  // multiple animations can attach to the same skeleton, this will NOT perform any blending
  // rather the last animation that updates "wins", this can be useful if multiple animations touch different bones
  T3DAnim animAttack = t3d_anim_create(model, "Snake_Attack");
  t3d_anim_set_looping(&animAttack, false); // don't loop this animation
  t3d_anim_set_playing(&animAttack, false); // start in a paused state
  t3d_anim_attach(&animAttack, &skel);

  rspq_block_begin();
    t3d_matrix_push(modelMatFP);
    rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
    t3d_model_draw_skinned(model, &skel); // as in the last example, draw skinned with the main skeleton

    rdpq_set_prim_color(RGBA32(0, 0, 0, 120));
    t3d_model_draw(modelShadow);
    t3d_matrix_pop(1);
  rspq_block_t *dplSnake = rspq_block_end();

  rspq_block_begin();
    t3d_matrix_push(mapMatFP);
    rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
    t3d_model_draw(modelMap);
    t3d_matrix_pop(1);
  rspq_block_t *dplMap = rspq_block_end();

  rspq_block_t *dplTextbox = NULL;
  // control whether the popup should be shown; prevents immediate re-creation
  bool showPopup = false;

  // Start menu open
  bool startMenuOpen = false;

  // Camera tuning variables (editable with controller)
  float camY = camPos.v[1];
  float camZ = camPos.v[2];
  float targetZ = camTarget.v[2];
  int camEditMode = 0; // 0 = camY, 1 = camZ, 2 = targetZ
  const char *camEditNames[3] = {"camY", "camZ", "tZ"};

  float lastTime = get_time_s() - (1.0f / 60.0f);
  rspq_syncpoint_t syncPoint = 0;

  T3DVec3 moveDir = {{0,0,0}};
  T3DVec3 playerPos = {{0,0.15f,0}};

  // Ball state
  typedef struct Ball {
    T3DVec3 pos;
    float vx;
    float vz;
    float speed;
  } Ball;

  Ball ball;
  // initialize ball at player position and shoot upwards (negative z)
  ball.pos = playerPos;
  ball.speed = 120.0f;
  ball.vx = 0.0f;
  ball.vz = -ball.speed;

  float rotY = 0.0f;
  float currSpeed = 0.0f;
  float animBlend = 0.0f;
  bool isAttack = false;

  for(;;)
  {
    // ======== Update ======== //
    joypad_poll();

    float newTime = get_time_s();
    float deltaTime = newTime - lastTime;
    lastTime = newTime;

    joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
    joypad_buttons_t btn = joypad_get_buttons_pressed(JOYPAD_PORT_1);

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
      if(joypad.btn.c_up) adjust += camAdjustSpeed * deltaTime;
      if(joypad.btn.c_down) adjust -= camAdjustSpeed * deltaTime;

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

    T3DVec3 newDir = {{
       (float)joypad.stick_x * 0.05f, 0,
      -(float)joypad.stick_y * 0.05f
    }};
    float speed = sqrtf(t3d_vec3_len2(&newDir));

    // Player Attack
    if((btn.a || btn.b) && !animAttack.isPlaying) {
      t3d_anim_set_playing(&animAttack, true);
      t3d_anim_set_time(&animAttack, 0.0f);
      isAttack = true;
    }

    // Player movement
    if(speed > 0.15f && !isAttack) {
      newDir.v[0] /= speed;
      newDir.v[2] /= speed;
      moveDir = newDir;

      float newAngle = atan2f(moveDir.v[0], moveDir.v[2]);
      rotY = t3d_lerp_angle(rotY, newAngle, 0.25f);
      currSpeed = t3d_lerp(currSpeed, speed * 0.15f, 0.15f);
    } else {
      currSpeed *= 0.8f;
    }

    // use blend based on speed for smooth transitions
    animBlend = currSpeed / 0.51f;
    if(animBlend > 1.0f)animBlend = 1.0f;

    // move player...
    playerPos.v[0] += moveDir.v[0] * currSpeed;
    playerPos.v[2] += moveDir.v[2] * currSpeed;
    // ...and limit position inside the box
    const float BOX_SIZE = 140.0f;
    if(playerPos.v[0] < -BOX_SIZE)playerPos.v[0] = -BOX_SIZE;
    if(playerPos.v[0] >  BOX_SIZE)playerPos.v[0] =  BOX_SIZE;
    if(playerPos.v[2] < -BOX_SIZE)playerPos.v[2] = -BOX_SIZE;
    if(playerPos.v[2] >  BOX_SIZE)playerPos.v[2] =  BOX_SIZE;

    // Camera is static (configured before the main loop)
    t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(85.0f), 10.0f, 150.0f);
    t3d_viewport_look_at(&viewport, &camPos, &camTarget, &(T3DVec3){{0,1,0}});

    // Update the animation and modify the skeleton, this will however NOT recalculate the matrices
    t3d_anim_update(&animIdle, deltaTime);
    t3d_anim_set_speed(&animWalk, animBlend + 0.15f);
    t3d_anim_update(&animWalk, deltaTime);

    if(isAttack) {
      t3d_anim_update(&animAttack, deltaTime); // attack animation now overrides the idle one
      if(!animAttack.isPlaying)isAttack = false;
    }

    // We now blend the walk animation with the idle/attack one
    t3d_skeleton_blend(&skel, &skel, &skelBlend, animBlend);

    if(syncPoint)rspq_syncpoint_wait(syncPoint); // wait for the RSP to process the previous frame

    // Now recalc. the matrices, this will cause any model referencing them to use the new pose
    t3d_skeleton_update(&skel);

    // Update player matrix
    // Keep yaw fixed so player always faces "up"; apply a small roll for sideways lean.
    float fixedYaw = T3D_DEG_TO_RAD(180.0f); // rotate model 180 degrees so it faces upward on screen
    // roll: lean into the lateral direction; scale by current speed for nicer effect
    float maxLean = 0.45f; // radians (~25deg)
    float roll = -moveDir.v[0] * maxLean * (currSpeed / 0.51f);
    // clamp roll
    if(roll > maxLean) roll = maxLean;
    if(roll < -maxLean) roll = -maxLean;

    t3d_mat4fp_from_srt_euler(modelMatFP,
      (float[3]){0.125f, 0.125f, 0.125f},
      (float[3]){0.0f, fixedYaw, roll},
      playerPos.v
    );

    // ===== Ball physics (2D X/Z) =====
    // Integrate
    ball.pos.v[0] += ball.vx * deltaTime;
    ball.pos.v[2] += ball.vz * deltaTime;

    // Bounce on arena walls (X) and top (Z min). Reset when hitting bottom (Z max)
    if(ball.pos.v[0] < -BOX_SIZE) {
      ball.pos.v[0] = -BOX_SIZE;
      ball.vx = -ball.vx;
    }
    if(ball.pos.v[0] > BOX_SIZE) {
      ball.pos.v[0] = BOX_SIZE;
      ball.vx = -ball.vx;
    }
    if(ball.pos.v[2] < -BOX_SIZE) {
      ball.pos.v[2] = -BOX_SIZE;
      ball.vz = -ball.vz;
    }

    if(ball.pos.v[2] > BOX_SIZE) {
      // hit bottom: reset to player and immediately shoot again
      ball.pos = playerPos;
      // initial shot direction: upward (-Z). add small x component from player's lateral movement
      float dirx = moveDir.v[0];
      float dirz = -1.0f;
      // normalize
      float len = sqrtf(dirx*dirx + dirz*dirz);
      if(len == 0.0f) len = 1.0f;
      ball.vx = (dirx / len) * ball.speed;
      ball.vz = (dirz / len) * ball.speed;
    }

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
    rspq_block_run(dplSnake);

    // Draw ball here while the 3D pipeline is active
    t3d_matrix_push(ballMatFP);
    // small scale so it looks like a sphere/shadow
    t3d_mat4fp_from_srt_euler(ballMatFP, (float[3]){0.08f,0.08f,0.08f}, (float[3]){0,0,0}, ball.pos.v);
    rdpq_set_prim_color(RGBA32(255, 200, 80, 255));
    t3d_model_draw(model); // using 'model' as temporary visual for the ball
    t3d_matrix_pop(1);

    syncPoint = rspq_syncpoint_new();

    // ======== Draw (UI) ======== //

    float posCenter = display_get_width() / 2;
    float posY = display_get_height() - 90;
    float bxWidth = 220.0f;
    float bxHeight = 72.0f;
    float posX = posCenter - bxWidth / 2;

    if(showPopup && !dplTextbox)
    {
      dplTextbox = create_textbox_popup(spriteBox, FONT_MAIN,
        "^01~ Test Title ~",
        "^02[A Button]^00 Test color text\n"
        "^02[C]^00 C buttons\n"
        "^02[Z]^03 Z Button\n"
      );
    }

    if(dplTextbox) rspq_block_run(dplTextbox);

    rdpq_text_printf(NULL, FONT_MAIN, 24, 24, "FPS: %.2f", display_get_fps());
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

  t3d_skeleton_destroy(&skel);
  t3d_skeleton_destroy(&skelBlend);

  t3d_anim_destroy(&animIdle);
  t3d_anim_destroy(&animWalk);
  t3d_anim_destroy(&animAttack);

  t3d_model_free(model);
  t3d_model_free(modelMap);
  t3d_model_free(modelShadow);

  t3d_destroy();
  return 0;
}

