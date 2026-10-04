#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pong.h"
#include "menu.h"

#define PONG_BOT_EASY_REACTION 0.20f
#define PONG_BOT_EASY_DEADZONE 12.0f

#define PONG_BOT_NORMAL_REACTION 0.10f
#define PONG_BOT_NORMAL_DEADZONE 6.0f

#define PONG_BOT_HARD_REACTION 0.10f
#define PONG_BOT_HARD_DEADZONE 10.0f

#define PONG_PLAYER_SPEED 200.0f
#define PONG_INITIAL_BALL_SPEED 280.0f
#define PONG_MAX_BALL_SPEED 420.0f
#define PONG_BALL_SPEED_STEP 18.0f
#define PONG_BALL_BOUNCE_EPSILON 0.01f
#define PONG_MAX_BOUNCE_ANGLE (60.0f * 0.01745329251994329577f)
#define PONG_MIN_SERVE_ANGLE (12.0f * 0.01745329251994329577f)
#define PONG_MAX_SERVE_ANGLE (30.0f * 0.01745329251994329577f)
#define PONG_SERVE_DELAY 0.55f
#define PONG_BOT_CENTER_Y 0.5f
#define PONG_BOT_TARGET_EPSILON 0.5f
#define PONG_CENTER_LINE_SPACING 20
#define PONG_CENTER_LINE_WIDTH 2.0f
#define PONG_CENTER_LINE_HEIGHT 10.0f
#define PONG_BUF_SIZE 16

enum pong_difficulty {
  PONG_DIFFICULTY_EASY,
  PONG_DIFFICULTY_NORMAL,
  PONG_DIFFICULTY_HARD,
  PONG_DIFFICULTY_COUNT
};

enum pong_side {
  PONG_SIDE_NONE,
  PONG_SIDE_PLAYER,
  PONG_SIDE_BOT
};

typedef struct pong_paddle pong_paddle_t;
typedef struct pong_ball pong_ball_t;
typedef struct pong_data pong_data_t;

struct pong_paddle {
  vec2_t pos;
  vec2_t size;
  int score;
};

struct pong_ball {
  vec2_t pos;
  vec2_t size;
  vec2_t vel;
};

struct pong_data {
  pong_paddle_t p1;
  pong_paddle_t p2;
  pong_ball_t ball;
  vec2_t win_size;

  enum pong_difficulty difficulty;
  enum pong_side last_point;
  menu_t difficulty_menu;
  bool selecting_difficulty;
  float bot_reaction_timer;
  int bot_direction;
  float serve_timer;
};

static void pong_init(game_t* game, vec2_t win_size);
static void pong_reset(game_t* game, vec2_t win_size);
static void pong_update(game_t* game, input_t* input, float dt);
static void pong_render(game_t* game, SDL_Renderer* renderer);
static void pong_destroy(game_t* game);

static void pong_data_init(pong_data_t* data, vec2_t win_size);
static void pong_start_round(pong_data_t* data, enum pong_side last_point);
static void pong_launch_ball(pong_data_t* data);
static void pong_move_paddle(
  pong_paddle_t* paddle,
  int direction,
  float dt,
  vec2_t win_size
);
static void pong_update_bot(pong_data_t* data, float dt);
static int pong_bot_direction(pong_data_t* data, float target_y);
static float pong_predict_linear_y(const pong_data_t* data, float target_x);
static float pong_predict_bounced_y(const pong_data_t* data, float target_x);
static float pong_bot_target_y(const pong_data_t* data);
static float pong_bot_reaction_time(enum pong_difficulty difficulty);
static float pong_bot_deadzone(enum pong_difficulty difficulty);
static void pong_clamp_paddle(pong_paddle_t* paddle, vec2_t win_size);
static void pong_bounce_from_paddle(
  pong_data_t* data,
  const pong_paddle_t* paddle,
  int horizontal_direction
);
static float pong_random_unit(void);
static float pong_clamp(float value, float min, float max);

static const enum pong_difficulty pong_difficulty_values[] = {
  PONG_DIFFICULTY_EASY,
  PONG_DIFFICULTY_NORMAL,
  PONG_DIFFICULTY_HARD
};

