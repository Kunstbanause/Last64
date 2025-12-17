#ifndef COLLISION_H
#define COLLISION_H

#include <t3d/t3dmath.h>
#include <stdbool.h>

// Generic 2D circle-circle collision (using X and Z coordinates)
bool collision_circle_circle_xz(T3DVec3 posA, float radiusA, T3DVec3 posB, float radiusB);

// Check if a point is within AABB bounds (2D, using X and Z)
bool collision_point_in_bounds_xz(T3DVec3 pos, float minX, float maxX, float minZ, float maxZ);

// Calculate 2D distance between two points (using X and Z)
float collision_distance_xz(T3DVec3 a, T3DVec3 b);

#endif // COLLISION_H
