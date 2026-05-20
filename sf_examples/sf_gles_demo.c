/*
 * sf_gles_demo.c  --  Render a saffron world on the GPU via EGL+GLES3 and
 *                     save the resulting frame as PNG.  No window, no SDL.
 *                     Intended target: Qualcomm RB5 (Adreno 650, Freedreno),
 *                     but runs on any Mesa desktop too.
 *
 *   build (via the project's CMakeLists.txt):  make sf_gles_demo
 *   run:                                       ./sf_gles_demo  [out.png]
 */

#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_GLES_IMPLEMENTATION
#include "sf_extras/sf_gles.h"
#undef SAFFRON_GLES_IMPLEMENTATION

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "sf_extras/stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

int main(int argc, char **argv) {
  const int W = 1280;
  const int H = 720;
  const char *out_path = (argc > 1) ? argv[1] : "sf_gles_demo.png";
  const char *world    = SF_ASSET_PATH "/sf_sff/example.sff";

  /* 1. Set up saffron context. main_camera is initialized inside sf_init. */
  sf_ctx_t ctx;
  sf_init(&ctx, W, H);

  /* 2. Load the world. This populates entities, textures, lights, cameras. */
  sf_load_sff(&ctx, world, "demo world");

  /* 3. Add a point light so geometry isn't pitch black if the world has none. */
  sf_light_t *fill = sf_add_light(&ctx, "demo_fill", SF_LIGHT_POINT,
                                  (sf_fvec3_t){1.0f, 1.0f, 1.0f}, 6.0f);
  if (fill && fill->frame) {
    fill->frame->pos = (sf_fvec3_t){4.0f, 6.0f, 4.0f};
    fill->frame->is_dirty = true;
  }

  /* 4. Bring up the GLES backend (offscreen FBO sized to the camera). */
  sf_gles_t gl;
  if (!sf_gles_init(&gl, W, H)) {
    fprintf(stderr, "sf_gles_init failed\n");
    return 1;
  }

  /* 5. Render one frame. First call also lazily uploads VBOs/textures, so
     time both that and a clean "second frame" to separate setup from steady
     state. sf_gles_render_ctx ends with glFinish(), so these timings include
     the actual GPU work. */
  double t0 = now_ms();
  sf_gles_render_ctx(&gl, &ctx, &ctx.main_camera);
  double t1 = now_ms();
  sf_gles_render_ctx(&gl, &ctx, &ctx.main_camera);
  double t2 = now_ms();
  printf("render: first=%.2f ms (incl. upload), second=%.2f ms (steady)\n",
         t1 - t0, t2 - t1);

  /* 6. Read back and save. */
  uint8_t *rgba = (uint8_t*)malloc((size_t)W * H * 4);
  double tr0 = now_ms();
  sf_gles_readback_rgba(&gl, W, H, rgba);
  double tr1 = now_ms();
  printf("readback (%dx%d RGBA): %.2f ms\n", W, H, tr1 - tr0);
  if (!stbi_write_png(out_path, W, H, 4, rgba, W * 4)) {
    fprintf(stderr, "stbi_write_png failed for %s\n", out_path);
    free(rgba);
    sf_gles_destroy(&gl);
    sf_destroy(&ctx);
    return 1;
  }
  printf("wrote %s  (%d x %d, %d entities)\n", out_path, W, H, ctx.enti_count);

  free(rgba);
  sf_gles_destroy(&gl);
  sf_destroy(&ctx);
  return 0;
}
