#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>

/* --- CALLBACKS --- */

/* Handles single, discrete button presses */
void on_key_down(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
  sf_camera_t *cam = &ctx->camera;
  
  /* Press 'R' to reset the camera to its starting position */
  if (ev->key == SF_KEY_R) {
    sf_camera_set_pos(cam, 0.0f, 0.0f, 8.0f);
    sf_camera_look_at(cam, (sf_fvec3_t){0.0f, 0.0f, 0.0f});
    SF_LOG(ctx, SF_LOG_INFO, "Camera reset to origin.\n");
  }
}

/* Fires right before rendering starts. Perfect for continuous updates! */
void on_render_start(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
  sf_camera_t *cam = &ctx->camera;
  float move_speed = 5.0f * ctx->delta_time;
  float look_speed = 60.0f * ctx->delta_time;

  /* Continuous Camera Movement */
  if (sf_key_down(ctx, SF_KEY_W)) sf_camera_move_loc(cam, move_speed, 0.0f, 0.0f);
  if (sf_key_down(ctx, SF_KEY_S)) sf_camera_move_loc(cam, -move_speed, 0.0f, 0.0f);
  if (sf_key_down(ctx, SF_KEY_A)) sf_camera_move_loc(cam, 0.0f, -move_speed, 0.0f);
  if (sf_key_down(ctx, SF_KEY_D)) sf_camera_move_loc(cam, 0.0f, move_speed, 0.0f);
  if (sf_key_down(ctx, SF_KEY_SPACE)) sf_camera_move_loc(cam, 0.0f, 0.0f, move_speed);
  if (sf_key_down(ctx, SF_KEY_LSHIFT)) sf_camera_move_loc(cam, 0.0f, 0.0f, -move_speed);

  /* Continuous Camera Look */
  if (sf_key_down(ctx, SF_KEY_UP))    sf_camera_add_yp(cam, 0.0f, look_speed);
  if (sf_key_down(ctx, SF_KEY_DOWN))  sf_camera_add_yp(cam, 0.0f, -look_speed);
  if (sf_key_down(ctx, SF_KEY_LEFT))  sf_camera_add_yp(cam, -look_speed, 0.0f);
  if (sf_key_down(ctx, SF_KEY_RIGHT)) sf_camera_add_yp(cam, look_speed, 0.0f);

  /* Grab entities from Saffron's internal registry and update them */
  sf_enti_t *teapot = sf_get_enti(ctx, "teapot");
  sf_enti_t *mk2 = sf_get_enti(ctx, "mk2");

  if (teapot) sf_enti_rotate(teapot, 1.0f * ctx->delta_time, 1.5f * ctx->delta_time, 0.0f);
  if (mk2) sf_enti_rotate(mk2, -2.0f * ctx->delta_time, -0.5f * ctx->delta_time, 0.0f);
}

/* --- MAIN PROGRAM --- */

int main(int argc, char* argv[]) {
  const int width = 683;
  const int height = 384;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* window = SDL_CreateWindow("Saffron 3D Engine Demo", 
                                      SDL_WINDOWPOS_UNDEFINED, 
                                      SDL_WINDOWPOS_UNDEFINED, 
                                      width, height, 
                                      SDL_WINDOW_FULLSCREEN_DESKTOP);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  SDL_Texture* texture = SDL_CreateTexture(renderer, 
                                            SDL_PIXELFORMAT_ARGB8888, 
                                            SDL_TEXTUREACCESS_STREAMING, 
                                            width, height);
  sf_ctx_t sf_ctx;
  sf_init(&sf_ctx, width, height);
  sf_set_logger(&sf_ctx, sf_logger_console, NULL);

  sf_camera_t* main_cam = &sf_ctx.camera;

  sf_camera_set_psp(main_cam, 60.0f, 0.1f, 100.0f);
  sf_camera_set_pos(main_cam, 0.0f, 0.0f, 8.0f);
  sf_camera_look_at(main_cam, (sf_fvec3_t){0.0f, 0.0f, 0.0f});

  sf_obj_t* mk2o = sf_load_obj(&sf_ctx, "../assets/mk2.obj", "mk2");
  sf_obj_t* teapoto = sf_load_obj(&sf_ctx, "../assets/teapot.obj", "teapot");
  sf_enti_t* teapot = sf_add_enti(&sf_ctx, teapoto, "teapot");
  sf_enti_t* mk2 = sf_add_enti(&sf_ctx, mk2o, "mk2");
  sf_enti_set_pos(mk2, 0.0, 0.0f, 0.0f);
  sf_enti_set_pos(teapot, -3.0f, 1.0f, 0.0f);
  sf_enti_set_scale(mk2, 15.0f, 15.0f, 15.0f);
  sf_enti_set_scale(teapot, 0.8f, 0.8f, 0.8f);

  sf_add_light_point(&sf_ctx, (sf_fvec3_t){2.0f, 5.0f, 2.0f}, (sf_fvec3_t){1.0f, 0.8f, 0.8f}, 2.0f);

  /* REGISTER ENGINE CALLBACKS */
  sf_reg_event(&sf_ctx, SF_EVT_KEY_DOWN, on_key_down, NULL);
  sf_reg_event(&sf_ctx, SF_EVT_RENDER_START, on_render_start, NULL);

  int running = 1;
  SDL_Event event;

  while (running) {
    /* 1. Cycle input state before polling OS events */
    sf_input_cycle_state(&sf_ctx);

    /* 2. Poll and translate SDL events directly into Saffron */
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = 0;
      if (sf_key_pressed(&sf_ctx, SF_KEY_Q)) running = 0;
      sf_sdl_process_event(&sf_ctx, &event);
    }

    sf_time_update(&sf_ctx);

    /* 3. Render Context (This automatically fires SF_EVT_RENDER_START, handling our movement) */
    sf_render_ctx(&sf_ctx);
    sf_draw_debug_axes(&sf_ctx);

    /* UI Rendering */
    sf_put_text(&sf_ctx, "SAFFRON 3D DEMO", (sf_ivec2_t){10, 10}, SF_CLR_WHITE, 1);
    sf_put_text(&sf_ctx, "WASD: Move | ARROWS: Look | R: Reset | Q: Quit", (sf_ivec2_t){10, 25}, SF_CLR_GREEN, 1);

    char fps_text[32];
    snprintf(fps_text, sizeof(fps_text), "FPS: %.0f", sf_ctx.fps);
    sf_put_text(&sf_ctx, fps_text, (sf_ivec2_t){width-80, 10}, SF_CLR_WHITE, 1);

    /* Present */
    SDL_UpdateTexture(texture, NULL, sf_ctx.buffer, sf_ctx.w * sizeof(sf_pkd_clr_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  sf_destroy(&sf_ctx);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

