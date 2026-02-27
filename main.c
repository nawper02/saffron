#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#include <SDL2/SDL.h>

int main(int argc, char* argv[])
{
  const int width = 800;
  const int height = 600;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* window = SDL_CreateWindow("Saffron 3D Engine Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
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

  sf_load_obj(&sf_ctx, "../assets/mk2.obj", "mk2");
  sf_load_obj(&sf_ctx, "../assets/teapot.obj", "teapot");
  sf_enti_t* teapot = sf_add_enti(&sf_ctx, sf_get_obj(&sf_ctx, "teapot"), "teapot");
  sf_enti_t* mk2 = sf_add_enti(&sf_ctx, sf_get_obj(&sf_ctx, "mk2"), "mk2");
  sf_enti_set_pos(mk2, 0.0, 0.0f, 0.0f);
  sf_enti_set_pos(teapot,-1.5f, 1.0f, 0.0f);
  sf_enti_set_scale(mk2, 15.0f, 15.0f, 15.0f);
  sf_enti_set_scale(teapot, 0.8f, 0.8f, 0.8f);

  sf_add_light_point(&sf_ctx, (sf_fvec3_t){2.0f, 5.0f, 2.0f}, (sf_fvec3_t){1.0f, 0.8f, 0.8f}, 2.0f);

  int running = 1;
  SDL_Event event;
  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = 0;
    }
    sf_time_update(&sf_ctx);
    const Uint8* state = SDL_GetKeyboardState(NULL);
    float move_speed = 5.0f * sf_ctx.delta_time;
    float look_speed = 60.0f * sf_ctx.delta_time;

    if (state[SDL_SCANCODE_W]) sf_camera_move_loc(main_cam, move_speed, 0.0f, 0.0f);
    if (state[SDL_SCANCODE_S]) sf_camera_move_loc(main_cam, -move_speed, 0.0f, 0.0f);
    if (state[SDL_SCANCODE_A]) sf_camera_move_loc(main_cam, 0.0f, -move_speed, 0.0f);
    if (state[SDL_SCANCODE_D]) sf_camera_move_loc(main_cam, 0.0f, move_speed, 0.0f);
    if (state[SDL_SCANCODE_SPACE]) sf_camera_move_loc(main_cam, 0.0f, 0.0f, move_speed);
    if (state[SDL_SCANCODE_LSHIFT]) sf_camera_move_loc(main_cam, 0.0f, 0.0f, -move_speed);

    if (state[SDL_SCANCODE_UP])    sf_camera_add_yp(main_cam, 0.0f, look_speed);
    if (state[SDL_SCANCODE_DOWN])  sf_camera_add_yp(main_cam, 0.0f, -look_speed);
    if (state[SDL_SCANCODE_LEFT])  sf_camera_add_yp(main_cam, -look_speed, 0.0f);
    if (state[SDL_SCANCODE_RIGHT]) sf_camera_add_yp(main_cam, look_speed, 0.0f);

    sf_enti_rotate(teapot, 1.0f * sf_ctx.delta_time, 1.5f * sf_ctx.delta_time, 0.0f);
    sf_enti_rotate(mk2, -2.0f * sf_ctx.delta_time, -0.5f * sf_ctx.delta_time, 0.0f);

    sf_render_ctx(&sf_ctx);
    sf_draw_debug_axes(&sf_ctx);

    sf_put_text(&sf_ctx, "SAFFRON 3D DEMO", (sf_ivec2_t){10, 10}, SF_CLR_WHITE, 1);
    sf_put_text(&sf_ctx, "WASD: Move | ARROWS: Look", (sf_ivec2_t){10, 25}, SF_CLR_GREEN, 1);
    char fps_text[32];
    snprintf(fps_text, sizeof(fps_text), "FPS: %.0f", sf_ctx.fps);
    sf_put_text(&sf_ctx, fps_text, (sf_ivec2_t){width-80, 10}, SF_CLR_WHITE, 1);

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
