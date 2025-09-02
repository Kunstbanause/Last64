/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "colorTest.h"
#include "colors.h"
#include <t3d/t3d.h>
#include <libdragon.h>
#include <malloc.h>

// Static vertices for color testing
static T3DVertPacked* testVertices = nullptr;
static T3DMat4FP* testMatrix = nullptr;
static bool initialized = false;

static const int NUM_COLORS = sizeof(Colors::testColors) / sizeof(Colors::testColors[0]);

// Initialize the color test utility
static void initialize() {
    if (initialized) return;
    
    // Allocate vertices for color testing (NUM_COLORS quads = NUM_COLORS*2 triangles = NUM_COLORS*4 vertices)
    // We're using NUM_COLORS*2 T3DVertPacked structures to hold NUM_COLORS*4 vertices
    testVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * NUM_COLORS * 2);
    
    // Create a matrix for positioning the color test strip
    testMatrix = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
    
    // Position the color test strip in the top
    T3DVec3 position = {{0.0f, 200.0f, 0.0f}}; // Center horizontally at the top
    t3d_mat4fp_from_srt_euler(
        testMatrix,
        (T3DVec3){{1.0f, 1.0f, 1.0f}},  // scale
        (T3DVec3){{0.0f, 0.0f, 0.0f}},  // rotation
        position                         // translation
    );
    
    // Calculate quad width to span the screen width
    float screenWidth = 320.0f; // Actual N64 screen width
    float quadWidth = screenWidth / NUM_COLORS;
    float quadHeight = 10.0f;
    
    // Create quads with different colors
    for (int i = 0; i < NUM_COLORS; i++) {
        // Calculate position for this quad
        float x_offset = i * quadWidth;
        
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
        testVertices[structIndex].rgbaA = Colors::testColors[i];
        testVertices[structIndex].stA[0] = 0;
        testVertices[structIndex].stA[1] = 0;
        
        // Vertex 1 (bottom-left)
        testVertices[structIndex].posB[0] = (int16_t)(0 + x_offset);
        testVertices[structIndex].posB[1] = (int16_t)quadHeight;
        testVertices[structIndex].posB[2] = 0;
        testVertices[structIndex].normB = 0;
        testVertices[structIndex].rgbaB = Colors::testColors[i];
        testVertices[structIndex].stB[0] = 0;
        testVertices[structIndex].stB[1] = 0;
        
        // Second structure (vertices 2 and 3 of this quad)
        testVertices[structIndex + 1] = (T3DVertPacked){};
        
        // Vertex 2 (bottom-right)
        testVertices[structIndex + 1].posA[0] = (int16_t)(quadWidth + x_offset);
        testVertices[structIndex + 1].posA[1] = (int16_t)quadHeight;
        testVertices[structIndex + 1].posA[2] = 0;
        testVertices[structIndex + 1].normA = 0;
        testVertices[structIndex + 1].rgbaA = Colors::testColors[i];
        testVertices[structIndex + 1].stA[0] = 0;
        testVertices[structIndex + 1].stA[1] = 0;
        
        // Vertex 3 (top-right)
        testVertices[structIndex + 1].posB[0] = (int16_t)(quadWidth + x_offset);
        testVertices[structIndex + 1].posB[1] = 0;
        testVertices[structIndex + 1].posB[2] = 0;
        testVertices[structIndex + 1].normB = 0;
        testVertices[structIndex + 1].rgbaB = Colors::testColors[i];
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
    t3d_vert_load(testVertices, 0, NUM_COLORS * 4); // Load all vertices
    
    // Draw quads using triangle pairs
    for (int i = 0; i < NUM_COLORS; i++) {
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