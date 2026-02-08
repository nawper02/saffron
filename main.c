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

  int running = 1;
  SDL_Event event;
  while (running) {
      while (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) running = 0;
      }

      sf_fill(&sf_ctx, SF_COLOR_BLUE); 
      //sf_line(&sf_ctx, SF_COLOR_RED, (sf_svec2_t){0,0}, (sf_svec2_t){200, 50});
      //sf_line(&sf_ctx, SF_COLOR_RED, (sf_svec2_t){0,0}, (sf_svec2_t){50, 200});
      sf_tri(&sf_ctx, SF_COLOR_GREEN, (sf_svec2_t){0,50}, (sf_svec2_t){50, 400}, (sf_svec2_t){200,100});
      sf_tri(&sf_ctx, SF_COLOR_WHITE, (sf_svec2_t){100,200}, (sf_svec2_t){300, 10}, (sf_svec2_t){400,600});

      SDL_UpdateTexture(texture, NULL, sf_ctx.buffer, sf_ctx.w * sizeof(sf_packed_color_t));
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
