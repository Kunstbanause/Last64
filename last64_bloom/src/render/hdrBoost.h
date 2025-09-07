/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once

namespace HDRBoost {
    void initialize(float defaultHDRFactor);
    void update(float deltaTime);
    void triggerBoost();
    float getCurrentHDRFactor();
    void setDefaultHDRFactor(float defaultFactor);
}