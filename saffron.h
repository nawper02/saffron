/* saffron.h
 * Saffron is an stb-style graphics engine library.
 * Eventually it may become a game/simulation engine.
 */

/* SF_TODO
 *  Direct buffer access in primatives (not sf_pixel calls)
 *  Find bottlenecks and optimize
 *  Backface culling
 *  Normal based lighting
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
#include <stdbool.h>
#include <math.h>
#include <time.h>

/* SF_DEFINES */
#define SF_ARENA_SIZE          10485760
#define SF_MAX_OBJS            10
#define SF_MAX_ENTITIES        100
#define SF_LOG_INDENT          "            "
#define SF_PI                  3.14159265359f
#define SF_NANOS_PER_SEC       1000000000ULL

#define SF_LOG(ctx, level, fmt, ...)  _sf_log(ctx, level, __func__, fmt, ##__VA_ARGS__)
#define SF_ALIGN_SIZE(size)           (((size) + 7) & ~7)
#define SF_DEG2RAD(d)                 ((d) * (SF_PI / 180.0f))
#define SF_RAD2DEG(r)                 ((r) * (180.0f / SF_PI))
#define sf_get_obj(ctx, name)         _sf_get_obj(ctx, name, true)
#define sf_get_enti(ctx, name)        _sf_get_enti(ctx, name, true)

#define SF_CLR_RED              ((sf_pkd_clr_t)0xFFFF0000)
#define SF_CLR_GREEN            ((sf_pkd_clr_t)0xFF00FF00)
#define SF_CLR_BLUE             ((sf_pkd_clr_t)0xFF0000FF)
#define SF_CLR_BLACK            ((sf_pkd_clr_t)0xFF000000)
#define SF_CLR_WHITE            ((sf_pkd_clr_t)0xFFFFFFFF)

/* SF_TYPES */
typedef void (*sf_log_fn)(const char* message, void* userdata);
typedef enum {
  SF_LOG_DEBUG,
  SF_LOG_INFO,
  SF_LOG_WARN,
  SF_LOG_ERROR
} sf_log_level_t;

typedef uint32_t sf_pkd_clr_t;
typedef struct { uint8_t  r, g, b, a; } sf_unpkd_clr_t;
typedef struct { int      x, y;       } sf_ivec2_t;
typedef struct { int      x, y, z;    } sf_svec3_t;
typedef struct { float    x, y, z;    } sf_fvec3_t;
typedef struct { int      x, y, z;    } sf_ivec3_t;
typedef struct { float m[4][4];       } sf_fmat4_t;

typedef struct {
  sf_fvec3_t                    pos;
  sf_fvec3_t                    target;
  sf_fmat4_t                    V;
  sf_fmat4_t                    P;
} sf_camera_t;
typedef struct {
  sf_fvec3_t                   *v;
  sf_ivec3_t                   *f;
  int32_t                       v_cnt;
  int32_t                       f_cnt;
  int32_t                       id;
  const char                   *name;
} sf_obj_t;
typedef struct {
  sf_obj_t                      obj;
  sf_fmat4_t                    M;
  int32_t                       id;
  const char                   *name;
} sf_enti_t;

typedef struct {
  size_t                        size;
  size_t                        offset;
  uint8_t                      *buffer;
} sf_arena_t;
typedef struct {
  int                           w;
  int                           h;
  int                           buffer_size;
  sf_pkd_clr_t                 *buffer;
  float                        *z_buffer;

  sf_arena_t                    arena;
  int                           arena_size;

  sf_obj_t                     *objs;
  int32_t                       obj_count;
  sf_enti_t                    *entities;
  int32_t                       enti_count;

  sf_camera_t                   camera;

  float                         delta_time;
  float                         elapsed_time;
  float                         fps;
  uint32_t                      frame_count;
  uint64_t                      _start_ticks;
  uint64_t                      _last_ticks;

  sf_log_fn                     log_cb;
  void*                         log_user;
  sf_log_level_t                log_min;
} sf_ctx_t;

