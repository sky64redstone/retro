#pragma once

#include <stdbool.h>

enum key {
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_ACTION,
  KEY_PAUSE,

  KEY_COUNT
};

enum button {
  BUTTON_LEFT,
  BUTTON_MIDDLE,
  BUTTON_RIGHT,

  BUTTON_COUNT
};

struct input_mouse {
  vec2_t position;

  bool down[BUTTON_COUNT];
  bool pressed[BUTTON_COUNT];
  bool released[BUTTON_COUNT];
};

struct input {
  bool down[KEY_COUNT];
  bool pressed[KEY_COUNT];
  bool released[KEY_COUNT];
  struct input_mouse mouse;
};

typedef struct input input_t;
