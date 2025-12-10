/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include <libdragon.h>
#include <vector>
#include "../actors/base.h"
#include "../camera/camera.h"

class Scene
{
  private:

  protected:
    std::vector<Actor::Base*> actors{};
    Camera camera{};

    virtual void updateScene(float deltaTime) = 0;
    virtual void draw3D(float deltaTime) = 0;

  public:

    virtual ~Scene() {
      for(auto actor : actors) {
        delete actor;
      }
    }

    void update(float deltaTime);
    void draw(float deltaTime);

    virtual void draw2D(float deltaTime) {}

    // Whether global UI (e.g., XP bar) should be shown this frame
    virtual bool shouldShowXPBar() const { return true; }

    Camera &getCam() { return camera; }
};