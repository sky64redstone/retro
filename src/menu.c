#include "menu.h"

#include <string.h>

static const menu_style_t default_menu_style = {
  .background = { 20, 20, 28, 255 },
  .panel = { 10, 12, 18, 255 },
  .border = { 100, 220, 120, 255 },
  .text = { 180, 230, 185, 255 },
  .title = { 220, 255, 220, 255 },
  .selected_background = { 70, 145, 80, 255 },
  .selected_text = { 10, 20, 12, 255 },
  .cursor = { 255, 255, 255, 255 },
  .position = { 40.0f, 48.0f },
  .text_scale = { 2.0f, 2.0f },
  .title_scale = { 3.0f, 3.0f },
  .item_height = 24.0f,
  .padding = 10.0f,
  .title_spacing = 12.0f,
  .border_width = 2.0f
};

static const menu_style_t* menu_get_style(const menu_t* menu) {
  if (menu->style != NULL) {
    return menu->style;
  }

  return &default_menu_style;
}

static float menu_text_width(const char* text, vec2_t scale) {
  if (text == NULL) {
    return 0.0f;
  }

  return (float)strlen(text) * 8.0f * scale.x;
}

static float menu_text_height(vec2_t scale) {
  return 8.0f * scale.y;
}

static float menu_content_width(const menu_t* menu, const menu_style_t* style) {
  float width = 0.0f;

  if (menu->title != NULL) {
    width = menu_text_width(menu->title, style->title_scale);
  }

  for (size_t i = 0; i < menu->item_count; ++i) {
    float item_width = menu_text_width(menu->items[i].text, style->text_scale);

    if (item_width > width) {
      width = item_width;
    }
  }

  width += 16.0f * style->text_scale.x;
  return width;
}

static float menu_content_height(const menu_t* menu, const menu_style_t* style) {
  float height = 0.0f;

  if (menu->title != NULL) {
    height += menu_text_height(style->title_scale);
    height += style->title_spacing;
  }

  height += (float)menu->item_count * style->item_height;
  return height;
}

static void menu_panel_rect(
  const menu_t* menu,
  const menu_style_t* style,
  SDL_FRect* rect
) {
  rect->x = style->position.x;
  rect->y = style->position.y;
  rect->w = menu_content_width(menu, style) + 2.0f * style->padding;
  rect->h = menu_content_height(menu, style) + 2.0f * style->padding;
}

static void menu_item_rect(
  const menu_t* menu,
  const menu_style_t* style,
  size_t index,
  SDL_FRect* rect
) {
  SDL_FRect panel;
  float title_offset = 0.0f;

  menu_panel_rect(menu, style, &panel);

  if (menu->title != NULL) {
    title_offset = menu_text_height(style->title_scale) + style->title_spacing;
  }

  rect->x = panel.x + style->padding;
  rect->y = panel.y + style->padding + title_offset;
  rect->y += (float)index * style->item_height;
  rect->w = panel.w - 2.0f * style->padding;
  rect->h = style->item_height;
}

static bool menu_point_in_rect(vec2_t point, const SDL_FRect* rect) {
  return point.x >= rect->x &&
    point.y >= rect->y &&
    point.x < rect->x + rect->w &&
    point.y < rect->y + rect->h;
}

static size_t menu_item_at_mouse(
  const menu_t* menu,
  const menu_style_t* style,
  vec2_t position
) {
  for (size_t i = 0; i < menu->item_count; ++i) {
    SDL_FRect rect;
    menu_item_rect(menu, style, i, &rect);

    if (menu_point_in_rect(position, &rect)) {
      return i;
    }
  }

  return menu->item_count;
}

void menu_init(menu_t* menu) {
  if (menu == NULL) {
    return;
  }

  menu->selected = 0;
}

