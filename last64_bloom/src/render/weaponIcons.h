/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include <libdragon.h>
#include <t3d/t3d.h>
#include "../systems/weapon_registry.h"

namespace WeaponIcons {
    // Initialize the weapon icons system
    void init();
    
    // Destroy the weapon icons system
    void destroy();
    
    // Draw a weapon icon at the specified position
    void drawIcon(float x, float y, Actor::WeaponType type, int level);
    
    // Get the width and height of a weapon icon
    float getIconWidth();
    float getIconHeight();
}