/* SF_CORE_FUNCTIONS */
void        sf_init             (sf_ctx_t *ctx, int w, int h);
void        sf_destroy          (sf_ctx_t *ctx);
sf_arena_t  sf_arena_init       (size_t size);
void*       sf_arena_alloc      (sf_ctx_t *ctx, sf_arena_t *arena, size_t size);
void        sf_set_logger       (sf_ctx_t *ctx, sf_log_fn log_cb, void* userdata);
void        sf_logger_console   (const char* message, void* userdata);
void        sf_log              (sf_ctx_t *ctx, sf_log_level_t level, const char* fmt, ...);
void        _sf_log             (sf_ctx_t *ctx, sf_log_level_t level, const char* func, const char* fmt, ...);
sf_obj_t*   sf_load_obj         (sf_ctx_t *ctx, const char *filename, const char *objname);
sf_obj_t*   _sf_get_obj         (sf_ctx_t *ctx, const char *objname, bool should_log_failure);
sf_enti_t*  sf_add_enti         (sf_ctx_t *ctx, sf_obj_t *obj, const char *entiname);
sf_enti_t*  _sf_get_enti        (sf_ctx_t *ctx, const char *entiname, bool should_log_failure);
void        sf_render_enti      (sf_ctx_t *ctx, sf_enti_t *enti);
void        sf_render_ctx       (sf_ctx_t *ctx);
void        sf_time_update      (sf_ctx_t *ctx);
uint64_t    _sf_get_ticks       (void);

/* SF_DRAWING_FUNCTIONS */
void        sf_fill             (sf_ctx_t *ctx, sf_pkd_clr_t c);
void        sf_pixel            (sf_ctx_t *ctx, sf_pkd_clr_t c, sf_ivec2_t v0);
void        sf_pixel_depth      (sf_ctx_t *ctx, sf_pkd_clr_t c, sf_ivec2_t v0, float z);
void        sf_line             (sf_ctx_t *ctx, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1);
void        sf_rect             (sf_ctx_t *ctx, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1);
void        sf_tri              (sf_ctx_t *ctx, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1, sf_ivec2_t v2);
void        sf_tri_depth        (sf_ctx_t *ctx, sf_pkd_clr_t c, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2);
void        sf_put_text         (sf_ctx_t *ctx, const char *text, sf_ivec2_t p, sf_pkd_clr_t c, int scale);
void        sf_clear_depth      (sf_ctx_t *ctx);

/* SF_LA_FUNCTIONS */
sf_fmat4_t  sf_fmat4_mul_fmat4  (sf_fmat4_t m0, sf_fmat4_t m1);
sf_fvec3_t  sf_fmat4_mul_vec3   (sf_fmat4_t m, sf_fvec3_t v);
sf_fvec3_t  sf_fvec3_sub        (sf_fvec3_t v0, sf_fvec3_t v1);
sf_fvec3_t  sf_fvec3_add        (sf_fvec3_t v0, sf_fvec3_t v1);
sf_fvec3_t  sf_fvec3_norm       (sf_fvec3_t v);
sf_fvec3_t  sf_fvec3_cross      (sf_fvec3_t v0, sf_fvec3_t v1);
float       sf_fvec3_dot        (sf_fvec3_t v0, sf_fvec3_t v1);
sf_fmat4_t  sf_make_tsl_fmat4   (float x, float y, float z);
sf_fmat4_t  sf_make_rot_fmat4   (sf_fvec3_t angles);
sf_fmat4_t  sf_make_psp_fmat4   (float fov_deg, float aspect, float near, float far);
sf_fmat4_t  sf_make_idn_fmat4   (void);
sf_fmat4_t  sf_make_view_fmat4  (sf_fvec3_t eye, sf_fvec3_t target, sf_fvec3_t up);

/* SF_IMPLEMENTATION_HELPERS */
uint32_t    _sf_vec_to_index    (sf_ctx_t *ctx, sf_ivec2_t v);
void        _sf_swap_svec2      (sf_ivec2_t *v0, sf_ivec2_t *v1);
void        _sf_interpolate_x   (sf_ivec2_t  v0, sf_ivec2_t v1, int *xs);
void        _sf_interpolate_y   (sf_ivec2_t  v0, sf_ivec2_t v1, int *ys);
void        _sf_interpolate_f   (float v0, float v1, int steps, float *out);
const char* _sf_log_lvl_to_str  (sf_log_level_t level);

/* SF_UTILITIES */
sf_pkd_clr_t sf_pack_color (sf_unpkd_clr_t);
size_t            sf_obj_memusg (sf_obj_t *obj);

/* SF_FONT_DATA */
static const uint8_t            _sf_font_8x8[];

#ifdef __cplusplus
}
#endif
#endif /* SAFFRON_H */

/* SF_IMPLEMENTATION */
#ifdef SAFFRON_IMPLEMENTATION

