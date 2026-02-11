/* saffron.h
 * Saffron is an stb-style graphics engine library.
 * Eventually it may become a game/simulatoin engine.
 */

/* SF_TODO
 *  Render text
 *  3D Math
 *  Render 3d objects
 *  ....
 *  Optimize. Primatives are not at all fast.
 */


/* SF_HEADER */
#ifndef SAFFRON_H
#define SAFFRON_H

#ifdef __cplusplus
extern "C" {
#endif

/* SF_INCLUDES */
#include <ctype.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* SF_DEFINES */
#define SF_ARENA_SIZE 10485760
#define SF_MAX_OBJS   10
#define SF_LOG(ctx, level, fmt, ...) _sf_log(ctx, level, __func__, fmt, ##__VA_ARGS__)
#define SF_LOG_INDENT "            "

/* SF_TYPES */
typedef void (*sf_log_fn)(const char* message, void* userdata);
typedef enum {
  SF_LOG_DEBUG,
  SF_LOG_INFO,
  SF_LOG_WARN,
  SF_LOG_ERROR
} sf_log_level_t;

typedef uint32_t sf_packed_color_t;
typedef struct { uint8_t  r, g, b, a; } sf_unpacked_color_t;

typedef struct { uint16_t x, y;       } sf_svec2_t;
typedef struct { uint16_t x, y, z;    } sf_svec3_t;
typedef struct { float    x, y, z;    } sf_fvec3_t;
typedef struct { int      x, y, z;    } sf_ivec3_t;

typedef struct {
  sf_fvec3_t *v;
  sf_ivec3_t *f;
  int32_t v_cnt;
  int32_t f_cnt;
} sf_obj_t;

typedef struct {
  size_t size;
  size_t offset;
  uint8_t *buffer;
} sf_arena_t;
typedef struct {
  int w;
  int h;
  int buffer_size;
  sf_packed_color_t *buffer;

  sf_arena_t arena;
  int        arena_size;

  sf_obj_t *objs;
  int32_t   obj_count;

  sf_log_fn      log_cb;
  void*          log_user;
  sf_log_level_t log_min;
} sf_ctx_t;

/* SF_CORE_FUNCTIONS */
void       sf_init           (sf_ctx_t *ctx, int w, int h);
void       sf_destroy        (sf_ctx_t *ctx);
sf_arena_t sf_arena_init     (size_t size);
void*      sf_arena_alloc    (sf_ctx_t *ctx, sf_arena_t *arena, size_t size);
void       sf_set_logger     (sf_ctx_t *ctx, sf_log_fn log_cb, void* userdata);
void       sf_logger_console (const char* message, void* userdata);
void       sf_log            (sf_ctx_t *ctx, sf_log_level_t level, const char* fmt, ...);
void       _sf_log           (sf_ctx_t *ctx, sf_log_level_t level, const char* func, const char* fmt, ...);
sf_obj_t*  sf_load_obj       (sf_ctx_t *ctx, const char *filename);

/* SF_DRAWING_FUNCTIONS */
void sf_fill     (sf_ctx_t *ctx, sf_packed_color_t c);
void sf_pixel    (sf_ctx_t *ctx, sf_packed_color_t c, sf_svec2_t v0);
void sf_line     (sf_ctx_t *ctx, sf_packed_color_t c, sf_svec2_t v0, sf_svec2_t v1);
void sf_rect     (sf_ctx_t *ctx, sf_packed_color_t c, sf_svec2_t v0, sf_svec2_t v1);
void sf_tri      (sf_ctx_t *ctx, sf_packed_color_t c, sf_svec2_t v0, sf_svec2_t v1, sf_svec2_t v2);
void sf_put_text (sf_ctx_t *ctx, const char *text, sf_svec2_t p, sf_packed_color_t c, int scale);

/* SF_IMPLEMENTATION_HELPERS */
uint32_t    _sf_vec_to_index    (sf_ctx_t *ctx, sf_svec2_t v);
void        _sf_swap_svec2      (sf_svec2_t *v0, sf_svec2_t *v1);
void        _sf_interpolate_x   (sf_svec2_t  v0, sf_svec2_t v1, uint16_t *xs);
void        _sf_interpolate_y   (sf_svec2_t  v0, sf_svec2_t v1, uint16_t *ys);
const char* _sf_log_lvl_to_str  (sf_log_level_t level);

/* SF_UTILITIES */
sf_packed_color_t sf_pack_color           (sf_unpacked_color_t);
size_t            sf_get_obj_memory_usage (sf_obj_t *obj);

/* SF_DEFINES */
#define SF_COLOR_RED   ((sf_packed_color_t)0xFFFF0000)
#define SF_COLOR_GREEN ((sf_packed_color_t)0xFF00FF00)
#define SF_COLOR_BLUE  ((sf_packed_color_t)0xFF0000FF)
#define SF_COLOR_BLACK ((sf_packed_color_t)0xFF000000)
#define SF_COLOR_WHITE ((sf_packed_color_t)0xFFFFFFFF)

#ifdef __cplusplus
}
#endif
#endif /* SAFFRON_H */

