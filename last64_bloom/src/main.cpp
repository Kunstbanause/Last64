/**
* @copyright 2025 - Max Bebök
* @license MIT
*/

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// Note: Check Readme.md for more details
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

#include <array>
#include <libdragon.h>
#include <rspq_constants.h>
#include <rspq_profile.h>
#include <mixer.h>

#include <t3d/t3d.h>
#include <t3d/tpx.h>

#include "main.h"
#include "debugMenu.h"
#include "postProcess.h"
#include "render/debugDraw.h"
#include "rsp/rspFX.h"

#include "scene/scene.h"
#include "scene/sceneManager.h"
#include "scene/scenes/sceneBunker.h"
#include "scene/scenes/sceneLast64.h" // Include SceneLast64 header
#include "systems/experience.h"
#include "audio.h"

State state{
  .ppConf = {
    .blurSteps = 1,
    .blurBrightness = 1.0f,
    .hdrFactor = 1.0f,
    .bloomThreshold = 0.2f,
    .scalingUseRDP = true,
   },
  .showOffscreen = false,
  .autoExposure = false,
  .activeScene = nullptr
};

namespace {
  constexpr int BUFF_COUNT = 3;

  rspq_profile_data_t profileData{};
  #if RSPQ_PROFILE
    uint64_t lastUcodeTime = 0;
  #endif
}

surface_t* fb = NULL;

