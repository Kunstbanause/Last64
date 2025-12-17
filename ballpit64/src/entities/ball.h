#ifndef BALL_H
#define BALL_H

#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <stdbool.h>

typedef struct {
  T3DVec3 pos;
  float vx;
  float vz;
  float speed;
  bool released; // whether ball has left the player and can be caught again
  T3DMat4FP* matrix; // Uncached matrix for rendering
} Ball;

// Initialize ball at a given position
void ball_init(Ball* ball, T3DVec3 start_pos);

// Update ball physics (movement, collisions with walls)
void ball_update(Ball* ball, float deltaTime, float box_size);

// Check if ball should be caught by player and reset to shoot toward reticle
// Returns true if ball was caught/reset
bool ball_check_catch(Ball* ball, T3DVec3 player_pos, T3DVec3 reticle_pos, float box_size);

// Render the ball
void ball_render(Ball* ball, T3DModel* model);

// Cleanup ball resources
void ball_destroy(Ball* ball);

#endif // BALL_H
