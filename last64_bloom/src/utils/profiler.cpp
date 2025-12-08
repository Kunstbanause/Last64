/**
 * @copyright 2025 - Max Bebök
 * @license MIT
 */
#include "profiler.h"
#include <cstring>

namespace Profiler {
    namespace {
        Section sections[MAX_SECTIONS];
        int sectionCount = 0;
        uint64_t startTicks[MAX_SECTIONS];
        int frameCount = 0;
        int maxResetCounter = 0;
        constexpr int FRAME_ACCUMULATE = 10;  // Accumulate over 10 frames for smoother display
        constexpr int MAX_RESET_FRAMES = 600; // Reset max every 600 frames (~10 seconds at 60fps)
    }
    
    void init() {
        for (int i = 0; i < MAX_SECTIONS; ++i) {
            sections[i] = Section{nullptr, 0, 0, 0, 0, false};
            startTicks[i] = 0;
        }
        sectionCount = 0;
        frameCount = 0;
        maxResetCounter = 0;
    }
    
    int begin(const char* name) {
        // Find existing section or create new one
        int sectionId = -1;
        for (int i = 0; i < sectionCount; ++i) {
            if (sections[i].name && strcmp(sections[i].name, name) == 0) {
                sectionId = i;
                break;
            }
        }
        
        if (sectionId == -1 && sectionCount < MAX_SECTIONS) {
            sectionId = sectionCount++;
            sections[sectionId].name = name;
            sections[sectionId].total_ticks = 0;
            sections[sectionId].frame_ticks = 0;
            sections[sectionId].max_ticks = 0;
            sections[sectionId].call_count = 0;
        }
        
        if (sectionId >= 0) {
            sections[sectionId].active = true;
            startTicks[sectionId] = get_ticks();
        }
        
        return sectionId;
    }
    
    void end(int sectionId) {
        if (sectionId < 0 || sectionId >= sectionCount) return;
        if (!sections[sectionId].active) return;
        
        uint64_t endTicks = get_ticks();
        uint64_t elapsed = endTicks - startTicks[sectionId];
        sections[sectionId].frame_ticks += elapsed;
        sections[sectionId].call_count++;
        sections[sectionId].active = false;
    }
    
    const Section* getSections(int& outCount) {
        outCount = sectionCount;
        return sections;
    }
    
    void display(float x, float y) {
        // This will be called from the rendering code
        // For now, just log to console - we'll integrate with UI later
        int count;
        const Section* secs = getSections(count);
        
        for (int i = 0; i < count; ++i) {
            if (secs[i].call_count > 0) {
                // Convert ticks to microseconds for precision
                uint64_t totalUs = (secs[i].total_ticks * 1000000ULL) / RCP_FREQUENCY;
                float avgUs = (float)totalUs / (float)secs[i].call_count;
                debugf("%s: %.2fuS (%.0f calls)\n", 
                       secs[i].name, avgUs, (float)secs[i].call_count);
            }
        }
    }
    
    // Called once per frame after display to accumulate data
    void frameEnd() {
        frameCount++;
        maxResetCounter++;
        
        // Update max values BEFORE accumulation (check this frame's peak)
        for (int i = 0; i < sectionCount; ++i) {
            if (sections[i].frame_ticks > sections[i].max_ticks) {
                sections[i].max_ticks = sections[i].frame_ticks;
            }
        }
        
        // Reset max values every ~10 seconds
        if (maxResetCounter >= MAX_RESET_FRAMES) {
            for (int i = 0; i < sectionCount; ++i) {
                sections[i].max_ticks = 0;
            }
            maxResetCounter = 0;
        }
        
        if (frameCount >= FRAME_ACCUMULATE) {
            // Transfer frame data to total
            for (int i = 0; i < sectionCount; ++i) {
                sections[i].total_ticks = sections[i].frame_ticks;
                sections[i].frame_ticks = 0;
                sections[i].call_count = 0;  // Reset call count each display cycle
            }
            frameCount = 0;
        } else {
            // Accumulate frame data into running total
            for (int i = 0; i < sectionCount; ++i) {
                sections[i].total_ticks += sections[i].frame_ticks;
                sections[i].frame_ticks = 0;
                // Don't reset call_count here - keep accumulating
            }
        }
    }
}  // namespace Profiler
