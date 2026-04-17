/* sf_extras/sf_sdl.h */
#ifndef SF_SDL_H
#define SF_SDL_H

#include "../saffron.h"
#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

void sf_sdl_process_event(sf_ctx_t *ctx, const SDL_Event *event);

#ifdef __cplusplus
}
#endif

#endif /* SF_SDL_H */

#ifdef SAFFRON_SDL_IMPLEMENTATION

static sf_key_t _sf_sdl_map_key(SDL_Scancode code) {
  switch (code) {
    case SDL_SCANCODE_A: return SF_KEY_A;
    case SDL_SCANCODE_B: return SF_KEY_B;
    case SDL_SCANCODE_C: return SF_KEY_C;
    case SDL_SCANCODE_D: return SF_KEY_D;
    case SDL_SCANCODE_E: return SF_KEY_E;
    case SDL_SCANCODE_F: return SF_KEY_F;
    case SDL_SCANCODE_G: return SF_KEY_G;
    case SDL_SCANCODE_H: return SF_KEY_H;
    case SDL_SCANCODE_I: return SF_KEY_I;
    case SDL_SCANCODE_J: return SF_KEY_J;
    case SDL_SCANCODE_K: return SF_KEY_K;
    case SDL_SCANCODE_L: return SF_KEY_L;
    case SDL_SCANCODE_M: return SF_KEY_M;
    case SDL_SCANCODE_N: return SF_KEY_N;
    case SDL_SCANCODE_O: return SF_KEY_O;
    case SDL_SCANCODE_P: return SF_KEY_P;
    case SDL_SCANCODE_Q: return SF_KEY_Q;
    case SDL_SCANCODE_R: return SF_KEY_R;
    case SDL_SCANCODE_S: return SF_KEY_S;
    case SDL_SCANCODE_T: return SF_KEY_T;
    case SDL_SCANCODE_U: return SF_KEY_U;
    case SDL_SCANCODE_V: return SF_KEY_V;
    case SDL_SCANCODE_W: return SF_KEY_W;
    case SDL_SCANCODE_X: return SF_KEY_X;
    case SDL_SCANCODE_Y: return SF_KEY_Y;
    case SDL_SCANCODE_Z: return SF_KEY_Z;

    case SDL_SCANCODE_0: return SF_KEY_0;
    case SDL_SCANCODE_1: return SF_KEY_1;
    case SDL_SCANCODE_2: return SF_KEY_2;
    case SDL_SCANCODE_3: return SF_KEY_3;
    case SDL_SCANCODE_4: return SF_KEY_4;
    case SDL_SCANCODE_5: return SF_KEY_5;
    case SDL_SCANCODE_6: return SF_KEY_6;
    case SDL_SCANCODE_7: return SF_KEY_7;
    case SDL_SCANCODE_8: return SF_KEY_8;
    case SDL_SCANCODE_9: return SF_KEY_9;

    case SDL_SCANCODE_SPACE:     return SF_KEY_SPACE;
    case SDL_SCANCODE_LSHIFT:    return SF_KEY_LSHIFT;
    case SDL_SCANCODE_LCTRL:     return SF_KEY_LCTRL;
    case SDL_SCANCODE_UP:        return SF_KEY_UP;
    case SDL_SCANCODE_DOWN:      return SF_KEY_DOWN;
    case SDL_SCANCODE_LEFT:      return SF_KEY_LEFT;
    case SDL_SCANCODE_RIGHT:     return SF_KEY_RIGHT;
    case SDL_SCANCODE_BACKSPACE: return SF_KEY_BACKSPACE;
    case SDL_SCANCODE_RETURN:    return SF_KEY_RETURN;
    case SDL_SCANCODE_ESCAPE:    return SF_KEY_ESC;
    case SDL_SCANCODE_TAB:       return SF_KEY_TAB;
    case SDL_SCANCODE_DELETE:    return SF_KEY_DEL;
    case SDL_SCANCODE_HOME:      return SF_KEY_HOME;
    case SDL_SCANCODE_END:       return SF_KEY_END;

    default: return SF_KEY_UNKNOWN;
  }
}

static sf_mouse_btn_t _sf_sdl_map_mouse(Uint8 btn) {
  switch(btn) {
    case SDL_BUTTON_LEFT: return SF_MOUSE_LEFT;
    case SDL_BUTTON_RIGHT: return SF_MOUSE_RIGHT;
    case SDL_BUTTON_MIDDLE: return SF_MOUSE_MIDDLE;
    default: return SF_MOUSE_MAX;
  }
}

void sf_sdl_process_event(sf_ctx_t *ctx, const SDL_Event *event) {
  if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
    /* Prevent OS key-repeat from triggering multiple "just pressed" frames */
    if (event->key.repeat) return; 
    sf_key_t key = _sf_sdl_map_key(event->key.keysym.scancode);
    sf_input_set_key(ctx, key, event->type == SDL_KEYDOWN);
  }
  else if (event->type == SDL_MOUSEMOTION) {
    sf_input_set_mouse_p(ctx, event->motion.x, event->motion.y);
  }
  else if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
    sf_mouse_btn_t btn = _sf_sdl_map_mouse(event->button.button);
    sf_input_set_mouse_b(ctx, btn, event->type == SDL_MOUSEBUTTONDOWN);
  }
  else if (event->type == SDL_MOUSEWHEEL) {
    sf_input_set_wheel(ctx, event->wheel.y);
  }
  else if (event->type == SDL_TEXTINPUT) {
    sf_input_set_text(ctx, event->text.text);
  }
}

#endif /* SAFFRON_SDL_IMPLEMENTATION */