/* SF_IMPLEMENTATION */
#ifdef SAFFRON_IMPLEMENTATION

/* SF_CORE_FUNCTIONS */
void sf_init(sf_ctx_t *ctx, int w, int h) {
  ctx->w             = w;
  ctx->h             = h;
  ctx->buffer_size   = w * h;
  ctx->buffer        = (sf_packed_color_t*)malloc(w*h*sizeof(sf_packed_color_t));
  ctx->arena         = sf_arena_init(SF_ARENA_SIZE);
  ctx->log_cb        = sf_logger_console;
  ctx->log_user      = NULL;
  ctx->log_min       = SF_LOG_DEBUG;
  ctx->objs          = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_OBJS * sizeof(sf_obj_t));
  ctx->obj_count     = 0;
  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "buffer : %dx%d\n"
              SF_LOG_INDENT "memory : %d\n"
              SF_LOG_INDENT "mxobjs : %d\n", 
              ctx->w, ctx->h, SF_ARENA_SIZE, SF_MAX_OBJS);
}

void sf_destroy(sf_ctx_t *ctx) {
  free(ctx->buffer);
  free(ctx->arena.buffer);
  ctx->arena.offset = 0;
  ctx->buffer       = NULL;
  ctx->buffer_size  = 0;
  ctx->w            = 0;
  ctx->h            = 0;
  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "buffer : %dx%d\n"
              SF_LOG_INDENT "memory : %d\n"
              SF_LOG_INDENT "mxobjs : %d\n", 
              ctx->w, ctx->h, SF_ARENA_SIZE, SF_MAX_OBJS);
}

sf_arena_t sf_arena_init(size_t size) {
  sf_arena_t arena;
  arena.size = size;
  arena.offset = 0;
  arena.buffer = malloc(size);
  return arena;
}

void* sf_arena_alloc(sf_ctx_t *ctx, sf_arena_t *arena, size_t size) {
  if (arena->offset + size > arena->size) return NULL;
  void *ptr = &arena->buffer[arena->offset];
  arena->offset += size;
  SF_LOG(ctx, SF_LOG_DEBUG,
              SF_LOG_INDENT "size : %zu\n"
              SF_LOG_INDENT "used : %zu\n"
              SF_LOG_INDENT "free : %zu\n", 
              size, arena->offset, arena->size - arena->offset);

  return ptr;
}

void sf_set_logger(sf_ctx_t *ctx, sf_log_fn callback, void* userdata) {
  ctx->log_cb = callback;
  ctx->log_user = userdata;
}

void sf_logger_console(const char* message, void* userdata) {
  fprintf(stdout, "%s", message);
}

void _sf_log(sf_ctx_t *ctx, sf_log_level_t level, const char* func, const char* fmt, ...) {
  if (!ctx->log_cb || level < ctx->log_min) return;

  char msg_buffer[512];
  char final_buffer[640];

  va_list args;
  va_start(args, fmt);
  vsnprintf(msg_buffer, sizeof(msg_buffer), fmt, args);
  va_end(args);

  snprintf(final_buffer, sizeof(final_buffer), "%s :: %s\n%s",
           func, _sf_log_lvl_to_str(level), msg_buffer);

  ctx->log_cb(final_buffer, ctx->log_user);
}

