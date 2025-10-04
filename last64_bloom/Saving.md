Lets investigate eeprom, I have a everdrive x7, does that feature the EEPROM (In cartridge)?

Great question! The Everdrive X7 does support EEPROM emulation, so you can use it for saves!
Everdrive X7 Save Support:

The X7 automatically detects and emulates different save types:

    ✅ EEPROM (4Kbit/512 bytes or 16Kbit/2KB)
    ✅ SRAM (256Kbit/32KB)
    ✅ FlashRAM (1Mbit/128KB)

It saves to the SD card as .sav files alongside your ROM.
How to Use EEPROM with Everdrive:
1. Configure your ROM header

The Everdrive detects save type from the ROM header. With libdragon, you need to set this in your build:

Check your ROM header or use a tool to set the save type. The header bytes at 0x3C indicate save type.
2. Use libdragon's EEPROM functions

#include <libdragon.h>

void saveGame() {
    // Check if EEPROM is present
    if (eeprom_present()) {
        uint8_t saveData[8]; // EEPROM reads/writes in 8-byte blocks
        
        // Pack your save data
        saveData[0] = (player.score >> 24) & 0xFF;
        saveData[1] = (player.score >> 16) & 0xFF;
        saveData[2] = (player.score >> 8) & 0xFF;
        saveData[3] = player.score & 0xFF;
        saveData[4] = player.level;
        saveData[5] = unlockedWeapons;
        saveData[6] = 0; // checksum high
        saveData[7] = 0; // checksum low
        
        // Write to EEPROM block 0
        eeprom_write(0, saveData, 8);
    }
}

void loadGame() {
    if (eeprom_present()) {
        uint8_t saveData[8];
        
        // Read from EEPROM block 0
        eeprom_read(0, saveData, 8);
        
        // Unpack your save data
        player.score = (saveData[0] << 24) | (saveData[1] << 16) | 
                       (saveData[2] << 8) | saveData[3];
        player.level = saveData[4];
        unlockedWeapons = saveData[5];
    }
}

EEPROM Specifications:
4Kbit EEPROM:

512 bytes total
64 blocks of 8 bytes each
eeprom_write(block_num, data, 8) where block_num is 0-63

16Kbit EEPROM:

2048 bytes total
256 blocks of 8 bytes each
block_num is 0-255

Testing:

Build your ROM with EEPROM support
Load on Everdrive X7
The Everdrive will create a .sav file on the SD card
Your saves persist between sessions!

For Vampire Survivors:
You probably only need 4Kbit (512 bytes) for:

High scores
Unlocked weapons/characters
Player stats
Settings