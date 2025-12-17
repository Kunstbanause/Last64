#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <libdragon.h>
#include <t3d/t3dmodel.h>

// Container for all game assets
typedef struct {
  // Models
  T3DModel *model_snake;
  T3DModel *model_map;
  T3DModel *model_shadow;
  T3DModel *model_enemy;
  
  // Sprites
  sprite_t *sprite_textbox;
  
  // Fonts
  rdpq_font_t *font_main;
} Resources;

// Load all game resources from ROM
void resources_load(Resources *res);

// Free all loaded resources
void resources_free(Resources *res);

#endif // RESOURCE_MANAGER_H
