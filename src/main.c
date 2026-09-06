#include <SDL3/SDL.h>
#include <stdbool.h>
#include <string.h>

#include "games/snake.h"
#include "games/pong.h"
#include "games/tetris.h"
#include "vec.h"

struct game_template {
  const game_t* game;
  const char* menu_name;
  const SDL_Scancode scancode;
} games[] = {
  {
    &snake_template,
    "Key 0: Snake",
    SDL_SCANCODE_0
  },
  {
    &pong_template,
    "Key 1: Pong",
    SDL_SCANCODE_1
  },
  {
    &tetris_template,
    "Key 2: Tetris",
    SDL_SCANCODE_2
  }
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

  while (running) {
    memset(&input.pressed, false, KEY_COUNT * sizeof(bool));
    memset(&input.released, false, KEY_COUNT * sizeof(bool));
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
          } else {
            /* TODO cleanup */
            if (menu) {
              for (int i = 0; i < sizeof(games) / sizeof(struct game_template); i++) {
                if (event.key.scancode == games[i].scancode) {
                  memcpy(&game, games[i].game, sizeof(game_t));
                  menu = false;
                  break;
                }
              }
              if (!menu) {
                game.init(&game, win_size);
                /* reenable vsync */
                if (!SDL_SetRenderVSync(renderer, 1)) {
                  SDL_Log("Warning: Failed to enable VSync 1: %s", SDL_GetError());
                }
              }
              break;
            }

            if (event.key.scancode == SDL_SCANCODE_BACKSPACE && !menu) {
              game.destroy(&game);
              menu = true;
              /* render only at a fourth of the display refresh rate */
              if (!SDL_SetRenderVSync(renderer, 4)) {
                SDL_Log("Warning: Failed to enable VSync 4: %s", SDL_GetError());
              }
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
      SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
      SDL_RenderClear(renderer);

      render_text(
        renderer,
        vec2(win_size.x * 0.125f, 10),
        vec2_splat(4.f),
        color_text,
        "Retro Games",
        ALIGN_MIDDLE
      );

      for (int i = 0; i < sizeof(games) / sizeof(struct game_template); i++) {
        render_text(
          renderer,
          vec2(10, 75 + i * 15),
          vec2_splat(2.f),
          color_text,
          games[i].menu_name,
          ALIGN_LEFT
        );
      }

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
