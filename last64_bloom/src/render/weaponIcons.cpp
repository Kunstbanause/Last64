/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weaponIcons.h"
#include "../render/debugDraw.h"

namespace {
    sprite_t *weaponIconsSprite = nullptr;
    rspq_block_t *dplSetup = nullptr;
    
    constexpr int ICON_WIDTH = 16;
    constexpr int ICON_HEIGHT = 16;
    constexpr int ICONS_PER_ROW = 6; // 6 weapon types
}

namespace WeaponIcons {
    void init() {
        // For now, we'll create a simple placeholder sprite
        // In a real implementation, this would load from a sprite sheet
        weaponIconsSprite = nullptr; // Will be set when we have actual icons
        
        rspq_block_begin();
            rdpq_sync_pipe();
            rdpq_sync_tile();
            rdpq_sync_load();
            
            rdpq_mode_begin();
                rdpq_set_mode_standard();
                rdpq_mode_blender(0);
                rdpq_mode_persp(false);
                rdpq_mode_antialias(AA_NONE);
                rdpq_mode_alphacompare(1);
            rdpq_mode_end();
        dplSetup = rspq_block_end();
    }
    
    void destroy() {
        if (weaponIconsSprite) {
            sprite_free(weaponIconsSprite);
            weaponIconsSprite = nullptr;
        }
        if (dplSetup) {
            rspq_block_free(dplSetup);
            dplSetup = nullptr;
        }
    }
    
    void drawIcon(float x, float y, Actor::WeaponType type, int level) {
        // Set color based on weapon type
        color_t iconColor = RGBA32(0xFF, 0xFF, 0xFF, 0xFF); // White default
        switch (type) {
            case Actor::WeaponType::PROJECTILE:
                iconColor = RGBA32(0xFF, 0x80, 0x80, 0xFF); // Light red
                break;
            case Actor::WeaponType::HOMING:
                iconColor = RGBA32(0x80, 0xFF, 0x80, 0xFF); // Light green
                break;
            case Actor::WeaponType::CIRCULAR:
                iconColor = RGBA32(0x80, 0x80, 0xFF, 0xFF); // Light blue
                break;
            case Actor::WeaponType::SPIRAL:
                iconColor = RGBA32(0xFF, 0xFF, 0x80, 0xFF); // Light yellow
                break;
            case Actor::WeaponType::SHIELD:
                iconColor = RGBA32(0xC0, 0xC0, 0xC0, 0xFF); // Light gray
                break;
            case Actor::WeaponType::SHAPE:
                iconColor = RGBA32(0xFF, 0x80, 0xFF, 0xFF); // Light magenta
                break;
        }
        
        // Draw a colored rectangle as a placeholder for the icon
        rdpq_set_mode_fill(iconColor);
        
        // Draw the icon background
        rdpq_fill_rectangle(x, y, x + ICON_WIDTH, y + ICON_HEIGHT);
        
        // Draw a border around the icon
        rdpq_set_mode_fill(RGBA32(0x00, 0x00, 0x00, 0xFF)); // Black border
        
        // Draw border lines manually with rectangles
        rdpq_fill_rectangle(x, y, x + ICON_WIDTH, y + 1); // Top
        rdpq_fill_rectangle(x, y, x + 1, y + ICON_HEIGHT); // Left
        rdpq_fill_rectangle(x + ICON_WIDTH - 1, y, x + ICON_WIDTH, y + ICON_HEIGHT); // Right
        rdpq_fill_rectangle(x, y + ICON_HEIGHT - 1, x + ICON_WIDTH, y + ICON_HEIGHT); // Bottom
        
        // Restore mode for text drawing
        Debug::printStart();
        
        // Draw the weapon type character in the center
        char weaponChar = 'X';
        const WeaponRegistry::WeaponMetadata* metadata = 
            WeaponRegistry::getWeaponMetadata(type);
        if (metadata) {
            weaponChar = metadata->shortName[0];
        }
        
        // Draw level number in the corner
        char levelStr[4] = {0};
        snprintf(levelStr, 4, "%d", level);

        // Draw text using the debug draw system
        float charX = x + (ICON_WIDTH / 2) - 3;
        float charY = y + (ICON_HEIGHT / 2) - 4;
        char weaponStr[2] = { weaponChar, '\0' };
        Debug::print(charX, charY, weaponStr);
        
        float levelX = x + ICON_WIDTH - 6;
        float levelY = y + ICON_HEIGHT - 8;
        Debug::print(levelX, levelY, levelStr);
    }
    
    float getIconWidth() {
        return (float)ICON_WIDTH;
    }
    
    float getIconHeight() {
        return (float)ICON_HEIGHT;
    }
}