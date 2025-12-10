/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "debugMenu.h"
#include <libdragon.h>
#include "render/debugDraw.h"
#include "scene/sceneManager.h"
#include <vector>
#include "memory/savegame.h"
#include "audio.h"

// External flags from scenes
extern bool showMarbleBackground;
extern bool marbleBackgroundChanged;

namespace
{
  constinit int menuSel{};
  constinit int maxMenuSel{};
  constinit int idxCustom{};

  constinit int sceneId{};
  constinit bool needsSceneLoad{false};

  template<typename T>
  constexpr T clamp(T val, T min, T max)
  {
    if(val < min) return min;
    if(val > max) return max;
    return val;
  }

  std::vector<DebugMenu::Entry> entries{};
  std::vector<bool*> changedFlags{};

  // Music toggle state and changed flag
  bool musicEnabledVar = true;
  bool musicChangedFlag = false;
  // Marble background toggle state and changed flag
  bool marbleEnabledVar = true;
  bool marbleChangedFlag = false;
  // Profiling toggle state and changed flag
  bool profilingEnabledVar = false;
  bool profilingChangedFlag = false;
  // Return to main menu flag and changed flag
  bool returnToMainMenuVar = false;
  bool returnToMainMenuChanged = false;

  // Helper function to find the index of the scene entry
  int findSceneEntryIndex() {
    for (size_t i = 0; i < entries.size(); i++) {
      if (entries[i].value == &sceneId) {
        return static_cast<int>(i);
      }
    }
    return -1; // Not found
  }
}

// Global variable for debug weapon selection
int debugWeaponSelection = 0; // 0 = Random, 1-5 = Specific weapons
bool isForceAllPlayers = false; 

static inline joypad_buttons_t joypad_get_all_pressed() {
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
    return combined;
}