static const menu_item_t pong_difficulty_items[] = {
  {
    .text = "EASY",
    .data = &pong_difficulty_values[PONG_DIFFICULTY_EASY]
  },
  {
    .text = "NORMAL",
    .data = &pong_difficulty_values[PONG_DIFFICULTY_NORMAL]
  },
  {
    .text = "HARD",
    .data = &pong_difficulty_values[PONG_DIFFICULTY_HARD]
  }
};

static const color_t color_player = { 255, 255, 255, 255 };
static const color_t color_ball = { 205, 205, 255, 255 };
static const color_t color_line = { 255, 255, 255, 255 };
static const color_t color_menu = { 30, 30, 40, 255 };
static const color_t color_white = { 255, 255, 255, 255 };

const struct game pong_template = {
  .init = pong_init,
  .reset = pong_reset,
  .update = pong_update,
  .render = pong_render,
  .destroy = pong_destroy,
  .data = NULL,
  .state = GAME_PAUSED,
  .color_bg = { 0, 0, 0, 255 }
};

static void pong_init(game_t* game, vec2_t win_size) {
  assert(game);
  assert(!game->data);

  pong_data_t* data = calloc(1, sizeof(*data));
  assert(data);
  game->data = data;

  pong_data_init(data, win_size);
}

static void pong_reset(game_t* game, vec2_t win_size) {
  assert(game);
  assert(game->data);

  memset(game->data, 0, sizeof(pong_data_t));
  pong_data_init((pong_data_t*)game->data, win_size);
}

static void pong_update(game_t* game, input_t* input, float dt) {
  assert(game);
  assert(game->data);
  assert(input);

  pong_data_t* data = (pong_data_t*)game->data;

  if (data->selecting_difficulty) {
    const menu_event_t event = menu_update(&data->difficulty_menu, input);
    if (event.type != MENU_EVENT_ACTIVATE) {
      return;
    }

    const menu_item_t* item = &data->difficulty_menu.items[event.index];
    const enum pong_difficulty difficulty = *(const enum pong_difficulty*)item->data;
    data->difficulty = difficulty;
    data->selecting_difficulty = false;
    pong_start_round(data, PONG_SIDE_NONE);
    game->state = GAME_RUNNING;
    return;
  }

  if (game->state == GAME_PAUSED) {
    if (input->pressed[KEY_ACTION]) {
      game->state = GAME_RUNNING;
    }
    return;
  }

  if (game->state == GAME_RUNNING && input->pressed[KEY_PAUSE]) {
    game->state = GAME_PAUSED;
    return;
  }

  pong_move_paddle(
    &data->p1,
    input->down[KEY_UP] ? -1 : input->down[KEY_DOWN] ? 1 : 0,
    dt,
    data->win_size
  );
  pong_update_bot(data, dt);

  if (data->serve_timer > 0.0f) {
    data->serve_timer -= dt;
    if (data->serve_timer <= 0.0f) {
      data->serve_timer = 0.0f;
      pong_launch_ball(data);
    }
    return;
  }

  float remaining_dt = dt;
  while (remaining_dt > 0.0f) {
    const float step_dt = fminf(remaining_dt, 0.02f);
    data->ball.pos = vec2_add(data->ball.pos, vec2_scale(data->ball.vel, step_dt));

    const float max_ball_y = data->win_size.y - data->ball.size.y;
    if (data->ball.pos.y <= 0.0f) {
      data->ball.pos.y = 0.0f;
      data->ball.vel.y = fabsf(data->ball.vel.y);
    } else if (data->ball.pos.y >= max_ball_y) {
      data->ball.pos.y = max_ball_y;
      data->ball.vel.y = -fabsf(data->ball.vel.y);
    }

    if (data->ball.pos.x + data->ball.size.x <= 0.0f) {
      data->p2.score++;
      pong_start_round(data, PONG_SIDE_BOT);
      return;
    }

    if (data->ball.pos.x >= data->win_size.x) {
      data->p1.score++;
      pong_start_round(data, PONG_SIDE_PLAYER);
      return;
    }

    const rect_t ball_rect = rect(data->ball.pos, data->ball.size);
    const rect_t p1_rect = rect(data->p1.pos, data->p1.size);
    const rect_t p2_rect = rect(data->p2.pos, data->p2.size);

    if (data->ball.vel.x < 0.0f && collision_aabb(ball_rect, p1_rect)) {
      data->ball.pos.x =
      data->p1.pos.x + data->p1.size.x + PONG_BALL_BOUNCE_EPSILON;
      pong_bounce_from_paddle(data, &data->p1, 1);
    } else if (data->ball.vel.x > 0.0f && collision_aabb(ball_rect, p2_rect)) {
      data->ball.pos.x =
      data->p2.pos.x - data->ball.size.x - PONG_BALL_BOUNCE_EPSILON;
      pong_bounce_from_paddle(data, &data->p2, -1);
    }

    remaining_dt -= step_dt;
  }
}

