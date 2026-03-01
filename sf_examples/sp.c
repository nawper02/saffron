/* Basic usage, spinning MK2 in upscaled SDL window */
#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>

/* --- CALLBACKS --- */

void on_render_start(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    /* 1. Spin the MK2 */
    sf_enti_t *mk2 = sf_get_enti(ctx, "mk2");
    if (mk2) {
        sf_enti_rotate(mk2, 1.0f * ctx->delta_time, 0.5f * ctx->delta_time, 0.0f);
    }

    /* 2. Update Light Color (Cycle through RGB using Time) */
    if (ctx->light_count > 0) {
        sf_light_t *light = &ctx->lights[0];

        /* We use sine waves to ensure the color definitely changes and stays in [0, 1] */
        /* This creates a smooth "RGB pulse" effect */
        light->color.x = (sinf(ctx->elapsed_time * 1.5f) + 1.0f) * 0.5f; // Red
        light->color.y = (sinf(ctx->elapsed_time * 2.0f) + 1.0f) * 0.5f; // Green
        light->color.z = (sinf(ctx->elapsed_time * 0.7f) + 1.0f) * 0.5f; // Blue
        
        /* Optional: Log it once every 60 frames to check values in console */
        if (ctx->frame_count % 60 == 0) {
            SF_LOG(ctx, SF_LOG_DEBUG, "Light Color: %.2f, %.2f, %.2f\n", 
                   light->color.x, light->color.y, light->color.z);
        }
    }
}

/* --- MAIN PROGRAM --- */

int main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL)); // Seed for color randomness

    const int render_w = 200;
    const int render_h = 200;
    const int window_w = 600;
    const int window_h = 600;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); 

    SDL_Window* window = SDL_CreateWindow("Saffron Shifting Light", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        window_w, window_h, 0);
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* texture = SDL_CreateTexture(renderer, 
                                            SDL_PIXELFORMAT_ARGB8888, 
                                            SDL_TEXTUREACCESS_STREAMING, 
                                            render_w, render_h);

    sf_ctx_t sf_ctx;
    sf_init(&sf_ctx, render_w, render_h);

    /* Setup Camera */
    sf_camera_set_psp(&sf_ctx.camera, 60.0f, 0.1f, 100.0f);
    sf_camera_set_pos(&sf_ctx.camera, 0.0f, 0.0f, 6.0f);
    sf_camera_look_at(&sf_ctx.camera, (sf_fvec3_t){0.0f, 0.0f, 0.0f});

    /* Load mk2 from ../assets/ */
    sf_obj_t* mk2_obj = sf_load_obj(&sf_ctx, SF_ASSET_PATH "/sf_objs/mk2.obj", "mk2_mesh");
    if (mk2_obj) {
        sf_enti_t* mk2_enti = sf_add_enti(&sf_ctx, mk2_obj, "mk2");
        sf_enti_set_scale(mk2_enti, 12.0f, 12.0f, 12.0f);
    }

    /* Initial light source */
    sf_add_light_dir(&sf_ctx, (sf_fvec3_t){1.0f, -1.0f, -1.0f}, (sf_fvec3_t){1.0f, 1.0f, 1.0f}, 1.2f);

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
        sf_render_ctx(&sf_ctx);

        SDL_UpdateTexture(texture, NULL, sf_ctx.buffer, render_w * sizeof(sf_pkd_clr_t));
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