/* SF_CORE_FUNCTIONS */
void sf_init(sf_ctx_t *ctx, int w, int h) {
  ctx->w                        = w;
  ctx->h                        = h;
  ctx->buffer_size              = w * h;
  ctx->buffer                   = (sf_pkd_clr_t*) malloc(w*h*sizeof(sf_pkd_clr_t));
  ctx->z_buffer                 = (float*)             malloc(w*h*sizeof(float));
  ctx->arena                    = sf_arena_init(SF_ARENA_SIZE);
  ctx->log_cb                   = sf_logger_console;
  ctx->log_user                 = NULL;
  ctx->log_min                  = SF_LOG_INFO;
  ctx->objs                     = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_OBJS     * sizeof(sf_obj_t));
  ctx->entities                 = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_ENTITIES * sizeof(sf_enti_t));
  ctx->obj_count                = 0;
  ctx->enti_count               = 0;
  ctx->_start_ticks             = _sf_get_ticks();
  ctx->_last_ticks              = ctx->_start_ticks;
  ctx->delta_time               = 0.0f;
  ctx->elapsed_time             = 0.0f;
  ctx->fps                      = 0.0f;
  ctx->frame_count              = 0;
  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "buffer : %dx%d\n"
              SF_LOG_INDENT "memory : %d\n"
              SF_LOG_INDENT "mxobjs : %d\n" 
              SF_LOG_INDENT "mxents : %d\n", 
              ctx->w, ctx->h, SF_ARENA_SIZE, SF_MAX_OBJS, SF_MAX_ENTITIES);
}

void sf_destroy(sf_ctx_t *ctx) {
  free(ctx->buffer);
  free(ctx->z_buffer);
  free(ctx->arena.buffer);
  ctx->arena.offset             = 0;
  ctx->buffer                   = NULL;
  ctx->buffer_size              = 0;
  ctx->w                        = 0;
  ctx->h                        = 0;
  ctx->enti_count               = 0;
  ctx->obj_count                = 0;
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
  size_t aligned_offset = SF_ALIGN_SIZE(arena->offset);
  if (aligned_offset + size > arena->size) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to allocate, arena out of memory\n");
    return NULL;
  }
  void *ptr = &arena->buffer[aligned_offset];
  arena->offset = aligned_offset + size;
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

sf_obj_t* sf_load_obj(sf_ctx_t *ctx, const char *filename, const char *objname) {
  char auto_name[32];
  if (objname == NULL) {
    snprintf(auto_name, sizeof(auto_name), "obj_%d", ctx->obj_count);
    objname = auto_name;
  }

  if (NULL != _sf_get_obj(ctx, objname, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add obj, name in use\n");
    return NULL;
  }

  if (ctx->obj_count >= SF_MAX_OBJS) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "max objects (%d) reached\n", SF_MAX_OBJS);
    return NULL;
  }

  FILE *f = fopen(filename, "r");
  if (!f) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "could not open %s\n", filename);
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
  size_t name_len = strlen(objname) + 1;
  obj->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (obj->name) {
    memcpy((void*)obj->name, objname, name_len);
  }
  obj->id = ctx->obj_count - 1;

  if (!obj->v || !obj->f || !obj->name) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "out of arena memory for %s\n", filename);
    fclose(f);
    return NULL;
  }

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "file   : %s\n"
              SF_LOG_INDENT "name   : %s\n"
              SF_LOG_INDENT "id     : %d\n"
              SF_LOG_INDENT "verts  : %d\n"
              SF_LOG_INDENT "faces  : %d\n"
              SF_LOG_INDENT "size   : %d\n"
              SF_LOG_INDENT "mem    : %.2f\n"
              SF_LOG_INDENT "obj_id : %d\n",
              filename, objname, obj->id, v_cnt, f_cnt, 
              sf_obj_memusg(obj), 
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

sf_obj_t* _sf_get_obj(sf_ctx_t *ctx, const char *objname, bool should_log_failure) {
  for (int32_t i = 0; i < ctx->obj_count; ++i) {
    if (ctx->objs[i].name && strcmp(ctx->objs[i].name, objname) == 0) {
      return &ctx->objs[i];
    }
  }
 
  if (should_log_failure) {
    SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "object '%s' not found\n", objname);
  }
  return NULL;
}

sf_enti_t* sf_add_enti(sf_ctx_t *ctx, sf_obj_t *obj, const char *entiname) {
  char auto_name[32];
  if (NULL == entiname) {
    snprintf(auto_name, sizeof(auto_name), "enti_%d", ctx->enti_count);
    entiname = auto_name;
  }

  if (obj == NULL) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "cannot add entity: obj is NULL\n");
    return NULL;
  }
  if (NULL != _sf_get_enti(ctx, entiname, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add entity, entity name in use\n");
    return NULL;
  }
  if (ctx->enti_count >= SF_MAX_ENTITIES) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add entity, max entities reached\n");
    return NULL;
  }

  sf_enti_t *enti = &ctx->entities[ctx->enti_count++];
  enti->obj   = *obj;
  enti->M     = sf_make_idn_fmat4();
  enti->id    = ctx->enti_count - 1;

  size_t name_len = strlen(entiname) + 1;
  enti->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (enti->name) {
    memcpy((void*)enti->name, entiname, name_len);
  }

  SF_LOG(ctx, SF_LOG_INFO,
            SF_LOG_INDENT "enti   : %s (id %d)\n"
            SF_LOG_INDENT "obj    : %s (id %d)\n"
            SF_LOG_INDENT "used   : %d/%d\n",
            enti->name, enti->id, enti->obj.name, enti->obj.id, ctx->enti_count, SF_MAX_ENTITIES);

  return enti;
}

