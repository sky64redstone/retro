#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "moonlander.h"
#include "vec.h"

#define TERRAIN_MIN_Y 0.62f
#define TERRAIN_MAX_Y 0.88f
#define TERRAIN_MAX_SLOPE 0.45f
#define TERRAIN_POINTS 15

#define STAR_DENSITY 1000000.f
#define STAR_MIN_SIZE 1.f
#define STAR_MAX_SIZE 4.f

#define PI 3.1415926f
#define PHYSICS_STEP (1.f / 120.f)
#define GRAVITY 72.f
#define THRUST 220.f
#define FUEL_CONSUMPTION 18.f
#define ROTATION_SPEED 2.4f
#define SHIP_HEIGHT 28.f
#define SHIP_WIDTH 22.f

#define LANDING_ANGLE 0.18f
#define LANDING_VERTICAL_SPEED 75.f
#define LANDING_HORIZONTAL_SPEED 65.f

#define MAX_FUEL 70.f

static void moonlander_init(game_t* game, vec2_t win_size);
static void moonlander_reset(game_t* game, vec2_t win_size);
static void moonlander_update(game_t* game, input_t* input, float dt);
static void moonlander_render(game_t* game, SDL_Renderer* renderer);
static void moonlander_destroy(game_t* game);

static const color_t color_white = { 235, 240, 255, 255 };
static const color_t color_ship = { 210, 220, 235, 255 };
static const color_t color_flame = { 255, 170, 70, 255 };
static const color_t color_terrain = { 110, 105, 120, 255 };
static const color_t color_pad = { 90, 220, 130, 255 };
static const color_t color_hud = { 30, 34, 58, 230 };
static const color_t color_warning = { 255, 90, 90, 255 };
static const color_t color_star = { 180, 190, 220, 255 };
static const color_t color_background = { 8, 10, 24, 255 };

const struct game moonlander_template = {
  .init = moonlander_init,
  .reset = moonlander_reset,
  .update = moonlander_update,
  .render = moonlander_render,
  .destroy = moonlander_destroy,
  .data = NULL,
  .state = GAME_RUNNING,
  .color_bg = color_background
};

typedef struct moonlander_star {
  float x;
  float y;
  float size;
  color_t color;
} moonlander_star_t;

struct moonlander_data {
  struct {
    vec2_t pos;
    vec2_t vel;
    float angle;
  } ship;
  vec2_t terrain[TERRAIN_POINTS];
  float fuel;
  float physics_accumulator;
  int thrusting;
  vec2_t win_size;
  float pad_start;
  float pad_end;
  float pad_y;
  moonlander_star_t* stars;
  int star_count;
};

typedef struct moonlander_data moonlander_data_t;

static void moonlander_data_init(moonlander_data_t* data, vec2_t win_size);
static float moonlander_terrain_height(const moonlander_data_t* data, float x);
static void moonlander_crash(game_t* game);
static void moonlander_land(game_t* game);
static float moonlander_wrap_angle(float angle);
static void moonlander_render_stars(
  SDL_Renderer* renderer, const moonlander_data_t* data
);
static float moonlander_randomf(float min, float max);
static void moonlander_generate_terrain(moonlander_data_t* data);
static void moonlander_generate_stars(moonlander_data_t* data);
static uint32_t moonlander_star_hash(uint32_t value);
static float moonlander_star_random(uint32_t value);
static void moonlander_render_ship(
  SDL_Renderer* renderer, vec2_t pos, float sx, float cx,
  float width, float height, color_t color
);
static void moonlander_simulate(
  moonlander_data_t* data, game_t* game, input_t* input
);

static void moonlander_init(game_t* game, vec2_t win_size) {
  assert(game);
  assert(!game->data);

  srand((unsigned)time(NULL));

  moonlander_data_t* data = calloc(1, sizeof(moonlander_data_t));
  assert(data);
  game->data = data;
  moonlander_data_init(data, win_size);
  game->state = GAME_RUNNING;
}

static void moonlander_reset(game_t* game, vec2_t win_size) {
  assert(game);
  assert(game->data);

  moonlander_data_init((moonlander_data_t*)game->data, win_size);
  game->state = GAME_RUNNING;
}