static void pong_render(game_t* game, SDL_Renderer* renderer) {
  assert(game);
  assert(game->data);
  assert(renderer);

  const pong_data_t* data = (const pong_data_t*)game->data;

  if (data->selecting_difficulty) {
    menu_render(&data->difficulty_menu, renderer, data->win_size);
    return;
  }

  render_rect(renderer, data->p1.pos, data->p1.size, color_player);
  render_rect(renderer, data->p2.pos, data->p2.size, color_player);
  render_rect(renderer, data->ball.pos, data->ball.size, color_ball);

  const vec2_t center_line_size = vec2(PONG_CENTER_LINE_WIDTH, PONG_CENTER_LINE_HEIGHT);
  const int line_count = (int)(data->win_size.y / PONG_CENTER_LINE_SPACING);
  const float line_x = (data->win_size.x - center_line_size.x) * 0.5f;

  for (int i = 0; i < line_count; ++i) {
    render_rect(
      renderer,
      vec2(line_x, PONG_CENTER_LINE_SPACING * i),
      center_line_size,
      color_line
    );
  }

  char score[PONG_BUF_SIZE] = {
    [PONG_BUF_SIZE - 1] = '\0'
  };
  snprintf(score, sizeof(score), "Score: %i", data->p1.score);
  render_text(
    renderer,
    vec2(10.0f, 10.0f),
    vec2_splat(2.0f),
    color_white,
    score,
    ALIGN_LEFT
  );

  snprintf(score, sizeof(score), "Score: %i", data->p2.score);
  render_text(
    renderer,
    vec2(data->win_size.x * 0.5f - 10.0f, 10.0f),
    vec2_splat(2.0f),
    color_white,
    score,
    ALIGN_RIGHT
  );

  if (data->serve_timer > 0.0f) {
    const char* message = "GET READY";
    if (data->last_point == PONG_SIDE_PLAYER) {
      message = "YOU SCORE";
    } else if (data->last_point == PONG_SIDE_BOT) {
      message = "BOT SCORES";
    }

    render_text(
      renderer,
      vec2(data->win_size.x * 0.5f / 2.0f, data->win_size.y * 0.5f / 2.0f),
      vec2_splat(3.0f),
      color_white,
      message,
      ALIGN_MIDDLE
    );
  }

  if (game->state == GAME_PAUSED) {
    const vec2_t pos = vec2_scale(data->win_size, 1.0f / 6.0f);
    const vec2_t size = vec2_scale(pos, 4.0f);
    render_rect(renderer, pos, size, color_menu);
    render_text(
      renderer,
      vec2(pos.x * 0.75f, pos.y * 0.25f + 10.0f),
      vec2_splat(4.0f),
      color_white,
      "PAUSED",
      ALIGN_MIDDLE
    );
    render_text(
      renderer,
      vec2(pos.x * 1.5f, pos.y),
      vec2_splat(2.0f),
      color_white,
      "PRESS <ACTION> TO CONTINUE",
      ALIGN_MIDDLE
    );
  }
}

static void pong_destroy(game_t* game) {
  assert(game);
  assert(game->data);

  free(game->data);
  game->data = NULL;
}

