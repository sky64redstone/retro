#pragma once

#include <SDL3/SDL.h>
#include <stddef.h>

#include "vec.h"
#include "graphics.h"
#include "input.h"

enum menu_event_type {
  MENU_EVENT_NONE,
  MENU_EVENT_ACTIVATE,
  MENU_EVENT_BACK
};

typedef struct menu_event menu_event_t;
typedef struct menu_item menu_item_t;
typedef struct menu_style menu_style_t;
typedef struct menu menu_t;

typedef void (*menu_render_fn)(
  const menu_t* menu,
  SDL_Renderer* renderer,
  vec2_t win_size
);

struct menu_event {
  enum menu_event_type type;
  size_t index;
};

struct menu_item {
  const char* text;
  const void* data;
};

struct menu_style {
  color_t background;
  color_t panel;
  color_t border;
  color_t text;
  color_t title;
  color_t selected_background;
  color_t selected_text;
  color_t cursor;

  vec2_t position;
  vec2_t text_scale;
  vec2_t title_scale;

  float item_height;
  float padding;
  float title_spacing;
  float border_width;
};

struct menu {
  const char* title;

  const menu_item_t* items;
  size_t item_count;

  size_t selected;

  const menu_style_t* style;
  menu_render_fn render;
};

void menu_init(menu_t* menu);

menu_event_t menu_update(
  menu_t* menu,
  const input_t* input
);

void menu_render(
  const menu_t* menu,
  SDL_Renderer* renderer,
  vec2_t win_size
);
