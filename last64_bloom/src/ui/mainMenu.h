/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include <rdpq.h>
#include <t3d/t3d.h>

#define FONT_MENU 10

namespace MainMenu {
    enum MenuState {
        MAIN_MENU,
        UPGRADES_MENU,
        STATS_MENU
    };
    
    enum UpgradeOption {
        UPGRADE_PICKUP_RANGE,
        UPGRADE_DAMAGE,
        UPGRADE_PROJECTILE_COUNT,
        UPGRADE_SHIELD_WEAPON,
        UPGRADE_SHAPE_WEAPON,
        UPGRADE_RESET_CREDITS,
        UPGRADE_COUNT
    };

    void initialize();
    void cleanup();
    void update(float deltaTime);
    void draw();
    
    MenuState getCurrentState();
    bool shouldStartGame();
    void reset();
}