static void moonlander_update(game_t* game, input_t* input, float dt) {
  assert(game);
  assert(game->data);
  assert(input);

  moonlander_data_t* data = (moonlander_data_t*)game->data;

  switch (game->state) {
    case GAME_RUNNING: {
      if (input->pressed[KEY_PAUSE]) {
        game->state = GAME_PAUSED;
        break;
      }

      data->physics_accumulator += dt;
      while (data->physics_accumulator >= PHYSICS_STEP &&
        game->state == GAME_RUNNING) {
        data->physics_accumulator -= PHYSICS_STEP;
        moonlander_simulate(data, game, input);
      }
      break;
    }
    case GAME_PAUSED: {
      if (input->pressed[KEY_PAUSE]) {
        game->state = GAME_RUNNING;
      }
      break;
    }
    case GAME_OVER:
    case GAME_WON: {
      if (input->pressed[KEY_ACTION]) {
        game->reset(game, data->win_size);
      }
      break;
    }
  }
}

static void moonlander_render(game_t* game, SDL_Renderer* renderer) {
  assert(game);
  assert(game->data);
  assert(renderer);

  moonlander_data_t* data = (moonlander_data_t*)game->data;

  moonlander_render_stars(renderer, data);

  /* TERRAIN */
  for (int i = 0; i < TERRAIN_POINTS - 1; i++) {
    const vec2_t a = data->terrain[i];
    const vec2_t b = data->terrain[i + 1];
    const vec2_t c = vec2(b.x, data->win_size.y);
    const vec2_t d = vec2(a.x, data->win_size.y);
    render_triangle(
      renderer, a, b, c, color_terrain
    );
    render_triangle(
      renderer, a, c, d, color_terrain
    );
  }

  /* LANDING PAD */
  render_rect(
    renderer,
    vec2(data->pad_start, data->pad_y - 2),
    vec2(data->pad_end - data->pad_start, 4),
    color_pad
  );

  /* SHIP */
  const float ship_sx = sinf(data->ship.angle);
  const float ship_cx = cosf(data->ship.angle);

  if (data->thrusting && game->state == GAME_RUNNING) {
    const float sx = ship_sx;
    const float cx = ship_cx;
    const vec2_t flame_start = vec2(
      data->ship.pos.x - sx * 9.f,
      data->ship.pos.y + cx * 9.f
    );
    const vec2_t flame_end = vec2(
      data->ship.pos.x - sx * 23.f,
      data->ship.pos.y + cx * 23.f
    );
    SDL_SetRenderDrawColor(
      renderer, color_flame.r, color_flame.g, color_flame.b, color_flame.a
    );
    SDL_RenderLine(
      renderer, flame_start.x - cx * 4.f, flame_start.y - sx * 4.f,
      flame_end.x, flame_end.y
    );
    SDL_RenderLine(
      renderer, flame_start.x + cx * 4.f, flame_start.y + sx * 4.f,
      flame_end.x, flame_end.y
    );
  }
  moonlander_render_ship(
    renderer,
    data->ship.pos,
    ship_sx,
    ship_cx,
    SHIP_WIDTH,
    SHIP_HEIGHT,
    color_ship
  );

  /* HUD */
  vec2_t hud_pos = vec2(20, 20);
  vec2_t hud_size = vec2(data->win_size.x / 2.5f, data->win_size.y / 7);
  render_rect(renderer, hud_pos, hud_size, color_hud);

  float hud_padding = 15;
  char hud[64] = { 0 };
  snprintf(hud, sizeof(hud), "FUEL %3.0f", data->fuel);
  render_text(
    renderer,
    vec2_scale(vec2(hud_pos.x + hud_padding, hud_pos.y + hud_padding), 0.5f),
    vec2_splat(2.f),
    data->fuel < 20.f ? color_warning : color_white,
    hud,
    ALIGN_LEFT
  );

  snprintf(hud, sizeof(hud), "VX %4.0f", data->ship.vel.x);
  render_text(
    renderer,
    vec2_scale(vec2(hud_pos.x + hud_padding, hud_pos.y + hud_size.y - hud_padding - 2*8), 0.5f),
    vec2_splat(2.f),
    color_white,
    hud,
    ALIGN_LEFT
  );

  snprintf(hud, sizeof(hud), "VY %4.0f", data->ship.vel.y);
  render_text(
    renderer,
    vec2_scale(vec2(hud_pos.x + hud_size.x - hud_padding, hud_pos.y + hud_size.y - hud_padding - 2*8), 0.5f),
    vec2_splat(2.f),
    color_white,
    hud,
    ALIGN_RIGHT
  );

  switch (game->state) {
    case GAME_RUNNING:
      break;
    case GAME_PAUSED:
      render_rect(
        renderer,
        vec2(data->win_size.x / 6.f, data->win_size.y / 3.f),
        vec2(data->win_size.x * 2.f / 3.f, 130),
        color_hud
      );
      render_text(
        renderer,
        vec2(data->win_size.x / 8.f, data->win_size.y / 11.f),
        vec2_splat(4.f),
        color_white,
        "PAUSED",
        ALIGN_MIDDLE
      );
      render_text(
        renderer,
        vec2(data->win_size.x / 4.f, data->win_size.y / 6.f + 40),
        vec2_splat(2.f),
        color_white,
        "PRESS <PAUSE> TO CONTINUE",
        ALIGN_MIDDLE
      );
      break;
    case GAME_OVER:
      render_rect(
        renderer,
        vec2(data->win_size.x / 6.f, data->win_size.y / 3.f),
        vec2(data->win_size.x * 2.f / 3.f, 130),
        color_hud
      );
      render_text(
        renderer,
        vec2(data->win_size.x / 8.f, data->win_size.y / 11.f),
        vec2_splat(4.f),
        color_warning,
        "CRASH",
        ALIGN_MIDDLE
      );
      render_text(
        renderer,
        vec2(data->win_size.x / 4.f, data->win_size.y / 6.f + 40),
        vec2_splat(2.f),
        color_white,
        "PRESS <ACTION> TO PLAY AGAIN",
        ALIGN_MIDDLE
      );
      break;
    case GAME_WON:
      render_rect(
        renderer,
        vec2(data->win_size.x / 6.f, data->win_size.y / 3.f),
        vec2(data->win_size.x * 2.f / 3.f, 130),
        color_hud
      );
      render_text(
        renderer,
        vec2(data->win_size.x / 8.f, data->win_size.y / 11.f),
        vec2_splat(4.f),
        color_pad,
        "LANDED",
        ALIGN_MIDDLE
      );
      render_text(
        renderer,
        vec2(data->win_size.x / 4.f, data->win_size.y / 6.f + 40),
        vec2_splat(2.f),
        color_white,
        "PRESS <ACTION> TO PLAY AGAIN",
        ALIGN_MIDDLE
      );
      break;
  }
}

