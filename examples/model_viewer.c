#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>

/* --- GLOBAL ENGINE STATE --- */
/* We keep a few flags here that our callbacks can modify */
bool g_auto_rotate = true;

/* --- CALLBACKS --- */

/* Handles single, discrete button presses */
void on_key_down(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
  /* Toggle auto-rotation with Spacebar */
  if (ev->data.key.key == SF_KEY_SPACE) {
    g_auto_rotate = !g_auto_rotate;
    SF_LOG(ctx, SF_LOG_INFO, "Auto-rotate: %s\n", g_auto_rotate ? "ON" : "OFF");
  }
  
  /* Randomize the main light color when pressing 'L' */
  if (ev->data.key.key == SF_KEY_L) {
    if (ctx->light_count > 0) {
      /* Generate random floats between 0.2 and 1.0 to avoid pitching to pitch black */
      float r = ((rand() % 80) + 20) / 100.0f;
      float g = ((rand() % 80) + 20) / 100.0f;
      float b = ((rand() % 80) + 20) / 100.0f;
      ctx->lights[0].color = (sf_fvec3_t){r, g, b};
      SF_LOG(ctx, SF_LOG_INFO, "Light color randomized.\n");
    }
  }

  /* Reset view with 'R' */
  if (ev->data.key.key == SF_KEY_R) {
    sf_camera_set_pos(&ctx->camera, 0.0f, 0.0f, 15.0f);
    sf_camera_look_at(&ctx->camera, (sf_fvec3_t){0.0f, 0.0f, 0.0f});
    sf_enti_t *model = sf_get_enti(ctx, "model");
    if (model) sf_enti_set_rot(model, 0.0f, 0.0f, 0.0f);
  }
}

/* Handles continuous input (held keys and mouse dragging) */
void on_render_start(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
  sf_camera_t *cam = &ctx->camera;
  sf_enti_t *model = sf_get_enti(ctx, "model");
  if (!model) return;

  /* 1. MOUSE CONTROLS */
  if (ctx->input.mouse_btns[SF_MOUSE_LEFT]) {
    /* Left drag: Rotate the model based on mouse movement */
    float rot_speed = 0.01f;
    sf_enti_rotate(model, ctx->input.mouse_dy * rot_speed, ctx->input.mouse_dx * rot_speed, 0.0f);
    g_auto_rotate = false; /* Disable auto-spin if the user takes control */
  } 
  else if (g_auto_rotate) {
    /* Idle auto-spin */
    sf_enti_rotate(model, 0.0f, 1.0f * ctx->delta_time, 0.0f);
  }

  if (ctx->input.mouse_btns[SF_MOUSE_RIGHT]) {
    /* Right drag: Pan the camera */
    float pan_speed = 0.02f;
    /* We invert dx so dragging left moves the camera right (feels natural) */
    sf_camera_move_loc(cam, 0.0f, -ctx->input.mouse_dx * pan_speed, ctx->input.mouse_dy * pan_speed);
  }

  /* 2. KEYBOARD ZOOM */
  float zoom_speed = 10.0f * ctx->delta_time;
  if (sf_key_down(ctx, SF_KEY_W)) sf_camera_move_loc(cam, zoom_speed, 0.0f, 0.0f);
  if (sf_key_down(ctx, SF_KEY_S)) sf_camera_move_loc(cam, -zoom_speed, 0.0f, 0.0f);
}

/* --- MAIN PROGRAM --- */

int main(int argc, char* argv[]) {
  const int width = 800;
  const int height = 600;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* window = SDL_CreateWindow("Saffron Model Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
  
  sf_ctx_t sf_ctx;
  sf_init(&sf_ctx, width, height);
  sf_set_logger(&sf_ctx, sf_logger_console, NULL);

  /* Seed random number generator for our lighting feature */
  srand(time(NULL));

  sf_camera_t* main_cam = &sf_ctx.camera;
  sf_camera_set_psp(main_cam, 60.0f, 0.1f, 100.0f);
  sf_camera_set_pos(main_cam, 0.0f, 0.0f, 15.0f);
  sf_camera_look_at(main_cam, (sf_fvec3_t){0.0f, 0.0f, 0.0f});

  /* Load just ONE model for inspection, scale it up so it looks cool */
  sf_load_obj(&sf_ctx, "../assets/mk2.obj", "mk2_mesh");
  sf_enti_t* model = sf_add_enti(&sf_ctx, sf_get_obj(&sf_ctx, "mk2_mesh"), "model");
  sf_enti_set_pos(model, 0.0f, 0.0f, 0.0f);
  sf_enti_set_scale(model, 15.0f, 15.0f, 15.0f);

  /* Add a nice bright light */
  sf_add_light_point(&sf_ctx, (sf_fvec3_t){5.0f, 10.0f, 5.0f}, (sf_fvec3_t){0.9f, 0.9f, 1.0f}, 2.5f);

  /* REGISTER ENGINE CALLBACKS */
  sf_reg_event(&sf_ctx, SF_EVT_KEY_DOWN, on_key_down, NULL);
  sf_reg_event(&sf_ctx, SF_EVT_RENDER_START, on_render_start, NULL);

  int running = 1;
  SDL_Event event;

  while (running) {
    sf_input_cycle_state(&sf_ctx);

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = 0;
      sf_sdl_process_event(&sf_ctx, &event);
    }

    sf_time_update(&sf_ctx);

    /* Render Context (fires SF_EVT_RENDER_START) */
    sf_render_ctx(&sf_ctx);
    sf_draw_debug_axes(&sf_ctx);

    /* UI Overlay */
    sf_put_text(&sf_ctx, "SAFFRON MODEL VIEWER", (sf_ivec2_t){10, 10}, SF_CLR_WHITE, 1);
    sf_put_text(&sf_ctx, "Left Click + Drag  : Rotate Model", (sf_ivec2_t){10, 30}, SF_CLR_GREEN, 1);
    sf_put_text(&sf_ctx, "Right Click + Drag : Pan Camera", (sf_ivec2_t){10, 45}, SF_CLR_GREEN, 1);
    sf_put_text(&sf_ctx, "W / S              : Zoom In/Out", (sf_ivec2_t){10, 60}, SF_CLR_GREEN, 1);
    sf_put_text(&sf_ctx, "SPACE              : Toggle Auto-Spin", (sf_ivec2_t){10, 75}, SF_CLR_GREEN, 1);
    sf_put_text(&sf_ctx, "L                  : Randomize Light", (sf_ivec2_t){10, 90}, SF_CLR_GREEN, 1);
    sf_put_text(&sf_ctx, "R                  : Reset View", (sf_ivec2_t){10, 105}, SF_CLR_GREEN, 1);

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

