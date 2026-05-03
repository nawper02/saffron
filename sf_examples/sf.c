/* More advances usage, keybinds and world loading */
#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>

/* Global state for the world / ui files */
const char* world_path = SF_ASSET_PATH "/sf_sff/example.sff";
const char* ui_path    = SF_ASSET_PATH "/sf_sfui/example.sfui";

/* --- TEST STATE VARIABLES --- */
static bool  draw_axes    = true;

/* --- UI CALLBACKS --- */

void fog_btn_cb(sf_ctx_t *ctx, void *userdata) {
    SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "Toggling fog...\n");
    ctx->fog_enabled = !ctx->fog_enabled;
}

void sky_btn_cb(sf_ctx_t *ctx, void *userdata) {
    SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "Toggling skybox...\n");
    ctx->skybox_enabled = !ctx->skybox_enabled;
}

void depth_btn_cb(sf_ctx_t *ctx, void *userdata) {
    ctx->render_mode = (ctx->render_mode == SF_RENDER_DEPTH) ? SF_RENDER_NORMAL : SF_RENDER_DEPTH;
}

void wire_btn_cb(sf_ctx_t *ctx, void *userdata) {
    ctx->render_mode = (ctx->render_mode == SF_RENDER_WIREFRAME) ? SF_RENDER_NORMAL : SF_RENDER_WIREFRAME;
}

void checkbox_test_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    draw_axes = el->checkbox.is_checked;
}

void fog_start_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    ctx->fog_start = el->slider.value;
}

void fog_end_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    ctx->fog_end = el->slider.value;
}

void fog_r_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    ctx->fog_color.x = el->slider.value;
}

void fog_g_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    ctx->fog_color.y = el->slider.value;
}

void fog_b_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    ctx->fog_color.z = el->slider.value;
}

/* --- ENGINE CALLBACKS --- */

void on_key_down(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    if (ev->key == SF_KEY_R) {
        SF_LOG(ctx, SF_LOG_INFO, "R key pressed.\n");
    }
}

void on_render_start(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    sf_cam_t *cam = &ctx->main_camera;
    float move_speed = 5.0f * ctx->delta_time;
    float look_speed = 60.0f * ctx->delta_time;

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

    sf_enti_t *teapot = sf_get_enti(ctx, "teapot");
    sf_enti_t *tux    = sf_get_enti(ctx, "tux");

    if (teapot) sf_enti_rotate(ctx, teapot, 1.0f * ctx->delta_time, 1.5f * ctx->delta_time, 0.0f);
    if (tux)    sf_enti_rotate(ctx, tux,    0.0f, -0.1f * ctx->delta_time, 0.0f);

    sf_light_t *rainbow = sf_get_light(ctx, "rainbow");
    if (rainbow) {
      float t = ctx->elapsed_time * 2.0f;
      float r = sinf(t)             * 0.5f + 0.5f;
      float g = sinf(t + 2.094395f) * 0.5f + 0.5f;
      float b = sinf(t + 4.188790f) * 0.5f + 0.5f;
      rainbow->color = (sf_fvec3_t){r, g, b};
    }
}

/* --- MAIN PROGRAM --- */

