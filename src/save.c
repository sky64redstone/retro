#include <SDL3/SDL.h>
#include <stdint.h>
#include <string.h>

#include "save.h"

#define ORG_PATH "de.retro.games"

static bool valid_key(const char* key) {
  if (key == NULL || key[0] == '\0') {
    return false;
  }

  for (const char* p = key; *p != '\0'; p++) {
    const char c = *p;
    const bool alnum =
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9');

    if (!alnum && c != '_' && c != '-' && c != '.') {
      return false;
    }
  }

  return true;
}

static char* key_path(const save_t* save, const char* key) {
  static const char suffix[] = ".dat";
  const size_t dir_len = strlen(save->directory);
  const size_t key_len = strlen(key);
  const size_t suffix_len = sizeof(suffix) - 1;

  if (dir_len > SIZE_MAX - key_len ||
      dir_len + key_len > SIZE_MAX - suffix_len - 1) {
    return NULL;
  }

  char* path = SDL_malloc(dir_len + key_len + suffix_len + 1);
  if (path == NULL) {
    return NULL;
  }

  memcpy(path, save->directory, dir_len);
  memcpy(path + dir_len, key, key_len);
  memcpy(path + dir_len + key_len, suffix, suffix_len + 1);

  return path;
}

bool save_init(save_t* save, const char* app) {
  *save = (save_t){0};

  save->directory = SDL_GetPrefPath(ORG_PATH, app);
  return save->directory != NULL;
}

void save_destroy(save_t* save) {
  SDL_free(save->directory);
  *save = (save_t){0};
}

bool save_set(
  const save_t* save, const char* key, const void* data, size_t size
) {
  if (!valid_key(key) || save->directory == NULL ||
      (data == NULL && size != 0)) {
    return false;
  }

  char* path = key_path(save, key);
  if (path == NULL) {
    return false;
  }

  const bool result = SDL_SaveFile(path, data, size);

  SDL_free(path);
  return result;
}

void* save_get(const save_t* save, const char* key, size_t* size) {
  if (!valid_key(key) || save->directory == NULL) {
    return NULL;
  }

  char* path = key_path(save, key);
  if (path == NULL) {
    return NULL;
  }

  void* data = SDL_LoadFile(path, size);

  SDL_free(path);
  return data;
}

bool save_has(const save_t* save, const char* key) {
  if (!valid_key(key) || save->directory == NULL) {
    return false;
  }

  char* path = key_path(save, key);
  if (path == NULL) {
    return false;
  }

  const bool result = SDL_GetPathInfo(path, NULL);

  SDL_free(path);
  return result;
}

bool save_remove(const save_t* save, const char* key) {
  if (!valid_key(key) || save->directory == NULL) {
    return false;
  }

  char* path = key_path(save, key);
  if (path == NULL) {
    return false;
  }

  const bool result = SDL_RemovePath(path);

  SDL_free(path);
  return result;
}
