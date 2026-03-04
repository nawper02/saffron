/* More advances usage, keybinds and world loading */
#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>

/* Global state for the world file */
const char* world_path = SF_ASSET_PATH "/sf_worlds/example_world.sfw";

/* --- CALLBACKS --- */

void on_key_down(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    // Press 'R' to do something
    if (ev->key == SF_KEY_R) {

    }
}

void on_render_start(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    sf_camera_t *cam = &ctx->camera;
    float move_speed = 5.0f * ctx->delta_time;
    float look_speed = 60.0f * ctx->delta_time;

    /* Continuous Input */
    if (sf_key_down(ctx, SF_KEY_W))      sf_camera_move_loc(ctx, cam,  move_speed, 0.0f, 0.0f);
    if (sf_key_down(ctx, SF_KEY_S))      sf_camera_move_loc(ctx, cam, -move_speed, 0.0f, 0.0f);
    if (sf_key_down(ctx, SF_KEY_A))      sf_camera_move_loc(ctx, cam, 0.0f, -move_speed, 0.0f);
    if (sf_key_down(ctx, SF_KEY_D))      sf_camera_move_loc(ctx, cam, 0.0f,  move_speed, 0.0f);
    if (sf_key_down(ctx, SF_KEY_SPACE))  sf_camera_move_loc(ctx, cam, 0.0f, 0.0f,  move_speed);
    if (sf_key_down(ctx, SF_KEY_LSHIFT)) sf_camera_move_loc(ctx, cam, 0.0f, 0.0f, -move_speed);

    if (sf_key_down(ctx, SF_KEY_UP))    sf_camera_add_yp(ctx, cam, 0.0f,  look_speed);
    if (sf_key_down(ctx, SF_KEY_DOWN))  sf_camera_add_yp(ctx, cam, 0.0f, -look_speed);
    if (sf_key_down(ctx, SF_KEY_LEFT))  sf_camera_add_yp(ctx, cam, -look_speed, 0.0f);
    if (sf_key_down(ctx, SF_KEY_RIGHT)) sf_camera_add_yp(ctx, cam,  look_speed, 0.0f);

    /* Update entities by name */
    sf_enti_t *teapot = sf_get_enti(ctx, "teapot");
    sf_enti_t *mk2    = sf_get_enti(ctx, "mk2");

    if (teapot) sf_enti_rotate(ctx, teapot, 1.0f * ctx->delta_time, 1.5f * ctx->delta_time, 0.0f);
    if (mk2)    sf_enti_rotate(ctx, mk2,   -2.0f * ctx->delta_time, -0.5f * ctx->delta_time, 0.0f);
}

/* --- MAIN PROGRAM --- */

int main(int argc, char* argv[]) {
    const int width = 683;
    const int height = 384;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Saffron 3D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    sf_ctx_t sf_ctx;
    sf_init(&sf_ctx, width, height);

    sf_event_reg(&sf_ctx, SF_EVT_KEY_DOWN, on_key_down, NULL);
    sf_event_reg(&sf_ctx, SF_EVT_RENDER_START, on_render_start, NULL);

    /* Initial Load */
    sf_load_world(&sf_ctx, world_path, "Example World");

    SDL_Event event;

    while (sf_running(&sf_ctx)) {
        sf_input_cycle_state(&sf_ctx);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) sf_stop(&sf_ctx);
            sf_sdl_process_event(&sf_ctx, &event);
        }

        if (sf_key_pressed(&sf_ctx, SF_KEY_Q)) sf_stop(&sf_ctx);
        
        sf_time_update(&sf_ctx);
        sf_render_ctx(&sf_ctx);
        sf_draw_debug_axes(&sf_ctx);

        sf_put_text(&sf_ctx, "SAFFRON 3D", (sf_ivec2_t){10, 10}, SF_CLR_WHITE, 1);

        SDL_UpdateTexture(texture, NULL, sf_ctx.buffer, sf_ctx.w * sizeof(sf_pkd_clr_t));
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