static inline joypad_buttons_t joypad_get_all_held() {
    joypad_buttons_t combined = {0};
    for (int i = JOYPAD_PORT_1; i <= JOYPAD_PORT_4; i++) {
        joypad_buttons_t b = joypad_get_buttons_held((joypad_port_t)i);

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
    return combined;
}

void DebugMenu::reset()
{
  entries.clear();
  changedFlags.clear();

  // entries.push_back({"        ", EntryType::NONE, nullptr}); // Separator
  entries.push_back({"Weapon  ", EntryType::INT, &debugWeaponSelection, 0, 6}); // 0 = Random, 1-5 = Specific weapons
  entries.push_back({"Force MP", EntryType::BOOL, &isForceAllPlayers});
  entries.push_back({"        ", EntryType::NONE, nullptr}); // Separator
  entries.push_back({"Scene   ", EntryType::INT, &sceneId, 0, 4});
  entries.push_back({"Tex     ", EntryType::BOOL, &state.showOffscreen});
  entries.push_back({"Blurs   ", EntryType::INT, &state.ppConf.blurSteps, 0, 50});
  entries.push_back({"Bloom   ", EntryType::FLOAT, &state.ppConf.blurBrightness, 0.0f, 8.0f, 0.01f});
  entries.push_back({"Expos   ", EntryType::FLOAT, &state.ppConf.hdrFactor, 0.0f, 8.0f, 0.03f});
  entries.push_back({"Thres   ", EntryType::FLOAT, &state.ppConf.bloomThreshold, 0.0f, 1.0f, 1.0f/256.0f});
  entries.push_back({"RDP-S   ", EntryType::BOOL, &state.ppConf.scalingUseRDP});
  entries.push_back({"Auto    ", EntryType::BOOL, &state.autoExposure});
  entries.push_back({"        ", EntryType::NONE, nullptr}); // Separator
  entries.push_back({"Music   ", EntryType::BOOL, &musicEnabledVar});
  entries.push_back({"Profile ", EntryType::BOOL, &profilingEnabledVar});
  entries.push_back({"MainMenu", EntryType::BOOL, &returnToMainMenuVar});

  changedFlags.resize(entries.size());
  
  // Set the needsSceneLoad flag for the scene entry
  int sceneEntryIndex = findSceneEntryIndex();
  if (sceneEntryIndex >= 0) {
    changedFlags[sceneEntryIndex] = &needsSceneLoad;
  }

  // Wire music changed flag to the Music entry
  for (size_t i = 0; i < entries.size(); ++i) {
    if (entries[i].value == &musicEnabledVar) {
      changedFlags[i] = &musicChangedFlag;
      break;
    }
  }
  // Wire profiling changed flag to the Profile entry
  for (size_t i = 0; i < entries.size(); ++i) {
    if (entries[i].value == &profilingEnabledVar) {
      changedFlags[i] = &profilingChangedFlag;
      break;
    }
  }
  // Wire return to main menu flag
  for (size_t i = 0; i < entries.size(); ++i) {
    if (entries[i].value == &returnToMainMenuVar) {
      changedFlags[i] = &returnToMainMenuChanged;
      break;
    }
  }

  menuSel = 0;
  idxCustom = entries.size();

  // Initialize music state from savegame
  musicEnabledVar = SaveGame::is_music_enabled();
  gSFXManager.setMusicEnabled(musicEnabledVar);
  
  // Initialize marble background state from savegame
  marbleEnabledVar = SaveGame::is_marble_enabled();
  
  // Initialize profiling state from savegame
  profilingEnabledVar = SaveGame::is_profiling_enabled();
}

void DebugMenu::addEntry(const Entry& entry, bool *changedFlag) {
  entries.push_back(entry);
  changedFlags.resize(entries.size());
  changedFlags[entries.size()-1] = changedFlag;
  // menuSel = entries.size()-1;
}

void DebugMenu::draw()
{
  auto btn  = joypad_get_all_pressed();
  auto held = joypad_get_all_held();

  // Close debug menu with B button
  if(btn.b) {
    Debug::setMenuVisible(false);
    return;
  }

  // Handle L/R buttons for scene selection
  // int sceneEntryIndex = findSceneEntryIndex();
  // if (sceneEntryIndex >= 0) {
  //   if(btn.l && sceneId > entries[sceneEntryIndex].min) {
  //     sceneId--; needsSceneLoad = true;
  //   }
  //   if(btn.r && sceneId < entries[sceneEntryIndex].max) {
  //     sceneId++; needsSceneLoad = true;
  //   }
  // }

  if(needsSceneLoad) {
    SceneManager::loadScene(sceneId);
    needsSceneLoad = false;
  }

  maxMenuSel = entries.size()-1;
  if(btn.d_up || btn.c_up)menuSel--;
  if(btn.d_down || btn.c_down)menuSel++;
  if(menuSel < 0)menuSel = maxMenuSel;
  if(menuSel > maxMenuSel)menuSel = 0;

  int selDir = 0;
  if(btn.d_right || btn.c_right)selDir = 1;
  if(btn.d_left || btn.c_left)selDir = -1;

  int heldDir = 0;
  if(held.d_right || held.c_right)heldDir = 1;
  if(held.d_left || held.c_left)heldDir = -1;

  // Modify current entry value
  Entry &curr = entries[menuSel];
  switch(curr.type) {
    case EntryType::INT:
      if(selDir != 0) {
        int *value = (int*)curr.value;
        *value = clamp(*value + selDir, (int)curr.min, (int)curr.max);
        if(changedFlags[menuSel])*changedFlags[menuSel] = true;
      }
      break;
    case EntryType::FLOAT:
      if(heldDir != 0) {
        float *value = (float*)curr.value;
        *value = clamp(*value + heldDir * curr.incr * (held.z ? 4.0f : 1.0f), curr.min, curr.max);
        if(changedFlags[menuSel])*changedFlags[menuSel] = true;
      }
      break;
    case EntryType::BOOL:
      if(selDir != 0) {
        bool *value = (bool*)curr.value;
        *value = !(*value);
        if(changedFlags[menuSel])*changedFlags[menuSel] = true;
      }
      break;
      default:
        // NONE
        break;
  }

  // If music toggle was changed, persist and apply immediately
  if (musicChangedFlag) {
    SaveGame::set_music_enabled(musicEnabledVar);
  gSFXManager.setMusicEnabled(musicEnabledVar);
    musicChangedFlag = false;
  }
  
  // If marble toggle was changed, persist immediately
  if (marbleChangedFlag) {
    SaveGame::set_marble_enabled(marbleEnabledVar);
    marbleChangedFlag = false;
  }
  
  // If profiling toggle was changed, persist immediately
  if (profilingChangedFlag) {
    SaveGame::set_profiling_enabled(profilingEnabledVar);
    profilingChangedFlag = false;
  }
  
  // If marble background was changed from scene, persist immediately
  if (marbleBackgroundChanged) {
    SaveGame::set_marble_enabled(showMarbleBackground);
    marbleBackgroundChanged = false;
  }

  // Return to main menu is handled by checking isReturnToMainMenuRequested()
  // Don't reset the flag here - let the scene consume it

  float posX = 20;
  float posY = 30;
  Debug::print(posX+30, posY, "START Menu");
  // {
  //   // Print savegame info block
  //   int type = SaveGame::get_type();
  //   uint32_t last = SaveGame::get_last_saved_value();
  //   const char *tstr = "NONE";
  //   if (type == 1) tstr = "4k";
  //   if (type == 2) tstr = "16k";
  //   int saveInfoX = posX + 150;
  //   int saveInfoY = posY + 56;
  //   Debug::printf(saveInfoX, saveInfoY, "SaveGame EEPROM: %s", tstr);
  //   saveInfoY += 10;
  //   // Show last action and structured save fields stacked vertically for readability
  //   if (SaveGame::was_last_action_load()) {
  //     Debug::printf(saveInfoX, saveInfoY, "Loaded: %lu", (unsigned long)SaveGame::get_last_loaded_value());
  //   } else {
  //     Debug::printf(saveInfoX, saveInfoY, "Saved:  %lu", (unsigned long)last);
  //   }
  //   saveInfoY += 10;
  //   // Structured save fields: total level-ups, best-time, level-complete flags (one per line)
  //   uint32_t totalUps = SaveGame::get_total_level_ups();
  //   uint32_t best = SaveGame::get_best_time();
  //   uint16_t flags = SaveGame::get_level_complete_flags();
  //   Debug::printf(saveInfoX, saveInfoY, "Level Ups: %lu", (unsigned long)totalUps);
  //   saveInfoY += 10;
  //   if (best > 0) {
  //     int bm = (int)(best / 60);
  //     int bs = (int)(best % 60);
  //     Debug::printf(saveInfoX, saveInfoY, "Best Time: %02d:%02d", bm, bs);
  //   } else {
  //     Debug::printf(saveInfoX, saveInfoY, "Best Time: --:--");
  //   }
  //   saveInfoY += 10;
  //   Debug::printf(saveInfoX, saveInfoY, "Maps Done: 0x%04x", (unsigned)flags);
  // }

  //Debug::print(display_get_width() - 100, posY, "[L/R] Scene");
  posY += 12;

  int idx = 0;
  for(auto &entry : entries) {
    // if(idx == idxCustom) {
    //   Debug::print(posX, posY+8, "Scene:");
    //   posY += 12+8;
    // }

    // Draw entry
    switch(entry.type) {
      case EntryType::INT:
        if (entry.value == &sceneId) {
          // Special handling for scene selection - Updated order to match SceneManager
          const char* sceneNames[] = {"Last64", "Env", "Magic", "Pixel", "Main"};
          int sceneIdx = *(int*)entry.value;
          if (sceneIdx >= 0 && sceneIdx <= 4) {
            Debug::printf(posX + 8, posY, "%s: %d (%s)", entry.name, sceneIdx, sceneNames[sceneIdx]);
          } else {
            Debug::printf(posX + 8, posY, "%s: %d", entry.name, sceneIdx);
          }
        } else if (entry.value == &debugWeaponSelection) {
          // Special handling for weapon selection
          const char* weaponNames[] = {"Random", "Projectile", "Homing", "Circular", "Spiral", "Shield", "Shape"};
          int weaponIdx = *(int*)entry.value;
          if (weaponIdx >= 0 && weaponIdx <= 6) {
            Debug::printf(posX + 8, posY, "%s: %d (%s)", entry.name, weaponIdx, weaponNames[weaponIdx]);
          } else {
            Debug::printf(posX + 8, posY, "%s: %d", entry.name, weaponIdx);
          }
        } else {
          Debug::printf(posX + 8, posY, "%s: %d", entry.name, *(int*)entry.value);
        }
        break;
      case EntryType::FLOAT:
        Debug::printf(posX + 8, posY, "%s: %.2f", entry.name, *(float*)entry.value);
        break;
      case EntryType::BOOL:
        if (entry.value == &isForceAllPlayers) {
          Debug::printf(posX + 8, posY, "%s: %s", entry.name, isForceAllPlayers ? "ON" : "OFF");
        } else {
          Debug::printf(posX + 8, posY, *((bool*)entry.value) ? "%s: ON" : "%s: OFF", entry.name);
          break;
        }
      default:
        // NONE
        break;
    }

    if(menuSel == idx) {
      Debug::print(posX, posY, ">");
    }

    posY += 8;
    ++idx;
  }
}

bool DebugMenu::isReturnToMainMenuRequested() {
  if (returnToMainMenuChanged && returnToMainMenuVar) {
    returnToMainMenuVar = false;
    returnToMainMenuChanged = false;
    return true;
  }
  return false;
}

void DebugMenu::resetReturnToMainMenuFlag() {
  returnToMainMenuVar = false;
  returnToMainMenuChanged = false;
}

bool DebugMenu::isReturnToPauseMenuRequested() {
  // This function is no longer used since B button now closes the menu directly
  // Keeping it for compatibility but it always returns false
  return false;
}

void DebugMenu::reloadSettings() {
  // Reload music state from savegame
  musicEnabledVar = SaveGame::is_music_enabled();
  gSFXManager.setMusicEnabled(musicEnabledVar);
  
  // Reload marble background state from savegame
  marbleEnabledVar = SaveGame::is_marble_enabled();
  showMarbleBackground = marbleEnabledVar;
}
