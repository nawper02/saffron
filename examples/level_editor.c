#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>

/* --- EDITOR CONSTANTS --- */
#define UI_PANEL_WIDTH 220
#define UI_PADDING 10
#define UI_BTN_HEIGHT 25

/* --- EDITOR STATE --- */
typedef enum { MODE_TRANSLATE, MODE_ROTATE, MODE_SCALE } edit_mode_t;

struct {
    sf_enti_t* selected_enti;
    int        selected_idx;
    edit_mode_t mode;
    bool       show_grid;
    char       status_msg[64];
} g_editor;

/* --- UI PRIMITIVES --- */

bool gui_button(sf_ctx_t *ctx, const char* text, sf_ivec2_t pos, int w, int h) {
    int mx = ctx->input.mouse_x;
    int my = ctx->input.mouse_y;
    bool hover = (mx >= pos.x && mx <= pos.x + w && my >= pos.y && my <= pos.y + h);
    bool clicked = hover && ctx->input.mouse_btns[SF_MOUSE_LEFT] && !ctx->input.mouse_btns_prev[SF_MOUSE_LEFT];

    sf_pkd_clr_t color = hover ? SF_CLR_GREEN : ((sf_pkd_clr_t)0xFF444444);
    if (hover && ctx->input.mouse_btns[SF_MOUSE_LEFT]) color = SF_CLR_WHITE;

    sf_rect(ctx, color, pos, (sf_ivec2_t){pos.x + w, pos.y + h});
    sf_put_text(ctx, text, (sf_ivec2_t){pos.x + 5, pos.y + 5}, hover ? SF_CLR_BLACK : SF_CLR_WHITE, 1);
    
    return clicked;
}

void draw_ui_panel(sf_ctx_t *ctx) {
    // Background Sidebar
    sf_rect(ctx, (sf_pkd_clr_t)0xFF222222, (sf_ivec2_t){0, 0}, (sf_ivec2_t){UI_PANEL_WIDTH, ctx->h});
    sf_line(ctx, SF_CLR_WHITE, (sf_ivec2_t){UI_PANEL_WIDTH, 0}, (sf_ivec2_t){UI_PANEL_WIDTH, ctx->h});

    sf_put_text(ctx, "SAFFRON EDITOR v1.0", (sf_ivec2_t){10, 10}, SF_CLR_WHITE, 1);
    
    // --- MODE SELECTION ---
    int y = 40;
    sf_put_text(ctx, "EDIT MODE:", (sf_ivec2_t){10, y}, SF_CLR_GREEN, 1); y += 20;
    if (gui_button(ctx, "TRANSLATE [1]", (sf_ivec2_t){10, y}, 180, UI_BTN_HEIGHT)) g_editor.mode = MODE_TRANSLATE; y += 30;
    if (gui_button(ctx, "ROTATE    [2]", (sf_ivec2_t){10, y}, 180, UI_BTN_HEIGHT)) g_editor.mode = MODE_ROTATE; y += 30;
    if (gui_button(ctx, "SCALE     [3]", (sf_ivec2_t){10, y}, 180, UI_BTN_HEIGHT)) g_editor.mode = MODE_SCALE; y += 40;

    // --- ENTITY LIST ---
    sf_put_text(ctx, "ENTITIES:", (sf_ivec2_t){10, y}, SF_CLR_GREEN, 1); y += 20;
    for (int i = 0; i < ctx->enti_count; i++) {
        char label[32];
        snprintf(label, 32, "%s %s", (g_editor.selected_idx == i) ? ">" : " ", ctx->entities[i].name);
        if (gui_button(ctx, label, (sf_ivec2_t){10, y}, 180, 20)) {
            g_editor.selected_enti = &ctx->entities[i];
            g_editor.selected_idx = i;
        }
        y += 22;
    }

    // --- WORLD ACTIONS ---
    y = ctx->h - 100;
    if (gui_button(ctx, "SAVE WORLD (world.sfw)", (sf_ivec2_t){10, y}, 180, UI_BTN_HEIGHT)) {
        FILE* f = fopen("world.sfw", "w");
        for(int i=0; i<ctx->enti_count; i++) {
            sf_enti_t* e = &ctx->entities[i];
            fprintf(f, "e %s %s %f %f %f %f %f %f %f %f %f\n", 
                e->obj.name, e->name, e->pos.x, e->pos.y, e->pos.z, 
                e->rot.x, e->rot.y, e->rot.z, e->scale.x, e->scale.y, e->scale.z);
        }
        fclose(f);
        strncpy(g_editor.status_msg, "Saved to world.sfw", 64);
    }
    y += 30;
    if (gui_button(ctx, "LOAD WORLD", (sf_ivec2_t){10, y}, 180, UI_BTN_HEIGHT)) {
        sf_load_world(ctx, "world.sfw", "MyWorld");
        strncpy(g_editor.status_msg, "Loaded world.sfw", 64);
    }

    // Status Message
    sf_put_text(ctx, g_editor.status_msg, (sf_ivec2_t){UI_PANEL_WIDTH + 20, ctx->h - 30}, SF_CLR_GREEN, 1);
}

