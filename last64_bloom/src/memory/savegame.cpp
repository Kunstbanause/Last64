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
// offset 8 (block 2) : level_complete_flags (2 bytes)
// offset 10           : music_enabled (1 byte)
// offset 11           : marble_enabled (1 byte)
// offset 12 (block 3) : credits_spent (4 bytes)
// offset 16           : pickup_range_level (1 byte)

static uint32_t s_total_level_ups = 0;
static uint32_t s_best_time = 0xFFFFFFFF;
static uint16_t s_level_complete_flags = 0;
static bool s_music_enabled = true;
static bool s_marble_enabled = true;
static bool s_profiling_enabled = false;
static uint32_t s_credits_spent = 0;
static uint8_t s_pickup_range_level = 0;
static uint8_t s_damage_level = 0;
static uint8_t s_projectile_count_level = 0;
static bool s_shield_weapon_unlocked = false;
static bool s_shape_weapon_unlocked = false;
static uint8_t s_movespeed_level = 0;
static uint8_t s_enemy_spawn_rate_level = 0;
static uint8_t s_projectile_speed_level = 0;
static uint8_t s_xp_multiplier_level = 0;

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
  // byte 3 in block2 stores marble background enabled flag (1 = enabled)
  s_marble_enabled = buf2[3] != 0;
  // byte 4 low bit stores profiling enabled flag (1 = enabled) - we'll use only the low byte
  // Note: bytes 4-7 in block2 now store: profiling (1 byte) + credits_spent (3 bytes)
  s_profiling_enabled = (buf2[4] & 0x01) != 0;
  // bytes 5-7 in block2 store credits_spent (changed from 4-7)
  s_credits_spent = ((uint32_t)buf2[5] << 16) | ((uint32_t)buf2[6] << 8) | ((uint32_t)buf2[7]);
  // read block 3 for pickup_range_level, damage_level, projectile_count_level, and weapon unlocks
  uint8_t buf3[8];
  eeprom_read(3, buf3);
  s_pickup_range_level = buf3[0];
  s_damage_level = buf3[1];
  s_projectile_count_level = buf3[2];
  s_shield_weapon_unlocked = buf3[3] != 0;
  s_shape_weapon_unlocked = buf3[4] != 0;
  s_movespeed_level = buf3[5];
  s_enemy_spawn_rate_level = buf3[6];
  s_projectile_speed_level = buf3[7];
  // read block 4 for xp_multiplier_level
  uint8_t buf4[8];
  eeprom_read(4, buf4);
  s_xp_multiplier_level = buf4[0];
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
  uint8_t buf2[8] = { 
    (uint8_t)((s_level_complete_flags >> 8) & 0xFFu), 
    (uint8_t)(s_level_complete_flags & 0xFFu), 
    (uint8_t)(s_music_enabled ? 1 : 0), 
    (uint8_t)(s_marble_enabled ? 1 : 0),
    (uint8_t)(s_profiling_enabled ? 1 : 0),
    (uint8_t)((s_credits_spent >> 16) & 0xFFu),
    (uint8_t)((s_credits_spent >> 8) & 0xFFu),
    (uint8_t)(s_credits_spent & 0xFFu)
  };
  uint8_t res2 = eeprom_write(2, buf2);
  uint8_t buf3[8] = { 
    s_pickup_range_level, 
    s_damage_level, 
    s_projectile_count_level, 
    (uint8_t)(s_shield_weapon_unlocked ? 1 : 0),
    (uint8_t)(s_shape_weapon_unlocked ? 1 : 0),
    s_movespeed_level,
    s_enemy_spawn_rate_level,
    s_projectile_speed_level
  };
  uint8_t res3 = eeprom_write(3, buf3);
  uint8_t buf4[8] = { 
    s_xp_multiplier_level, 
    0, 0, 0, 0, 0, 0, 0 
  };
  uint8_t res4 = eeprom_write(4, buf4);
  if (res1 == 0 && res2 == 0 && res3 == 0 && res4 == 0) {
    if (s_best_time == 0xFFFFFFFF) {
      debugf("SaveGame: structured write OK - LevelUps:%lu Best:--:-- Flags:0x%04x\n", (unsigned long)s_total_level_ups, (unsigned)s_level_complete_flags);
    } else {
      int bm = (int)(s_best_time / 60);
      int bs = (int)(s_best_time % 60);
      debugf("SaveGame: structured write OK - LevelUps:%lu Best:%02d:%02d Flags:0x%04x\n", (unsigned long)s_total_level_ups, bm, bs, (unsigned)s_level_complete_flags);
    }
  } else {
      debugf("SaveGame: structured write FAILED res1=%u res2=%u res3=%u res4=%u\n", (unsigned)res1, (unsigned)res2, (unsigned)res3, (unsigned)res4);
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

void set_marble_enabled(bool enabled) { s_marble_enabled = enabled; save_structured_state(); }
bool is_marble_enabled() { return s_marble_enabled; }

void set_profiling_enabled(bool enabled) { s_profiling_enabled = enabled; save_structured_state(); }
bool is_profiling_enabled() { return s_profiling_enabled; }

// Credits system implementation
uint32_t get_total_credits() {
  return s_total_level_ups * 10;
}

uint32_t get_credits_spent() {
  return s_credits_spent;
}

uint32_t get_credits_available() {
  uint32_t total = get_total_credits();
  if (s_credits_spent > total) return 0; // Safety check
  return total - s_credits_spent;
}

void spend_credits(uint32_t amount) {
  s_credits_spent += amount;
  save_structured_state();
}

void reset_credits_spent() {
  s_credits_spent = 0;
  save_structured_state();
  debugf("SaveGame: credits reset\n");
}

// Permanent upgrades
uint8_t get_pickup_range_level() {
  return s_pickup_range_level;
}

void set_pickup_range_level(uint8_t level) {
  s_pickup_range_level = level;
  save_structured_state();
}

float get_pickup_range_multiplier() {
  return 1.0f + (s_pickup_range_level * 0.1f);
}

// Damage upgrade implementation
uint8_t get_damage_level() {
  return s_damage_level;
}

void set_damage_level(uint8_t level) {
  s_damage_level = level;
  save_structured_state();
}

float get_damage_multiplier() {
  return 1.0f + (s_damage_level * 0.05f); // 5% per level
}

// Projectile count upgrade implementation
uint8_t get_projectile_count_level() {
  return s_projectile_count_level;
}

void set_projectile_count_level(uint8_t level) {
  s_projectile_count_level = level;
  save_structured_state();
}

int get_projectile_count_bonus() {
  return (int)s_projectile_count_level; // Flat bonus
}

// Weapon unlock implementation
bool is_shield_weapon_unlocked() {
  return s_shield_weapon_unlocked;
}

void set_shield_weapon_unlocked(bool unlocked) {
  s_shield_weapon_unlocked = unlocked;
  save_structured_state();
}

bool is_shape_weapon_unlocked() {
  return s_shape_weapon_unlocked;
}

void set_shape_weapon_unlocked(bool unlocked) {
  s_shape_weapon_unlocked = unlocked;
  save_structured_state();
}

void purge_save() {
  if (!eeprom_present()) return;
  // Reset in-memory state first
  s_total_level_ups = 0;
  s_best_time = 0xFFFFFFFF;
  s_level_complete_flags = 0;
  s_music_enabled = true;
  s_marble_enabled = true;
  s_credits_spent = 0;
  s_pickup_range_level = 0;
  s_damage_level = 0;
  s_projectile_count_level = 0;
  s_shield_weapon_unlocked = false;
  s_shape_weapon_unlocked = false;
  s_movespeed_level = 0;
  s_enemy_spawn_rate_level = 0;
  s_projectile_speed_level = 0;
  s_xp_multiplier_level = 0;
  // Write the default state to EEPROM using save_structured_state
  // This ensures the default values (music=true, marble=true) are properly written
  save_structured_state();
  // Also clear block 0 (test value)
  uint8_t zeros[8] = {0};
  eeprom_write(0, zeros);
  debugf("SaveGame: purge complete\n");
}

// Passive upgrade: Player movement speed
uint8_t get_movespeed_level() {
  return s_movespeed_level;
}

void set_movespeed_level(uint8_t level) {
  s_movespeed_level = level;
  save_structured_state();
}

float get_movespeed_multiplier() {
  return 1.0f + (s_movespeed_level * 0.05f); // 5% per level
}

// Passive upgrade: Enemy spawn rate
uint8_t get_enemy_spawn_rate_level() {
  return s_enemy_spawn_rate_level;
}

void set_enemy_spawn_rate_level(uint8_t level) {
  s_enemy_spawn_rate_level = level;
  save_structured_state();
}

float get_enemy_spawn_rate_multiplier() {
  return 1.0f + (s_enemy_spawn_rate_level * 0.1f); // 10% per level
}

// Passive upgrade: Projectile speed
uint8_t get_projectile_speed_level() {
  return s_projectile_speed_level;
}

void set_projectile_speed_level(uint8_t level) {
  s_projectile_speed_level = level;
  save_structured_state();
}

float get_projectile_speed_multiplier() {
  return 1.0f + (s_projectile_speed_level * 0.05f); // 5% per level
}

// Passive upgrade: XP collection multiplier
uint8_t get_xp_multiplier_level() {
  return s_xp_multiplier_level;
}

void set_xp_multiplier_level(uint8_t level) {
  s_xp_multiplier_level = level;
  save_structured_state();
}

float get_xp_multiplier() {
  return 1.0f + (s_xp_multiplier_level * 0.1f); // 10% per level
}

}
