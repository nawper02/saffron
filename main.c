#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#include <SDL2/SDL.h>

int main(int argc, char* argv[])
{
  const int width = 800;
  const int height = 600;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* window = SDL_CreateWindow("Saffron Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  SDL_Texture* texture = SDL_CreateTexture(renderer, 
                                            SDL_PIXELFORMAT_ARGB8888, 
                                            SDL_TEXTUREACCESS_STREAMING, 
                                            width, height);
  sf_ctx_t sf_ctx;
  sf_init(&sf_ctx, width, height);
  sf_ctx.camera.pos    = (sf_fvec3_t){0.0f, 0.0f, -5.0f};
  sf_ctx.camera.target = (sf_fvec3_t){0.0f, 0.0f, 0.0f};
  sf_ctx.camera.P      = sf_make_psp_fmat4(60.0f, (float)width/height, 0.1f, 100.0f);
  sf_ctx.camera.V      = sf_make_view_fmat4(sf_ctx.camera.pos, sf_ctx.camera.target, (sf_fvec3_t){0,1,0});
  sf_set_logger(&sf_ctx, sf_logger_console, NULL);
  sf_load_obj(&sf_ctx, "../assets/cube.obj", "teapot");
  sf_enti_t* teapot = sf_add_enti(&sf_ctx, sf_get_obj(&sf_ctx, "teapot"), "teapot");
  float rotation = 0.0f;

  int running = 1;
  SDL_Event event;
  while (running) {
      while (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) running = 0;
      }

      rotation += 0.001f;
      sf_fvec3_t angles = { rotation, rotation * 0.5f, 0.0f };
      teapot->M = sf_fmat4_mul_fmat4(sf_make_tsl_fmat4(0, 0, 0), sf_make_rot_fmat4(angles));

      sf_time_update(&sf_ctx);
      sf_render_ctx(&sf_ctx);

      sf_put_text(&sf_ctx, "SAFFRON 3D TEST", (sf_svec2_t){10, 10}, SF_CLR_WHITE, 1);
      char fps_text[32];
      snprintf(fps_text, sizeof(fps_text), "FPS: %.0f", sf_ctx.fps);
      sf_put_text(&sf_ctx, fps_text, (sf_svec2_t){width-80, 10}, SF_CLR_WHITE, 1);

      //sf_fill(&sf_ctx, SF_CLR_BLUE);
      //for (int i = 0; i < 80; ++i) {
      //  sf_line(&sf_ctx, SF_CLR_RED, (sf_svec2_t){10*i+(width/2),0}, (sf_svec2_t){10*i, height});
      //}
      //sf_tri(&sf_ctx, SF_CLR_GREEN, (sf_svec2_t){0,50}, (sf_svec2_t){50, 400}, (sf_svec2_t){200,100});
      //sf_tri(&sf_ctx, SF_CLR_WHITE, (sf_svec2_t){100,200}, (sf_svec2_t){300, 10}, (sf_svec2_t){400,600});
      //sf_rect(&sf_ctx, SF_CLR_RED, (sf_svec2_t){0,20}, (sf_svec2_t){20,0});
      //sf_rect(&sf_ctx, SF_CLR_RED, (sf_svec2_t){20,20}, (sf_svec2_t){50,50});
      //sf_rect(&sf_ctx, SF_CLR_WHITE, (sf_svec2_t){50,50}, (sf_svec2_t){30,30});
      //for (int i = 0; i < 10; ++i) {
      //  sf_put_text(&sf_ctx, "SAFFRON", (sf_svec2_t){20,105+(40*i)}, SF_CLR_RED, i);
      //}

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
