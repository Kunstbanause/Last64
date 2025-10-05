// EEPROM save/load helpers
#include "savegame.h"
#include <libdragon.h>
#include <cstring>

namespace SaveGame {

// forward declare helper so init() can call it before the static definitions
static void load_structured_state();

void init() {
  // Load structured state on startup so main can display/save immediately
  if (!eeprom_present()) {
    debugf("SaveGame: EEPROM not present at init\n");
    return;
  }
  load_structured_state();
  // Print loaded structured values using public accessors
  uint32_t total = get_total_level_ups();
  uint32_t best = get_best_time();
  uint16_t flags = get_level_complete_flags();
  if (best == 0) {
    debugf("SaveGame: init loaded - LevelUps:%lu Best:--:-- Flags:0x%04x\n", (unsigned long)total, (unsigned)flags);
  } else {
    int bm = (int)(best / 60);
    int bs = (int)(best % 60);
    debugf("SaveGame: init loaded - LevelUps:%lu Best:%02d:%02d Flags:0x%04x\n", (unsigned long)total, bm, bs, (unsigned)flags);
  }
}

static uint32_t s_last_saved_value = 0;
static uint32_t s_last_loaded_value = 0;
static bool s_last_action_was_load = false;
// Structured state stored in block 1 (and following bytes)
// Layout (all big-endian as used before):
// offset 0 (block 1) : total_level_ups (4 bytes)
// offset 4            : best_time_seconds (4 bytes)
// offset 8            : level_complete_flags (2 bytes)

static uint32_t s_total_level_ups = 0;
static uint32_t s_best_time = 0xFFFFFFFF;
static uint16_t s_level_complete_flags = 0;
static bool s_music_enabled = true;

static void load_structured_state() {
  if (!eeprom_present()) return;
  uint8_t buf[8];
  // read block 1
  eeprom_read(1, buf);
  s_total_level_ups = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | ((uint32_t)buf[3]);
  s_best_time = ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16) | ((uint32_t)buf[6] << 8) | ((uint32_t)buf[7]);
  // read block 2 low bytes for flags
  uint8_t buf2[8];
  eeprom_read(2, buf2);
  s_level_complete_flags = ((uint16_t)buf2[0] << 8) | (uint16_t)buf2[1];
  // byte 2 in block2 stores music enabled flag (1 = enabled)
  s_music_enabled = buf2[2] != 0;
}

static void save_structured_state() {
  if (!eeprom_present()) {
    debugf("SaveGame: EEPROM not present - structured save skipped\n");
    return;
  }
  uint8_t buf[8];
  buf[0] = (s_total_level_ups >> 24) & 0xFFu;
  buf[1] = (s_total_level_ups >> 16) & 0xFFu;
  buf[2] = (s_total_level_ups >> 8) & 0xFFu;
  buf[3] = (s_total_level_ups) & 0xFFu;
  buf[4] = (s_best_time >> 24) & 0xFFu;
  buf[5] = (s_best_time >> 16) & 0xFFu;
  buf[6] = (s_best_time >> 8) & 0xFFu;
  buf[7] = (s_best_time) & 0xFFu;
  uint8_t res1 = eeprom_write(1, buf);
  uint8_t buf2[8] = { (uint8_t)((s_level_complete_flags >> 8) & 0xFFu), (uint8_t)(s_level_complete_flags & 0xFFu), (uint8_t)(s_music_enabled ? 1 : 0),0,0,0,0,0 };
  uint8_t res2 = eeprom_write(2, buf2);
  if (res1 == 0 && res2 == 0) {
    if (s_best_time == 0xFFFFFFFF) {
      debugf("SaveGame: structured write OK - LevelUps:%lu Best:--:-- Flags:0x%04x\n", (unsigned long)s_total_level_ups, (unsigned)s_level_complete_flags);
    } else {
      int bm = (int)(s_best_time / 60);
      int bs = (int)(s_best_time % 60);
      debugf("SaveGame: structured write OK - LevelUps:%lu Best:%02d:%02d Flags:0x%04x\n", (unsigned long)s_total_level_ups, bm, bs, (unsigned)s_level_complete_flags);
    }
  } else {
    debugf("SaveGame: structured write FAILED res1=%u res2=%u\n", (unsigned)res1, (unsigned)res2);
  }
}

bool is_present() {
  return eeprom_present() != 0;
}

int get_type() {
  return (int)eeprom_present();
}

uint32_t get_last_saved_value() {
  return s_last_saved_value;
}

uint32_t get_last_loaded_value() {
  return s_last_loaded_value;
}

bool was_last_action_load() {
  return s_last_action_was_load;
}

bool save_test_value(uint32_t value) {
    if (!eeprom_present()) {
        debugf("SaveGame: EEPROM not present\n");
        return false;
    }
    uint8_t buffer[8];
    // pack value into first 4 bytes, remaining bytes zeroed
    buffer[0] = (value >> 24) & 0xFFu;
    buffer[1] = (value >> 16) & 0xFFu;
    buffer[2] = (value >> 8) & 0xFFu;
    buffer[3] = value & 0xFFu;
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;

    // write to block 0 (8 bytes)
  uint8_t res = eeprom_write(0, buffer);
  if (res != 0) {
    debugf("SaveGame: eeprom_write returned %u\n", (unsigned)res);
    return false;
  }
  s_last_saved_value = value;
  s_last_action_was_load = false;
  debugf("SaveGame: write OK (value=%lu)\n", (unsigned long)value);
  return true;
}

bool load_test_value(uint32_t &out_value) {
    if (!eeprom_present()) return false;
    uint8_t buffer[8];
  eeprom_read(0, buffer);
  debugf("SaveGame: read raw bytes: %02x %02x %02x %02x %02x %02x %02x %02x\n",
         buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
    out_value = (uint32_t)buffer[0] << 24 | (uint32_t)buffer[1] << 16 | (uint32_t)buffer[2] << 8 | (uint32_t)buffer[3];
  s_last_loaded_value = out_value;
  s_last_action_was_load = true;
  // refresh structured state when loading
  load_structured_state();
  return true;
}

// Game-state helpers
void accum_level_up() {
  s_total_level_ups++;
  save_structured_state();
}

void maybe_update_best_time(uint32_t seconds) {
  // Treat best time as a high-score (longer survival is better).
  // Update when no best exists, or when the current run survived longer.
  if (s_best_time == 0xFFFFFFFF || seconds > s_best_time) {
    s_best_time = seconds;
    save_structured_state();
  }
}

void set_level_complete(int levelIndex) {
  if (levelIndex < 0 || levelIndex >= 16) return;
  s_level_complete_flags |= (1u << levelIndex);
  save_structured_state();
}

uint32_t get_total_level_ups() { return s_total_level_ups; }
uint32_t get_best_time() { return s_best_time == 0xFFFFFFFF ? 0 : s_best_time; }
uint16_t get_level_complete_flags() { return s_level_complete_flags; }

void set_music_enabled(bool enabled) { s_music_enabled = enabled; save_structured_state(); }
bool is_music_enabled() { return s_music_enabled; }

void purge_save() {
  if (!eeprom_present()) return;
  // Zero out blocks 0..2 used by our save format
  uint8_t zeros[8] = {0};
  eeprom_write(0, zeros);
  eeprom_write(1, zeros);
  eeprom_write(2, zeros);
  // Reset in-memory state
  s_total_level_ups = 0;
  s_best_time = 0xFFFFFFFF;
  s_level_complete_flags = 0;
  s_music_enabled = true;
  debugf("SaveGame: purge complete\n");
}

}