sf_enti_t* _sf_get_enti(sf_ctx_t *ctx, const char *entiname, bool should_log_failure) {
  for (int32_t i = 0; i < ctx->enti_count; ++i) {
    if (ctx->entities[i].name && strcmp(ctx->entities[i].name, entiname) == 0) {
      return &ctx->entities[i];
    }
  }

  if (should_log_failure) {
    SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "entity '%s' not found\n", entiname);
  }

  return NULL;
}

void sf_render_enti(sf_ctx_t *ctx, sf_enti_t *enti) {
  sf_fmat4_t MVP = sf_fmat4_mul_fmat4(enti->M, sf_fmat4_mul_fmat4(ctx->camera.V, ctx->camera.P));

  for (int i = 0; i < enti->obj.f_cnt; i++) {
    sf_fvec3_t p[3];
    sf_ivec3_t f = enti->obj.f[i];
    for (int j = 0; j < 3; j++) {
      int idx = (j == 0) ? f.x : (j == 1) ? f.y : f.z;
      sf_fvec3_t proj = sf_fmat4_mul_vec3(MVP, enti->obj.v[idx]);
      p[j].x = (proj.x + 1.0f) * 0.5f * (float)ctx->w;
      p[j].y = (1.0f - (proj.y + 1.0f) * 0.5f) * (float)ctx->h;
      p[j].z = proj.z;
    }
    sf_tri_depth(ctx, SF_CLR_RED, p[0], p[1], p[2]);
  }
}

//void sf_render_enti(sf_ctx_t *ctx, sf_enti_t *enti) {
//  sf_fmat4_t MVP = sf_fmat4_mul_fmat4(enti->M, sf_fmat4_mul_fmat4(ctx->camera.V, ctx->camera.P));
//
//  sf_fvec3_t light_dir = {0.0f, 0.0f, -1.0f};
//
//  for (int i = 0; i < enti->obj.f_cnt; i++) {
//    sf_ivec3_t face = enti->obj.f[i];
//    sf_fvec3_t world_v[3];
//    sf_ivec2_t screen_coords[3];
//    float depths[3];
//
//    world_v[0] = enti->obj.v[face.x];
//    world_v[1] = enti->obj.v[face.y];
//    world_v[2] = enti->obj.v[face.z];
//
//    sf_fvec3_t a = sf_fvec3_sub(world_v[1], world_v[0]);
//    sf_fvec3_t b = sf_fvec3_sub(world_v[2], world_v[0]);
//    sf_fvec3_t normal = sf_fvec3_norm(sf_fvec3_cross(a, b));
//    
//    sf_fvec3_t view_ray = sf_fvec3_sub(world_v[0], ctx->camera.pos);
//    if (sf_fvec3_dot(normal, view_ray) >= 0) continue;
//
//    for (int j = 0; j < 3; j++) {
//      sf_fvec3_t proj_v = sf_fmat4_mul_vec3(MVP, world_v[j]);
//      screen_coords[j].x = (int)((proj_v.x + 1.0f) * 0.5f * ctx->w);
//      screen_coords[j].y = (int)((1.0f - (proj_v.y + 1.0f) * 0.5f) * ctx->h);
//      depths[j] = proj_v.z;
//    }
//
//    float intensity = sf_fvec3_dot(normal, sf_fvec3_norm(light_dir));
//    if (intensity < 0.1f) intensity = 0.1f;
//    
//    sf_unpkd_clr_t color = { (uint8_t)(255 * intensity), (uint8_t)(200 * intensity), 0, 255 };
//    sf_tri(ctx, sf_pack_color(color), screen_coords[0], screen_coords[1], screen_coords[2]);
//  }
//}

void sf_render_ctx(sf_ctx_t *ctx) {
  sf_fill(ctx, SF_CLR_BLACK);
  sf_clear_depth(ctx); 
  for (int i = 0; i < ctx->enti_count; i++) {
    sf_render_enti(ctx, &ctx->entities[i]);
  }
}

