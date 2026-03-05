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

/* --- TEST STATE VARIABLES --- */
static float teapot_speed = 1.0f;
static bool  draw_axes    = true;

/* --- UI CALLBACKS --- */

void btn_test_cb(sf_ctx_t *ctx, void *userdata) {
    SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "Button Clicked! Changing theme color.\n");
    // Change the global theme color randomly on click
    ctx->ui->default_style.color_base = (sf_pkd_clr_t)(0xFF000000 | (rand() % 0xFFFFFF));
    
    // Apply it to existing elements (optional, just for fun)
    for(int i=0; i<ctx->ui->count; i++) {
        ctx->ui->elements[i].style.color_base = ctx->ui->default_style.color_base;
    }
}

void slider_test_cb(sf_ctx_t *ctx, void *userdata) {
    // We pass the slider element itself as userdata so we can read its value
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    teapot_speed = el->slider.value;
}

void checkbox_test_cb(sf_ctx_t *ctx, void *userdata) {
    // Read the checkbox state
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    draw_axes = el->checkbox.is_checked;
}

/* --- ENGINE CALLBACKS --- */

void on_key_down(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    // Press 'R' to do something
    if (ev->key == SF_KEY_R) {
        SF_LOG(ctx, SF_LOG_INFO, "R key pressed.\n");
    }
}

void on_render_start(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    sf_cam_t *cam = &ctx->camera;
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
    sf_enti_t *tux    = sf_get_enti(ctx, "tux");

    // Applied teapot_speed from the UI slider here!
    if (teapot) sf_enti_rotate(ctx, teapot, teapot_speed * ctx->delta_time, (teapot_speed * 1.5f) * ctx->delta_time, 0.0f);
    if (tux)    sf_enti_rotate(ctx, tux,    0.0f, -0.1f * ctx->delta_time, 0.0f);
    
    char fps_text[32];
    snprintf(fps_text, sizeof(fps_text), "FPS: %.2f", ctx->fps);
    sf_put_text(ctx, &ctx->camera, fps_text, (sf_ivec2_t){ctx->camera.w-93, 10}, SF_CLR_WHITE, 1.0);
}

/* --- MAIN PROGRAM --- */

int main(int argc, char* argv[]) {
    const int width = 683;
    const int height = 384;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Saffron 3D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(renderer, width, height);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    sf_ctx_t sf_ctx;
    sf_init(&sf_ctx, width, height);

    sf_event_reg(&sf_ctx, SF_EVT_KEY_DOWN, on_key_down, NULL);
    sf_event_reg(&sf_ctx, SF_EVT_RENDER_START, on_render_start, NULL);

    /* Initial Load */
    sf_load_world(&sf_ctx, world_path, "Example World");
    sf_cam_t* pip_cam = sf_get_cam(&sf_ctx, "pip_cam");
    sf_camera_look_at(&sf_ctx, pip_cam, (sf_fvec3_t){0.0f, 0.0f, 0.0f});

    sf_enti_t *tux1 = sf_get_enti(&sf_ctx, "tux");
    sf_enti_t *tux2 = sf_get_enti(&sf_ctx, "tux2");

    if (tux1 && tux2) {
      // tux2 now "lives" inside tux1's coordinate system
      sf_frame_set_parent(tux2->frame, tux1->frame);
      
      // Position tux2 2 units to the right of tux1 locally.
      // No matter how tux1 rotates, tux2 will stay stuck to its side!
      tux2->frame->pos = (sf_fvec3_t){ 20.0f, 10.0f, 0.0f };
      tux2->frame->is_dirty = true;
    }

    /* --- INITIALIZE UI ELEMENTS --- */
    sf_add_button(&sf_ctx, "Random Theme", (sf_ivec2_t){10, 50}, (sf_ivec2_t){130, 80}, btn_test_cb, NULL);
    
    sf_ui_lmn_t* sldr = sf_add_slider(&sf_ctx, (sf_ivec2_t){10, 90}, (sf_ivec2_t){130, 110}, 0.0f, 5.0f, teapot_speed, slider_test_cb, NULL);
    sldr->slider.userdata = sldr; // Pass itself so the callback can read the value

    sf_ui_lmn_t* chk = sf_add_checkbox(&sf_ctx, "Draw Axes", (sf_ivec2_t){10, 120}, (sf_ivec2_t){130, 140}, draw_axes, checkbox_test_cb, NULL);
    chk->checkbox.userdata = chk; // Pass itself so the callback can read the state


    SDL_Event event;

    while (sf_running(&sf_ctx)) {
        sf_input_cycle_state(&sf_ctx);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) sf_stop(&sf_ctx);
            sf_sdl_process_event(&sf_ctx, &event);
        }

        if (sf_key_pressed(&sf_ctx, SF_KEY_Q)) sf_stop(&sf_ctx);
        
        sf_time_update(&sf_ctx);
        
        // --- UPDATE UI LOGIC --- 
        sf_update_ui(&sf_ctx, sf_ctx.ui);

        sf_render_ctx(&sf_ctx);
        sf_rect(&sf_ctx, &sf_ctx.camera, SF_CLR_WHITE, (sf_ivec2_t){width-pip_cam->w-22, height-pip_cam->h-22}, (sf_ivec2_t){width-19, height-19});
        sf_draw_cam_pip(&sf_ctx, &sf_ctx.camera, pip_cam, (sf_ivec2_t){width-pip_cam->w-20,height-pip_cam->h-20}); 
        
        // Tied to Checkbox!
        if (draw_axes) {
            sf_draw_debug_axes(&sf_ctx, &sf_ctx.camera);
            sf_draw_debug_frames(&sf_ctx, &sf_ctx.camera, 1.0f);
        }

        sf_put_text(&sf_ctx, &sf_ctx.camera, "SAFFRON 3D", (sf_ivec2_t){10, 10}, SF_CLR_WHITE, 1);

        // --- RENDER UI ON TOP OF 3D ---
        sf_render_ui(&sf_ctx, &sf_ctx.camera, sf_ctx.ui);

        SDL_UpdateTexture(texture, NULL, sf_ctx.camera.buffer, sf_ctx.camera.w * sizeof(sf_pkd_clr_t));
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
