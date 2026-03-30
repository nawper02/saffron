/* Audio test app */
#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>

/* ── Audio playback state ────────────────────────────────── */
static sf_snd_t   *current_snd   = NULL;
static int32_t     play_cursor   = 0;
static float       master_vol    = 0.5f;
static bool        is_playing    = false;

/* SDL audio callback — called from audio thread */
static void audio_callback(void *userdata, Uint8 *stream, int len) {
    float *out = (float *)stream;
    int samples = len / sizeof(float);
    (void)userdata;

    for (int i = 0; i < samples; i++) {
        if (is_playing && current_snd && play_cursor < current_snd->length) {
            out[i] = current_snd->samples[play_cursor++] * master_vol;
        } else {
            out[i] = 0.0f;
        }
    }

    /* Loop */
    if (current_snd && play_cursor >= current_snd->length) {
        play_cursor = 0;
    }
}

/* ── UI state ────────────────────────────────────────────── */
static float freq = 440.0f;
static int   wave_type = 0;   /* 0=sine 1=saw 2=square 3=noise */
static const char *wave_names[] = {"SINE", "SAW", "SQUARE", "NOISE"};

static void regenerate(sf_ctx_t *ctx) {
    /* Reuse slot 0 — just overwrite the samples */
    is_playing = false;
    play_cursor = 0;

    switch (wave_type) {
        case 0: current_snd = sf_snd_gen_sine(ctx, "live", freq, 1.0f, 0.8f); break;
        case 1: current_snd = sf_snd_gen_saw(ctx, "live", freq, 1.0f, 0.8f); break;
        case 2: current_snd = sf_snd_gen_square(ctx, "live", freq, 1.0f, 0.8f); break;
        case 3: current_snd = sf_snd_gen_noise(ctx, "live", 1.0f, 0.8f); break;
    }

    is_playing = true;
}

/* ── UI callbacks ────────────────────────────────────────── */
static void freq_slider_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t *)userdata;
    freq = el->slider.value;
    regenerate(ctx);
}

static void vol_slider_cb(sf_ctx_t *ctx, void *userdata) {
    sf_ui_lmn_t *el = (sf_ui_lmn_t *)userdata;
    master_vol = el->slider.value;
}

static void wave_btn_cb(sf_ctx_t *ctx, void *userdata) {
    wave_type = (wave_type + 1) % 4;
    regenerate(ctx);
}

static void play_btn_cb(sf_ctx_t *ctx, void *userdata) {
    (void)ctx; (void)userdata;
    is_playing = !is_playing;
    if (is_playing) play_cursor = 0;
}

/* ── Render callback: draw waveform ──────────────────────── */
static void on_render_start(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
    (void)ev; (void)userdata;
    sf_cam_t *cam = &ctx->main_camera;
    int w = cam->w;
    int h = cam->h;

    /* Draw waveform visualization */
    if (current_snd && current_snd->length > 0) {
        int y_center = h / 2;
        int wave_h = h / 3;
        int prev_y = y_center;

        for (int x = 1; x < w; x++) {
            int si = (int)((float)x / w * current_snd->length);
            if (si >= current_snd->length) si = current_snd->length - 1;
            int cur_y = y_center - (int)(current_snd->samples[si] * wave_h);
            sf_line(ctx, cam, SF_CLR_GREEN,
                    (sf_ivec2_t){x - 1, prev_y},
                    (sf_ivec2_t){x, cur_y});
            prev_y = cur_y;
        }

        /* Playback cursor */
        if (is_playing) {
            int cx = (int)((float)play_cursor / current_snd->length * w);
            sf_line(ctx, cam, SF_CLR_RED,
                    (sf_ivec2_t){cx, 0}, (sf_ivec2_t){cx, h});
        }
    }

    /* Labels */
    char buf[64];
    snprintf(buf, sizeof(buf), "WAVE: %s", wave_names[wave_type]);
    sf_put_text(ctx, cam, buf, (sf_ivec2_t){10, 10}, SF_CLR_WHITE, 1);

    snprintf(buf, sizeof(buf), "FREQ: %.0f Hz", freq);
    sf_put_text(ctx, cam, buf, (sf_ivec2_t){10, 25}, SF_CLR_WHITE, 1);

    snprintf(buf, sizeof(buf), "VOL: %.0f%%", master_vol * 100.0f);
    sf_put_text(ctx, cam, buf, (sf_ivec2_t){10, 40}, SF_CLR_WHITE, 1);

    snprintf(buf, sizeof(buf), "%s", is_playing ? "PLAYING" : "STOPPED");
    sf_put_text(ctx, cam, buf, (sf_ivec2_t){10, 55},
                is_playing ? SF_CLR_GREEN : SF_CLR_RED, 1);
}

/* ── Main ────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    const int width = 640;
    const int height = 480;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    /* Open audio device */
    SDL_AudioSpec want = {0}, have;
    want.freq = SF_SAMPLE_RATE;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = audio_callback;

    SDL_AudioDeviceID audio_dev = SDL_OpenAudioDevice(
        NULL, 0, &want, &have, 0);

    if (audio_dev == 0) {
        fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Saffron Audio",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        width, height);

    sf_ctx_t ctx;
    sf_init(&ctx, width, height);

    sf_event_reg(&ctx, SF_EVT_RENDER_START, on_render_start, NULL);

    /* UI */
    sf_add_button(&ctx, "Wave Type",
        (sf_ivec2_t){10, height - 120}, (sf_ivec2_t){130, height - 95},
        wave_btn_cb, NULL);

    sf_ui_lmn_t *freq_sl = sf_add_slider(&ctx,
        (sf_ivec2_t){10, height - 85}, (sf_ivec2_t){200, height - 65},
        20.0f, 2000.0f, freq, freq_slider_cb, NULL);
    freq_sl->slider.userdata = freq_sl;

    sf_ui_lmn_t *vol_sl = sf_add_slider(&ctx,
        (sf_ivec2_t){10, height - 55}, (sf_ivec2_t){200, height - 35},
        0.0f, 1.0f, master_vol, vol_slider_cb, NULL);
    vol_sl->slider.userdata = vol_sl;

    sf_add_button(&ctx, "Play/Stop",
        (sf_ivec2_t){10, height - 25}, (sf_ivec2_t){130, height - 5},
        play_btn_cb, NULL);

    /* Generate initial sound and start playback */
    regenerate(&ctx);
    SDL_PauseAudioDevice(audio_dev, 0);

    SDL_Event event;
    while (sf_running(&ctx)) {
        sf_input_cycle_state(&ctx);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) sf_stop(&ctx);
            sf_sdl_process_event(&ctx, &event);
        }

        if (sf_key_pressed(&ctx, SF_KEY_Q)) sf_stop(&ctx);

        sf_time_update(&ctx);
        sf_update_ui(&ctx, ctx.ui);

        sf_fill(&ctx, &ctx.main_camera, SF_CLR_BLACK);
        sf_render_ui(&ctx, &ctx.main_camera, ctx.ui);

        SDL_UpdateTexture(texture, NULL, ctx.main_camera.buffer,
                          ctx.main_camera.w * sizeof(sf_pkd_clr_t));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_CloseAudioDevice(audio_dev);
    sf_destroy(&ctx);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