menu_event_t menu_update(
  menu_t* menu,
  const input_t* input
) {
  menu_event_t event = {
    .type = MENU_EVENT_NONE,
    .index = 0
  };

  if (menu == NULL || input == NULL || menu->item_count == 0) {
    return event;
  }

  if (menu->selected >= menu->item_count) {
    menu->selected = menu->item_count - 1;
  }

  if (input->pressed[KEY_PAUSE]) {
    event.type = MENU_EVENT_BACK;
    return event;
  }

  if (input->pressed[KEY_UP]) {
    if (menu->selected == 0) {
      menu->selected = menu->item_count - 1;
    } else {
      --menu->selected;
    }
  }

  if (input->pressed[KEY_DOWN]) {
    menu->selected = (menu->selected + 1) % menu->item_count;
  }

  const menu_style_t* style = menu_get_style(menu);

  if (input->mouse.moved) {
    size_t mouse_item = menu_item_at_mouse(menu, style, input->mouse.position);

    if (mouse_item < menu->item_count) {
      menu->selected = mouse_item;

      if (input->mouse.pressed[BUTTON_LEFT]) {
        event.type = MENU_EVENT_ACTIVATE;
        event.index = menu->selected;
        return event;
      }
    }
  }

  if (input->pressed[KEY_ACTION]) {
    event.type = MENU_EVENT_ACTIVATE;
    event.index = menu->selected;
  }

  return event;
}

static void menu_render_default(
  const menu_t* menu,
  SDL_Renderer* renderer,
  vec2_t win_size
) {
  const menu_style_t* style = menu_get_style(menu);

  render_rect(renderer, vec2_zero(), win_size, style->background);
  SDL_FRect panel;

  menu_panel_rect(menu, style, &panel);
  render_rect(
    renderer,
    vec2(panel.x, panel.y),
    vec2(panel.w, panel.h),
    style->panel
  );

  render_rect(
    renderer,
    vec2(panel.x, panel.y),
    vec2(panel.w, style->border_width),
    style->border
  );
  render_rect(
    renderer,
    vec2(panel.x, panel.y + panel.h - style->border_width),
    vec2(panel.w, style->border_width),
    style->border
  );
  render_rect(
    renderer,
    vec2(panel.x, panel.y),
    vec2(style->border_width, panel.h),
    style->border
  );
  render_rect(
    renderer,
    vec2(panel.x + panel.w - style->border_width, panel.y),
    vec2(style->border_width, panel.h),
    style->border
  );

  if (menu->title != NULL) {
    float title_x = panel.x + panel.w * 0.5f;
    float title_y = panel.y + style->padding;

    render_text(
      renderer,
      vec2(
        title_x / style->title_scale.x,
        title_y / style->title_scale.y
      ),
      style->title_scale,
      style->title,
      menu->title,
      ALIGN_MIDDLE
    );

  }

  for (size_t i = 0; i < menu->item_count; ++i) {
    SDL_FRect rect;
    menu_item_rect(menu, style, i, &rect);

    if (i == menu->selected) {
      render_rect(
        renderer,
        vec2(rect.x, rect.y),
        vec2(rect.w, rect.h),
        style->selected_background
      );
    }

    float text_y = rect.y + (rect.h - menu_text_height(style->text_scale)) * 0.5f;
    float text_x = rect.x + 8.0f * style->text_scale.x;
    color_t color = i == menu->selected ? style->selected_text : style->text;

    const char* text = menu->items[i].text != NULL ? menu->items[i].text : "";

    render_text(
      renderer,
      vec2(
        text_x / style->text_scale.x,
        text_y / style->text_scale.y
      ),
      style->text_scale,
      color,
      text,
      ALIGN_LEFT
    );

    if (i == menu->selected) {
      float cursor_x = rect.x + 2.0f;
      render_text(
        renderer,
        vec2(
          cursor_x / style->text_scale.x,
          text_y / style->text_scale.y
        ),
        style->text_scale,
        style->cursor,
        ">",
        ALIGN_LEFT
      );
    }
  }
}

void menu_render(
  const menu_t* menu,
  SDL_Renderer* renderer,
  vec2_t win_size
) {
  if (menu == NULL || renderer == NULL) {
    return;
  }

  if (menu->render != NULL) {
    menu->render(menu, renderer, win_size);
    return;
  }

  menu_render_default(menu, renderer, win_size);
}