static void moonlander_destroy(game_t* game) {
  assert(game);
  assert(game->data);

  moonlander_data_t* data = (moonlander_data_t*)game->data;
  free(data->stars);
  free(data);
  game->data = NULL;
}

static void moonlander_data_init(moonlander_data_t* data, vec2_t win_size) {
  free(data->stars);

  *data = (moonlander_data_t){
    .ship = {
      .pos = vec2(win_size.x * 0.2f, win_size.y * 0.18f),
      .vel = vec2_zero(),
      .angle = 0.f
    },
    .fuel = MAX_FUEL,
    .physics_accumulator = 0.f,
    .thrusting = 0,
    .win_size = win_size,
    .pad_start = 0,
    .pad_end = 0,
    .pad_y = 0,
    .stars = NULL,
    .star_count = 0
  };

  moonlander_generate_terrain(data);
  moonlander_generate_stars(data);
}

static void moonlander_simulate(
  moonlander_data_t* data, game_t* game, input_t* input
) {
  const float dt = PHYSICS_STEP;

  if (input->down[KEY_LEFT]) {
    data->ship.angle -= ROTATION_SPEED * dt;
  }
  if (input->down[KEY_RIGHT]) {
    data->ship.angle += ROTATION_SPEED * dt;
  }
  data->ship.angle = moonlander_wrap_angle(data->ship.angle);

  data->thrusting = input->down[KEY_UP] && data->fuel > 0.f;
  data->ship.vel.y += GRAVITY * dt;

  if (data->thrusting) {
    const float sx = sinf(data->ship.angle);
    const float cx = cosf(data->ship.angle);
    const float thrust = THRUST * dt;

    data->ship.vel.x += sx * thrust;
    data->ship.vel.y -= cx * thrust;
    data->fuel -= FUEL_CONSUMPTION * dt;
    if (data->fuel < 0.f) {
      data->fuel = 0.f;
    }
  }

  data->ship.pos.x += data->ship.vel.x * dt;
  data->ship.pos.y += data->ship.vel.y * dt;

  const float contact_y = data->ship.pos.y + SHIP_HEIGHT * 0.5f;
  const float terrain_y = moonlander_terrain_height(data, data->ship.pos.x);
  if (data->ship.pos.x < 0.f || data->ship.pos.x > data->win_size.x) {
    moonlander_crash(game);
    return;
  }

  if (contact_y >= terrain_y) {
    data->ship.pos.y = terrain_y - SHIP_HEIGHT * 0.5f;

    const int on_pad = data->ship.pos.x >= data->pad_start &&
                       data->ship.pos.x <= data->pad_end;
    const int safe = fabsf(data->ship.angle) <= LANDING_ANGLE &&
                     fabsf(data->ship.vel.x) <= LANDING_HORIZONTAL_SPEED &&
                     fabsf(data->ship.vel.y) <= LANDING_VERTICAL_SPEED;

    if (on_pad && safe) {
      data->ship.vel = vec2_zero();
      moonlander_land(game);
    } else {
      moonlander_crash(game);
    }
  }
}

