/* Spinning MK2 for Kitty terminals */
#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <signal.h>

#include <unistd.h>
#define sleep_ms(x) usleep((x) * 1000)

#define ASSET_PATH "/sf_objs/mk2.obj"

static sf_ctx_t *g_sf_ctx = NULL;

void handle_sigint(int sig) {
  if (g_sf_ctx) sf_stop(g_sf_ctx);
}

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* base64_encode(const uint8_t *data, size_t input_length, size_t *output_length) {
    *output_length = 4 * ((input_length + 2) / 3);
    char *encoded = malloc(*output_length + 1);
    if (encoded == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded[j++] = b64_table[(triple >> 3 * 6) & 0x3F];
        encoded[j++] = b64_table[(triple >> 2 * 6) & 0x3F];
        encoded[j++] = b64_table[(triple >> 1 * 6) & 0x3F];
        encoded[j++] = b64_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < (3 - input_length % 3) % 3; i++) {
        encoded[*output_length - 1 - i] = '=';
    }
    encoded[*output_length] = '\0';
    return encoded;
}

/* --- TRUE PIXEL TERMINAL RENDERER (Kitty Protocol) --- */
void render_to_terminal_pixels(uint32_t *buffer, int width, int height, int scale) {
    /* Calculate the new crisp resolution */
    int out_w = width * scale;
    int out_h = height * scale;

    /* 1. Extract raw RGB and apply Nearest-Neighbor scaling simultaneously */
    size_t rgb_size = out_w * out_h * 3;
    uint8_t *rgb = malloc(rgb_size);
    
    for(int y = 0; y < out_h; y++) {
        for(int x = 0; x < out_w; x++) {
            /* Find which original pixel this new scaled pixel belongs to */
            int src_x = x / scale;
            int src_y = y / scale;
            uint32_t pixel = buffer[src_y * width + src_x];
            
            int dst_idx = (y * out_w + x) * 3;
            rgb[dst_idx + 0] = (pixel >> 16) & 0xFF; // R
            rgb[dst_idx + 1] = (pixel >> 8)  & 0xFF; // G
            rgb[dst_idx + 2] = (pixel)       & 0xFF; // B
        }
    }

    /* 2. Base64 Encode the scaled RGB data */
    size_t b64_len;
    char *b64 = base64_encode(rgb, rgb_size, &b64_len);

    printf("\x1b[?2026h");

    /* Move cursor to top-left */
    printf("\x1b[H");

    /* 3. Send in chunks */
    size_t offset = 0;
    int first_chunk = 1;
    
    while(offset < b64_len) {
        size_t chunk = (b64_len - offset > 4096) ? 4096 : (b64_len - offset);
        int more = (offset + chunk < b64_len) ? 1 : 0; 
        
        if (first_chunk) {
            printf("\x1b_Ga=T,q=2,i=1,f=24,s=%d,v=%d,m=%d;", out_w, out_h, more);
            first_chunk = 0;
        } else {
            printf("\x1b_Gm=%d;", more);
        }
        
        fwrite(b64 + offset, 1, chunk, stdout);
        printf("\x1b\\");
        offset += chunk;
    }
    
    printf("\x1b[?2026l");
    
    fflush(stdout);
    free(rgb);
    free(b64);
}