int main(int argc, char* argv[]) {
    const int width = 683*2;
    const int height = 384*2;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Saffron 3D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(renderer, width, height);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    sf_ctx_t sf_ctx;
    sf_init(&sf_ctx, width, height);

    sf_event_reg(&sf_ctx, SF_EVT_KEY_DOWN, on_key_down, NULL);
    sf_event_reg(&sf_ctx, SF_EVT_RENDER_START, on_render_start, NULL);

    sf_load_sff(&sf_ctx, world_path, "Example World");
    sf_cam_t* pip_cam = sf_get_cam(&sf_ctx, "pip_cam");
    sf_camera_look_at(&sf_ctx, pip_cam, (sf_fvec3_t){0.0f, 0.0f, 0.0f});

    sf_light_t *rainbow = sf_add_light(&sf_ctx, "rainbow", SF_LIGHT_POINT, (sf_fvec3_t){1.0f, 0.0f, 0.0f}, 8.0f);
    if (rainbow) {
      rainbow->frame->pos = (sf_fvec3_t){3.0f, -4.0f, 0.0f};
      rainbow->frame->is_dirty = true;
    }

    /* --- LOAD UI FROM .sfui AND BIND CALLBACKS BY NAME --- */
    sf_ctx.ui = sf_load_sfui(&sf_ctx, ui_path);

    sf_ui_lmn_t *fog_btn = sf_ui_get_by_name(sf_ctx.ui, "fog_btn");
    sf_ui_set_callback(fog_btn, fog_btn_cb, NULL);

    sf_ui_lmn_t *sky_btn = sf_ui_get_by_name(sf_ctx.ui, "sky_btn");
    sf_ui_set_callback(sky_btn, sky_btn_cb, NULL);

    sf_ui_lmn_t *depth_btn = sf_ui_get_by_name(sf_ctx.ui, "depth_btn");
    sf_ui_set_callback(depth_btn, depth_btn_cb, NULL);

    sf_ui_lmn_t *wire_btn = sf_ui_get_by_name(sf_ctx.ui, "wire_btn");
    sf_ui_set_callback(wire_btn, wire_btn_cb, NULL);

    sf_ui_lmn_t *dbg = sf_ui_get_by_name(sf_ctx.ui, "debug_ovrlay");
    sf_ui_set_callback(dbg, checkbox_test_cb, dbg);
    if (dbg) draw_axes = dbg->checkbox.is_checked;

    sf_ui_lmn_t *fog_start = sf_ui_get_by_name(sf_ctx.ui, "fog_start");
    sf_ui_set_callback(fog_start, fog_start_cb, fog_start);
    if (fog_start) sf_ctx.fog_start = fog_start->slider.value;

    sf_ui_lmn_t *fog_end = sf_ui_get_by_name(sf_ctx.ui, "fog_end");
    sf_ui_set_callback(fog_end, fog_end_cb, fog_end);
    if (fog_end) sf_ctx.fog_end = fog_end->slider.value;

    sf_ui_lmn_t *fog_r = sf_ui_get_by_name(sf_ctx.ui, "fog_r");
    sf_ui_set_callback(fog_r, fog_r_cb, fog_r);
    if (fog_r) sf_ctx.fog_color.x = fog_r->slider.value;

    sf_ui_lmn_t *fog_g = sf_ui_get_by_name(sf_ctx.ui, "fog_g");
    sf_ui_set_callback(fog_g, fog_g_cb, fog_g);
    if (fog_g) sf_ctx.fog_color.y = fog_g->slider.value;

    sf_ui_lmn_t *fog_b = sf_ui_get_by_name(sf_ctx.ui, "fog_b");
    sf_ui_set_callback(fog_b, fog_b_cb, fog_b);
    if (fog_b) sf_ctx.fog_color.z = fog_b->slider.value;

    SDL_Event event;

    while (sf_running(&sf_ctx)) {
        sf_input_cycle_state(&sf_ctx);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) sf_stop(&sf_ctx);
            sf_sdl_process_event(&sf_ctx, &event);
        }

        if (sf_key_pressed(&sf_ctx, SF_KEY_Q)) sf_stop(&sf_ctx);

        sf_time_update(&sf_ctx);

        sf_ui_update(&sf_ctx, sf_ctx.ui);

        sf_render_ctx(&sf_ctx);
        sf_rect(&sf_ctx, &sf_ctx.main_camera, SF_CLR_WHITE, (sf_ivec2_t){width-pip_cam->w-22, height-pip_cam->h-22}, (sf_ivec2_t){width-19, height-19});
        sf_draw_cam_pip(&sf_ctx, &sf_ctx.main_camera, pip_cam, (sf_ivec2_t){width-pip_cam->w-20,height-pip_cam->h-20});

        if (draw_axes) {
            sf_draw_debug_ovrlay(&sf_ctx, &sf_ctx.main_camera);
        }

        sf_ui_render(&sf_ctx, &sf_ctx.main_camera, sf_ctx.ui);

        SDL_UpdateTexture(texture, NULL, sf_ctx.main_camera.buffer, sf_ctx.main_camera.w * sizeof(sf_pkd_clr_t));
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
