#include "collision.h"
#include <math.h>

bool collision_circle_circle_xz(T3DVec3 posA, float radiusA, T3DVec3 posB, float radiusB) {
    float dx = posA.v[0] - posB.v[0];
    float dz = posA.v[2] - posB.v[2];
    float distSq = dx * dx + dz * dz;
    float radSum = radiusA + radiusB;
    return distSq < (radSum * radSum);
}

bool collision_point_in_bounds_xz(T3DVec3 pos, float minX, float maxX, float minZ, float maxZ) {
    return (pos.v[0] >= minX && pos.v[0] <= maxX &&
            pos.v[2] >= minZ && pos.v[2] <= maxZ);
}

float collision_distance_xz(T3DVec3 a, T3DVec3 b) {
    float dx = a.v[0] - b.v[0];
    float dz = a.v[2] - b.v[2];
    return sqrtf(dx * dx + dz * dz);
}