uint64_t _sf_get_ticks(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * SF_NANOS_PER_SEC + (uint64_t)ts.tv_nsec;
}

void sf_time_update(sf_ctx_t *ctx) {
  uint64_t current_ticks = _sf_get_ticks();
  uint64_t diff = current_ticks - ctx->_last_ticks;
  ctx->delta_time = (float)((double)diff / (double)SF_NANOS_PER_SEC);
  ctx->elapsed_time = (float)((double)(current_ticks - ctx->_start_ticks) / (double)SF_NANOS_PER_SEC);
  ctx->_last_ticks = current_ticks;
  if (ctx->delta_time > 0.0f) {
    float instant_fps = 1.0f / ctx->delta_time;
    ctx->fps = (ctx->fps * 0.9f) + (instant_fps * 0.1f);
  }
  ctx->frame_count++;
}

/* SF_DRAWING_FUNCTIONS */
void sf_fill(sf_ctx_t *ctx, sf_pkd_clr_t c) {
 for(size_t i = 0; i < ctx->buffer_size; ++i) { ctx->buffer[i] = c; }
}

void sf_pixel(sf_ctx_t *ctx, sf_pkd_clr_t c, sf_ivec2_t v0) {
  if (v0.x >= ctx->w || v0.y >= ctx->h) return;
  ctx->buffer[_sf_vec_to_index(ctx, v0)] = c;
}

void sf_pixel_depth(sf_ctx_t *ctx, sf_pkd_clr_t c, sf_ivec2_t v, float z) {
  if (v.x >= ctx->w || v.y >= ctx->h) return;
  uint32_t idx = _sf_vec_to_index(ctx, v);
  if (z < ctx->z_buffer[idx]) {
    ctx->z_buffer[idx] = z;
    ctx->buffer[idx] = c;
  }
}

void sf_line(sf_ctx_t *ctx, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1) {
  if (v1.y - v0.y > v1.x - v0.x) {
    int x01[v1.y-v0.y+1];
    _sf_interpolate_x(v0, v1, x01);
    for (int y = v0.y; y <= v1.y; ++y) {
      sf_pixel(ctx, c, (sf_ivec2_t){x01[y-v0.y],y});
    }
  }
  else {
    int y01[v1.x-v0.x+1];
    _sf_interpolate_y(v0, v1, y01);
    for (int x = v0.x; x <= v1.x; ++x) {
      sf_pixel(ctx, c, (sf_ivec2_t){x,y01[x-v0.x]});
    }
  }
}

void sf_rect(sf_ctx_t *ctx, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1) {
  int l = (v0.x < v1.x) ? v0.x : v1.x;
  int r = (v0.x > v1.x) ? v0.x : v1.x;
  int t = (v0.y < v1.y) ? v0.y : v1.y;
  int b = (v0.y > v1.y) ? v0.y : v1.y;
  for (int y = t; y <= b; ++y) {
    for (int x = l; x <= r; ++x) {
      sf_pixel(ctx, c, (sf_ivec2_t){x,y});
    }
  }
}

void sf_tri(sf_ctx_t *ctx, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1, sf_ivec2_t v2) {
  if (v1.y < v0.y) _sf_swap_svec2(&v0, &v1);
  if (v2.y < v0.y) _sf_swap_svec2(&v2, &v0);
  if (v2.y < v1.y) _sf_swap_svec2(&v2, &v1);
  int h = v2.y - v0.y + 1;
  int x02[h], x012[h];
  _sf_interpolate_x(v0, v1, x012);
  _sf_interpolate_x(v1, v2, &x012[v1.y - v0.y]);
  _sf_interpolate_x(v0, v2, x02);
  int mid_idx = v1.y - v0.y;
  int *xl = (x02[mid_idx] < x012[mid_idx]) ? x02  : x012;
  int *xr = (x02[mid_idx] < x012[mid_idx]) ? x012 : x02;
  for (int y = v0.y; y <= v2.y; ++y) {
    for (int x = xl[y - v0.y]; x <= xr[y - v0.y]; ++x) {
      sf_pixel(ctx, c, (sf_ivec2_t){x, y});
    }
  }
}