/* --- CALLBACKS --- */
void on_render_end(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    /* 1. Spin the MK2 */
    sf_enti_t *mk2 = sf_get_enti(ctx, "mk2");
    if (mk2) {
        sf_enti_rotate(ctx, mk2, 1.0f * ctx->delta_time, 0.5f * ctx->delta_time, 0.0f);
    }

    /* 2. Light Color Prep */
    sf_fvec3_t l_clr = {1.0f, 1.0f, 1.0f};
    if (ctx->light_count > 0) {
        sf_light_t *light = &ctx->lights[0];
        light->color.x = (sinf(ctx->elapsed_time * 1.5f) + 1.0f) * 0.5f;
        light->color.y = (sinf(ctx->elapsed_time * 2.0f) + 1.0f) * 0.5f;
        light->color.z = (sinf(ctx->elapsed_time * 0.7f) + 1.0f) * 0.5f;
        l_clr = light->color;
    }

    /* 3. The Clockwise Scrolling Snake */
    const char *text = "SAFFRON   ";
    int text_len = (int)strlen(text);
    int char_spacing = 10; 
    float speed = 80.0f;
    
    int margin_lt = 6;  // Left/Top margin
    int margin_rb = 14; // Right/Bottom margin (extra room for character width)
    
    int W = ctx->camera.w - (margin_lt + margin_rb);
    int H = ctx->camera.h - (margin_lt + margin_rb);
    int perimeter = 2 * (W + H);
    
    // We subtract the offset to move 'forward' in a clockwise direction
    float scroll_offset = ctx->elapsed_time * speed;
    int total_chars = perimeter / char_spacing;

    for (int i = 0; i < total_chars; i++) {
        // 'd' is the position of this specific character in the snake
        // We use (scroll_offset - i * spacing) to make the tail follow the head
        float d = fmodf(scroll_offset - (i * char_spacing), (float)perimeter);
        if (d < 0) d += perimeter; 

        sf_ivec2_t pos;

        // CLOCKWISE Logic
        if (d < W) { 
            // Top Edge: Left to Right
            pos.x = margin_lt + (int)d;
            pos.y = margin_lt;
        } else if (d < (W + H)) { 
            // Right Edge: Top to Bottom
            pos.x = W + margin_lt;
            pos.y = margin_lt + (int)(d - W);
        } else if (d < (2 * W + H)) { 
            // Bottom Edge: Right to Left
            pos.x = (W + margin_lt) - (int)(d - (W + H));
            pos.y = H + margin_lt;
        } else { 
            // Left Edge: Bottom to Top
            pos.x = margin_lt;
            pos.y = (H + margin_lt) - (int)(d - (2 * W + H));
        }

        // FADE: i=0 is the head (brightest)
        float fade = 1.0f - ((float)i / total_chars);
        if (fade < 0.1f) fade = 0.1f; // Keep a faint ghosting for the tail

        uint8_t r = (uint8_t)(l_clr.x * 255 * fade);
        uint8_t g = (uint8_t)(l_clr.y * 255 * fade);
        uint8_t b = (uint8_t)(l_clr.z * 255 * fade);
        sf_pkd_clr_t char_clr = (0xFF << 24) | (b << 16) | (g << 8) | r;

        // The character at index 'i' in the snake
        char c[2] = { text[i % text_len], '\0' };
        
        sf_put_text(ctx, &ctx->camera, c, pos, char_clr, 1);
    }
}

/* --- MAIN PROGRAM --- */
int main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL));
    signal(SIGINT, handle_sigint);

    const int render_w = 200;
    const int render_h = 200;

    sf_ctx_t sf_ctx;
    sf_init(&sf_ctx, render_w, render_h);
    g_sf_ctx = &sf_ctx;

    /* Setup Camera */
    sf_camera_set_psp(&sf_ctx, &sf_ctx.camera, 60.0f, 0.1f, 100.0f);
    sf_camera_set_pos(&sf_ctx, &sf_ctx.camera, 0.0f, 0.0f, 6.0f);
    sf_camera_look_at(&sf_ctx, &sf_ctx.camera, (sf_fvec3_t){0.0f, 0.0f, 0.0f});

    /* Load mk2 from the defined system path */
    sf_obj_t* mk2_obj = sf_load_obj(&sf_ctx, SF_ASSET_PATH "/sf_objs/mk2.obj", "mk2_mesh");
    if (mk2_obj) {
        sf_enti_t* mk2_enti = sf_add_enti(&sf_ctx, mk2_obj, "mk2");
        sf_enti_set_scale(&sf_ctx, mk2_enti, 12.0f, 12.0f, 12.0f);
    }

    /* Initial light source */
    sf_add_light_dir(&sf_ctx, (sf_fvec3_t){1.0f, -1.0f, -1.0f}, (sf_fvec3_t){1.0f, 1.0f, 1.0f}, 1.2f);
    sf_event_reg(&sf_ctx, SF_EVT_RENDER_END, on_render_end, NULL);

    /* Increase stdout buffer size to prevent tearing/flickering */
    setvbuf(stdout, NULL, _IOFBF, 65536);

    /* Clear screen and hide cursor */
    printf("\x1b[2J\x1b[?25l");

    while (sf_running(&sf_ctx)) {
        sf_time_update(&sf_ctx);
        sf_render_cam(&sf_ctx, &sf_ctx.camera);

        render_to_terminal_pixels((uint32_t*)sf_ctx.camera.buffer, render_w, render_h, 3);

        /* Cap frame rate to ~30 FPS */
        sleep_ms(33); 
    }

    /* Restore terminal on exit: clear screen, show cursor */
    printf("\x1b[2J\x1b[?25h");
    
    sf_destroy(&sf_ctx);
    return 0;
}
