/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include <libdragon.h>
#include <t3d/t3d.h>

// Forward declarations
namespace Actor
{
  class Player;
  class Enemy;
  class Projectile;
  class Weapon;
  class ProjectileWeapon;
}

namespace Actor
{
  class Base
  {
    private:

    protected:
      T3DVec3 pos{};

      bool checkFrustumAABB(const T3DVec3 &aabbMin, const T3DVec3 &aabbMax) const;
      bool checkFrustumSphere(const T3DVec3 &center, float radius) const;

    public:
      constexpr static uint32_t FLAG_DISABLED = (1 << 0);

      uint32_t flags{0};

      virtual ~Base() {}
      virtual void update(float deltaTime) = 0;

      virtual void draw3D(float deltaTime) {}
      virtual void drawPTX(float deltaTime) {}

      virtual T3DVec3 getPosition() const { return {0,0,0}; }
      virtual float getRadius() const { return 0.0f; }
      
      // Collision methods
      virtual bool collidesWith(Base* other) { return false; } // Default implementation
      
      // AABB collision methods
      virtual void getAABBSize(float& width, float& height) const {
          // Default implementation using radius
          float radius = getRadius();
          width = radius * 2.0f;
          height = radius * 2.0f;
      }
      
      virtual bool collidesWithAABB(const Base* other) const;
  };
}