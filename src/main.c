#include <SDL3/SDL.h>
#include <stdbool.h>
#include <string.h>

#include "vec.h"
#include "menu.h"
#include "games/snake.h"
#include "games/pong.h"
#include "games/tetris.h"
#include "games/moonlander.h"

static const menu_item_t game_menu_items[] = {
  {
    .text = "Snake",
    .data = &snake_template
  },
  {
    .text = "Pong",
    .data = &pong_template
  },
  {
    .text = "Tetris",
    .data = &tetris_template
  },
  {
    .text = "Moonlander",
    .data = &moonlander_template
  }
};

static menu_t game_menu = {
  .title = "Retro Games",
  .items = game_menu_items,
  .item_count = sizeof(game_menu_items) / sizeof(game_menu_items[0]),
  .selected = 0,
  .style = NULL,
  .render = NULL
};

static enum key key_from_scancode(SDL_Scancode scancode) {
  switch (scancode) {
    case SDL_SCANCODE_W:
    case SDL_SCANCODE_UP:
      return KEY_UP;

    case SDL_SCANCODE_S:
    case SDL_SCANCODE_DOWN:
      return KEY_DOWN;

    case SDL_SCANCODE_A:
    case SDL_SCANCODE_LEFT:
      return KEY_LEFT;

    case SDL_SCANCODE_D:
    case SDL_SCANCODE_RIGHT:
      return KEY_RIGHT;

    case SDL_SCANCODE_SPACE:
    case SDL_SCANCODE_RETURN:
      return KEY_ACTION;

    case SDL_SCANCODE_ESCAPE:
      return KEY_PAUSE;

    default:
      return KEY_COUNT;
  }
}

static enum button button_from_sdl(Uint8 button) {
  switch (button) {
    case SDL_BUTTON_LEFT:
      return BUTTON_LEFT;

    case SDL_BUTTON_MIDDLE:
      return BUTTON_MIDDLE;

    case SDL_BUTTON_RIGHT:
      return BUTTON_RIGHT;

    default:
      return BUTTON_COUNT;
  }
}

int main(int argc, char** argv) {
  SDL_Window* window = NULL;
  SDL_Renderer* renderer = NULL;
  SDL_Event event;
  game_t game;
  input_t input;
  bool running = true;
  bool menu = true;
  const vec2_t win_size = vec2(800, 640);
  const color_t color_text = (color_t){ 255, 255, 255, 255 };

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  if (!SDL_CreateWindowAndRenderer(
    "Retro Games",
    win_size.x, win_size.y,
    0,
    &window, &renderer
  )) {
    SDL_Log("CreateWindowAndRenderer failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  if (!SDL_SetRenderVSync(renderer, 4)) {
    SDL_Log("Warning: Failed to enable VSync 4: %s", SDL_GetError());
    /* fallback to vsync if the driver doesn't support rendering ¼ vsync*/
    if (!SDL_SetRenderVSync(renderer, 1)) {
      SDL_Log("Warning: Failed to enable VSync 1: %s", SDL_GetError());
    }
  }

  Uint64 previous_time = SDL_GetTicksNS();
  memset(&input, 0, sizeof(input_t));

  menu_init(&game_menu);

  while (running) {
    memset(&input.pressed, false, KEY_COUNT * sizeof(bool));
    memset(&input.released, false, KEY_COUNT * sizeof(bool));
    memset(&input.mouse.pressed, false, sizeof(input.mouse.pressed));
    memset(&input.mouse.released, false, sizeof(input.mouse.released));
    input.mouse.moved = false;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_QUIT: {
          running = false;
          break;
        }
        case SDL_EVENT_KEY_DOWN: {
          enum key k = key_from_scancode(event.key.scancode);

          if (k != KEY_COUNT) {
            if (!event.key.repeat && !input.down[k]) {
              input.pressed[k] = true;
            }
            input.down[k] = true;
          } else if (event.key.scancode == SDL_SCANCODE_BACKSPACE && !menu) {
            game.destroy(&game);
            menu = true;

            if (!SDL_SetRenderVSync(renderer, 4)) {
              SDL_Log(
                "Warning: Failed to enable VSync 4: %s",
                SDL_GetError()
              );
            }
          }
          break;
        }
        case SDL_EVENT_KEY_UP: {
          enum key k = key_from_scancode(event.key.scancode);

          if (k != KEY_COUNT) {
            input.down[k] = false;
            input.released[k] = true;
          }
          break;
        }
        case SDL_EVENT_MOUSE_MOTION: {
          input.mouse.position = vec2(
            event.motion.x,
            event.motion.y
          );
          input.mouse.moved = true;
          break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
          enum button button = button_from_sdl(event.button.button);

          input.mouse.position = vec2(
            event.button.x,
            event.button.y
          );

          if (button != BUTTON_COUNT) {
            if (!input.mouse.down[button]) {
              input.mouse.pressed[button] = true;
            }

            input.mouse.down[button] = true;
          }

          break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
          enum button button = button_from_sdl(event.button.button);

          input.mouse.position = vec2(
            event.button.x,
            event.button.y
          );

          if (button != BUTTON_COUNT) {
            input.mouse.down[button] = false;
            input.mouse.released[button] = true;
          }

          break;
        }
      }
    }

    Uint64 current_time = SDL_GetTicksNS();
    float delta_time = (float)(current_time - previous_time) / 1000000000.0f;
    previous_time = current_time;

    /* Prevent huge jumps after pausing, resizing, or debugging */
    if (delta_time > 0.1f) {
      delta_time = 0.1f;
    }

    if (menu) {
      /* UPDATE */
      menu_event_t event = menu_update(&game_menu, &input);

      if (event.type == MENU_EVENT_ACTIVATE) {
        const game_t* selected_game =
        game_menu.items[event.index].data;

        memcpy(&game, selected_game, sizeof(game_t));
        menu = false;

        game.init(&game, win_size);

        if (!SDL_SetRenderVSync(renderer, 1)) {
          SDL_Log(
            "Warning: Failed to enable VSync 1: %s",
            SDL_GetError()
          );
        }
      }

      /* RENDER */
      menu_render(&game_menu, renderer, win_size);
    } else {
      game.update(&game, &input, delta_time);

      SDL_SetRenderDrawColor(renderer,
        game.color_bg.r, game.color_bg.g, game.color_bg.b, 255
      );
      SDL_RenderClear(renderer);

      game.render(&game, renderer);
    }

    SDL_RenderPresent(renderer);
  }

  if (!menu) {
    game.destroy(&game);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
