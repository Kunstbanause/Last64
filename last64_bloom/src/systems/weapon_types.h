/**
* @copyright 2025 - Max Bebok
* @license MIT
*/
#pragma once
#include <cstdint>

namespace Actor {
    enum class WeaponType : uint8_t {
        PROJECTILE = 0,
        HOMING,
        CIRCULAR,
        SPIRAL,
        SHIELD,
        SHAPE,
        COUNT
    };
}