static void pong_data_init(pong_data_t* data, vec2_t win_size) {
  const vec2_t paddle_size = vec2(20.0f, win_size.y / 6.0f);
  const vec2_t ball_size = vec2(20.0f, 20.0f);

  *data = (pong_data_t){
    .p1 = {
      .pos = vec2(10.0f, (win_size.y - paddle_size.y) * PONG_BOT_CENTER_Y),
      .size = paddle_size,
      .score = 0
    },
    .p2 = {
      .pos = vec2(
        win_size.x - 10.0f - paddle_size.x,
        (win_size.y - paddle_size.y) * PONG_BOT_CENTER_Y
      ),
      .size = paddle_size,
      .score = 0
    },
    .ball = {
      .pos = vec2(
        (win_size.x - ball_size.x) * 0.5f,
        (win_size.y - ball_size.y) * 0.5f
      ),
      .size = ball_size,
      .vel = vec2_zero()
    },
    .win_size = win_size,
    .difficulty = PONG_DIFFICULTY_NORMAL,
    .last_point = PONG_SIDE_NONE,
    .difficulty_menu = {
      .title = "PONG DIFFICULTY",
      .items = pong_difficulty_items,
      .item_count = sizeof(pong_difficulty_items) / sizeof(pong_difficulty_items[0]),
      .selected = PONG_DIFFICULTY_NORMAL,
      .style = NULL,
      .render = NULL
    },
    .selecting_difficulty = true,
    .bot_reaction_timer = 0.0f,
    .bot_direction = 0,
    .serve_timer = 0.0f
  };

  menu_init(&data->difficulty_menu);
  data->difficulty_menu.selected = PONG_DIFFICULTY_NORMAL;
}

static void pong_start_round(pong_data_t* data, enum pong_side last_point) {
  const float paddle_y = (data->win_size.y - data->p1.size.y) * 0.5f;

  data->p1.pos.y = paddle_y;
  data->p2.pos.y = paddle_y;
  data->ball.pos = vec2(
    (data->win_size.x - data->ball.size.x) * 0.5f,
    (data->win_size.y - data->ball.size.y) * 0.5f
  );
  data->ball.vel = vec2_zero();
  data->last_point = last_point;
  data->serve_timer = PONG_SERVE_DELAY;
  data->bot_reaction_timer = 0.0f;
  data->bot_direction = 0;
}

static void pong_launch_ball(pong_data_t* data) {
  const float magnitude = PONG_MIN_SERVE_ANGLE +
    pong_random_unit() * (PONG_MAX_SERVE_ANGLE - PONG_MIN_SERVE_ANGLE);
  const float sign = rand() % 2 == 0 ? -1.0f : 1.0f;
  const float angle = magnitude * sign;
  const float horizontal = rand() % 2 == 0 ? -1.0f : 1.0f;

  data->ball.vel = vec2(
    cosf(angle) * horizontal * PONG_INITIAL_BALL_SPEED,
    sinf(angle) * PONG_INITIAL_BALL_SPEED
  );
}

static void pong_move_paddle(
  pong_paddle_t* paddle,
  int direction,
  float dt,
  vec2_t win_size
) {
  paddle->pos.y += direction * PONG_PLAYER_SPEED * dt;
  pong_clamp_paddle(paddle, win_size);
}

static void pong_update_bot(pong_data_t* data, float dt) {
  data->bot_reaction_timer -= dt;
  if (data->bot_reaction_timer <= 0.0f) {
    const float target_y = pong_bot_target_y(data);
    data->bot_direction = pong_bot_direction(data, target_y);
    data->bot_reaction_timer = pong_bot_reaction_time(data->difficulty);
  }

  pong_move_paddle(
    &data->p2,
    data->bot_direction,
    dt,
    data->win_size
  );
}

static int pong_bot_direction(pong_data_t* data, float target_y) {
  const float paddle_center = data->p2.pos.y + data->p2.size.y * 0.5f;
  const float difference = target_y - paddle_center;
  const float deadzone = pong_bot_deadzone(data->difficulty);

  if (difference > deadzone + PONG_BOT_TARGET_EPSILON) {
    return 1;
  }
  if (difference < -deadzone - PONG_BOT_TARGET_EPSILON) {
    return -1;
  }
  return 0;
}

static float pong_predict_linear_y(const pong_data_t* data, float target_x) {
  const float vx = data->ball.vel.x;
  if (fabsf(vx) < PONG_BOT_TARGET_EPSILON) {
    return data->ball.pos.y + data->ball.size.y * 0.5f;
  }

  const float time = (target_x - data->ball.pos.x) / vx;
  if (time <= 0.0f) {
    return data->ball.pos.y + data->ball.size.y * 0.5f;
  }

  return data->ball.pos.y + data->ball.size.y * 0.5f + data->ball.vel.y * time;
}

