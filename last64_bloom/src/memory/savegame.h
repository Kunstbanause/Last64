// Simple EEPROM save/load helpers for Last64_Bloom
#pragma once

#include <cstdint>

namespace SaveGame {

// Initialize save system (no-op for now)
void init();

// Save a 32-bit test value to EEPROM block 0
bool save_test_value(uint32_t value);

// Load a 32-bit test value from EEPROM block 0. Returns true if read succeeded.
bool load_test_value(uint32_t &out_value);

// Diagnostics
bool is_present();
int get_type(); // 0 none, 1 = 4k, 2 = 16k
uint32_t get_last_saved_value();
uint32_t get_last_loaded_value();
bool was_last_action_load();

// Game-state saving helpers
void accum_level_up();
void maybe_update_best_time(uint32_t seconds);
void set_level_complete(int levelIndex);

// Music setting
void set_music_enabled(bool enabled);
bool is_music_enabled();

// Marble background setting
void set_marble_enabled(bool enabled);
bool is_marble_enabled();

// Volume settings (0-10, default 10 = 100%)
void set_music_volume(uint8_t volume);
uint8_t get_music_volume();
void set_sfx_volume(uint8_t volume);
uint8_t get_sfx_volume();

// Profiling display setting
void set_profiling_enabled(bool enabled);
bool is_profiling_enabled();

// Purge all save data (for testing)
void purge_save();

// Accessors
uint32_t get_total_level_ups();
uint32_t get_best_time();
uint16_t get_level_complete_flags();

// Credits system
uint32_t get_total_credits();      // Total credits earned (level_ups * 10)
uint32_t get_credits_spent();      // Credits spent on upgrades
uint32_t get_credits_available();  // Available to spend (total - spent)
void spend_credits(uint32_t amount);
void reset_credits_spent();        // Reset spent credits (refund all)

// Permanent upgrades
uint8_t get_pickup_range_level();  // Pickup range upgrade level
void set_pickup_range_level(uint8_t level);
float get_pickup_range_multiplier(); // Returns 1.0 + (level * 0.1)

uint8_t get_damage_level();        // Damage upgrade level
void set_damage_level(uint8_t level);
float get_damage_multiplier();     // Returns 1.0 + (level * 0.05) - 5% per level

uint8_t get_projectile_count_level(); // Projectile count upgrade level
void set_projectile_count_level(uint8_t level);
int get_projectile_count_bonus();  // Returns level (flat bonus)

// Weapon unlocks
bool is_shield_weapon_unlocked();   // Defense/Shield weapon unlock state
void set_shield_weapon_unlocked(bool unlocked);

bool is_shape_weapon_unlocked();    // Shape/Whip weapon unlock state
void set_shape_weapon_unlocked(bool unlocked);

// Passive upgrades (stat bonuses, not weapon-specific)
uint8_t get_movespeed_level();      // Player movement speed upgrade level
void set_movespeed_level(uint8_t level);
float get_movespeed_multiplier();   // Returns 1.0 + (level * 0.05) - 5% per level

uint8_t get_enemy_spawn_rate_level(); // Enemy spawn rate upgrade level
void set_enemy_spawn_rate_level(uint8_t level);
float get_enemy_spawn_rate_multiplier(); // Returns 1.0 + (level * 0.1) - 10% per level

uint8_t get_projectile_speed_level(); // Projectile speed upgrade level
void set_projectile_speed_level(uint8_t level);
float get_projectile_speed_multiplier(); // Returns 1.0 + (level * 0.05) - 5% per level

uint8_t get_xp_multiplier_level();  // XP collection multiplier upgrade level
void set_xp_multiplier_level(uint8_t level);
float get_xp_multiplier();          // Returns 1.0 + (level * 0.1) - 10% per level

}
