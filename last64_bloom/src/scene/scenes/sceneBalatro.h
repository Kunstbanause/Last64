/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once

#include "../scene.h"
#include <libdragon.h>

class SceneBalatro : public Scene
{
public:
    SceneBalatro();
    ~SceneBalatro() override;

    void updateScene(float deltaTime) override;
    void draw3D(float deltaTime) override;
    void draw2D(float deltaTime) override;
};