#include "hdrBoost.h"
#include <libdragon.h>
#include <algorithm>

namespace {
    float defaultHDRFactor = 0.8f;  // Default HDR factor
    float currentHDRFactor = 0.8f;  // Current HDR factor
    float targetHDRFactor = 0.8f;   // Target HDR factor
    float hdrBoostTimer = 0.0f;     // Timer for the boost
    constexpr float HDR_BOOST_DURATION = 5.0f;  // Total duration of the effect
    constexpr float HDR_PEAK_DURATION = 0.1f;   // Duration to stay at peak
    constexpr float HDR_PEAK_VALUE = 5.0f;      // Peak HDR value
}

void HDRBoost::initialize(float defaultHDRFactor) {
    ::defaultHDRFactor = defaultHDRFactor;
    currentHDRFactor = defaultHDRFactor;
    targetHDRFactor = defaultHDRFactor;
    hdrBoostTimer = 0.0f;
}

void HDRBoost::update(float deltaTime) {
    if (hdrBoostTimer > 0.0f) {
        hdrBoostTimer -= deltaTime;
        
        if (hdrBoostTimer <= 0.0f) {
            // Boost is finished, reset to default
            hdrBoostTimer = 0.0f;
            currentHDRFactor = defaultHDRFactor;
            targetHDRFactor = defaultHDRFactor;
        } else {
            // Calculate interpolation factor
            float progress = 1.0f - (hdrBoostTimer / HDR_BOOST_DURATION);
            float speed = 5.0f; // Adjust this value to change the smoothness
            
            // Determine target based on progress
            if (progress < (HDR_PEAK_DURATION / HDR_BOOST_DURATION)) {
                // Still in peak phase
                targetHDRFactor = HDR_PEAK_VALUE;
                speed = 150.0f; // Adjust this value to change the smoothness
            } else {
                // Fade back to default
                targetHDRFactor = defaultHDRFactor;
                speed = 15.0f; // Adjust this value to change the smoothness
            }
            
            // Smoothly interpolate towards target
            if (currentHDRFactor < targetHDRFactor) {
                currentHDRFactor = fminf(currentHDRFactor + speed * deltaTime, targetHDRFactor);
            } else if (currentHDRFactor > targetHDRFactor) {
                currentHDRFactor = fmaxf(currentHDRFactor - speed * deltaTime, targetHDRFactor);
            }
        }
    }
}

void HDRBoost::triggerBoost() {
    targetHDRFactor = HDR_PEAK_VALUE;
    hdrBoostTimer = HDR_BOOST_DURATION;
}

float HDRBoost::getCurrentHDRFactor() {
    return currentHDRFactor;
}

void HDRBoost::setDefaultHDRFactor(float defaultFactor) {
    defaultHDRFactor = defaultFactor;
    // If no boost is active, update the current factor to match
    if (hdrBoostTimer <= 0.0f) {
        currentHDRFactor = defaultFactor;
        targetHDRFactor = defaultFactor;
    }
}