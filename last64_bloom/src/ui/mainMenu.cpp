/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "mainMenu.h"
#include "../main.h"
#include "../memory/savegame.h"
#include "../audio.h"
#include "../debugMenu.h"
#include <libdragon.h>
#include <cstdio>
#include <algorithm>

// External flags from scenes
extern bool showMarbleBackground;
extern bool marbleBackgroundChanged;

namespace MainMenu {
    // Menu state
    static MenuState currentState = MAIN_MENU;
    static int currentSelection = 0;
    static int upgradeSelection = 0;  // Selection in upgrades menu
    static int settingsSelection = 0; // Selection in settings menu
    static bool shouldStart = false;
    static rdpq_font_t* font = nullptr;
    static bool initialized = false;
    static sprite_t* titleSprite = nullptr;
    static int selectedLevel = 0;
    
    // Purge confirmation
    static bool showPurgeConfirm = false;

    static constexpr int kMaxLevels = 3;
    static const char* kLevelNames[kMaxLevels] = {
        "Crimson Bloom",
        "Verdant Bloom",
        "Rose Bloom"
    };

    static int highestUnlockedLevel()
    {
        // Sequential unlock: level 0 always unlocked, level 1 unlocks after beating level 0, etc.
        uint16_t flags = SaveGame::get_level_complete_flags();
        bool unlocked[kMaxLevels] = {true, false, false};
        unlocked[1] = (flags & (1u << 0)) != 0u;  // Beat level 0 to unlock level 1
        unlocked[2] = (flags & (1u << 1)) != 0u;  // Beat level 1 to unlock level 2

        int highest = 0;
        for (int i = 0; i < kMaxLevels; ++i) {
            if (unlocked[i]) {
                highest = i;
            } else {
                break; // stop at first locked because unlocks are sequential
            }
        }
        return highest;
    }

    void initialize() {
        // Only do full initialization once
        if (!initialized) {
            font = rdpq_font_load("rom:/fibberish.font64");
            
            // Set up font styles
            rdpq_fontstyle_t style0 = {.color = (color_t){0xFF, 0xFF, 0xFF, 0xFF}};  // White
            rdpq_font_style(font, 0, &style0);
            
            rdpq_fontstyle_t style1 = {.color = (color_t){232, 101, 65, 0xFF}};      // Orange
            rdpq_font_style(font, 1, &style1);
            
            rdpq_fontstyle_t style2 = {.color = (color_t){79, 209, 133, 0xFF}};      // Green
            rdpq_font_style(font, 2, &style2);
            
            rdpq_fontstyle_t style3 = {.color = (color_t){216, 220, 180, 0xFF}};     // Tan
            rdpq_font_style(font, 3, &style3);
            
            rdpq_text_register_font(FONT_MENU, font);
            
            // Load title sprite
            titleSprite = sprite_load("rom://small.sprite");
            
            initialized = true;
        }
        reset();
    }

    void cleanup() {
        // Don't free the font or unregister it - let it persist across scene reloads
        // Free title sprite if loaded
        if (titleSprite) {
            sprite_free(titleSprite);
            titleSprite = nullptr;
        }
        // Just reset the menu state
    }

    void reset() {
        currentState = MAIN_MENU;
        currentSelection = 0;
        upgradeSelection = 0;
        settingsSelection = 0;
        shouldStart = false;
        showPurgeConfirm = false;
        selectedLevel = std::min(selectedLevel, highestUnlockedLevel());
    }

