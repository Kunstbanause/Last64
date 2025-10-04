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

}