sf_obj_t* sf_load_obj(sf_ctx_t *ctx, const char *filename) {
  if (ctx->obj_count >= SF_MAX_OBJS) {
    SF_LOG(ctx, SF_LOG_ERROR, "max objects (%d) reached\n", SF_MAX_OBJS);
    return NULL;
  }

  FILE *f = fopen(filename, "r");
  if (!f) {
    SF_LOG(ctx, SF_LOG_ERROR, "could not open %s\n", filename);
    return NULL;
  }

  int32_t v_cnt = 0, f_cnt = 0; char line[256];
  while (fgets(line, sizeof(line), f)) {
    if      (line[0] == 'v' && line[1] == ' ') v_cnt++;
    else if (line[0] == 'f' && line[1] == ' ') f_cnt++;
  }

  sf_obj_t *obj = &ctx->objs[ctx->obj_count++];
  obj->f_cnt = f_cnt; obj->v_cnt = v_cnt;
  obj->v = sf_arena_alloc(ctx, &ctx->arena, v_cnt * sizeof(sf_fvec3_t));
  obj->f = sf_arena_alloc(ctx, &ctx->arena, f_cnt * sizeof(sf_ivec3_t));

  if (!obj->v || !obj->f) {
    SF_LOG(ctx, SF_LOG_ERROR, "out of arena memory for %s\n", filename);
    fclose(f);
    return NULL;
  }

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "file   : %s\n"
              SF_LOG_INDENT "verts  : %d\n"
              SF_LOG_INDENT "faces  : %d\n"
              SF_LOG_INDENT "size   : %d\n"
              SF_LOG_INDENT "mem    : %.2f\n"
              SF_LOG_INDENT "obj_id : %d\n",
              filename, v_cnt, f_cnt, 
              sf_get_obj_memory_usage(obj), 
              ((float)ctx->arena.offset / (float)ctx->arena.size) * 100.0f,
              ctx->obj_count - 1);

  rewind(f);
  int v_idx = 0, f_idx = 0;
  while (fgets(line, sizeof(line), f)) {
    if (line[0] == 'v' && line[1] == ' ') {
      sscanf(line, "v %f %f %f", &obj->v[v_idx].x, &obj->v[v_idx].y, &obj->v[v_idx].z);
      v_idx++;
    }
    else if (line[0] == 'f' && line[1] == ' ') {
      sscanf(line, "f %d %d %d", &obj->f[f_idx].x, &obj->f[f_idx].y, &obj->f[f_idx].z);
      obj->f[f_idx].x -= 1; obj->f[f_idx].y -= 1; obj->f[f_idx].z -= 1;
      f_idx++;
    }
  }

  fclose(f);
  return obj;
}

/* SF_DRAWING_FUNCTIONS */

void sf_fill(sf_ctx_t *ctx, sf_packed_color_t c) {
 for(size_t i = 0; i < ctx->buffer_size; ++i) { ctx->buffer[i] = c; }
}

void sf_pixel(sf_ctx_t *ctx, sf_packed_color_t c, sf_svec2_t v0) {
  if (v0.x >= ctx->w || v0.y >= ctx->h) return;
  ctx->buffer[_sf_vec_to_index(ctx, v0)] = c;
}

void sf_line(sf_ctx_t *ctx, sf_packed_color_t c, sf_svec2_t v0, sf_svec2_t v1) {
  if (v1.y - v0.y > v1.x - v0.x) {
    uint16_t x01[v1.y-v0.y+1];
    _sf_interpolate_x(v0, v1, x01);
    for (uint16_t y = v0.y; y <= v1.y; ++y) {
      sf_pixel(ctx, c, (sf_svec2_t){x01[y-v0.y],y});
    }
  }
  else {
    uint16_t y01[v1.x-v0.x+1];
    _sf_interpolate_y(v0, v1, y01);
    for (uint16_t x = v0.x; x <= v1.x; ++x) {
      sf_pixel(ctx, c, (sf_svec2_t){x,y01[x-v0.x]});
    }
  }
}