static float pong_predict_bounced_y(const pong_data_t* data, float target_x) {
  const float vx = data->ball.vel.x;
  if (fabsf(vx) < PONG_BOT_TARGET_EPSILON) {
    return data->ball.pos.y + data->ball.size.y * 0.5f;
  }

  const float time = (target_x - data->ball.pos.x) / vx;
  if (time <= 0.0f) {
    return data->ball.pos.y + data->ball.size.y * 0.5f;
  }

  const float min_y = data->ball.size.y * 0.5f;
  const float max_y = data->win_size.y - data->ball.size.y * 0.5f;
  const float span = max_y - min_y;
  if (span <= 0.0f) {
    return min_y;
  }

  const float period = span * 2.0f;
  float relative = data->ball.pos.y + data->ball.size.y * 0.5f - min_y;
  relative += data->ball.vel.y * time;
  relative = fmodf(relative, period);
  if (relative < 0.0f) {
    relative += period;
  }

  if (relative > span) {
    relative = period - relative;
  }
  return min_y + relative;
}

static float pong_bot_target_y(const pong_data_t* data) {
  const float paddle_center = data->p2.pos.y + data->p2.size.y * 0.5f;
  const float center = data->win_size.y * PONG_BOT_CENTER_Y;
  if (data->ball.vel.x <= 0.0f) {
    return center;
  }

  const float target_x = data->p2.pos.x - data->ball.size.x;
  switch (data->difficulty) {
    case PONG_DIFFICULTY_EASY:
      return data->ball.pos.y + data->ball.size.y * 0.5f;
    case PONG_DIFFICULTY_NORMAL:
      return pong_predict_linear_y(data, target_x);
    case PONG_DIFFICULTY_HARD:
      return pong_predict_bounced_y(data, target_x);
    default:
      return paddle_center;
  }
}

static float pong_bot_reaction_time(enum pong_difficulty difficulty) {
  switch (difficulty) {
    case PONG_DIFFICULTY_EASY:
      return PONG_BOT_EASY_REACTION;
    case PONG_DIFFICULTY_NORMAL:
      return PONG_BOT_NORMAL_REACTION;
    case PONG_DIFFICULTY_HARD:
      return PONG_BOT_HARD_REACTION;
    default:
      return PONG_BOT_NORMAL_REACTION;
  }
}

static float pong_bot_deadzone(enum pong_difficulty difficulty) {
  switch (difficulty) {
    case PONG_DIFFICULTY_EASY:
      return PONG_BOT_EASY_DEADZONE;
    case PONG_DIFFICULTY_NORMAL:
      return PONG_BOT_NORMAL_DEADZONE;
    case PONG_DIFFICULTY_HARD:
      return PONG_BOT_HARD_DEADZONE;
    default:
      return PONG_BOT_NORMAL_DEADZONE;
  }
}

static void pong_clamp_paddle(pong_paddle_t* paddle, vec2_t win_size) {
  const float max_y = win_size.y - paddle->size.y;
  paddle->pos.y = pong_clamp(paddle->pos.y, 0.0f, max_y);
}

static void pong_bounce_from_paddle(
  pong_data_t* data,
  const pong_paddle_t* paddle,
  int horizontal_direction
) {
  const float paddle_center = paddle->pos.y + paddle->size.y * 0.5f;
  const float ball_center = data->ball.pos.y + data->ball.size.y * 0.5f;
  const float half_height = paddle->size.y * 0.5f;
  const float hit_position =
  pong_clamp((ball_center - paddle_center) / half_height, -1.0f, 1.0f);
  const float angle = hit_position * PONG_MAX_BOUNCE_ANGLE;
  const float speed = fminf(
    vec2_len(data->ball.vel) + PONG_BALL_SPEED_STEP,
    PONG_MAX_BALL_SPEED
  );

  data->ball.vel = vec2(
    cosf(angle) * horizontal_direction * speed,
    sinf(angle) * speed
  );
}

static float pong_random_unit(void) {
  return (float)rand() / (float)RAND_MAX;
}

static float pong_clamp(float value, float min, float max) {
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }
  return value;
}