static float moonlander_terrain_height(
  const moonlander_data_t* data, float x
) {
  if (x <= data->terrain[0].x) {
    return data->terrain[0].y;
  }
  for (int i = 0; i < TERRAIN_POINTS - 1; i++) {
    const vec2_t a = data->terrain[i];
    const vec2_t b = data->terrain[i + 1];
    if (x <= b.x) {
      const float t = (x - a.x) / (b.x - a.x);
      return a.y + (b.y - a.y) * t;
    }
  }
  return data->terrain[TERRAIN_POINTS - 1].y;
}

static void moonlander_crash(game_t* game) {
  game->state = GAME_OVER;
}

static void moonlander_land(game_t* game) {
  game->state = GAME_WON;
}

static float moonlander_wrap_angle(float angle) {
  while (angle > PI) {
    angle -= 2.f * PI;
  }
  while (angle < -PI) {
    angle += 2.f * PI;
  }
  return angle;
}

static void moonlander_render_ship(
  SDL_Renderer* renderer, vec2_t pos, float sx, float cx,
  float width, float height, color_t color
) {
  const vec2_t dir = vec2(sx, -cx);
  const vec2_t side = vec2(cx, sx);
  const vec2_t nose = vec2_add(pos, vec2_scale(dir, height * 0.6f));
  const vec2_t base = vec2_add(pos, vec2_scale(dir, -height * 0.4f));
  const vec2_t left = vec2_add(base, vec2_scale(side, width * 0.5f));
  const vec2_t right = vec2_add(base, vec2_scale(side, -width * 0.5f));

  render_triangle(renderer, nose, left, right, (color_t){0,0,0,255});
  draw_triangle(renderer, nose, left, right, color);
}

static float moonlander_randomf(float min, float max) {
  float t = (float)rand() / (float)RAND_MAX;
  return min + (max - min) * t;
}