void sf_rect(sf_ctx_t *ctx, sf_packed_color_t c, sf_svec2_t v0, sf_svec2_t v1) {
  uint16_t l = (v0.x < v1.x) ? v0.x : v1.x;
  uint16_t r = (v0.x > v1.x) ? v0.x : v1.x;
  uint16_t t = (v0.y < v1.y) ? v0.y : v1.y;
  uint16_t b = (v0.y > v1.y) ? v0.y : v1.y;
  for (uint16_t y = t; y <= b; ++y) {
    for (uint16_t x = l; x <= r; ++x) {
      sf_pixel(ctx, c, (sf_svec2_t){x,y});
    }
  }
}

void sf_tri(sf_ctx_t *ctx, sf_packed_color_t c, sf_svec2_t v0, sf_svec2_t v1, sf_svec2_t v2) {
  if (v1.y < v0.y) _sf_swap_svec2(&v0, &v1);
  if (v2.y < v0.y) _sf_swap_svec2(&v2, &v0);
  if (v2.y < v1.y) _sf_swap_svec2(&v2, &v1);
  uint16_t h = v2.y - v0.y + 1;
  uint16_t x02[h], x012[h];
  _sf_interpolate_x(v0, v1, x012);
  _sf_interpolate_x(v1, v2, &x012[v1.y - v0.y]);
  _sf_interpolate_x(v0, v2, x02);
  uint16_t mid_idx = v1.y - v0.y;
  uint16_t *xl = (x02[mid_idx] < x012[mid_idx]) ? x02  : x012;
  uint16_t *xr = (x02[mid_idx] < x012[mid_idx]) ? x012 : x02;
  for (uint16_t y = v0.y; y <= v2.y; ++y) {
    for (uint16_t x = xl[y - v0.y]; x <= xr[y - v0.y]; ++x) {
      sf_pixel(ctx, c, (sf_svec2_t){x, y});
    }
  }
}

/* SF_IMPLEMENTATION_HELPERS */
uint32_t _sf_vec_to_index(sf_ctx_t *ctx, sf_svec2_t v) {
  return v.y * ctx->w + v.x;
}

void _sf_swap_svec2(sf_svec2_t *v0, sf_svec2_t *v1) {
  sf_svec2_t t = *v0; *v0 = *v1; *v1 = t;
}

void _sf_interpolate_x(sf_svec2_t v0, sf_svec2_t v1, uint16_t *xs) {
  if (v0.y == v1.y) {
    xs[0] = v0.x;
    return;
  }
  for (uint16_t y = v0.y; y <= v1.y; ++y) {
    xs[y - v0.y] = (y - v0.y) * (v1.x - v0.x) / (v1.y - v0.y) + v0.x;
  }
}

void _sf_interpolate_y(sf_svec2_t v0, sf_svec2_t v1, uint16_t *ys) {
  if (v0.x == v1.x) {
    ys[0] = v0.y;
    return;
  }
  for (uint16_t x = v0.x; x <= v1.x; ++x) {
    ys[x - v0.x] = (x - v0.x) * (v1.y - v0.y) / (v1.x - v0.x) + v0.y;
  }
}

const char* _sf_log_lvl_to_str(sf_log_level_t level) {
    switch (level) {
        case SF_LOG_DEBUG: return "debug";
        case SF_LOG_INFO:  return "info";
        case SF_LOG_WARN:  return "warn";
        case SF_LOG_ERROR: return "error";
        default:           return "unknown";
    }
}

/* SF_UTILITIES */
sf_packed_color_t sf_pack_color(sf_unpacked_color_t c) {
  return ((uint32_t)c.a << 24) | 
         ((uint32_t)c.r << 16) | 
         ((uint32_t)c.g << 8 ) |
          (uint32_t)c.b;
}

size_t sf_get_obj_memory_usage(sf_obj_t *obj) {
  if (!obj) return 0;
  size_t v_size = obj->v_cnt * sizeof(sf_fvec3_t);
  size_t f_size = obj->f_cnt * sizeof(sf_ivec3_t);
  return v_size + f_size;
}

#endif /* SAFFRON_IMPLEMENTATION */