void sf_tri_depth(sf_ctx_t *ctx, sf_pkd_clr_t c, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2) {
  if (v1.y < v0.y) { sf_fvec3_t t = v0; v0 = v1; v1 = t; }
  if (v2.y < v0.y) { sf_fvec3_t t = v0; v0 = v2; v2 = t; }
  if (v2.y < v1.y) { sf_fvec3_t t = v1; v1 = v2; v2 = t; }

  int h = (int)v2.y - (int)v0.y + 1;
  if (h <= 1 || h > 1024) return;

  int x02[1024], x012[1024];
  float z02[1024], z012[1024];

  _sf_interpolate_x((sf_ivec2_t){(int)v0.x, (int)v0.y}, (sf_ivec2_t){(int)v2.x, (int)v2.y}, x02);
  _sf_interpolate_f(v0.z, v2.z, h - 1, z02);

  int h01 = (int)v1.y - (int)v0.y;
  int h12 = (int)v2.y - (int)v1.y;

  _sf_interpolate_x((sf_ivec2_t){(int)v0.x, (int)v0.y}, (sf_ivec2_t){(int)v1.x, (int)v1.y}, x012);
  _sf_interpolate_f(v0.z, v1.z, h01, z012);
  _sf_interpolate_x((sf_ivec2_t){(int)v1.x, (int)v1.y}, (sf_ivec2_t){(int)v2.x, (int)v2.y}, &x012[h01]);
  _sf_interpolate_f(v1.z, v2.z, h12, &z012[h01]);

  int *xl = x02, *xr = x012;
  float *zl = z02, *zr = z012;

  if (h01 < h && x012[h01] < x02[h01]) {
    xl = x012; xr = x02;
    zl = z012; zr = z02;
  }

  for (int y = (int)v0.y; y <= (int)v2.y; ++y) {
    int idx = y - (int)v0.y;
    if (y < 0 || y >= ctx->h) continue;

    int x_start = xl[idx];
    int x_end = xr[idx];
    float z_start = zl[idx];
    float z_end = zr[idx];

    float width = (float)(x_end - x_start);
    for (int x = x_start; x <= x_end; ++x) {
      if (x < 0 || x >= ctx->w) continue;
      float t = (width == 0) ? 0.0f : (float)(x - x_start) / width;
      float z = z_start + (z_end - z_start) * t;
      sf_pixel_depth(ctx, c, (sf_ivec2_t){(int)x, (int)y}, z);
    }
  }
}

void sf_put_text(sf_ctx_t *ctx, const char *text, sf_ivec2_t p, sf_pkd_clr_t c, int scale) {
  if (scale < 1) scale = 1;
  int start_x = p.x;
  int stride = 8 * scale;
  for (int i = 0; text[i] != '\0'; ++i) {
    char ch = text[i];
    if (ch == '\n') {
      p.x = start_x;
      p.y += stride;
      continue;
    }
    if (ch < 32 || ch > 127) {
      if (ch == '\t') p.x += stride * 4;
      continue;
    }
    const uint8_t *glyph = &_sf_font_8x8[(ch - 32) * 8];
    for (int y = 0; y < 8; ++y) {
      for (int x = 0; x < 8; ++x) {
        if ((glyph[y] >> (7 - x)) & 1) {
          if (scale == 1) {
            sf_pixel(ctx, c, (sf_ivec2_t){ (int)(p.x + x), (int)(p.y + y) });
          } else {
            sf_rect(ctx, c,
              (sf_ivec2_t){ (int)(p.x + x * scale), (int)(p.y + y * scale) },
              (sf_ivec2_t){ (int)(p.x + x * scale + scale - 1), (int)(p.y + y * scale + scale - 1) }
            );
          }
        }
      }
    }
    p.x += stride;
  }
}

void sf_clear_depth(sf_ctx_t *ctx) {
  for (int i = 0; i < ctx->buffer_size; ++i) {
    ctx->z_buffer[i] = 1000000.0f;
  }
}

/* SF_LA_FUNCTIONS */
sf_fmat4_t sf_fmat4_mul_fmat4(sf_fmat4_t m0, sf_fmat4_t m1) {
  sf_fmat4_t result = {0};
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        result.m[i][j] += m0.m[i][k] * m1.m[k][j];
      }
    }
  }
  return result;
}

sf_fvec3_t sf_fmat4_mul_vec3(sf_fmat4_t m, sf_fvec3_t v) {
    sf_fvec3_t result;
    float w;
    result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    w        = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
    if (w != 0.0f) {
        result.x /= w; result.y /= w; result.z /= w;
    }
    return result;
}

sf_fvec3_t sf_fvec3_sub(sf_fvec3_t v0, sf_fvec3_t v1) {
    return (sf_fvec3_t){ v0.x - v1.x, v0.y - v1.y, v0.z - v1.z };
}

sf_fvec3_t sf_fvec3_add(sf_fvec3_t v0, sf_fvec3_t v1) {
    return (sf_fvec3_t){ v0.x + v1.x, v0.y + v1.y, v0.z + v1.z };
}

sf_fvec3_t sf_fvec3_norm(sf_fvec3_t v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len == 0.0f) return (sf_fvec3_t){0,0,0};
    return (sf_fvec3_t){ v.x/len, v.y/len, v.z/len };
}