[[noreturn]]
int main()
{
  profileData.frame_count = 0;
	debug_init_isviewer();
	debug_init_usblog();

  dfs_init(DFS_DEFAULT_LOCATION);
  audio_init(44100, 4);
  mixer_init(16);
  
  // Initialize random number generator
  srand(TICKS_READ());

  display_init(RESOLUTION_320x240, DEPTH_16_BPP, BUFF_COUNT, GAMMA_NONE, FILTERS_RESAMPLE);

  rdpq_init();
  //rdpq_debug_start();
  #if RSPQ_PROFILE
    rspq_profile_start();
  #endif

  RspFX::init();
  PostProcess postProc[BUFF_COUNT]{};
  Debug::init();

  joypad_init();
  gSFXManager.init();

  t3d_init((T3DInitParams){});
  tpx_init((TPXInitParams){});

  t3d_fog_set_enabled(false);

  SceneManager::loadScene(0);

  uint32_t frameIdx = 0;
  bool showMenu = false;

  int lastBrightnessIdx = 0;
  std::array<float, 8> lastBrightness{};

  for(uint64_t frame = 0;; ++frame)
  {
    if (audio_can_write()) {
      int nsamples = audio_get_buffer_length();
      int16_t *buf = audio_write_begin();
      mixer_poll(buf, nsamples);
      audio_write_end();
    }

    uint32_t frameIdxLast = (frameIdx+BUFF_COUNT-1) % BUFF_COUNT;

    SceneManager::update();

    joypad_poll();
    if(joypad_get_buttons_pressed(JOYPAD_PORT_1).start)showMenu = !showMenu;
    if(joypad_get_buttons_pressed(JOYPAD_PORT_2).start)showMenu = !showMenu;
    
    // Toggle between static and fly camera
    auto pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if(pressed.z && state.activeScene) {
      SceneBunker* sceneBunker = dynamic_cast<SceneBunker*>(state.activeScene);
      if(sceneBunker) {
        sceneBunker->useFlyCam = !sceneBunker->useFlyCam;
        //debugf("Camera mode: %s\n", sceneBunker->useFlyCam ? "Fly" : "Static");
      }
    }

    float deltaTime = display_get_delta_time();
    // "Pause"
    if (showMenu) {
      deltaTime *= 0.1f; // Slow down to 10% speed
    }
      state.activeScene->update(deltaTime);

      // Check if the current scene (if it's SceneLast64) has requested a restart
      SceneLast64* currentLast64Scene = dynamic_cast<SceneLast64*>(state.activeScene);
      if (currentLast64Scene && currentLast64Scene->isRestartRequested()) {
          debugf("Restarting scene...\n");
          SceneManager::loadScene(0); // Reload scene 0
      }

    // ----------- DRAW ------------ //
    fb = display_get();
    rdpq_attach(fb, display_get_zbuf());

    // Add debug output every 60 frames to show the game is running
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter % 60 == 0) {
        debugf("Game running... Frame: %d\n", frameCounter);
    }

    if(state.autoExposure) {
      lastBrightnessIdx = (lastBrightnessIdx+1) % lastBrightness.size();
      lastBrightness[lastBrightnessIdx] = postProc[frameIdxLast].getBrightness();

      float avg = 0;
      for(auto b : lastBrightness)avg += b;
      avg /= lastBrightness.size();

      float delta = 0.05f;
      float target = 0.40f;
      if(avg > (target+delta)) {
        float adjust = (0.8f) * deltaTime;
        state.ppConf.hdrFactor = fmaxf(state.ppConf.hdrFactor-adjust, 0.0f);
      }
      if(avg < (target-delta)) {
        float adjust = (0.8f) * deltaTime;
        state.ppConf.hdrFactor = fmin(state.ppConf.hdrFactor+adjust, 8.0f);
      }
    }

    postProc[frameIdx].setConf(state.ppConf);
    postProc[frameIdx].beginFrame();

    t3d_frame_start();
    rdpq_mode_antialias(AA_NONE);
    rdpq_mode_dithering(DITHER_NONE_NONE);
    rdpq_mode_fog(0);

    state.activeScene->draw(deltaTime);

    postProc[frameIdx].endFrame();
    auto surfBlur = postProc[frameIdxLast].applyEffects(*fb);

    rdpq_sync_pipe();
    rdpq_set_color_image(fb);

    // Debug: show last offscreen buffer (downscaled and/or blur)
    if(state.showOffscreen)
    {
      rdpq_sync_tile();
      rdpq_sync_load();

      rdpq_set_mode_standard();
      rdpq_mode_combiner(RDPQ_COMBINER_TEX);
      rdpq_mode_blender(0);
      rdpq_mode_antialias(AA_NONE);
      rdpq_mode_filter(FILTER_POINT);

      rdpq_blitparms_t param{};
      param.scale_x = 4.0f;
      param.scale_y = 4.0f;
      rdpq_tex_blit(&surfBlur, 0, 0, &param);
    }

    rdpq_sync_pipe(); // Ensure all previous RDPQ commands are flushed
    Debug::printStart();
    if(showMenu) {
      DebugMenu::draw();
      Debug::printf(20, 200, "%d%%", (int)(postProc[frameIdxLast].getBrightness() * 100));
      Debug::printf(SCREEN_WIDTH-50, SCREEN_HEIGHT-15, "fps:%.0f", display_get_fps());
    }
    if ( display_get_fps() < 30.0f ) {
      Debug::printf(SCREEN_WIDTH-80, SCREEN_HEIGHT-25, "LowFPS:%.0f", display_get_fps());
    }
    
    

    #if RSPQ_PROFILE
      Debug::printf(20, 220, "%.2fms", lastUcodeTime / 1000.0f);
    #endif

    state.activeScene->draw2D(deltaTime);

    // Draw XP Bar
    const int barHeight = 10;
    float xpPercentage = Experience::getXPPercentage();
    int barWidth = static_cast<int>(xpPercentage * SCREEN_WIDTH);

    rdpq_set_scissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Use fill mode with color
    rdpq_set_mode_fill(RGBA32(100, 200, 255, 255)); // Light blue color
    rdpq_fill_rectangle(0, SCREEN_HEIGHT - (barHeight * 2), barWidth, SCREEN_HEIGHT);
    rdpq_detach_show();

    #if RSPQ_PROFILE
      rspq_profile_next_frame();
      if(++profileData.frame_count == 30) {
        rspq_profile_get_data(&profileData);
        //rspq_profile_dump();
        rspq_profile_reset();

        for(auto &p : profileData.slots) {
          if(p.name && strcmp(p.name, "rsp_fx") == 0) {
            lastUcodeTime = (((p.total_ticks) * 1000000ULL) / RCP_FREQUENCY) / profileData.frame_count;
            break;
          }
        }
        profileData.frame_count = 0;
      }
    #endif

    frameIdx = (frameIdx+1) % BUFF_COUNT;
  }
}

