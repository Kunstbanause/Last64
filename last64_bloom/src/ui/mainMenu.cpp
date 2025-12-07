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

// External flags from scenes
extern bool showMarbleBackground;
extern bool marbleBackgroundChanged;

namespace MainMenu {
    // Menu state
    static MenuState currentState = MAIN_MENU;
    static int currentSelection = 0;
    static bool shouldStart = false;
    static rdpq_font_t* font = nullptr;
    static bool initialized = false;
    static sprite_t* titleSprite = nullptr;
    
    // Purge confirmation
    static bool showPurgeConfirm = false;

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
        shouldStart = false;
        showPurgeConfirm = false;
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
                if (pressed.d_up || stickUp) {
                    currentSelection = (currentSelection - 1 + 3) % 3;
                }
                if (pressed.d_down || stickDown) {
                    currentSelection = (currentSelection + 1) % 3;
                }
                if (pressed.a || pressed.z) {
                    if (currentSelection == 0) {
                        shouldStart = true;
                    } else {
                        currentState = (MenuState)(UPGRADES_MENU + (currentSelection - 1));
                    }
                }
                break;
            }
            case UPGRADES_MENU: {
                if (pressed.b) {
                    currentState = MAIN_MENU;
                    currentSelection = 1;
                }
                // Placeholder for upgrades menu interaction
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
                    currentSelection = 2;
                    showPurgeConfirm = false;
                }
                break;
            }
        }
    }

    void draw() {
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
                rdpq_text_printf(nullptr, FONT_MENU, 100, 120, "Start Level");
                rdpq_text_printf(nullptr, FONT_MENU, 100, 140, "Upgrades");
                rdpq_text_printf(nullptr, FONT_MENU, 100, 160, "Stats");

                // Draw selection indicator
                int yPos = 120 + (currentSelection * 20);
                rdpq_text_printf(nullptr, FONT_MENU, 90, yPos, ">");
                break;
            }
            case UPGRADES_MENU: {
                rdpq_text_printf(nullptr, FONT_MENU, 50, 80, "UPGRADES");
                rdpq_text_printf(nullptr, FONT_MENU, 50, 130, "Permanent Passive Upgrades");
                rdpq_text_printf(nullptr, FONT_MENU, 50, 160, "(Placeholder)");
                rdpq_text_printf(nullptr, FONT_MENU, 50, 200, "Press ^01B^00 to return");
                break;
            }
            case STATS_MENU: {
                rdpq_text_printf(nullptr, FONT_MENU, 50, 80, "STATS");
                
                // Display save game stats
                uint32_t totalLevelUps = SaveGame::get_total_level_ups();
                uint32_t bestTime = SaveGame::get_best_time();
                //uint16_t flags = SaveGame::get_level_complete_flags();
                
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "Total Level Ups: %lu", (unsigned long)totalLevelUps);
                rdpq_text_printf(nullptr, FONT_MENU, 50, 130, buffer);

                if (bestTime == 0) {
                    rdpq_text_printf(nullptr, FONT_MENU, 50, 160, "Best Time: --:--");
                } else {
                    int minutes = bestTime / 60;
                    int seconds = bestTime % 60;
                    snprintf(buffer, sizeof(buffer), "Best Time: %02d:%02d", minutes, seconds);
                    rdpq_text_printf(nullptr, FONT_MENU, 50, 160, buffer);
                }

                // snprintf(buffer, sizeof(buffer), "Levels Complete: 0x%04x", (unsigned)flags);
                // rdpq_text_printf(nullptr, FONT_MENU, 50, 190, buffer);

                // Draw purge option
                if (showPurgeConfirm) {
                    rdpq_text_printf(nullptr, FONT_MENU, 50, 180, "^01PURGE SAVE DATA?");
                    rdpq_text_printf(nullptr, FONT_MENU, 50, 200, "^01Press A to confirm, D-Up to cancel");
                } else {
                    rdpq_text_printf(nullptr, FONT_MENU, 50, 180, "Press D-Down for ^01PURGE^00 or ^01B^00 to return");
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
}