sf_fvec3_t sf_fvec3_cross(sf_fvec3_t v0, sf_fvec3_t v1) {
    return (sf_fvec3_t){
        v0.y * v1.z - v0.z * v1.y,
        v0.z * v1.x - v0.x * v1.z,
        v0.x * v1.y - v0.y * v1.x
    };
}

float sf_fvec3_dot(sf_fvec3_t v0, sf_fvec3_t v1) {
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

sf_fmat4_t sf_make_tsl_fmat4(float x, float y, float z) {
    sf_fmat4_t m = sf_make_idn_fmat4();
    m.m[3][0] = x;
    m.m[3][1] = y;
    m.m[3][2] = z;
    return m;
}

sf_fmat4_t sf_make_rot_fmat4(sf_fvec3_t angles) {
    float cx = cosf(angles.x);
    float sx = sinf(angles.x);
    float cy = cosf(angles.y);
    float sy = sinf(angles.y);
    float cz = cosf(angles.z);
    float sz = sinf(angles.z);

    sf_fmat4_t m = sf_make_idn_fmat4();

    m.m[0][0] = cy * cz;
    m.m[0][1] = cy * sz;
    m.m[0][2] = -sy;

    m.m[1][0] = sx * sy * cz - cx * sz;
    m.m[1][1] = sx * sy * sz + cx * cz;
    m.m[1][2] = sx * cy;

    m.m[2][0] = cx * sy * cz + sx * sz;
    m.m[2][1] = cx * sy * sz - sx * cz;
    m.m[2][2] = cx * cy;

    return m;
}

sf_fmat4_t sf_make_psp_fmat4(float fov_deg, float aspect, float near, float far) {
    sf_fmat4_t m = {0};
    float fov_rad = fov_deg * (3.14159f / 180.0f);
    float tan_half_fov = tanf(fov_rad / 2.0f);
    float z_range = near - far;

    m.m[0][0] = 1.0f / (tan_half_fov * aspect);
    m.m[1][1] = 1.0f / tan_half_fov;
    m.m[2][2] = (-near - far) / z_range;
    m.m[2][3] = 1.0f; 
    m.m[3][2] = (2.0f * far * near) / z_range;

    return m;
}

sf_fmat4_t sf_make_idn_fmat4() {
  sf_fmat4_t result;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (i == j) result.m[i][j] = 1;
      else        result.m[i][j] = 0;
    }
  }
  return result;
}

sf_fmat4_t sf_make_view_fmat4(sf_fvec3_t eye, sf_fvec3_t target, sf_fvec3_t up) {
  sf_fvec3_t f = sf_fvec3_norm(sf_fvec3_sub(target, eye));
  sf_fvec3_t r = sf_fvec3_norm(sf_fvec3_cross(f, up));
  sf_fvec3_t u = sf_fvec3_cross(r, f);

  sf_fmat4_t m = sf_make_idn_fmat4();

  m.m[0][0] = r.x;
  m.m[1][0] = r.y;
  m.m[2][0] = r.z;

  m.m[0][1] = u.x;
  m.m[1][1] = u.y;
  m.m[2][1] = u.z;

  m.m[0][2] = f.x;
  m.m[1][2] = f.y;
  m.m[2][2] = f.z;

  m.m[3][0] = -sf_fvec3_dot(r, eye);
  m.m[3][1] = -sf_fvec3_dot(u, eye);
  m.m[3][2] = -sf_fvec3_dot(f, eye);

  return m;
}

/* SF_IMPLEMENTATION_HELPERS */
uint32_t _sf_vec_to_index(sf_ctx_t *ctx, sf_ivec2_t v) {
  return v.y * ctx->w + v.x;
}

void _sf_swap_svec2(sf_ivec2_t *v0, sf_ivec2_t *v1) {
  sf_ivec2_t t = *v0; *v0 = *v1; *v1 = t;
}

void _sf_interpolate_x(sf_ivec2_t v0, sf_ivec2_t v1, int *xs) {
  if (v0.y == v1.y) {
    xs[0] = v0.x;
    return;
  }
  for (int y = v0.y; y <= v1.y; ++y) {
    xs[y - v0.y] = (y - v0.y) * (v1.x - v0.x) / (v1.y - v0.y) + v0.x;
  }
}

void _sf_interpolate_y(sf_ivec2_t v0, sf_ivec2_t v1, int *ys) {
  if (v0.x == v1.x) {
    ys[0] = v0.y;
    return;
  }
  for (int x = v0.x; x <= v1.x; ++x) {
    ys[x - v0.x] = (x - v0.x) * (v1.y - v0.y) / (v1.x - v0.x) + v0.y;
  }
}

