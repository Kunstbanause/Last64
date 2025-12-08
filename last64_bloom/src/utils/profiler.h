/**
 * @copyright 2025 - Max Bebök
 * @license MIT
 * 
 * CPU-side performance profiling utilities
 */
#pragma once
#include <libdragon.h>
#include <cstdint>

namespace Profiler {
    // Maximum number of profiling sections
    constexpr int MAX_SECTIONS = 32;
    
    struct Section {
        const char* name;
        uint64_t total_ticks;
        uint64_t frame_ticks;  // Accumulated ticks for current frame
        uint64_t max_ticks;    // Maximum ticks seen in last 10 seconds
        uint32_t call_count;
        bool active;
    };
    
    // Initialize profiler
    void init();
    
    // Begin timing a section (returns section ID)
    int begin(const char* name);
    
    // End timing a section
    void end(int sectionId);
    
    // Get section data
    const Section* getSections(int& outCount);
    
    // Display profiling results on screen
    void display(float x, float y);
    
    // Called at end of frame to accumulate and reset data
    void frameEnd();
}

// RAII helper for automatic scope-based timing
class ProfileScope {
public:
    explicit ProfileScope(const char* name) 
        : sectionId(Profiler::begin(name)) {}
    
    ~ProfileScope() {
        Profiler::end(sectionId);
    }
    
    // Non-copyable
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;
    
private:
    int sectionId;
};
