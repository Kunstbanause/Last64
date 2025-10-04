// EEPROM save/load helpers
#include "savegame.h"
#include <libdragon.h>
#include <cstring>

namespace SaveGame {

void init() {
  // nothing to init for libdragon eeprom
}

static uint32_t s_last_saved_value = 0;
static uint32_t s_last_loaded_value = 0;
static bool s_last_action_was_load = false;

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
  return true;
}

}