/* --- LOGIC CALLBACKS --- */

void on_render_start(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    sf_camera_t *cam = &ctx->camera;
    
    // 1. Scene Navigation (Right Mouse held)
    if (ctx->input.mouse_btns[SF_MOUSE_RIGHT]) {
        float speed = 10.0f * ctx->delta_time;
        if (sf_key_down(ctx, SF_KEY_W)) sf_camera_move_loc(cam, speed, 0, 0);
        if (sf_key_down(ctx, SF_KEY_S)) sf_camera_move_loc(cam, -speed, 0, 0);
        if (sf_key_down(ctx, SF_KEY_A)) sf_camera_move_loc(cam, 0, -speed, 0);
        if (sf_key_down(ctx, SF_KEY_D)) sf_camera_move_loc(cam, 0, speed, 0);
        
        sf_camera_add_yp(cam, ctx->input.mouse_dx * 0.2f, -ctx->input.mouse_dy * 0.2f);
    }

    // 2. Gizmo / Transform Logic
    if (g_editor.selected_enti && ctx->input.mouse_btns[SF_MOUSE_LEFT] && ctx->input.mouse_x > UI_PANEL_WIDTH) {
        float dx = ctx->input.mouse_dx * 0.05f;
        float dy = -ctx->input.mouse_dy * 0.05f;

        switch(g_editor.mode) {
            case MODE_TRANSLATE:
                sf_enti_move(g_editor.selected_enti, dx, dy, 0);
                break;
            case MODE_ROTATE:
                sf_enti_rotate(g_editor.selected_enti, dy, dx, 0);
                break;
            case MODE_SCALE:
                g_editor.selected_enti->scale.x += dx;
                g_editor.selected_enti->scale.y += dx;
                g_editor.selected_enti->scale.z += dx;
                g_editor.selected_enti->is_dirty = true;
                break;
        }
    }
}

void on_key_down(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    if (ev->key == SF_KEY_1) g_editor.mode = MODE_TRANSLATE;
    if (ev->key == SF_KEY_2) g_editor.mode = MODE_ROTATE;
    if (ev->key == SF_KEY_3) g_editor.mode = MODE_SCALE;
}

int main(int argc, char* argv[]) {
    const int w = 683, h = 384;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win = SDL_CreateWindow("Saffron Level Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

    sf_ctx_t ctx;
    sf_init(&ctx, w, h);
    
    // Default Scene
    sf_camera_set_pos(&ctx.camera, 0, 2, 10);
    sf_add_light_dir(&ctx, (sf_fvec3_t){-1, -1, -1}, (sf_fvec3_t){1, 1, 1}, 0.8f);
    
    // Load some assets
    sf_obj_t* cube_mesh = sf_load_obj(&ctx, "../assets/cube.obj", "cube");
    if(cube_mesh) {
        sf_enti_t* e1 = sf_add_enti(&ctx, cube_mesh, "Ground");
        sf_enti_set_pos(e1, 0, -2, 0);
        sf_enti_set_scale(e1, 10, 0.2f, 10);
        
        sf_enti_t* e2 = sf_add_enti(&ctx, cube_mesh, "PlayerStart");
        sf_enti_set_pos(e2, 0, 0, 0);
    }

    sf_reg_event(&ctx, SF_EVT_RENDER_START, on_render_start, NULL);
    sf_reg_event(&ctx, SF_EVT_KEY_DOWN, on_key_down, NULL);

    bool running = true;
    SDL_Event ev;
    while (running) {
        sf_input_cycle_state(&ctx);
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            sf_sdl_process_event(&ctx, &ev);
        }

        sf_time_update(&ctx);
        sf_render_ctx(&ctx);

        // Draw selection gizmo (Simplified)
        if (g_editor.selected_enti) {
            sf_fvec3_t p = g_editor.selected_enti->pos;
            // Project 3D position to 2D to draw a "selection bracket"
            sf_fvec3_t screen_p = _sf_project_vertex(&ctx, sf_fmat4_mul_vec3(ctx.camera.V, p), ctx.camera.P);
            sf_rect(&ctx, SF_CLR_GREEN, (sf_ivec2_t){screen_p.x-5, screen_p.y-5}, (sf_ivec2_t){screen_p.x+5, screen_p.y+5});
        }

        draw_ui_panel(&ctx);
        sf_draw_debug_axes(&ctx);

        SDL_UpdateTexture(tex, NULL, ctx.buffer, ctx.w * sizeof(sf_pkd_clr_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    sf_destroy(&ctx);
    return 0;
}
