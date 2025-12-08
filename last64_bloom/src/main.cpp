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
#include "render/colors.h"
#include "render/colorTest.h"

#include "main.h"
#include "debugMenu.h"
#include "postProcess.h"
#include "render/debugDraw.h"
#include "rsp/rspFX.h"

#include "scene/scene.h"
#include "scene/sceneManager.h"
#include "scene/scenes/sceneBunker.h"
#include "scene/scenes/sceneLast64.h"
#include "systems/experience.h"
#include "render/hdrBoost.h"
#include "audio.h"
#include "memory/savegame.h"
#include "actors/xpShard.h"
#include "utils/profiler.h"

State state{
  .ppConf = {
    .blurSteps = 3,
    .blurBrightness = 1.1f, // BLOOM (2.1 was too extreme)
    .hdrFactor = 0.8f, // EXPOS (1.1 was too extreme)
    .bloomThreshold = 0.5f,
    .scalingUseRDP = true,
   },
  .showOffscreen = false,
  .autoExposure = false,
  .activeScene = nullptr
};

namespace {
  constexpr int BUFF_COUNT = 3;
  bool showMenu = false; // Moved here so it's accessible to isMenuVisible

  rspq_profile_data_t profileData{};
  #if RSPQ_PROFILE
    uint64_t lastUcodeTime = 0;
  #endif
}

// Implementation of debug menu visibility check
bool Debug::isMenuVisible() { 
    return showMenu; 
}

void Debug::setMenuVisible(bool visible) {
    showMenu = visible;
}

surface_t* fb = NULL;

