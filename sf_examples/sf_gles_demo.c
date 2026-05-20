/* sf_gles_demo.c
 * Render a saffron world on the GPU and save the frame as PNG.
 * No window, no SDL.  Built for Qualcomm RB5 (Adreno 650 / Freedreno)
 * but runs on any Mesa desktop too.
 *
 *   build:  make sf_gles_demo
 *   run:    ./sf_gles_demo  [out.png]
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

  /* Saffron context drives geometry, cameras, lights, frames. */
  sf_ctx_t ctx;
  sf_init(&ctx, W, H);
  sf_load_sff(&ctx, world, "demo world");

  sf_light_t *fill = sf_add_light(&ctx, "demo_fill", SF_LIGHT_POINT,
                                  (sf_fvec3_t){1.0f, 1.0f, 1.0f}, 6.0f);
  if (fill && fill->frame) {
    fill->frame->pos = (sf_fvec3_t){4.0f, 6.0f, 4.0f};
    fill->frame->is_dirty = true;
  }

  /* GPU backend: one renderer + one render target sized to the camera. */
  sf_gles_t     gl;
  sf_gles_tgt_t tgt;
  if (!sf_gles_init(&gl, &ctx))            { fprintf(stderr, "sf_gles_init failed\n");     return 1; }
  if (!sf_gles_tgt_init(&gl, &tgt, W, H))  { fprintf(stderr, "sf_gles_tgt_init failed\n"); return 1; }

  /* First call lazily uploads VBOs/textures; second is steady-state. */
  double t0 = now_ms();
  sf_gles_render_ctx(&gl, &ctx, &ctx.main_camera, &tgt);
  double t1 = now_ms();
  sf_gles_render_ctx(&gl, &ctx, &ctx.main_camera, &tgt);
  double t2 = now_ms();
  printf("render: first=%.2f ms (incl. upload), second=%.2f ms (steady)\n",
         t1 - t0, t2 - t1);

  uint8_t *rgba = (uint8_t*)malloc((size_t)W * H * 4);
  double tr0 = now_ms();
  sf_gles_tgt_readback(&gl, &tgt, rgba);
  double tr1 = now_ms();
  printf("readback (%dx%d RGBA): %.2f ms\n", W, H, tr1 - tr0);

  if (!stbi_write_png(out_path, W, H, 4, rgba, W * 4)) {
    fprintf(stderr, "stbi_write_png failed for %s\n", out_path);
    free(rgba); sf_gles_tgt_destroy(&gl, &tgt); sf_gles_destroy(&gl); sf_destroy(&ctx);
    return 1;
  }
  printf("wrote %s  (%d x %d, %d entities)\n", out_path, W, H, ctx.enti_count);

  free(rgba);
  sf_gles_tgt_destroy(&gl, &tgt);
  sf_gles_destroy(&gl);
  sf_destroy(&ctx);
  return 0;
}
