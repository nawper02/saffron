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

#define ASSET_PATH "/usr/local/share/saffron/assets/mk2.obj"

volatile int running = 1;

void handle_sigint(int sig) {
    running = 0;
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

    /* --- THE FIX: BEGIN SYNCHRONIZED UPDATE --- */
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
    
    /* --- THE FIX: END SYNCHRONIZED UPDATE --- */
    printf("\x1b[?2026l");
    
    fflush(stdout);
    free(rgb);
    free(b64);
}

/* --- CALLBACKS --- */
void on_render_start(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    /* 1. Spin the MK2 */
    sf_enti_t *mk2 = sf_get_enti(ctx, "mk2");
    if (mk2) {
        sf_enti_rotate(mk2, 1.0f * ctx->delta_time, 0.5f * ctx->delta_time, 0.0f);
    }

    /* 2. Update Light Color */
    if (ctx->light_count > 0) {
        sf_light_t *light = &ctx->lights[0];
        light->color.x = (sinf(ctx->elapsed_time * 1.5f) + 1.0f) * 0.5f;
        light->color.y = (sinf(ctx->elapsed_time * 2.0f) + 1.0f) * 0.5f;
        light->color.z = (sinf(ctx->elapsed_time * 0.7f) + 1.0f) * 0.5f;
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

    /* Setup Camera */
    sf_camera_set_psp(&sf_ctx.camera, 60.0f, 0.1f, 100.0f);
    sf_camera_set_pos(&sf_ctx.camera, 0.0f, 0.0f, 6.0f);
    sf_camera_look_at(&sf_ctx.camera, (sf_fvec3_t){0.0f, 0.0f, 0.0f});

    /* Load mk2 from the defined system path */
    sf_obj_t* mk2_obj = sf_load_obj(&sf_ctx, ASSET_PATH, "mk2_mesh");
    if (mk2_obj) {
        sf_enti_t* mk2_enti = sf_add_enti(&sf_ctx, mk2_obj, "mk2");
        sf_enti_set_scale(mk2_enti, 12.0f, 12.0f, 12.0f);
    }

    /* Initial light source */
    sf_add_light_dir(&sf_ctx, (sf_fvec3_t){1.0f, -1.0f, -1.0f}, (sf_fvec3_t){1.0f, 1.0f, 1.0f}, 1.2f);
    sf_reg_event(&sf_ctx, SF_EVT_RENDER_START, on_render_start, NULL);

    /* Increase stdout buffer size to prevent tearing/flickering */
    setvbuf(stdout, NULL, _IOFBF, 65536);

    /* Clear screen and hide cursor */
    printf("\x1b[2J\x1b[?25l");

    while (running) {
        sf_time_update(&sf_ctx);
        sf_render_ctx(&sf_ctx);

        render_to_terminal_pixels((uint32_t*)sf_ctx.buffer, render_w, render_h, 3);

        /* Cap frame rate to ~30 FPS */
        sleep_ms(33); 
    }

    /* Restore terminal on exit: clear screen, show cursor */
    printf("\x1b[2J\x1b[?25h");
    
    sf_destroy(&sf_ctx);
    return 0;
}