void _sf_interpolate_f(float v0, float v1, int steps, float *out) {
  if (steps == 0) return;
  float step = (v1 - v0) / steps;
  for (int i = 0; i <= steps; ++i) {
    out[i] = v0 + (step * i);
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
sf_pkd_clr_t sf_pack_color(sf_unpkd_clr_t c) {
  return ((uint32_t)c.a << 24) | 
         ((uint32_t)c.r << 16) | 
         ((uint32_t)c.g << 8 ) |
          (uint32_t)c.b;
}

size_t sf_obj_memusg(sf_obj_t *obj) {
  if (!obj) return 0;
  size_t v_size = obj->v_cnt * sizeof(sf_fvec3_t);
  size_t f_size = obj->f_cnt * sizeof(sf_ivec3_t);
  return v_size + f_size;
}

/* SF_FONT_DATA */
static const uint8_t _sf_font_8x8[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // [space]
    0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00, // !
    0x6C,0x6C,0x00,0x00,0x00,0x00,0x00,0x00, // "
    0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00, // #
    0x18,0x7E,0xC0,0x7C,0x06,0xFC,0x18,0x00, // $
    0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00, // %
    0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00, // &
    0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00, // '
    0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00, // (
    0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00, // )
    0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00, // *
    0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00, // +
    0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30, // ,
    0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00, // -
    0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00, // .
    0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00, // /
    0x3C,0x66,0xC3,0xC3,0xC3,0x66,0x3C,0x00, // 0
    0x18,0x38,0x18,0x18,0x18,0x18,0x3C,0x00, // 1
    0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00, // 2
    0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00, // 3
    0x06,0x0E,0x1E,0x36,0x66,0x7F,0x06,0x00, // 4
    0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00, // 5
    0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00, // 6
    0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00, // 7
    0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00, // 8
    0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00, // 9
    0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00, // :
    0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30, // ;
    0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00, // <
    0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00, // =
    0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00, // >
    0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00, // ?
    0x3C,0x66,0x6E,0x6E,0x60,0x62,0x3C,0x00, // @
    0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00, // A
    0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00, // B
    0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00, // C
    0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00, // D
    0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00, // E
    0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00, // F
    0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00, // G
    0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00, // H
    0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00, // I
    0x1E,0x0C,0x0C,0x0C,0x0C,0xCC,0x78,0x00, // J
    0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00, // K
    0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00, // L
    0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00, // M
    0x66,0x66,0x76,0x7E,0x6E,0x66,0x66,0x00, // N
    0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00, // O
    0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00, // P
    0x3C,0x66,0x66,0x66,0x66,0x3C,0x0E,0x00, // Q
    0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00, // R
    0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00, // S
    0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00, // T
    0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00, // U
    0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00, // V
    0x63,0x63,0x6B,0x7F,0x7F,0x77,0x63,0x00, // W
    0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00, // X
    0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00, // Y
    0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00, // Z
    0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00, // [
    0x00,0x40,0x20,0x10,0x08,0x04,0x02,0x00, // backslash
    0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00, // ]
    0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00, // ^
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF, // _
    0x18,0x0C,0x06,0x00,0x00,0x00,0x00,0x00, // `
    0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00, // a
    0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00, // b
    0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00, // c
    0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00, // d
    0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00, // e
    0x1C,0x30,0x78,0x30,0x30,0x30,0x30,0x00, // f
    0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C, // g
    0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00, // h
    0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00, // i
    0x06,0x00,0x06,0x06,0x06,0x06,0x0C,0x38, // j
    0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00, // k
    0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00, // l
    0x00,0x00,0x6C,0xFE,0xFE,0xD6,0xC6,0x00, // m
    0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00, // n
    0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00, // o
    0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60, // p
    0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06, // q
    0x00,0x00,0x6E,0x70,0x60,0x60,0x60,0x00, // r
    0x00,0x00,0x3C,0x60,0x3C,0x06,0x3C,0x00, // s
    0x30,0x30,0x78,0x30,0x30,0x30,0x1C,0x00, // t
    0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00, // u
    0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00, // v
    0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x6C,0x00, // w
    0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00, // x
    0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C, // y
    0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00, // z
    0x0C,0x18,0x18,0x70,0x18,0x18,0x0C,0x00, // {
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00, // |
    0x30,0x18,0x18,0x0E,0x18,0x18,0x30,0x00, // }
    0x31,0x64,0x00,0x00,0x00,0x00,0x00,0x00, // ~
    0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x00  // DEL
};

#endif /* SAFFRON_IMPLEMENTATION */