    void update(float deltaTime) {
        // Merge input from all controllers (same pattern as main.cpp)
        joypad_buttons_t pressed = {0};
        joypad_inputs_t stick = {0};
        
        for (int i = JOYPAD_PORT_1; i <= JOYPAD_PORT_4; i++) {
            joypad_buttons_t b = joypad_get_buttons_pressed((joypad_port_t)i);
            pressed.a      |= b.a;
            pressed.b      |= b.b;
            pressed.z      |= b.z;
            pressed.start  |= b.start;
            pressed.d_up   |= b.d_up;
            pressed.d_down |= b.d_down;
            pressed.d_left |= b.d_left;
            pressed.d_right|= b.d_right;
            
            // Get analog stick input from any controller
            joypad_inputs_t inputs = joypad_get_inputs((joypad_port_t)i);
            if (fabsf(inputs.stick_y) > fabsf(stick.stick_y)) {
                stick.stick_y = inputs.stick_y;
            }
            if (fabsf(inputs.stick_x) > fabsf(stick.stick_x)) {
                stick.stick_x = inputs.stick_x;
            }
        }
        
        // Debounce analog stick navigation
        static float stickDebounce = 0.0f;
        if (stickDebounce > 0.0f) {
            stickDebounce -= deltaTime;
        }
        
        bool stickUp = false;
        bool stickDown = false;
        bool stickLeft = false;
        bool stickRight = false;
        
        if (stickDebounce <= 0.0f) {
            const float threshold = 0.5f;
            if (stick.stick_y > threshold) {
                stickUp = true;
                stickDebounce = 0.2f; // 200ms debounce
            } else if (stick.stick_y < -threshold) {
                stickDown = true;
                stickDebounce = 0.2f;
            }
            if (stick.stick_x < -threshold) {
                stickLeft = true;
                stickDebounce = 0.2f;
            } else if (stick.stick_x > threshold) {
                stickRight = true;
                stickDebounce = 0.2f;
            }
        }

        switch (currentState) {
            case MAIN_MENU: {
                int maxUnlocked = highestUnlockedLevel();
                if (selectedLevel > maxUnlocked) selectedLevel = maxUnlocked;

                if (pressed.d_up || stickUp) {
                    currentSelection = (currentSelection - 1 + 4) % 4;
                }
                if (pressed.d_down || stickDown) {
                    currentSelection = (currentSelection + 1) % 4;
                }
                if (currentSelection == 0) {
                    if (pressed.d_left || stickLeft) {
                        selectedLevel = (selectedLevel - 1 + (maxUnlocked + 1)) % (maxUnlocked + 1);
                    }
                    if (pressed.d_right || stickRight) {
                        selectedLevel = (selectedLevel + 1) % (maxUnlocked + 1);
                    }
                }
                if (pressed.a || pressed.z) {
                    if (currentSelection == 0) {
                        shouldStart = true;
                    } else if (currentSelection == 1) {
                        currentState = UPGRADES_MENU;
                    } else if (currentSelection == 2) {
                        currentState = SETTINGS_MENU;
                    } else if (currentSelection == 3) {
                        currentState = STATS_MENU;
                    }
                }
                break;
            }
            case UPGRADES_MENU: {
                if (pressed.d_up || stickUp) {
                    upgradeSelection = (upgradeSelection - 1 + UPGRADE_COUNT) % UPGRADE_COUNT;
                }
                if (pressed.d_down || stickDown) {
                    upgradeSelection = (upgradeSelection + 1) % UPGRADE_COUNT;
                }
                if (pressed.a || pressed.z) {
                    if (upgradeSelection == UPGRADE_PICKUP_RANGE) {
                        // Purchase pickup range upgrade (10 credits per level)
                        uint32_t available = SaveGame::get_credits_available();
                        uint8_t currentLevel = SaveGame::get_pickup_range_level();
                        const uint32_t upgradeCost = 10;
                        
                        if (available >= upgradeCost && currentLevel < 10) { // Max 10 levels
                            SaveGame::spend_credits(upgradeCost);
                            SaveGame::set_pickup_range_level(currentLevel + 1);
                            debugf("MainMenu: Purchased pickup range upgrade (level %d)\n", currentLevel + 1);
                        }
                    } else if (upgradeSelection == UPGRADE_DAMAGE) {
                        // Purchase damage upgrade (20 credits per level, expensive!)
                        uint32_t available = SaveGame::get_credits_available();
                        uint8_t currentLevel = SaveGame::get_damage_level();
                        const uint32_t upgradeCost = 20;
                        
                        if (available >= upgradeCost && currentLevel < 20) { // Max 20 levels for +100% damage
                            SaveGame::spend_credits(upgradeCost);
                            SaveGame::set_damage_level(currentLevel + 1);
                            debugf("MainMenu: Purchased damage upgrade (level %d)\n", currentLevel + 1);
                        }
                    } else if (upgradeSelection == UPGRADE_PROJECTILE_COUNT) {
                        // Purchase projectile count upgrade (50 credits per level, very expensive!)
                        uint32_t available = SaveGame::get_credits_available();
                        uint8_t currentLevel = SaveGame::get_projectile_count_level();
                        const uint32_t upgradeCost = 50;
                        
                        if (available >= upgradeCost && currentLevel < 5) { // Max 5 levels
                            SaveGame::spend_credits(upgradeCost);
                            SaveGame::set_projectile_count_level(currentLevel + 1);
                            debugf("MainMenu: Purchased projectile count upgrade (level %d)\n", currentLevel + 1);
                        }
                       } else if (upgradeSelection == UPGRADE_MOVESPEED) {
                           // Purchase movespeed upgrade (15 credits per level)
                           uint32_t available = SaveGame::get_credits_available();
                           uint8_t currentLevel = SaveGame::get_movespeed_level();
                           const uint32_t upgradeCost = 15;
                       
                           if (available >= upgradeCost && currentLevel < 10) { // Max 10 levels
                               SaveGame::spend_credits(upgradeCost);
                               SaveGame::set_movespeed_level(currentLevel + 1);
                               debugf("MainMenu: Purchased movespeed upgrade (level %d)\n", currentLevel + 1);
                           }
                       } else if (upgradeSelection == UPGRADE_ENEMY_SPAWN_RATE) {
                           // Purchase enemy spawn rate upgrade (25 credits per level)
                           uint32_t available = SaveGame::get_credits_available();
                           uint8_t currentLevel = SaveGame::get_enemy_spawn_rate_level();
                           const uint32_t upgradeCost = 25;
                       
                           if (available >= upgradeCost && currentLevel < 8) { // Max 8 levels
                               SaveGame::spend_credits(upgradeCost);
                               SaveGame::set_enemy_spawn_rate_level(currentLevel + 1);
                               debugf("MainMenu: Purchased enemy spawn rate upgrade (level %d)\n", currentLevel + 1);
                           }
                       } else if (upgradeSelection == UPGRADE_PROJECTILE_SPEED) {
                           // Purchase projectile speed upgrade (20 credits per level)
                           uint32_t available = SaveGame::get_credits_available();
                           uint8_t currentLevel = SaveGame::get_projectile_speed_level();
                           const uint32_t upgradeCost = 20;
                       
                           if (available >= upgradeCost && currentLevel < 10) { // Max 10 levels
                               SaveGame::spend_credits(upgradeCost);
                               SaveGame::set_projectile_speed_level(currentLevel + 1);
                               debugf("MainMenu: Purchased projectile speed upgrade (level %d)\n", currentLevel + 1);
                           }
                       } else if (upgradeSelection == UPGRADE_XP_MULTIPLIER) {
                           // Purchase XP multiplier upgrade (30 credits per level)
                           uint32_t available = SaveGame::get_credits_available();
                           uint8_t currentLevel = SaveGame::get_xp_multiplier_level();
                           const uint32_t upgradeCost = 30;
                       
                           if (available >= upgradeCost && currentLevel < 8) { // Max 8 levels
                               SaveGame::spend_credits(upgradeCost);
                               SaveGame::set_xp_multiplier_level(currentLevel + 1);
                               debugf("MainMenu: Purchased XP multiplier upgrade (level %d)\n", currentLevel + 1);
                           }
                    } else if (upgradeSelection == UPGRADE_SHIELD_WEAPON) {
                        // Unlock Shield weapon (100 credits, one-time purchase)
                        uint32_t available = SaveGame::get_credits_available();
                        bool isUnlocked = SaveGame::is_shield_weapon_unlocked();
                        const uint32_t unlockCost = 100;
                        
                        if (!isUnlocked && available >= unlockCost) {
                            SaveGame::spend_credits(unlockCost);
                            SaveGame::set_shield_weapon_unlocked(true);
                            debugf("MainMenu: Unlocked Shield weapon\n");
                        }
                    } else if (upgradeSelection == UPGRADE_SHAPE_WEAPON) {
                        // Unlock Shape weapon (150 credits, one-time purchase)
                        uint32_t available = SaveGame::get_credits_available();
                        bool isUnlocked = SaveGame::is_shape_weapon_unlocked();
                        const uint32_t unlockCost = 150;
                        
                        if (!isUnlocked && available >= unlockCost) {
                            SaveGame::spend_credits(unlockCost);
                            SaveGame::set_shape_weapon_unlocked(true);
                            debugf("MainMenu: Unlocked Shape weapon\n");
                        }
                    } else if (upgradeSelection == UPGRADE_RESET_CREDITS) {
                        // Reset all spent credits (refund)
                        SaveGame::reset_credits_spent();
                        SaveGame::set_pickup_range_level(0); // Reset upgrades too
                        SaveGame::set_damage_level(0);
                        SaveGame::set_projectile_count_level(0);
                           SaveGame::set_movespeed_level(0);
                           SaveGame::set_enemy_spawn_rate_level(0);
                           SaveGame::set_projectile_speed_level(0);
                           SaveGame::set_xp_multiplier_level(0);
                        debugf("MainMenu: Reset all upgrades\n");
                    }
                }
                if (pressed.b) {
                    currentState = MAIN_MENU;
                    currentSelection = 1;
                }
                break;
            }
            case SETTINGS_MENU: {
                // Settings menu has 3 items: Music Volume, SFX Volume, Marble Background
                if (pressed.d_up || stickUp) {
                    settingsSelection = (settingsSelection - 1 + 3) % 3;
                }
                if (pressed.d_down || stickDown) {
                    settingsSelection = (settingsSelection + 1) % 3;
                }
                
                // Adjust values with left/right
                if (settingsSelection == 0) { // Music Volume
                    uint8_t vol = SaveGame::get_music_volume();
                    if (pressed.d_left || stickLeft) {
                        if (vol > 0) {
                            SaveGame::set_music_volume(vol - 1);
                            // Apply immediately
                            gSFXManager.setVolume_Music((vol - 1) / 10.0f, 0.0f);
                        }
                    }
                    if (pressed.d_right || stickRight) {
                        if (vol < 10) {
                            SaveGame::set_music_volume(vol + 1);
                            // Apply immediately
                            gSFXManager.setVolume_Music((vol + 1) / 10.0f, 0.0f);
                        }
                    }
                } else if (settingsSelection == 1) { // SFX Volume
                    uint8_t vol = SaveGame::get_sfx_volume();
                    if (pressed.d_left || stickLeft) {
                        if (vol > 0) {
                            SaveGame::set_sfx_volume(vol - 1);
                            // Play test sound
                            gSFXManager.play(SFXManager::SFX_HIT);
                        }
                    }
                    if (pressed.d_right || stickRight) {
                        if (vol < 10) {
                            SaveGame::set_sfx_volume(vol + 1);
                            // Play test sound
                            gSFXManager.play(SFXManager::SFX_HIT);
                        }
                    }
                } else if (settingsSelection == 2) { // Marble Background
                    if (pressed.d_left || stickLeft || pressed.d_right || stickRight || pressed.a || pressed.z) {
                        bool current = SaveGame::is_marble_enabled();
                        SaveGame::set_marble_enabled(!current);
                        showMarbleBackground = !current;
                        marbleBackgroundChanged = true;
                    }
                }
                
                if (pressed.b) {
                    currentState = MAIN_MENU;
                    currentSelection = 2;  // Return to settings in main menu
                }
                break;
            }
            case STATS_MENU: {
                if ((pressed.d_down || stickDown) && !showPurgeConfirm) {
                    // Move to purge option
                    showPurgeConfirm = true;
                }
                if ((pressed.d_up || stickUp) && showPurgeConfirm) {
                    showPurgeConfirm = false;
                }
                if ((pressed.a || pressed.z) && showPurgeConfirm) {
                    // Purge save game
                    SaveGame::purge_save();
                    
                    // Reload settings from purged save (all defaults)
                    bool musicEnabled = SaveGame::is_music_enabled();
                    gSFXManager.setMusicEnabled(musicEnabled);
                    
                    // Reload marble background setting
                    showMarbleBackground = SaveGame::is_marble_enabled();
                    marbleBackgroundChanged = false;
                    
                    // Reload debug menu settings to sync toggles
                    DebugMenu::reloadSettings();
                    
                    showPurgeConfirm = false;
                }
                if (pressed.b) {
                    currentState = MAIN_MENU;
                    currentSelection = 3;  // Return to stats position
                    showPurgeConfirm = false;
                }
                break;
            }
        }
    }

