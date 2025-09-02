/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "colorTest.h"
#include <t3d/t3d.h>
#include <libdragon.h>
#include <malloc.h>

// Static vertices for color testing
static T3DVertPacked* testVertices = nullptr;
static T3DMat4FP* testMatrix = nullptr;
static bool initialized = false;

// Initialize the color test utility
static void initialize() {
    if (initialized) return;
    
    // Allocate vertices for color testing (16 quads = 32 triangles = 64 vertices)
    // We're using 32 T3DVertPacked structures to hold 64 vertices
    testVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * 32);
    
    // Create a matrix for positioning the color test strip
    testMatrix = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
    
    // Position the color test strip in the top-left corner of the screen
    T3DVec3 position = {{-100.0f, 80.0f, 0.0f}};
    t3d_mat4fp_from_srt_euler(
        testMatrix,
        (T3DVec3){{1.0f, 1.0f, 1.0f}},  // scale
        (T3DVec3){{0.0f, 0.0f, 0.0f}},  // rotation
        position                         // translation
    );
    
    // Define test colors (RGBA8 format)
    uint32_t testColors[] = {
        0xFFFFFFFF, // White
        0xFF0000FF, // Red
        0x00FF00FF, // Green
        0x0000FFFF, // Blue
        0xFFFF00FF, // Yellow
        0xFF00FFFF, // Magenta
        0x00FFFFFF, // Cyan
        0xFFA500FF, // Orange
        0x800080FF, // Purple
        0xFFC0CBFF, // Pink
        0xA52A2AFF, // Brown
        0x808080FF, // Gray
        0x000000FF, // Black
        0x800000FF, // Maroon
        0x008000FF, // Dark Green
        0x000080FF  // Navy
    };
    
    // Create quads with different colors
    for (int i = 0; i < 16; i++) {
        // Calculate position for this quad
        float x_offset = i * 12.0f; // 12 units between quads
        
        // Each quad needs 4 vertices, but we store them in T3DVertPacked structures (2 vertices each)
        // So we need 2 T3DVertPacked structures per quad
        
        int structIndex = i * 2; // Index of the first structure for this quad
        
        // First structure (vertices 0 and 1 of this quad)
        testVertices[structIndex] = (T3DVertPacked){};
        
        // Vertex 0 (top-left)
        testVertices[structIndex].posA[0] = (int16_t)(0 + x_offset);
        testVertices[structIndex].posA[1] = 0;
        testVertices[structIndex].posA[2] = 0;
        testVertices[structIndex].normA = 0;
        testVertices[structIndex].rgbaA = testColors[i % 16];
        testVertices[structIndex].stA[0] = 0;
        testVertices[structIndex].stA[1] = 0;
        
        // Vertex 1 (bottom-left)
        testVertices[structIndex].posB[0] = (int16_t)(0 + x_offset);
        testVertices[structIndex].posB[1] = 10;
        testVertices[structIndex].posB[2] = 0;
        testVertices[structIndex].normB = 0;
        testVertices[structIndex].rgbaB = testColors[i % 16];
        testVertices[structIndex].stB[0] = 0;
        testVertices[structIndex].stB[1] = 0;
        
        // Second structure (vertices 2 and 3 of this quad)
        testVertices[structIndex + 1] = (T3DVertPacked){};
        
        // Vertex 2 (bottom-right)
        testVertices[structIndex + 1].posA[0] = (int16_t)(10 + x_offset);
        testVertices[structIndex + 1].posA[1] = 10;
        testVertices[structIndex + 1].posA[2] = 0;
        testVertices[structIndex + 1].normA = 0;
        testVertices[structIndex + 1].rgbaA = testColors[i % 16];
        testVertices[structIndex + 1].stA[0] = 0;
        testVertices[structIndex + 1].stA[1] = 0;
        
        // Vertex 3 (top-right)
        testVertices[structIndex + 1].posB[0] = (int16_t)(10 + x_offset);
        testVertices[structIndex + 1].posB[1] = 0;
        testVertices[structIndex + 1].posB[2] = 0;
        testVertices[structIndex + 1].normB = 0;
        testVertices[structIndex + 1].rgbaB = testColors[i % 16];
        testVertices[structIndex + 1].stB[0] = 0;
        testVertices[structIndex + 1].stB[1] = 0;
    }
    
    initialized = true;
}

// Cleanup the color test utility
static void cleanup() {
    if (testVertices) {
        free_uncached(testVertices);
        testVertices = nullptr;
    }
    
    if (testMatrix) {
        free_uncached(testMatrix);
        testMatrix = nullptr;
    }
    
    initialized = false;
}

// Draw the color test strip
void color_test_draw() {
    // Initialize if not already done
    if (!initialized) {
        initialize();
    }
    
    // Set up rendering state for untextured, shaded polygons
    t3d_state_set_drawflags((enum T3DDrawFlags)(T3D_FLAG_SHADED));
    
    // Push our test matrix
    t3d_matrix_push(testMatrix);
    
    // Load vertices
    t3d_vert_load(testVertices, 0, 64); // Load 64 vertices
    
    // Draw 16 quads using triangle strips
    for (int i = 0; i < 16; i++) {
        // Each quad is made of 4 vertices, indexed as 0,1,2,3
        // We draw two triangles: (0,1,2) and (0,2,3)
        int baseVertex = i * 4;
        
        // Draw first triangle (0,1,2)
        t3d_tri_draw(baseVertex, baseVertex + 1, baseVertex + 2);
        
        // Draw second triangle (0,2,3)
        t3d_tri_draw(baseVertex, baseVertex + 2, baseVertex + 3);
    }
    
    // Sync triangles
    t3d_tri_sync();
    
    // Pop matrix
    t3d_matrix_pop(1);
}