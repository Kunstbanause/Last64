/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "weaponIcons.h"
#include "../render/debugDraw.h"

namespace {
    sprite_t *weaponIconSprites[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    rspq_block_t *dplSetup = nullptr;
    
    constexpr int ICON_WIDTH = 16;
    constexpr int ICON_HEIGHT = 16;
}

namespace WeaponIcons {
    void init() {
        // Load individual weapon icon sprites
        weaponIconSprites[0] = sprite_load("rom:/icons/1_spread.sprite");
        weaponIconSprites[1] = sprite_load("rom:/icons/2_homing.sprite");
        weaponIconSprites[2] = sprite_load("rom:/icons/3_star.sprite");
        weaponIconSprites[3] = sprite_load("rom:/icons/4_spiral.sprite");
        weaponIconSprites[4] = sprite_load("rom:/icons/5_shield.sprite");
        weaponIconSprites[5] = sprite_load("rom:/icons/6_whip.sprite");
        
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
        for (int i = 0; i < 6; i++) {
            if (weaponIconSprites[i]) {
                sprite_free(weaponIconSprites[i]);
                weaponIconSprites[i] = nullptr;
            }
        }
        if (dplSetup) {
            rspq_block_free(dplSetup);
            dplSetup = nullptr;
        }
    }
    
    void drawIcon(float x, float y, Actor::WeaponType type, int level) {
        // Map weapon type to sprite index
        int spriteIndex = 0;
        switch (type) {
            case Actor::WeaponType::PROJECTILE:
                spriteIndex = 0; // 1_spread.png
                break;
            case Actor::WeaponType::HOMING:
                spriteIndex = 1; // 2_homing.png
                break;
            case Actor::WeaponType::CIRCULAR:
                spriteIndex = 2; // 3_star.png
                break;
            case Actor::WeaponType::SPIRAL:
                spriteIndex = 3; // 4_spiral.png
                break;
            case Actor::WeaponType::SHIELD:
                spriteIndex = 4; // 5_shield.png
                break;
            case Actor::WeaponType::SHAPE:
                spriteIndex = 5; // 6_whip.png
                break;
            case Actor::WeaponType::COUNT:
                // Invalid type, default to projectile
                spriteIndex = 0;
                break;
        }
        
        // Run the display list setup
        if (dplSetup) {
            rspq_block_run(dplSetup);
        }
        
        // Draw the sprite if it's loaded
        if (weaponIconSprites[spriteIndex]) {
            rdpq_sprite_blit(weaponIconSprites[spriteIndex], x, y, NULL);
        }
        
        // Draw the level number in the corner
        Debug::printStart();
        
        char levelStr[4] = {0};
        snprintf(levelStr, 4, "%d", level);
        
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