    void draw() {
        // Draw CREDITS in top right corner for all menu states
        uint32_t creditsAvailable = SaveGame::get_credits_available();
        char creditsBuffer[32];
        snprintf(creditsBuffer, sizeof(creditsBuffer), "CREDITS: %lu", (unsigned long)creditsAvailable);
        rdpq_text_printf(nullptr, FONT_MENU, SCREEN_WIDTH - 120, 10, creditsBuffer);
        
        // Render title sprite if loaded (only in main menu)
        if (currentState == MAIN_MENU && titleSprite) {
            // Set up RDP for sprite rendering with transparency
            rdpq_sync_pipe();
            rdpq_sync_tile();
            rdpq_sync_load();
            
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER_TEX);
            rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            rdpq_mode_alphacompare(1);  // Enable alpha threshold
            rdpq_mode_antialias(AA_NONE);
            
            // Center the sprite
            int x = (SCREEN_WIDTH - titleSprite->width) / 2;
            int y = 0;  // Top of screen with some padding
            
            rdpq_sprite_blit(titleSprite, x, y, NULL);
        }

        switch (currentState) {
            case MAIN_MENU: {
                int maxUnlocked = highestUnlockedLevel();
                rdpq_text_printf(nullptr, FONT_MENU, 100, 120, "Start (Level%d/%d)", selectedLevel + 1, kMaxLevels);
                rdpq_text_printf(nullptr, FONT_MENU, 120, 140, "%s", kLevelNames[selectedLevel]);

                rdpq_text_printf(nullptr, FONT_MENU, 100, 180, "Upgrades");
                rdpq_text_printf(nullptr, FONT_MENU, 100, 200, "Settings");
                rdpq_text_printf(nullptr, FONT_MENU, 100, 220, "Stats");

                // Draw selection indicator
                int yPos = 120;
                if (currentSelection == 1) yPos = 180;
                else if (currentSelection == 2) yPos = 200;
                else if (currentSelection == 3) yPos = 220;
                rdpq_text_printf(nullptr, FONT_MENU, 90, yPos, ">");
                break;
            }
            case UPGRADES_MENU: {
                rdpq_text_printf(nullptr, FONT_MENU, 50, 12, "UPGRADES");
                
                uint32_t available = SaveGame::get_credits_available();
                uint8_t pickupLevel = SaveGame::get_pickup_range_level();
                uint8_t damageLevel = SaveGame::get_damage_level();
                uint8_t projectileLevel = SaveGame::get_projectile_count_level();
                   uint8_t movespeedLevel = SaveGame::get_movespeed_level();
                   uint8_t spawnRateLevel = SaveGame::get_enemy_spawn_rate_level();
                   uint8_t projectileSpeedLevel = SaveGame::get_projectile_speed_level();
                   uint8_t xpMultiplierLevel = SaveGame::get_xp_multiplier_level();
                
                char buffer[256];
                
                // Pickup Range Upgrade
                int yPos = 40;
                if (upgradeSelection == UPGRADE_PICKUP_RANGE) {
                    rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                }
                snprintf(buffer, sizeof(buffer), "Pickup Range +10%% (Lv %d/10)", pickupLevel);
                rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, buffer);
                if (pickupLevel >= 10) {
                    rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^02MAX^00");
                } else if (available >= 10) {
                    rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0210 CR^00");
                } else {
                    rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0110 CR^00");
                }
                
                // Damage Upgrade
                yPos += 12;
                if (upgradeSelection == UPGRADE_DAMAGE) {
                    rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                }
                snprintf(buffer, sizeof(buffer), "Damage +5%% (Lv %d/20)", damageLevel);
                rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, buffer);
                if (damageLevel >= 20) {
                    rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^02MAX^00");
                } else if (available >= 20) {
                    rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0220 CR^00");
                } else {
                    rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0120 CR^00");
                }
                