static void moonlander_generate_terrain(moonlander_data_t* data) {
  _Static_assert(
    TERRAIN_POINTS >= 4,
    "TERRAIN_POINTS must be at least 4"
  );

  const float width = data->win_size.x;
  const float height = data->win_size.y;

  /*
   * The pad position is randomized, but kept away from both edges.
   * Its width stays constant relative to the screen.
   */
  data->pad_start = moonlander_randomf(0.68f, 0.76f) * width;
  data->pad_end = data->pad_start + 0.12f * width;
  data->pad_y = moonlander_randomf(0.70f, 0.80f) * height;

  /*
   * Two points are reserved for the landing pad.
   * The remaining points are split between the left and right sides.
   */
  const int terrain_points = TERRAIN_POINTS - 2;
  const int left_points = (terrain_points + 1) / 2;
  const int right_points = terrain_points - left_points;

  const int pad_start_index = left_points;
  const int pad_end_index = left_points + 1;

  /*
   * Landing pad.
   *
   * These two points are consecutive, so the interpolation code will
   * always produce a completely flat landing zone.
   */
  data->terrain[pad_start_index] = (vec2_t){
    data->pad_start,
    data->pad_y
  };

  data->terrain[pad_end_index] = (vec2_t){
    data->pad_end,
    data->pad_y
  };

  /*
   * Generate the terrain to the left of the pad.
   *
   * We generate backwards starting from the pad. This guarantees that
   * the final terrain segment also satisfies TERRAIN_MAX_SLOPE.
   */
  float next_y = data->pad_y;
  float next_x = data->pad_start;

  for (int i = left_points - 1; i >= 0; i--) {
    float x;

    if (left_points == 1) {
      x = 0.f;
    } else {
      x = data->pad_start * (float)i / (float)(left_points - 1);
    }

    const float dx = next_x - x;
    const float max_delta = TERRAIN_MAX_SLOPE * dx;

    const float min_y = fmaxf(
      height * TERRAIN_MIN_Y,
      next_y - max_delta
    );
    const float max_y = fminf(
      height * TERRAIN_MAX_Y,
      next_y + max_delta
    );

    const float y = moonlander_randomf(min_y, max_y);

    data->terrain[i] = (vec2_t){ x, y };

    next_x = x;
    next_y = y;
  }

  /*
   * Generate the terrain to the right of the pad.
   *
   * Same invariant as on the left, but generated forwards.
   */
  next_x = data->pad_end;
  next_y = data->pad_y;

  for (int i = 0; i < right_points; i++) {
    float x;

    if (right_points == 1) {
      x = width;
    } else {
      x = data->pad_end +
      (width - data->pad_end) *
      (float)(i + 1) / (float)right_points;
    }

    const float dx = x - next_x;
    const float max_delta = TERRAIN_MAX_SLOPE * dx;

    const float min_y = fmaxf(
      height * TERRAIN_MIN_Y,
      next_y - max_delta
    );
    const float max_y = fminf(
      height * TERRAIN_MAX_Y,
      next_y + max_delta
    );

    const float y = moonlander_randomf(min_y, max_y);

    data->terrain[pad_end_index + 1 + i] =
    (vec2_t){ x, y };

    next_x = x;
    next_y = y;
  }
}

static uint32_t moonlander_star_hash(uint32_t value) {
  value ^= value >> 16;
  value *= 0x7feb352dU;
  value ^= value >> 15;
  value *= 0x846ca68bU;
  value ^= value >> 16;
  return value;
}

static float moonlander_star_random(uint32_t value) {
  return (float)moonlander_star_hash(value) / (float)UINT32_MAX;
}

static void moonlander_generate_stars(moonlander_data_t* data) {
  const float width = data->win_size.x;
  const float height = data->win_size.y;
  int count = (int)((width * height) / STAR_DENSITY);

  if (count < 32) {
    count = 32;
  }

  data->stars = malloc((size_t)count * sizeof(*data->stars));
  assert(data->stars);
  data->star_count = count;

  uint32_t star_seed = rand();

  for (int i = 0; i < count; i++) {
    const uint32_t index = (uint32_t)i;
    const uint32_t position_seed = star_seed + index * 3u;
    const float brightness = 140.f +
      moonlander_star_random(star_seed + index * 7u + 3u) * 115.f;

    data->stars[i] = (moonlander_star_t){
      .x = moonlander_star_random(position_seed) * width,
      .y = moonlander_star_random(position_seed + 1u) * height * 0.65f,
      .size = STAR_MIN_SIZE +
        moonlander_star_random(position_seed + 2u) *
        (STAR_MAX_SIZE - STAR_MIN_SIZE),
      .color = {
        (uint8_t)brightness,
        (uint8_t)brightness,
        (uint8_t)brightness,
        255
      }
    };
  }
}

static void moonlander_render_stars(
  SDL_Renderer* renderer,
  const moonlander_data_t* data
) {
  for (int i = 0; i < data->star_count; i++) {
    const moonlander_star_t* star = &data->stars[i];

    render_rect(
      renderer,
      vec2(star->x, star->y),
      vec2_splat(star->size),
      star->color
    );
  }
}
