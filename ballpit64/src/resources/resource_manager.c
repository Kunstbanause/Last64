#include "resource_manager.h"

void resources_load(Resources *res) {
  // Load models
  res->model_snake = t3d_model_load("rom:/snake.t3dm");
  res->model_map = t3d_model_load("rom:/map.t3dm");
  res->model_shadow = t3d_model_load("rom:/shadow.t3dm");
  res->model_enemy = t3d_model_load("rom:/box.t3dm");
  
  // Load sprites
  res->sprite_textbox = sprite_load("rom:/textbox.i8.sprite");
  
  // Load and configure fonts
  res->font_main = rdpq_font_load("rom:/fibberish.font64");
  rdpq_font_style(res->font_main, 0, &(rdpq_fontstyle_t){.color = (color_t){0xFF, 0xFF, 0xFF, 0xFF}});
  rdpq_font_style(res->font_main, 1, &(rdpq_fontstyle_t){.color = (color_t){232, 101, 65, 0xFF}});
  rdpq_font_style(res->font_main, 2, &(rdpq_fontstyle_t){.color = (color_t){79, 209, 133, 0xFF}});
  rdpq_font_style(res->font_main, 3, &(rdpq_fontstyle_t){.color = (color_t){216, 220, 180, 0xFF}});
}

void resources_free(Resources *res) {
  // Free models
  if (res->model_snake) t3d_model_free(res->model_snake);
  if (res->model_map) t3d_model_free(res->model_map);
  if (res->model_shadow) t3d_model_free(res->model_shadow);
  if (res->model_enemy) t3d_model_free(res->model_enemy);
  
  // Free sprites
  if (res->sprite_textbox) sprite_free(res->sprite_textbox);
  
  // Free fonts
  if (res->font_main) rdpq_font_free(res->font_main);
}