                // Projectile Count Upgrade
                yPos += 12;
                if (upgradeSelection == UPGRADE_PROJECTILE_COUNT) {
                    rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                }
                snprintf(buffer, sizeof(buffer), "Projectile Count +1 (Lv %d/5)", projectileLevel);
                rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, buffer);
                if (projectileLevel >= 5) {
                    rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^02MAX^00");
                } else if (available >= 50) {
                    rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0250 CR^00");
                } else {
                    rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0150 CR^00");
                }
                
                   // Movespeed Upgrade
                   yPos += 12;
                   if (upgradeSelection == UPGRADE_MOVESPEED) {
                       rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                   }
                   snprintf(buffer, sizeof(buffer), "Movespeed +5%% (Lv %d/10)", movespeedLevel);
                   rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, buffer);
                   if (movespeedLevel >= 10) {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^02MAX^00");
                   } else if (available >= 15) {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0215 CR^00");
                   } else {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0115 CR^00");
                   }
               
                   // Enemy Spawn Rate Upgrade
                   yPos += 12;
                   if (upgradeSelection == UPGRADE_ENEMY_SPAWN_RATE) {
                       rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                   }
                   snprintf(buffer, sizeof(buffer), "Enemy Spawn +10%% (Lv %d/8)", spawnRateLevel);
                   rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, buffer);
                   if (spawnRateLevel >= 8) {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^02MAX^00");
                   } else if (available >= 25) {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0225 CR^00");
                   } else {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0125 CR^00");
                   }
               
                   // Projectile Speed Upgrade
                   yPos += 12;
                   if (upgradeSelection == UPGRADE_PROJECTILE_SPEED) {
                       rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                   }
                   snprintf(buffer, sizeof(buffer), "Projectile Speed +5%% (Lv %d/10)", projectileSpeedLevel);
                   rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, buffer);
                   if (projectileSpeedLevel >= 10) {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^02MAX^00");
                   } else if (available >= 20) {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0220 CR^00");
                   } else {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0120 CR^00");
                   }
               
                   // XP Multiplier Upgrade
                   yPos += 12;
                   if (upgradeSelection == UPGRADE_XP_MULTIPLIER) {
                       rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                   }
                   snprintf(buffer, sizeof(buffer), "XP Gain +10%% (Lv %d/8)", xpMultiplierLevel);
                   rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, buffer);
                   if (xpMultiplierLevel >= 8) {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^02MAX^00");
                   } else if (available >= 30) {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0230 CR^00");
                   } else {
                       rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^0130 CR^00");
                   }
               
                // Shield Weapon Unlock
                yPos += 12;
                if (upgradeSelection == UPGRADE_SHIELD_WEAPON) {
                    rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                }
                bool shieldUnlocked = SaveGame::is_shield_weapon_unlocked();
                if (shieldUnlocked) {
                    rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, "Defense (Unlocked)");
                    rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^02OWNED^00");
                } else {
                    rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, "Unlock: Defense");
                    if (available >= 100) {
                        rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^02100 CR^00");
                    } else {
                        rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^01100 CR^00");
                    }
                }
                
                // Shape Weapon Unlock
                yPos += 12;
                if (upgradeSelection == UPGRADE_SHAPE_WEAPON) {
                    rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                }
                bool shapeUnlocked = SaveGame::is_shape_weapon_unlocked();
                if (shapeUnlocked) {
                    rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, "Whip (Unlocked)");
                    rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^02OWNED^00");
                } else {
                    rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, "Unlock: Whip");
                    if (available >= 150) {
                        rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^02150 CR^00");
                    } else {
                        rdpq_text_printf(nullptr, FONT_MENU, 250, yPos, "^01150 CR^00");
                    }
                }
                
                // Reset Credits
                yPos += 25;
                if (upgradeSelection == UPGRADE_RESET_CREDITS) {
                    rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                }
                rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, "Reset All Upgrades (Refund)");
                
                // Instructions
                rdpq_text_printf(nullptr, FONT_MENU, 50, 200, "Press ^02A^00 to purchase/reset");
                rdpq_text_printf(nullptr, FONT_MENU, 50, 220, "Press ^01B^00 to return");
                break;
            }
            case SETTINGS_MENU: {
                rdpq_text_printf(nullptr, FONT_MENU, 50, 12, "SETTINGS");
                
                int yPos = 40;
                
                // Music Volume
                if (settingsSelection == 0) {
                    rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                }
                uint8_t musicVol = SaveGame::get_music_volume();
                rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, "Music Volume");
                
                // Draw slider bar
                char sliderBar[13]; // "[##########]" = 12 chars + null
                sliderBar[0] = '[';
                for (int i = 0; i < 10; i++) {
                    sliderBar[i + 1] = (i < musicVol) ? '#' : '-';
                }
                sliderBar[11] = ']';
                sliderBar[12] = '\0';
                rdpq_text_printf(nullptr, FONT_MENU, 190, yPos, "%s %d%%", sliderBar, musicVol * 10);
                
                yPos += 20;
                
                // SFX Volume
                if (settingsSelection == 1) {
                    rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                }
                uint8_t sfxVol = SaveGame::get_sfx_volume();
                rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, "SFX Volume");
                
                // Draw slider bar
                sliderBar[0] = '[';
                for (int i = 0; i < 10; i++) {
                    sliderBar[i + 1] = (i < sfxVol) ? '#' : '-';
                }
                sliderBar[11] = ']';
                sliderBar[12] = '\0';
                rdpq_text_printf(nullptr, FONT_MENU, 190, yPos, "%s %d%%", sliderBar, sfxVol * 10);
                
                yPos += 20;
                
                // Marble Background Toggle
                if (settingsSelection == 2) {
                    rdpq_text_printf(nullptr, FONT_MENU, 40, yPos, ">");
                }
                bool marbleEnabled = SaveGame::is_marble_enabled();
                rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, "Marble Background");
                rdpq_text_printf(nullptr, FONT_MENU, 190, yPos, marbleEnabled ? "^02ON^00" : "^01OFF^00");
                
                // Instructions
                rdpq_text_printf(nullptr, FONT_MENU, 50, 200, "Use ^02D-Pad^00 to adjust");
                rdpq_text_printf(nullptr, FONT_MENU, 50, 220, "Press ^01B^00 to return");
                break;
            }
            case STATS_MENU: {
                int yPos = 20;
                rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, "STATS");
                yPos += 20;
                
                // Display save game stats
                uint32_t totalLevelUps = SaveGame::get_total_level_ups();
                uint32_t bestTime = SaveGame::get_best_time();
                uint16_t flags = SaveGame::get_level_complete_flags();
                
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "Total Level Ups: %lu", (unsigned long)totalLevelUps);
                rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, buffer);
                yPos += 20;

                if (bestTime == 0) {
                    rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, "Best Time: --:--");
                } else {
                    int minutes = bestTime / 60;
                    int seconds = bestTime % 60;
                    snprintf(buffer, sizeof(buffer), "Best Time: %02d:%02d", minutes, seconds);
                    rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, buffer);
                }

                yPos += 20;

                // Show level unlock mask and per-level status
                snprintf(buffer, sizeof(buffer), "Levels: (0x%04x)", (unsigned)flags);
                rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, buffer);
                yPos += 15;

                for (int i = 0; i < kMaxLevels; ++i) {
                    bool completed = (flags & (1u << i)) != 0u;
                    bool unlocked = (i == 0) || (flags & (1u << (i - 1))) || completed;
                    const char* state = completed ? "COMPLETED" : (unlocked ? "UNLOCKED" : "LOCKED");
                    snprintf(buffer, sizeof(buffer), "L%d %s - %s", i + 1, kLevelNames[i], state);
                    rdpq_text_printf(nullptr, FONT_MENU, 50, yPos, buffer);
                    yPos += 15;
                }

                // snprintf(buffer, sizeof(buffer), "Levels Complete: 0x%04x", (unsigned)flags);
                // rdpq_text_printf(nullptr, FONT_MENU, 50, 190, buffer);

                // Draw purge option
                if (showPurgeConfirm) {
                    rdpq_text_printf(nullptr, FONT_MENU, 50, 180, "^01PURGE SAVE DATA?");
                    rdpq_text_printf(nullptr, FONT_MENU, 50, 200, "^01Press A to confirm, D-Up to cancel");
                } else {
                    rdpq_text_printf(nullptr, FONT_MENU, 50, 180, "Press D-Down for ^01PURGE^00 or \n ^01B^00 to return");
                }
                break;
            }
        }
    }

    MenuState getCurrentState() {
        return currentState;
    }

    bool shouldStartGame() {
        return shouldStart;
    }

    int getSelectedLevel() {
        return selectedLevel;
    }
}
