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
static float teapot_speed  = 1.0f;
static bool  draw_axes     = true;
static char  fps_buf[32]   = "FPS: --";
static char  name_buf[64]  = {0};

/* --- UI CALLBACKS --- */

void btn_test_cb(sf_ctx_t *ctx, void *userdata) {
    SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "Button Clicked! Changing theme color.\n");
    ctx->ui->default_style.color_base = (sf_pkd_clr_t)(0xFF000000 | (rand() % 0xFFFFFF));
    for (int i = 0; i < ctx->ui->count; i++)
        ctx->ui->elements[i].style.color_base = ctx->ui->default_style.color_base;
}

void slider_test_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    teapot_speed = el->slider.value;
}

void checkbox_test_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    draw_axes = el->checkbox.is_checked;
}

void textinput_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    snprintf(name_buf, sizeof(name_buf), "%s", el->textinput.buf);
    SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "Name submitted: %s\n", name_buf);
}

void dropdown_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t*)userdata;
    SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "Dropdown selected: %s\n",
           el->dropdown.items[el->dropdown.selected]);
}

/* --- ENGINE CALLBACKS --- */

void on_key_down(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    // Press 'R' to do something
    if (ev->key == SF_KEY_R) {
        SF_LOG(ctx, SF_LOG_INFO, "R key pressed.\n");
    }
}

void on_render_start(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    sf_cam_t *cam = &ctx->main_camera;
    float move_speed = 5.0f * ctx->delta_time;
    float look_speed = 60.0f * ctx->delta_time;

    /* Skip camera/game input while a text field has focus */
    if (!sf_ui_has_focus(ctx)) {
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
    }

    /* Update entities by name */
    sf_enti_t *teapot = sf_get_enti(ctx, "teapot");
    sf_enti_t *tux    = sf_get_enti(ctx, "tux");

    // Applied teapot_speed from the UI slider here!
    if (teapot) sf_enti_rotate(ctx, teapot, teapot_speed * ctx->delta_time, (teapot_speed * 1.5f) * ctx->delta_time, 0.0f);
    if (tux)    sf_enti_rotate(ctx, tux,    0.0f, -0.1f * ctx->delta_time, 0.0f);

    sf_light_t *rainbow = sf_get_light(ctx, "rainbow");
    if (rainbow) {
      float t = ctx->elapsed_time * 2.0f;
      float r = sinf(t)             * 0.5f + 0.5f;
      float g = sinf(t + 2.094395f) * 0.5f + 0.5f;
      float b = sinf(t + 4.188790f) * 0.5f + 0.5f;
      rainbow->color = (sf_fvec3_t){r, g, b};
    }

    /* Update live FPS label and progress bar */
    snprintf(fps_buf, sizeof(fps_buf), "FPS: %.0f", ctx->fps);
    sf_ui_lmn_t *fps_lbl = NULL;
    sf_ui_lmn_t *prog    = NULL;
    for (int i = 0; i < ctx->ui->count; i++) {
      sf_ui_lmn_t *el = &ctx->ui->elements[i];
      if (el->type == SF_UI_LABEL    && el->label.text    == (const char*)fps_buf) fps_lbl = el;
      if (el->type == SF_UI_PROGRESS && el->progress.label != NULL)                prog    = el;
    }
    (void)fps_lbl; /* text pointer already points to fps_buf, updates automatically */
    /* Drive the progress bar with teapot_speed normalized over [0, 5] */
    if (prog) prog->progress.value = teapot_speed / 5.0f;
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

    /* Initial Load */
    sf_load_world(&sf_ctx, world_path, "Example World");
    sf_cam_t* pip_cam = sf_get_cam(&sf_ctx, "pip_cam");
    sf_camera_look_at(&sf_ctx, pip_cam, (sf_fvec3_t){0.0f, 0.0f, 0.0f});

    sf_light_t *rainbow = sf_add_light(&sf_ctx, "rainbow", SF_LIGHT_POINT, (sf_fvec3_t){1.0f, 0.0f, 0.0f}, 8.0f);
    if (rainbow) {
      rainbow->frame->pos = (sf_fvec3_t){3.0f, -4.0f, 0.0f};
      rainbow->frame->is_dirty = true;
    }

    /* --- INITIALIZE UI ELEMENTS --- */

    /* sf_ui_begin_panel creates the panel and sets up the internal layout.
       All sf_row_* calls between begin and end are automatically positioned
       and parented to the panel (so it can be dragged as a unit). */
    sf_ui_begin_panel(&sf_ctx, "Controls", 5, 5, 160, 4);

    /* Live FPS label — text pointer points to fps_buf, updated each frame */
    sf_row_label(&sf_ctx, fps_buf, 14, 0);

    sf_row_button(&sf_ctx, "Random Theme", 24, btn_test_cb, NULL);

    sf_ui_lmn_t *sldr = sf_row_slider(&sf_ctx, "Speed", 20, 0.0f, 5.0f, teapot_speed, slider_test_cb, NULL);
    sldr->slider.userdata = sldr;

    sf_row_progress(&sf_ctx, "Power", 20, teapot_speed / 5.0f);

    sf_ui_lmn_t *chk = sf_row_checkbox(&sf_ctx, "Debug Overlay", 20, draw_axes, checkbox_test_cb, NULL);
    chk->checkbox.userdata = chk;

    sf_row_label(&sf_ctx, "-- Scene --", 14, 0);

    const char *mesh_names[] = {"tux", "teapot", "cube"};
    sf_ui_lmn_t *ddwn = sf_row_dropdown(&sf_ctx, 3, mesh_names, 0, 20, dropdown_cb, NULL);
    ddwn->dropdown.userdata = ddwn;

    sf_ui_lmn_t *ti = sf_row_textinput(&sf_ctx, "Enter name...", 20, 32, textinput_cb, NULL);
    ti->textinput.userdata = ti;

    sf_ui_end_panel(&sf_ctx);


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
        sf_rect(&sf_ctx, &sf_ctx.main_camera, SF_CLR_WHITE, (sf_ivec2_t){width-pip_cam->w-22, height-pip_cam->h-22}, (sf_ivec2_t){width-19, height-19});
        sf_draw_cam_pip(&sf_ctx, &sf_ctx.main_camera, pip_cam, (sf_ivec2_t){width-pip_cam->w-20,height-pip_cam->h-20}); 
        
        // Tied to Checkbox!
        if (draw_axes) {
            sf_draw_debug_ovrlay(&sf_ctx, &sf_ctx.main_camera);
        }

        // --- RENDER UI ON TOP OF 3D ---
        sf_render_ui(&sf_ctx, &sf_ctx.main_camera, sf_ctx.ui);

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