// Define the global SFX manager here so it lives for the lifetime of the program
// and does not get recreated when scenes are reloaded.
SFXManager gSFXManager;

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
  SaveGame::init();
  debugf("SaveGame: eeprom_present=%d\n", eeprom_present());

  joypad_init();
  gSFXManager.init();
  // Play background music once when main starts
  gSFXManager.play(SFXManager::SFX_MUSIC1);

  t3d_init((T3DInitParams){});
  tpx_init((TPXInitParams){});

  t3d_fog_set_enabled(false);

  // Initialize HDR boost system
  HDRBoost::initialize(state.ppConf.hdrFactor);
  
  // Initialize profiler
  Profiler::init();

  SceneManager::loadScene(0);

  uint32_t frameIdx = 0;
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
    joypad_buttons_t combined = {0};
    for (int i = JOYPAD_PORT_1; i <= JOYPAD_PORT_4; i++) {
      joypad_buttons_t b = joypad_get_buttons_pressed((joypad_port_t)i);
      combined.a      |= b.a;
      combined.b      |= b.b;
      combined.z      |= b.z;
      combined.start  |= b.start;
      combined.l      |= b.l;
      combined.r      |= b.r;
      combined.d_up   |= b.d_up;
      combined.d_down |= b.d_down;
      combined.d_left |= b.d_left;
      combined.d_right|= b.d_right;
      combined.c_up   |= b.c_up;
      combined.c_down |= b.c_down;
      combined.c_left |= b.c_left;
      combined.c_right|= b.c_right;
    }
    // Toggle menu on Start button press (rising edge across any port)
    static bool lastStart = false;
    if (combined.start && !lastStart) {
      showMenu = !showMenu;
    }
    lastStart = combined.start;

  // Toggle between static and fly camera
    if(combined.z && state.activeScene) {
      SceneBunker* sceneBunker = dynamic_cast<SceneBunker*>(state.activeScene);
      if(sceneBunker) {
        sceneBunker->useFlyCam = !sceneBunker->useFlyCam;
        //debugf("Camera mode: %s\n", sceneBunker->useFlyCam ? "Fly" : "Static");
      }
    }

    // Simple save/load test bindings when debug menu is visible:
    // L + R -> save incrementing counter (on press)
    // L + Z -> load and print (on press)
    static uint32_t testSaveCounter = 0;

    if (showMenu) {
      // Save when R pressed together (edge detect)
      if (combined.r) {
        testSaveCounter++;
        bool ok = SaveGame::save_test_value(testSaveCounter);
        debugf("Save test value %lu -> %s\n", (unsigned long)testSaveCounter, ok ? "OK" : "FAIL");
      }
      // Load when L pressed together (edge detect)
      if (combined.l) {
        uint32_t val = 0;
        bool ok = SaveGame::load_test_value(val);
        debugf("Load test value -> %lu (%s)\n", (unsigned long)val, ok ? "OK" : "FAIL");
      }
    }

  { // Frame profiling scope
    ProfileScope frameProfile("Frame");
  
    float realDelta = display_get_delta_time();
  float deltaTime = realDelta;
    // "Pause"
    // if (showMenu) {
    //   deltaTime *= 0.1f; // Slow-motion time down to 10% speed
    // }

    // Tick Experience slow-motion realtime timer (counts down in real seconds)
    Experience::tickSlowMotionRealtime(realDelta);
    if (Experience::getSlowMotionRemaining() > 0.0f) {
      // deltaTime *= Experience::getSlowMotionScale();
    }
    // Advance audio fades
    gSFXManager.update(deltaTime);
      {
        ProfileScope profile("Update");
        state.activeScene->update(deltaTime);
      }

      // Check if the current scene (if it's SceneLast64) has requested a restart
      SceneLast64* currentLast64Scene = dynamic_cast<SceneLast64*>(state.activeScene);
      if (currentLast64Scene && currentLast64Scene->isRestartRequested()) {
          debugf("Restarting scene...\n");
          SceneManager::loadScene(0); // Reload scene 0
      }

    // ----------- DRAW ------------ //
    fb = display_get();
    rdpq_attach(fb, display_get_zbuf());

    // Set the default HDR factor from the state
    HDRBoost::setDefaultHDRFactor(state.ppConf.hdrFactor);

    // Add debug output every x frames to show the game is running
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter % 1500 == 0) {
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

    // Update HDR boost system
    HDRBoost::update(deltaTime);
    
    // Apply HDR boost factor
    PostProcessConf modifiedConf = state.ppConf;
    modifiedConf.hdrFactor = HDRBoost::getCurrentHDRFactor();
    postProc[frameIdx].setConf(modifiedConf);
    postProc[frameIdx].beginFrame();

    t3d_frame_start();
    rdpq_mode_antialias(AA_NONE);
    rdpq_mode_dithering(DITHER_NONE_NONE);
    rdpq_mode_fog(0);

    {
      ProfileScope profile("Render3D");
      state.activeScene->draw(deltaTime);
    }

    postProc[frameIdx].endFrame();
    surface_t surfBlur;
    {
      ProfileScope profile("PostFX");
      surfBlur = postProc[frameIdxLast].applyEffects(*fb);
    }

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
      Debug::printf(SCREEN_WIDTH-64, SCREEN_HEIGHT-20, "fps:%.0f", display_get_fps());
      
      // Display profiling data
      #if RSPQ_PROFILE
        Debug::printf(20, 220, "RSP:%.2fms", lastUcodeTime / 1000.0f);
      #endif
      
      // Display CPU profiling data (show total frame time + max)
      if (SaveGame::is_profiling_enabled()) {
        int sectionCount = 0;
        const Profiler::Section* sections = Profiler::getSections(sectionCount);
        int yPos = 40;
        
        // Display all sections (increased limit to show new high-level sections)
        for (int i = 0; i < sectionCount && i < 16; ++i) {
          // Show sections that have accumulated time OR have a max recorded
          if (sections[i].total_ticks > 0 || sections[i].max_ticks > 0) {
            // Convert total accumulated ticks to microseconds (not averaged)
            uint64_t totalUs = (sections[i].total_ticks * 1000000ULL) / RCP_FREQUENCY;
            uint64_t maxUs = (sections[i].max_ticks * 1000000ULL) / RCP_FREQUENCY;
            // Divide by number of accumulated frames (10) to get per-frame time
            float frameUs = (float)totalUs / 10.0f;
            // Max is already per-frame (single frame peak), don't divide by 10
            float maxFrameUs = (float)maxUs;
            
            // Display with fixed-width formatting to prevent flickering
            // Always use same format (ms) for stability
            Debug::printf(170, yPos, "%s:%4.1f/%4.1fms", sections[i].name, 
                         frameUs / 1000.0f, maxFrameUs / 1000.0f);
            yPos += 12;
          }
        }
      }
    }
    if ( display_get_fps() < 30.0f ) {
      Debug::printf(SCREEN_WIDTH-80, SCREEN_HEIGHT-30, "LowFPS:%.0f", display_get_fps());
    }

    #if RSPQ_PROFILE
      Debug::printf(20, 220, "%.2fms", lastUcodeTime / 1000.0f);
    #endif

    {
      ProfileScope profile("UI");
      state.activeScene->draw2D(deltaTime);
    }
    
    // Profiler frame end - accumulate data for display
    Profiler::frameEnd();

    // Tick experience system for UI flash animations
    Experience::tick(deltaTime);
    
    if (showMenu) {
      // Draw color test strip RGBA32
      static float screenAdjustedWidth = SCREEN_WIDTH-90;
      constexpr int colorCount = sizeof(Colors::testColors2D) / sizeof(Colors::testColors2D[0]);
      for (int i = 0; i < colorCount; i++) {
          float x = (float)i * (screenAdjustedWidth / colorCount);
          float width = screenAdjustedWidth / colorCount;
          //Extract color components from testColors2D array
          uint8_t r = (Colors::testColors2D[i] >> 24) & 0xFF;
          uint8_t g = (Colors::testColors2D[i] >> 16) & 0xFF;
          uint8_t b = (Colors::testColors2D[i] >> 8) & 0xFF;
          uint8_t a = Colors::testColors2D[i] & 0xFF;
          rdpq_set_mode_fill(RGBA32(r, g, b, a));
          rdpq_fill_rectangle(x+45, 170, x+45 + width, 170 + 4);
      }
    }

    // Draw XP Bar
    const int barHeight = 3;  // Reduced to 1/3 of original height (was 10, now 3 gives ~6px bar)
    float xpPercentage = Experience::getXPPercentage();
    int barWidth = static_cast<int>(xpPercentage * SCREEN_WIDTH);

    rdpq_set_scissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Use fill mode with color for the XP bar body
    rdpq_set_mode_fill(RGBA32(100, 200, 255, 255)); // Light blue color
    rdpq_fill_rectangle(0, SCREEN_HEIGHT - (barHeight * 2), barWidth, SCREEN_HEIGHT);

    // Draw a short white flash at the leading edge when XP is collected
    float flash = Experience::getXPBarFlash();
    if (flash > 0.001f) {
      // Flash width scales with bar width but caps to a reasonable size
      int flashWidth = (int)(16 + (barWidth * 0.05f));
      if (flashWidth > 64) flashWidth = 64;
      int fx0 = barWidth - flashWidth;
      if (fx0 < 0) fx0 = 0;
      uint8_t a = (uint8_t)(255.0f * (flash));
      rdpq_set_mode_fill(RGBA32(255,255,255,a));
      rdpq_fill_rectangle(fx0, SCREEN_HEIGHT - (barHeight * 2), barWidth, SCREEN_HEIGHT);
    }
    rdpq_detach_show();
  } // End Frame profiling scope

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

