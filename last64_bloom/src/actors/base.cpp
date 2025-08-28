/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#include "base.h"

bool Actor::Base::checkFrustumAABB(const T3DVec3 &aabbMin, const T3DVec3 &aabbMax) const
{
	// Simple frustum check: check if the AABB is inside the screen boundaries
	// This is a very basic check and might need to be improved for more complex scenes
	return !(aabbMax.x < -1.0f || aabbMax.y < -1.0f || aabbMin.x > 1.0f || aabbMin.y > 1.0f);
}

bool Actor::Base::checkFrustumSphere(const T3DVec3 &center, float radius) const
{
	// Simple frustum check: check if the sphere is inside the screen boundaries
	// This is a very basic check and might need to be improved for more complex scenes
	return !(center.x + radius < -1.0f || center.y + radius < -1.0f || center.x - radius > 1.0f || center.y - radius > 1.0f);
}

bool Actor::Base::collidesWithAABB(const Base* other) const
{
	if (!other || (other->flags & FLAG_DISABLED) || (flags & FLAG_DISABLED)) {
		return false;
	}

	T3DVec3 thisPos = getPosition();
	T3DVec3 otherPos = other->getPosition();
	
	// Get dimensions for both objects
	float thisWidth, thisHeight;
	float otherWidth, otherHeight;
	
	getAABBSize(thisWidth, thisHeight);
	other->getAABBSize(otherWidth, otherHeight);
	
	// Calculate half dimensions
	float thisHalfWidth = thisWidth * 0.5f;
	float thisHalfHeight = thisHeight * 0.5f;
	float otherHalfWidth = otherWidth * 0.5f;
	float otherHalfHeight = otherHeight * 0.5f;
	
	// Check for overlap on X axis
	bool xOverlap = fabsf(thisPos.x - otherPos.x) < (thisHalfWidth + otherHalfWidth);
	
	// Check for overlap on Y axis
	bool yOverlap = fabsf(thisPos.y - otherPos.y) < (thisHalfHeight + otherHalfHeight);
	
	// Collision occurs if there's overlap on both axes
	return xOverlap && yOverlap;
}