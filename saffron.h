/* saffron.h
 * Saffron is an stb-style graphics engine library.
 * Eventually it may become a game/simulation engine.

                              ████████████                          ██████                               
                           ███████████    █████                           █████                           
                         █████████████        ███                             ████                        
                       ███████████████          ███                             ███                       
                     █████████████████            ██                              ███                     
                    ██████████████████             ██             ██████           ███                    
                    ██████████████████              ██         ███████ ████         ██                    
                   ███████████████████              ███       ████████    ██         ██                   
                   ███████████████████               ██      █████████     ██        ██                   
                   ███████████████████               ██      █████████     ███       ██                   
                   ███████████████████               ██  ██  █████████     ███       ███                  
                   ███████████████████               ██ ███  █████████     ██        ███                  
                   ███████████████████              ███ ██   █████████   ███         ███                  
                   ███████████████████              ██  ██   ██ ██████████           ███                  
                   ███████████████████             ██  ██    ██     ██               ███                  
                   ██ ████████████████           ███  ██     ██     ██               ███                  
                   ██  ███████████████          ███  ██    ████     ██               ███                  
                   ██    █████████████       ████  ███  ████    ██████████           ███                  
                   ██       ██████████  ██████   ███  ███   ██████  ██████████       ███                  
                   ██           ██████████    ████  ███  ████       █████████████    ███                  
                   ██               ██     ████    ██  ███          ███████████████  ███                  
                   ██               ██     ██     ██  ███           ████████████████ ███                  
                   ██               ██     ██    ██  ██             ████████████████████                  
                   ██           ██████████ ██   ██  ██              ████████████████████                  
                   ██         ███   █████████   ██ ███              ████████████████████                  
                   ██        ██     █████████  ███ ██               ████████████████████                  
                   ██       ███     █████████  ██  ██               ████████████████████                  
                   ██       ███     █████████      ██               ███████████████████                   
                   ██        ██     █████████      ██               ███████████████████                   
                   ██         ██    ████████       ███              ███████████████████                   
                    ██         ████████████         ██              ██████████████████                    
                    ███           ██████             ██             ██████████████████                    
                     ███                              ██            █████████████████                     
                       ███                             ███          ███████████████                       
                         ███                             ███        █████████████                         
                           █████                           █████    ███████████                           
                               ██████                          ████████████                               
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
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

/* SF_DEFINES */
#define SF_ARENA_SIZE          10485760
#define SF_MAX_OBJS            10
#define SF_MAX_ENTITIES        100
#define SF_MAX_LIGHTS          10
#define SF_MAX_TEXTURES        20
#define SF_MAX_CB_PER_EVT      4
#define SF_LOG_INDENT          "            "
#define SF_PI                  3.14159265359f
#define SF_NANOS_PER_SEC       1000000000ULL
#define SF_ASSET_PATH          "/usr/local/share/saffron/sf_assets"

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
typedef struct sf_ctx_t_ sf_ctx_t;

typedef enum {
  SF_RUN_STATE_RUNNING,
  SF_RUN_STATE_STOPPED
} sf_run_state_t;

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
typedef struct { float    x, y;       } sf_fvec2_t;
typedef struct { int      x, y, z;    } sf_svec3_t;
typedef struct { float    x, y, z;    } sf_fvec3_t;
typedef struct { int      x, y, z;    } sf_ivec3_t;
typedef struct { float m[4][4];       } sf_fmat4_t;

typedef struct {
  sf_fvec3_t                    pos;
  sf_fvec3_t                    front;
  sf_fvec3_t                    up;
  sf_fvec3_t                    right;
  sf_fvec3_t                    world_up;
  float                         yaw;
  float                         pitch;
  float                         fov;
  float                         near_plane;
  float                         far_plane;
  bool                          is_view_dirty;
  bool                          is_proj_dirty;
  sf_fmat4_t                    V;
  sf_fmat4_t                    P;
} sf_camera_t;

typedef struct {
  sf_fvec3_t   *px;
  int           w;
  int           h;
  int32_t       id;
  const char   *name;
} sf_tex_t;

typedef struct { 
  int v;
  int vt;
  int vn;
} sf_vtx_idx_t;

typedef struct {
  sf_vtx_idx_t                  idx[3];
} sf_face_t;

typedef struct {
  sf_fvec3_t                   *v;
  sf_fvec2_t                   *vt;
  sf_fvec3_t                   *vn;
  sf_face_t                    *f;
  int32_t                       v_cnt;
  int32_t                       vt_cnt;
  int32_t                       vn_cnt;
  int32_t                       f_cnt;
  int32_t                       id;
  const char                   *name;
} sf_obj_t;

typedef struct {
  sf_obj_t                      obj;
  sf_fmat4_t                    M;
  sf_fvec3_t                    pos;
  sf_fvec3_t                    rot;
  sf_fvec3_t                    scale;
  bool                          is_dirty;
  int32_t                       id;
  sf_tex_t                 *tex;
  const char                   *name;
} sf_enti_t;

typedef enum {
  SF_LIGHT_DIR,
  SF_LIGHT_POINT
} sf_light_type_t;

typedef struct {
  sf_light_type_t               type;
  sf_fvec3_t                    pos_dir;
  sf_fvec3_t                    color;
  float                         intensity;
} sf_light_t;

typedef struct {
  const char*                   name;
  int32_t                       obj_start_idx;
  int32_t                       obj_count;
  int32_t                       enti_start_idx;
  int32_t                       enti_count;
  int32_t                       light_start_idx;
  int32_t                       light_count;
} sf_world_t;

typedef struct {
  size_t                        size;
  size_t                        offset;
  uint8_t                      *buffer;
} sf_arena_t;

typedef enum {
  SF_KEY_UNKNOWN = 0,
  SF_KEY_A, SF_KEY_B, SF_KEY_C, SF_KEY_D, SF_KEY_E, SF_KEY_F, SF_KEY_G, SF_KEY_H,
  SF_KEY_I, SF_KEY_J, SF_KEY_K, SF_KEY_L, SF_KEY_M, SF_KEY_N, SF_KEY_O, SF_KEY_P,
  SF_KEY_Q, SF_KEY_R, SF_KEY_S, SF_KEY_T, SF_KEY_U, SF_KEY_V, SF_KEY_W, SF_KEY_X,
  SF_KEY_Y, SF_KEY_Z,
  SF_KEY_0, SF_KEY_1, SF_KEY_2, SF_KEY_3, SF_KEY_4,
  SF_KEY_5, SF_KEY_6, SF_KEY_7, SF_KEY_8, SF_KEY_9,
  SF_KEY_SPACE, SF_KEY_LSHIFT, SF_KEY_UP, SF_KEY_DOWN,
  SF_KEY_LEFT, SF_KEY_RIGHT,
  SF_KEY_MAX
} sf_key_t;

typedef enum {
  SF_MOUSE_LEFT = 0,
  SF_MOUSE_RIGHT,
  SF_MOUSE_MIDDLE,
  SF_MOUSE_MAX
} sf_mouse_btn_t;

typedef enum {
  SF_EVT_RENDER_START = 0,
  SF_EVT_RENDER_END,
  SF_EVT_KEY_DOWN,
  SF_EVT_KEY_UP,
  SF_EVT_MOUSE_MOVE,
  SF_EVT_MOUSE_DOWN,
  SF_EVT_MOUSE_UP,
  SF_EVT_MAX
} sf_event_type_t;

typedef struct {
  sf_event_type_t type;
  union {
    sf_key_t key;
    struct { int x, y, dx, dy; } mouse_move;
    struct { sf_mouse_btn_t btn; int x, y; } mouse_btn;
  };
} sf_event_t;

typedef void (*sf_event_cb)(struct sf_ctx_t_ *ctx, const sf_event_t *event, void *userdata);

typedef struct {
  sf_event_cb                   cb;
  void                         *userdata;
} sf_callback_entry_t;

typedef struct {
    bool                        keys[SF_KEY_MAX];
    bool                        keys_prev[SF_KEY_MAX];
    int                         mouse_x, mouse_y;
    int                         mouse_dx, mouse_dy;
    bool                        mouse_btns[SF_MOUSE_MAX];
    bool                        mouse_btns_prev[SF_MOUSE_MAX];
} sf_input_state_t;

struct sf_ctx_t_ {
  sf_run_state_t                state;
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
  sf_tex_t                     *textures;
  int32_t                       tex_count;

  sf_light_t                   *lights;
  int32_t                       light_count;

  sf_input_state_t              input;
  sf_callback_entry_t           callbacks[SF_EVT_MAX][SF_MAX_CB_PER_EVT];

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
};

/* SF_CORE_FUNCTIONS */
void        sf_init             (sf_ctx_t *ctx, int w, int h);
void        sf_destroy          (sf_ctx_t *ctx);
bool        sf_running          (sf_ctx_t *ctx);
void        sf_stop             (sf_ctx_t *ctx);
sf_arena_t  sf_arena_init       (size_t size);
void*       sf_arena_alloc      (sf_ctx_t *ctx, sf_arena_t *arena, size_t size);
size_t      sf_arena_save       (sf_arena_t *arena);
void        sf_arena_restore    (sf_arena_t *arena, size_t mark);
void        sf_set_logger       (sf_ctx_t *ctx, sf_log_fn log_cb, void* userdata);
void        sf_logger_console   (const char* message, void* userdata);
void        sf_log              (sf_ctx_t *ctx, sf_log_level_t level, const char* fmt, ...);
void        _sf_log             (sf_ctx_t *ctx, sf_log_level_t level, const char* func, const char* fmt, ...);
sf_tex_t*   sf_load_texture_bmp (sf_ctx_t *ctx, const char *filename, const char *texname);
sf_tex_t*   _sf_get_texture     (sf_ctx_t *ctx, const char *texname, bool should_log_failure);
sf_obj_t*   sf_load_obj         (sf_ctx_t *ctx, const char *filename, const char *objname);
sf_obj_t*   _sf_get_obj         (sf_ctx_t *ctx, const char *objname, bool should_log_failure);
sf_enti_t*  sf_add_enti         (sf_ctx_t *ctx, sf_obj_t *obj, const char *entiname);
sf_enti_t*  _sf_get_enti        (sf_ctx_t *ctx, const char *entiname, bool should_log_failure);
void        sf_enti_set_pos     (sf_enti_t *enti, float x, float y, float z);
void        sf_enti_move        (sf_enti_t *enti, float dx, float dy, float dz);
void        sf_enti_set_rot     (sf_enti_t *enti, float rx, float ry, float rz);
void        sf_enti_rotate      (sf_enti_t *enti, float drx, float dry, float drz);
void        sf_enti_set_scale   (sf_enti_t *enti, float sx, float sy, float sz);
void        sf_enti_set_tex     (sf_ctx_t *ctx, const char *enti_name, const char *tex_name);
sf_light_t* sf_add_light_dir    (sf_ctx_t *ctx, sf_fvec3_t dir, sf_fvec3_t color, float intensity);
sf_light_t* sf_add_light_point  (sf_ctx_t *ctx, sf_fvec3_t pos, sf_fvec3_t color, float intensity);
sf_world_t* sf_load_world       (sf_ctx_t *ctx, const char *filename, const char *world_name);
void        sf_clear_world      (sf_ctx_t *ctx, sf_world_t *world);
void        sf_camera_set_psp   (sf_camera_t *cam, float fov, float near_plane, float far_plane);
void        sf_camera_set_pos   (sf_camera_t *cam, float x, float y, float z);
void        sf_camera_move_loc  (sf_camera_t *cam, float fwd, float right, float up);
void        sf_camera_look_at   (sf_camera_t *cam, sf_fvec3_t target);
void        sf_camera_add_yp    (sf_camera_t *cam, float yaw_offset, float pitch_offset);
void        sf_reg_event        (sf_ctx_t *ctx, sf_event_type_t type, sf_event_cb cb, void *userdata);
void        sf_trigger_event    (sf_ctx_t *ctx, const sf_event_t *event);
void        sf_input_cycle_state(sf_ctx_t *ctx);
void        sf_input_set_key    (sf_ctx_t *ctx, sf_key_t key, bool is_down);
void        sf_input_set_mouse_p(sf_ctx_t *ctx, int x, int y);
void        sf_input_set_mouse_b(sf_ctx_t *ctx, sf_mouse_btn_t btn, bool is_down);
bool        sf_key_down         (sf_ctx_t *ctx, sf_key_t key);
bool        sf_key_pressed      (sf_ctx_t *ctx, sf_key_t key);
void        _sf_update_cam_vecs (sf_camera_t *cam);
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
void        sf_tri              (sf_ctx_t *ctx, sf_pkd_clr_t c, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, bool use_depth);
void        sf_tri_tex          (sf_ctx_t *ctx, sf_tex_t *tex, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, sf_fvec3_t uvz0, sf_fvec3_t uvz1, sf_fvec3_t uvz2, sf_fvec3_t l_int);
void        sf_put_text         (sf_ctx_t *ctx, const char *text, sf_ivec2_t p, sf_pkd_clr_t c, int scale);
void        sf_clear_depth      (sf_ctx_t *ctx);
void        sf_draw_debug_axes  (sf_ctx_t *ctx);

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
sf_fmat4_t  sf_make_scale_fmat4 (sf_fvec3_t scale);

/* SF_IMPLEMENTATION_HELPERS */
uint32_t    _sf_vec_to_index    (sf_ctx_t *ctx, sf_ivec2_t v);
void        _sf_swap_svec2      (sf_ivec2_t *v0, sf_ivec2_t *v1);
void        _sf_swap_fvec3      (sf_fvec3_t *v0, sf_fvec3_t *v1);
void        _sf_interp_fvec3    (sf_fvec3_t v0, sf_fvec3_t v1, int steps, sf_fvec3_t *out);
void        _sf_interp_x        (sf_ivec2_t  v0, sf_ivec2_t v1, int *xs);
void        _sf_interp_y        (sf_ivec2_t  v0, sf_ivec2_t v1, int *ys);
void        _sf_interp_f        (float v0, float v1, int steps, float *out);
float       _sf_lerp_f          (float a, float b, float t);
sf_fvec3_t  _sf_lerp_fvec3      (sf_fvec3_t a, sf_fvec3_t b, float t);
sf_fvec3_t  _sf_intersect_near  (sf_fvec3_t v0, sf_fvec3_t v1, float near);
sf_fvec3_t  _sf_project_vertex  (sf_ctx_t *ctx, sf_fvec3_t v, sf_fmat4_t P);
const char* _sf_log_lvl_to_str  (sf_log_level_t level);
bool        _sf_resolve_asset   (const char* filename, char* out_path, size_t max_len);

/* SF_UTILITIES */
sf_pkd_clr_t sf_pack_color (sf_unpkd_clr_t);
size_t       sf_obj_memusg (sf_obj_t *obj);

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
  memset(ctx, 0, sizeof(sf_ctx_t));
  ctx->state                    = SF_RUN_STATE_RUNNING;
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
  ctx->lights                   = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_LIGHTS   * sizeof(sf_light_t));
  ctx->textures                 = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_TEXTURES * sizeof(sf_tex_t));
  ctx->obj_count                = 0;
  ctx->enti_count               = 0;
  ctx->light_count              = 0;
  ctx->tex_count                = 0;
  ctx->camera.pos               = (sf_fvec3_t){0.0f, 0.0f, 0.0f};
  ctx->camera.world_up          = (sf_fvec3_t){0.0f, 1.0f, 0.0f};
  ctx->camera.yaw               = -90.0f;
  ctx->camera.pitch             = 0.0f;
  ctx->camera.fov               = 60.0f;
  ctx->camera.near_plane        = 0.1f;
  ctx->camera.far_plane         = 100.0f;
  ctx->camera.is_view_dirty     = true;
  ctx->camera.is_proj_dirty     = true;
  ctx->_start_ticks             = _sf_get_ticks();
  ctx->_last_ticks              = ctx->_start_ticks;
  ctx->delta_time               = 0.0f;
  ctx->elapsed_time             = 0.0f;
  ctx->fps                      = 0.0f;
  ctx->frame_count              = 0;
  _sf_update_cam_vecs(&ctx->camera);
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
  ctx->state                    = SF_RUN_STATE_STOPPED;
  ctx->arena.offset             = 0;
  ctx->buffer                   = NULL;
  ctx->buffer_size              = 0;
  ctx->w                        = 0;
  ctx->h                        = 0;
  ctx->enti_count               = 0;
  ctx->obj_count                = 0;
  ctx->tex_count                = 0;
  ctx->light_count              = 0;
  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "buffer : %dx%d\n"
              SF_LOG_INDENT "memory : %d\n"
              SF_LOG_INDENT "mxobjs : %d\n", 
              ctx->w, ctx->h, SF_ARENA_SIZE, SF_MAX_OBJS);
}

bool sf_running(sf_ctx_t *ctx) {
  return (ctx->state == SF_RUN_STATE_RUNNING);
}

void sf_stop(sf_ctx_t *ctx) {
  ctx->state = SF_RUN_STATE_STOPPED;
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

size_t sf_arena_save(sf_arena_t *arena) {
  return arena->offset;
}

void sf_arena_restore(sf_arena_t *arena, size_t mark) {
  arena->offset = mark;
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

sf_tex_t* sf_load_texture_bmp(sf_ctx_t *ctx, const char *filename, const char *texname) {
  if (ctx->tex_count >= SF_MAX_TEXTURES) return NULL;
  if (_sf_get_texture(ctx, texname, false) != NULL) return NULL;
  char path[512];
  if (!_sf_resolve_asset(filename, path, sizeof(path))) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "Missing texture file: %s\n", filename);
    return NULL;
  }
  FILE *file = fopen(path, "rb");
  if (!file) return NULL;
  uint8_t header[54];
  if (fread(header, 1, 54, file) != 54 || header[0] != 'B' || header[1] != 'M') {
    fclose(file);
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "Invalid BMP format: %s\n", filename);
    return NULL;
  }
  uint32_t data_offset = header[10] | (header[11]<<8) | (header[12]<<16) | (header[13]<<24);
  int32_t w = header[18] | (header[19]<<8) | (header[20]<<16) | (header[21]<<24);
  int32_t h = header[22] | (header[23]<<8) | (header[24]<<16) | (header[25]<<24);
  int32_t h_abs = abs(h);
  sf_tex_t *tex = &ctx->textures[ctx->tex_count++];
  tex->w = w;
  tex->h = h_abs;
  tex->id = ctx->tex_count - 1;
  tex->px = sf_arena_alloc(ctx, &ctx->arena, w * h_abs * sizeof(sf_fvec3_t));
  size_t name_len = strlen(texname) + 1;
  tex->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (tex->name) memcpy((void*)tex->name, texname, name_len);
  fseek(file, data_offset, SEEK_SET);
  int padding = (4 - (w * 3) % 4) % 4;
  uint8_t bgr[3];
  for (int y = 0; y < h_abs; y++) {
    int dest_y = (h > 0) ? (h_abs - 1 - y) : y; 
    for (int x = 0; x < w; x++) {
      fread(bgr, 1, 3, file);
      float r = bgr[2] / 255.0f;
      float g = bgr[1] / 255.0f;
      float b = bgr[0] / 255.0f;
      tex->px[dest_y * w + x] = (sf_fvec3_t){ powf(r, 2.2f), powf(g, 2.2f), powf(b, 2.2f) };
    }
    fseek(file, padding, SEEK_CUR);
  }
  fclose(file);
  SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "loaded texture : %s (%dx%d)\n", texname, w, h_abs);
  return tex;
}

sf_tex_t* _sf_get_texture(sf_ctx_t *ctx, const char *texname, bool should_log_failure) {
  for (int32_t i = 0; i < ctx->tex_count; ++i) {
    if (ctx->textures[i].name && strcmp(ctx->textures[i].name, texname) == 0) {
      return &ctx->textures[i];
    }
  }
  if (should_log_failure) {
    SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "texture '%s' not found\n", texname);
  }
  return NULL;
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

  FILE *file = fopen(filename, "r");
  if (!file) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "could not open %s\n", filename);
    return NULL;
  }

  int32_t v_cnt = 0, vt_cnt = 0, vn_cnt = 0, f_cnt = 0; char line[256];
  while (fgets(line, sizeof(line), file)) {
    if      (line[0] == 'v' && line[1] == ' ') v_cnt++;
    else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') vt_cnt++;
    else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') vn_cnt++;
    else if (line[0] == 'f' && line[1] == ' ') f_cnt++;
  }

  sf_obj_t *obj = &ctx->objs[ctx->obj_count++];
  obj->v_cnt = v_cnt; obj->vt_cnt = vt_cnt; obj->vn_cnt = vn_cnt; obj->f_cnt = f_cnt;
  
  obj->v  = sf_arena_alloc(ctx, &ctx->arena, v_cnt * sizeof(sf_fvec3_t));
  obj->vt = sf_arena_alloc(ctx, &ctx->arena, vt_cnt * sizeof(sf_fvec2_t));
  obj->vn = sf_arena_alloc(ctx, &ctx->arena, vn_cnt * sizeof(sf_fvec3_t));
  obj->f  = sf_arena_alloc(ctx, &ctx->arena, f_cnt * sizeof(sf_face_t));
  
  size_t name_len = strlen(objname) + 1;
  obj->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (obj->name) {
    memcpy((void*)obj->name, objname, name_len);
  }
  obj->id = ctx->obj_count - 1;

  if (!obj->v || !obj->f || !obj->name) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "out of arena memory for %s\n", filename);
    fclose(file);
    return NULL;
  }

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "file   : %s\n"
              SF_LOG_INDENT "name   : %s\n"
              SF_LOG_INDENT "id     : %d\n"
              SF_LOG_INDENT "verts  : %d\n"
              SF_LOG_INDENT "uvs    : %d\n"
              SF_LOG_INDENT "norms  : %d\n"
              SF_LOG_INDENT "faces  : %d\n"
              SF_LOG_INDENT "size   : %zu\n"
              SF_LOG_INDENT "mem    : %.2f\n"
              SF_LOG_INDENT "obj_id : %d\n",
              filename, objname, obj->id, v_cnt, vt_cnt, vn_cnt, f_cnt, 
              sf_obj_memusg(obj), 
              ((float)ctx->arena.offset / (float)ctx->arena.size) * 100.0f,
              ctx->obj_count - 1);

  rewind(file);
  int v_idx = 0, vt_idx = 0, vn_idx = 0, f_idx = 0;
  while (fgets(line, sizeof(line), file)) {
    if (line[0] == 'v' && line[1] == ' ') {
      sscanf(line, "v %f %f %f", &obj->v[v_idx].x, &obj->v[v_idx].y, &obj->v[v_idx].z);
      v_idx++;
    }
    else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
      sscanf(line, "vt %f %f", &obj->vt[vt_idx].x, &obj->vt[vt_idx].y);
      vt_idx++;
    }
    else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {
      sscanf(line, "vn %f %f %f", &obj->vn[vn_idx].x, &obj->vn[vn_idx].y, &obj->vn[vn_idx].z);
      vn_idx++;
    }
    else if (line[0] == 'f' && line[1] == ' ') {
      int v[3] = {0}, vt[3] = {0}, vn[3] = {0};
      char t1[32], t2[32], t3[32];
      if (sscanf(line, "f %31s %31s %31s", t1, t2, t3) == 3) {
        sscanf(t1, "%d/%d/%d", &v[0], &vt[0], &vn[0]);
        if (strstr(t1, "//")) sscanf(t1, "%d//%d", &v[0], &vn[0]);

        sscanf(t2, "%d/%d/%d", &v[1], &vt[1], &vn[1]);
        if (strstr(t2, "//")) sscanf(t2, "%d//%d", &v[1], &vn[1]);

        sscanf(t3, "%d/%d/%d", &v[2], &vt[2], &vn[2]);
        if (strstr(t3, "//")) sscanf(t3, "%d//%d", &v[2], &vn[2]);

        for (int i = 0; i < 3; i++) {
          obj->f[f_idx].idx[i].v  = v[i] > 0 ? v[i] - 1 : -1;
          obj->f[f_idx].idx[i].vt = vt[i] > 0 ? vt[i] - 1 : -1;
          obj->f[f_idx].idx[i].vn = vn[i] > 0 ? vn[i] - 1 : -1;
        }
        f_idx++;
      }
    }
  }

  fclose(file);
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
  enti->obj       = *obj;
  enti->M         = sf_make_idn_fmat4();
  enti->pos       = (sf_fvec3_t){0.0f, 0.0f, 0.0f};
  enti->rot       = (sf_fvec3_t){0.0f, 0.0f, 0.0f};
  enti->scale     = (sf_fvec3_t){1.0f, 1.0f, 1.0f};
  enti->is_dirty  = true;
  enti->id        = ctx->enti_count - 1;

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

void sf_enti_set_pos(sf_enti_t *enti, float x, float y, float z) {
  enti->pos = (sf_fvec3_t){x, y, z};
  enti->is_dirty = true;
}

void sf_enti_move(sf_enti_t *enti, float dx, float dy, float dz) {
  enti->pos.x += dx;
  enti->pos.y += dy;
  enti->pos.z += dz;
  enti->is_dirty = true;
}

void sf_enti_set_rot(sf_enti_t *enti, float rx, float ry, float rz) {
  enti->rot = (sf_fvec3_t){rx, ry, rz};
  enti->is_dirty = true;
}

void sf_enti_rotate(sf_enti_t *enti, float drx, float dry, float drz) {
  enti->rot.x += drx;
  enti->rot.y += dry;
  enti->rot.z += drz;
  enti->is_dirty = true;
}

void sf_enti_set_scale(sf_enti_t *enti, float sx, float sy, float sz) {
  enti->scale = (sf_fvec3_t){sx, sy, sz};
  enti->is_dirty = true;
}

sf_light_t* sf_add_light_dir(sf_ctx_t *ctx, sf_fvec3_t dir, sf_fvec3_t color, float intensity) {
  if (ctx->light_count >= SF_MAX_LIGHTS) return NULL;
  sf_light_t *l = &ctx->lights[ctx->light_count++];
  l->type      = SF_LIGHT_DIR;
  l->pos_dir   = sf_fvec3_norm(dir);
  l->color     = color;
  l->intensity = intensity;
  return l;
}

sf_light_t* sf_add_light_point(sf_ctx_t *ctx, sf_fvec3_t pos, sf_fvec3_t color, float intensity) {
  if (ctx->light_count >= SF_MAX_LIGHTS) return NULL;
  sf_light_t *l = &ctx->lights[ctx->light_count++];
  l->type      = SF_LIGHT_POINT;
  l->pos_dir   = pos;
  l->color     = color;
  l->intensity = intensity;
  return l;
}

sf_world_t* sf_load_world(sf_ctx_t *ctx, const char *filename, const char *world_name) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "could not open %s\n", filename);
    return NULL;
  }
  sf_world_t *world = (sf_world_t*)sf_arena_alloc(ctx, &ctx->arena, sizeof(sf_world_t));
  world->name = world_name;
  world->obj_start_idx = ctx->obj_count;
  world->enti_start_idx = ctx->enti_count;
  world->light_start_idx = ctx->light_count;
  char line[512];
  while (fgets(line, sizeof(line), file)) {
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
    char cmd;
    sscanf(line, " %c", &cmd);
    if (cmd == 't') {
      char t_name[64], t_file[256];
      if (sscanf(line, "t %63s %255s", t_name, t_file) == 2) {
        sf_load_texture_bmp(ctx, t_file, t_name);
      }
    }
    else if (cmd == 'm') {
      char m_name[64], m_file[256], r_path[512];
      if (sscanf(line, "m %63s %255s", m_name, m_file) == 2) {
        if (_sf_resolve_asset(m_file, r_path, sizeof(r_path))) {
          sf_load_obj(ctx, r_path, m_name);
        }
      }
    } 
    else if (cmd == 'e') {
      char m_name[64], e_name[64], t_name[64] = {0};
      float px, py, pz, rx, ry, rz, sx, sy, sz;
      int res = sscanf(line, "e %63s %63s %f %f %f %f %f %f %f %f %f %63s", 
        m_name, e_name, &px, &py, &pz, &rx, &ry, &rz, &sx, &sy, &sz, t_name);
      if (res >= 11) {
        sf_obj_t *obj = _sf_get_obj(ctx, m_name, true);
        if (obj) {
          sf_enti_t *enti = sf_add_enti(ctx, obj, e_name);
          sf_enti_set_pos(enti, px, py, pz);
          sf_enti_set_rot(enti, rx, ry, rz);
          sf_enti_set_scale(enti, sx, sy, sz);
          if (res == 12) {
            enti->tex = _sf_get_texture(ctx, t_name, true);
            SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "Assigned texture %s to %s\n", t_name, e_name);
          }
        }
      }
    }
    else if (cmd == 'l') {
      char l_type[16];
      float x, y, z, r, g, b, i;
      sscanf(line, "l %15s %f %f %f %f %f %f %f", l_type, &x, &y, &z, &r, &g, &b, &i);
      if (strcmp(l_type, "dir") == 0) {
        sf_add_light_dir(ctx, (sf_fvec3_t){x, y, z}, (sf_fvec3_t){r, g, b}, i);
      } else if (strcmp(l_type, "point") == 0) {
        sf_add_light_point(ctx, (sf_fvec3_t){x, y, z}, (sf_fvec3_t){r, g, b}, i);
      }
    }
    else if (cmd == 'c') {
      float px, py, pz, tx, ty, tz;
      if (sscanf(line, "c %f %f %f %f %f %f", &px, &py, &pz, &tx, &ty, &tz) == 6) {
        sf_camera_set_pos(&ctx->camera, px, py, pz);
        sf_camera_look_at(&ctx->camera, (sf_fvec3_t){tx, ty, tz});
      }
    }
  }
  world->obj_count = ctx->obj_count - world->obj_start_idx;
  world->enti_count = ctx->enti_count - world->enti_start_idx;
  world->light_count = ctx->light_count - world->light_start_idx;
  fclose(file);
  SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "loaded '%s' (%d entities)\n", world_name, world->enti_count);
  return world;
}

void sf_clear_world(sf_ctx_t *ctx, sf_world_t *world) {
    if (!world) return;

    SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "clearing '%s' (%d entities)\n", world->name, world->enti_count);

    // 1. Reset counts to the state before this world was loaded
    // This effectively "deletes" the entities/objs/lights from the context's linear arrays
    ctx->obj_count   = world->obj_start_idx;
    ctx->enti_count  = world->enti_count > 0 ? world->enti_start_idx : ctx->enti_count;
    ctx->light_count = world->light_count > 0 ? world->light_start_idx : ctx->light_count;

    // 2. Roll back the arena memory
    // We calculate the mark based on the first allocation made during sf_load_world
    // Note: This assumes the world structure itself was the first thing allocated in that session.
    size_t world_mem_mark = (size_t)((uint8_t*)world - ctx->arena.buffer);
    sf_arena_restore(&ctx->arena, world_mem_mark);
    
    // Optional: Clear the camera or input if you want a totally fresh slate
    // ctx->camera.pos = (sf_fvec3_t){0,0,0}; 
}

void _sf_update_cam_vecs(sf_camera_t *cam) {
  sf_fvec3_t front;
  front.x = cosf(SF_DEG2RAD(cam->yaw)) * cosf(SF_DEG2RAD(cam->pitch));
  front.y = sinf(SF_DEG2RAD(cam->pitch));
  front.z = sinf(SF_DEG2RAD(cam->yaw)) * cosf(SF_DEG2RAD(cam->pitch));
  cam->front = sf_fvec3_norm(front);
  cam->right = sf_fvec3_norm(sf_fvec3_cross(cam->front, cam->world_up));
  cam->up    = sf_fvec3_norm(sf_fvec3_cross(cam->right, cam->front));
  cam->is_view_dirty = true;
}

void sf_camera_set_psp(sf_camera_t *cam, float fov, float near_plane, float far_plane) {
  cam->fov        = fov;
  cam->near_plane = near_plane;
  cam->far_plane  = far_plane;
  cam->is_proj_dirty = true;
}

void sf_camera_set_pos(sf_camera_t *cam, float x, float y, float z) {
  cam->pos = (sf_fvec3_t){x, y, z};
  cam->is_view_dirty = true;
}

void sf_camera_move_loc(sf_camera_t *cam, float fwd, float right, float up) {
  sf_fvec3_t m_fwd = {cam->front.x * fwd, cam->front.y * fwd, cam->front.z * fwd};
  sf_fvec3_t m_rgt = {cam->right.x * right, cam->right.y * right, cam->right.z * right};
  sf_fvec3_t m_up  = {cam->up.x * up, cam->up.y * up, cam->up.z * up};
  cam->pos = sf_fvec3_add(cam->pos, m_fwd);
  cam->pos = sf_fvec3_add(cam->pos, m_rgt);
  cam->pos = sf_fvec3_add(cam->pos, m_up);
  cam->is_view_dirty = true;
}

void sf_camera_look_at(sf_camera_t *cam, sf_fvec3_t target) {
  cam->front = sf_fvec3_norm(sf_fvec3_sub(target, cam->pos));
  cam->right = sf_fvec3_norm(sf_fvec3_cross(cam->front, cam->world_up));
  cam->up    = sf_fvec3_norm(sf_fvec3_cross(cam->right, cam->front));
  cam->pitch = SF_RAD2DEG(asinf(cam->front.y));
  cam->yaw   = SF_RAD2DEG(atan2f(cam->front.z, cam->front.x));
  cam->is_view_dirty = true;
}

void sf_camera_add_yp(sf_camera_t *cam, float yaw_offset, float pitch_offset) {
  cam->yaw   += yaw_offset;
  cam->pitch += pitch_offset;
  if (cam->pitch > 89.0f)  cam->pitch = 89.0f;
  if (cam->pitch < -89.0f) cam->pitch = -89.0f;
  _sf_update_cam_vecs(cam);
}

void sf_reg_event(sf_ctx_t *ctx, sf_event_type_t type, sf_event_cb cb, void *userdata) {
  if (type >= SF_EVT_MAX) return;
  for (int i = 0; i < SF_MAX_CB_PER_EVT; ++i) {
    if (ctx->callbacks[type][i].cb == NULL) {
      ctx->callbacks[type][i].cb = cb;
      ctx->callbacks[type][i].userdata = userdata;
      return;
    }
  }
  SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "Callback slots full for event type %d\n", type);
}

void sf_trigger_event(sf_ctx_t *ctx, const sf_event_t *event) {
  if (event->type >= SF_EVT_MAX) return;
  for (int i = 0; i < SF_MAX_CB_PER_EVT; ++i) {
    if (ctx->callbacks[event->type][i].cb) {
      ctx->callbacks[event->type][i].cb(ctx, event, ctx->callbacks[event->type][i].userdata);
    }
  }
}

void sf_input_cycle_state(sf_ctx_t *ctx) {
  memcpy(ctx->input.keys_prev, ctx->input.keys, sizeof(ctx->input.keys));
  memcpy(ctx->input.mouse_btns_prev, ctx->input.mouse_btns, sizeof(ctx->input.mouse_btns));
  ctx->input.mouse_dx = 0;
  ctx->input.mouse_dy = 0;
}

void sf_input_set_key(sf_ctx_t *ctx, sf_key_t key, bool is_down) {
  if (key == SF_KEY_UNKNOWN || key >= SF_KEY_MAX) return;

  if (ctx->input.keys[key] != is_down) {
    ctx->input.keys[key] = is_down;
    sf_event_t ev;
    ev.type = is_down ? SF_EVT_KEY_DOWN : SF_EVT_KEY_UP;
    ev.key = key;
    sf_trigger_event(ctx, &ev);
  }
}

void sf_input_set_mouse_p(sf_ctx_t *ctx, int x, int y) {
  ctx->input.mouse_dx = x - ctx->input.mouse_x;
  ctx->input.mouse_dy = y - ctx->input.mouse_y;
  ctx->input.mouse_x  = x;
  ctx->input.mouse_y  = y;

  if (ctx->input.mouse_dx != 0 || ctx->input.mouse_dy != 0) {
    sf_event_t ev;
    ev.type = SF_EVT_MOUSE_MOVE;
    ev.mouse_move.x  = x;
    ev.mouse_move.y  = y;
    ev.mouse_move.dx = ctx->input.mouse_dx;
    ev.mouse_move.dy = ctx->input.mouse_dy;
    sf_trigger_event(ctx, &ev);
  }
}

void sf_input_set_mouse_b(sf_ctx_t *ctx, sf_mouse_btn_t btn, bool is_down) {
  if (btn >= SF_MOUSE_MAX) return;

  if (ctx->input.mouse_btns[btn] != is_down) {
    ctx->input.mouse_btns[btn] = is_down;
    sf_event_t ev;
    ev.type = is_down ? SF_EVT_MOUSE_DOWN : SF_EVT_MOUSE_UP;
    ev.mouse_btn.btn = btn;
    ev.mouse_btn.x = ctx->input.mouse_x;
    ev.mouse_btn.y = ctx->input.mouse_y;
    sf_trigger_event(ctx, &ev);
  }
}

bool sf_key_down(sf_ctx_t *ctx, sf_key_t key) {
  return ctx->input.keys[key];
}

bool sf_key_pressed(sf_ctx_t *ctx, sf_key_t key) {
  return ctx->input.keys[key] && !ctx->input.keys_prev[key];
}

void sf_render_enti(sf_ctx_t *ctx, sf_enti_t *enti) {
  size_t mark = sf_arena_save(&ctx->arena);
  sf_fmat4_t M = enti->M;
  sf_fmat4_t V = ctx->camera.V;
  sf_fmat4_t P = ctx->camera.P;
  float near = 0.1f;
  sf_fvec3_t* wv = sf_arena_alloc(ctx, &ctx->arena, enti->obj.v_cnt * sizeof(sf_fvec3_t));
  sf_fvec3_t* vv = sf_arena_alloc(ctx, &ctx->arena, enti->obj.v_cnt * sizeof(sf_fvec3_t));
  if (!wv || !vv) return;
  for (int i = 0; i < enti->obj.v_cnt; i++) {
    wv[i] = sf_fmat4_mul_vec3(M, enti->obj.v[i]);
    vv[i] = sf_fmat4_mul_vec3(V, wv[i]);
  }
  for (int i = 0; i < enti->obj.f_cnt; i++) {
    sf_face_t face = enti->obj.f[i];
    sf_fvec3_t v_world[3] = { wv[face.idx[0].v], wv[face.idx[1].v], wv[face.idx[2].v] };
    sf_fvec3_t v_view[3]  = { vv[face.idx[0].v], vv[face.idx[1].v], vv[face.idx[2].v] };
    sf_fvec3_t a_v = sf_fvec3_sub(v_view[1], v_view[0]);
    sf_fvec3_t b_v = sf_fvec3_sub(v_view[2], v_view[0]);
    sf_fvec3_t n_v = sf_fvec3_cross(a_v, b_v);
    if (sf_fvec3_dot(n_v, v_view[0]) >= 0) continue;
    sf_fvec3_t a_w = sf_fvec3_sub(v_world[1], v_world[0]);
    sf_fvec3_t b_w = sf_fvec3_sub(v_world[2], v_world[0]);
    sf_fvec3_t n_w = sf_fvec3_norm(sf_fvec3_cross(a_w, b_w));
    sf_fvec3_t centroid = {
      (v_world[0].x + v_world[1].x + v_world[2].x) / 3.0f,
      (v_world[0].y + v_world[1].y + v_world[2].y) / 3.0f,
      (v_world[0].z + v_world[1].z + v_world[2].z) / 3.0f
    };
    sf_fvec3_t l_int = {0.1f, 0.1f, 0.1f};
    for (int l = 0; l < ctx->light_count; l++) {
      sf_light_t *light = &ctx->lights[l];
      sf_fvec3_t light_dir;
      float atten = 1.0f;
      if (light->type == SF_LIGHT_DIR) {
        light_dir = sf_fvec3_norm((sf_fvec3_t){-light->pos_dir.x, -light->pos_dir.y, -light->pos_dir.z});
      } else {
        sf_fvec3_t diff = sf_fvec3_sub(light->pos_dir, centroid);
        float dist = sqrtf(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
        light_dir = sf_fvec3_norm(diff);
        atten = 1.0f / (1.0f + 0.09f * dist + 0.032f * (dist * dist));
      }
      float diff_factor = sf_fvec3_dot(n_w, light_dir);
      if (diff_factor > 0.0f) {
        l_int.x += light->color.x * light->intensity * diff_factor * atten;
        l_int.y += light->color.y * light->intensity * diff_factor * atten;
        l_int.z += light->color.z * light->intensity * diff_factor * atten;
      }
    }
    l_int.x = l_int.x > 1.0f ? 1.0f : l_int.x;
    l_int.y = l_int.y > 1.0f ? 1.0f : l_int.y;
    l_int.z = l_int.z > 1.0f ? 1.0f : l_int.z;
    sf_fvec2_t uvs[3] = {0};
    bool has_uvs = (enti->obj.vt_cnt > 0 && face.idx[0].vt != -1);
    if (has_uvs) {
      uvs[0] = enti->obj.vt[face.idx[0].vt];
      uvs[1] = enti->obj.vt[face.idx[1].vt];
      uvs[2] = enti->obj.vt[face.idx[2].vt];
    }
    sf_fvec3_t uvz[3];
    for (int j = 0; j < 3; j++) {
      float z = -v_view[j].z;
      if (z < near) z = near;
      float iz = 1.0f / z;
      uvz[j] = (sf_fvec3_t){ uvs[j].x * iz, uvs[j].y * iz, iz };
    }
    sf_fvec3_t in[3], out[3], in_uvz[3], out_uvz[3];
    int inc = 0, outc = 0;
    for (int j = 0; j < 3; j++) {
      if (v_view[j].z <= -near) {
        in_uvz[inc] = uvz[j];
        in[inc++] = v_view[j];
      } else {
        out_uvz[outc] = uvz[j];
        out[outc++] = v_view[j];
      }
    }
    if (enti->tex && has_uvs) {
      if (inc == 3) {
        sf_tri_tex(ctx, enti->tex, _sf_project_vertex(ctx, in[0], P), _sf_project_vertex(ctx, in[1], P), _sf_project_vertex(ctx, in[2], P), in_uvz[0], in_uvz[1], in_uvz[2], l_int);
      } else if (inc == 1) {
        float t1 = ((-near) - in[0].z) / (out[0].z - in[0].z);
        float t2 = ((-near) - in[0].z) / (out[1].z - in[0].z);
        sf_fvec3_t v1 = { in[0].x + (out[0].x - in[0].x) * t1, in[0].y + (out[0].y - in[0].y) * t1, -near };
        sf_fvec3_t v2 = { in[0].x + (out[1].x - in[0].x) * t2, in[0].y + (out[1].y - in[0].y) * t2, -near };
        sf_fvec3_t uvz1 = { in_uvz[0].x + (out_uvz[0].x - in_uvz[0].x) * t1, in_uvz[0].y + (out_uvz[0].y - in_uvz[0].y) * t1, in_uvz[0].z + (out_uvz[0].z - in_uvz[0].z) * t1 };
        sf_fvec3_t uvz2 = { in_uvz[0].x + (out_uvz[1].x - in_uvz[0].x) * t2, in_uvz[0].y + (out_uvz[1].y - in_uvz[0].y) * t2, in_uvz[0].z + (out_uvz[1].z - in_uvz[0].z) * t2 };
        sf_tri_tex(ctx, enti->tex, _sf_project_vertex(ctx, in[0], P), _sf_project_vertex(ctx, v1, P), _sf_project_vertex(ctx, v2, P), in_uvz[0], uvz1, uvz2, l_int);
      } else if (inc == 2) {
        float t1 = ((-near) - in[0].z) / (out[0].z - in[0].z);
        float t2 = ((-near) - in[1].z) / (out[0].z - in[1].z);
        sf_fvec3_t v1 = { in[0].x + (out[0].x - in[0].x) * t1, in[0].y + (out[0].y - in[0].y) * t1, -near };
        sf_fvec3_t v2 = { in[1].x + (out[0].x - in[1].x) * t2, in[1].y + (out[0].y - in[1].y) * t2, -near };
        sf_fvec3_t uvz1 = { in_uvz[0].x + (out_uvz[0].x - in_uvz[0].x) * t1, in_uvz[0].y + (out_uvz[0].y - in_uvz[0].y) * t1, in_uvz[0].z + (out_uvz[0].z - in_uvz[0].z) * t1 };
        sf_fvec3_t uvz2 = { in_uvz[1].x + (out_uvz[0].x - in_uvz[1].x) * t2, in_uvz[1].y + (out_uvz[0].y - in_uvz[1].y) * t2, in_uvz[1].z + (out_uvz[0].z - in_uvz[1].z) * t2 };
        sf_tri_tex(ctx, enti->tex, _sf_project_vertex(ctx, in[0], P), _sf_project_vertex(ctx, in[1], P), _sf_project_vertex(ctx, v1, P), in_uvz[0], in_uvz[1], uvz1, l_int);
        sf_tri_tex(ctx, enti->tex, _sf_project_vertex(ctx, in[1], P), _sf_project_vertex(ctx, v1, P), _sf_project_vertex(ctx, v2, P), in_uvz[1], uvz1, uvz2, l_int);
      }
    } else {
      sf_pkd_clr_t shaded_color = sf_pack_color((sf_unpkd_clr_t){(uint8_t)(l_int.x * 255), (uint8_t)(l_int.y * 255), (uint8_t)(l_int.z * 255), 255});
      if (inc == 3) {
        sf_tri(ctx, shaded_color, _sf_project_vertex(ctx, v_view[0], P), _sf_project_vertex(ctx, v_view[1], P), _sf_project_vertex(ctx, v_view[2], P), true);
      } else if (inc == 1) {
        sf_fvec3_t v1 = _sf_intersect_near(in[0], out[0], -near);
        sf_fvec3_t v2 = _sf_intersect_near(in[0], out[1], -near);
        sf_tri(ctx, shaded_color, _sf_project_vertex(ctx, in[0], P), _sf_project_vertex(ctx, v1, P), _sf_project_vertex(ctx, v2, P), true);
      } else if (inc == 2) {
        sf_fvec3_t v1 = _sf_intersect_near(in[0], out[0], -near);
        sf_fvec3_t v2 = _sf_intersect_near(in[1], out[0], -near);
        sf_tri(ctx, shaded_color, _sf_project_vertex(ctx, in[0], P), _sf_project_vertex(ctx, in[1], P), _sf_project_vertex(ctx, v1, P), true);
        sf_tri(ctx, shaded_color, _sf_project_vertex(ctx, in[1], P), _sf_project_vertex(ctx, v1, P), _sf_project_vertex(ctx, v2, P), true);
      }
    }
  }
  sf_arena_restore(&ctx->arena, mark);
}

void sf_render_ctx(sf_ctx_t *ctx) {
  sf_event_t ev_start;
  ev_start.type = SF_EVT_RENDER_START;
  sf_trigger_event(ctx, &ev_start);

  if (ctx->camera.is_proj_dirty) {
    float aspect = (float)ctx->w / (float)ctx->h;
    ctx->camera.P = sf_make_psp_fmat4(ctx->camera.fov, aspect, ctx->camera.near_plane, ctx->camera.far_plane);
    ctx->camera.is_proj_dirty = false;
  }
  if (ctx->camera.is_view_dirty) {
    sf_fvec3_t target = sf_fvec3_add(ctx->camera.pos, ctx->camera.front);
    ctx->camera.V = sf_make_view_fmat4(ctx->camera.pos, target, ctx->camera.up);
    ctx->camera.is_view_dirty = false;
  }
  sf_fill(ctx, SF_CLR_BLACK);
  sf_clear_depth(ctx); 
  for (int i = 0; i < ctx->enti_count; i++) {
    sf_enti_t *enti = &ctx->entities[i];
    if (enti->is_dirty) {
      sf_fmat4_t t_mat  = sf_make_tsl_fmat4(enti->pos.x, enti->pos.y, enti->pos.z);
      sf_fmat4_t r_mat  = sf_make_rot_fmat4(enti->rot);
      sf_fmat4_t s_mat  = sf_make_scale_fmat4(enti->scale);
      sf_fmat4_t rs_mat = sf_fmat4_mul_fmat4(r_mat, s_mat);
      enti->M           = sf_fmat4_mul_fmat4(rs_mat, t_mat);
      enti->is_dirty = false;
    }
    sf_render_enti(ctx, enti);
  }

  sf_event_t ev_end;
  ev_end.type = SF_EVT_RENDER_END;
  sf_trigger_event(ctx, &ev_end);
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
  int dx = abs(v1.x - v0.x);
  int sx = v0.x < v1.x ? 1 : -1;
  int dy = -abs(v1.y - v0.y);
  int sy = v0.y < v1.y ? 1 : -1;
  int err = dx + dy;
  int e2;

  while (1) {
    sf_pixel(ctx, c, v0);
    if (v0.x == v1.x && v0.y == v1.y) break;
    e2 = 2 * err;
    if (e2 >= dy) {
      err  += dy;
      v0.x += sx; 
    }
    if (e2 <= dx) {
      err  += dx;
      v0.y += sy;
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

void sf_tri(sf_ctx_t *ctx, sf_pkd_clr_t c, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, bool use_depth) {
  if (v1.y < v0.y) { sf_fvec3_t t = v0; v0 = v1; v1 = t; }
  if (v2.y < v0.y) { sf_fvec3_t t = v0; v0 = v2; v2 = t; }
  if (v2.y < v1.y) { sf_fvec3_t t = v1; v1 = v2; v2 = t; }
  if (v2.y < 0 || v0.y >= ctx->h) return;
  int h = (int)v2.y - (int)v0.y + 1;
  if (h <= 1 || h > 8192) return;
  int x02[h], x012[h];
  float z02[h], z012[h];
  _sf_interp_x((sf_ivec2_t){(int)v0.x, (int)v0.y}, (sf_ivec2_t){(int)v2.x, (int)v2.y}, x02);
  _sf_interp_f(v0.z, v2.z, h - 1, z02);
  int h01 = (int)v1.y - (int)v0.y;
  int h12 = (int)v2.y - (int)v1.y;
  _sf_interp_x((sf_ivec2_t){(int)v0.x, (int)v0.y}, (sf_ivec2_t){(int)v1.x, (int)v1.y}, x012);
  _sf_interp_f(v0.z, v1.z, h01, z012);
  _sf_interp_x((sf_ivec2_t){(int)v1.x, (int)v1.y}, (sf_ivec2_t){(int)v2.x, (int)v2.y}, &x012[h01]);
  _sf_interp_f(v1.z, v2.z, h12, &z012[h01]);
  int *xl = x02, *xr = x012;
  float *zl = z02, *zr = z012;
  if (h01 < h && x012[h01] < x02[h01]) {
    xl = x012; xr = x02;
    zl = z012; zr = z02;
  }
  for (int y = (int)v0.y; y <= (int)v2.y; ++y) {
    if (y < 0 || y >= ctx->h) continue;
    int idx = y - (int)v0.y;
    int x_s = xl[idx], x_e = xr[idx];
    float z_s = zl[idx], z_e = zr[idx];
    float w = (float)(x_e - x_s);
    float dz = (w <= 0.0f) ? 0.0f : (z_e - z_s) / w;
    int ox = x_s;
    if (x_s < 0) x_s = 0;
    if (x_e >= ctx->w) x_e = ctx->w - 1;
    float cz = z_s + (dz * (float)(x_s - ox));
    int bi = y * ctx->w + x_s;
    for (int x = x_s; x <= x_e; ++x) {
      if (use_depth) {
        if (cz < ctx->z_buffer[bi]) {
          ctx->z_buffer[bi] = cz;
          ctx->buffer[bi] = c;
        }
      } else {
        ctx->buffer[bi] = c;
      }
      cz += dz;
      bi++;
    }
  }
}

void sf_tri_tex(sf_ctx_t *ctx, sf_tex_t *tex, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, sf_fvec3_t uvz0, sf_fvec3_t uvz1, sf_fvec3_t uvz2, sf_fvec3_t l_int) {
  if (v1.y < v0.y) { _sf_swap_fvec3(&v0, &v1); _sf_swap_fvec3(&uvz0, &uvz1); }
  if (v2.y < v0.y) { _sf_swap_fvec3(&v0, &v2); _sf_swap_fvec3(&uvz0, &uvz2); }
  if (v2.y < v1.y) { _sf_swap_fvec3(&v1, &v2); _sf_swap_fvec3(&uvz1, &uvz2); }
  if (v2.y < 0 || v0.y >= ctx->h) return;
  int th = (int)v2.y - (int)v0.y + 1;
  if (th <= 1 || th > 1024) return;

  int x02[th], x012[th];
  float z02[th], z012[th];
  sf_fvec3_t uvz02[th], uvz012[th];

  _sf_interp_x((sf_ivec2_t){(int)v0.x, (int)v0.y}, (sf_ivec2_t){(int)v2.x, (int)v2.y}, x02);
  _sf_interp_f(v0.z, v2.z, th - 1, z02);
  _sf_interp_fvec3(uvz0, uvz2, th - 1, uvz02);

  int h01 = (int)v1.y - (int)v0.y;
  int h12 = (int)v2.y - (int)v1.y;

  if (h01 > 0) {
    _sf_interp_x((sf_ivec2_t){(int)v0.x, (int)v0.y}, (sf_ivec2_t){(int)v1.x, (int)v1.y}, x012);
    _sf_interp_f(v0.z, v1.z, h01, z012);
    _sf_interp_fvec3(uvz0, uvz1, h01, uvz012);
  }
  if (h12 > 0) {
    _sf_interp_x((sf_ivec2_t){(int)v1.x, (int)v1.y}, (sf_ivec2_t){(int)v2.x, (int)v2.y}, &x012[h01]);
    _sf_interp_f(v1.z, v2.z, h12, &z012[h01]);
    _sf_interp_fvec3(uvz1, uvz2, h12, &uvz012[h01]);
  }

  int *xl = x02, *xr = x012;
  float *zl = z02, *zr = z012;
  sf_fvec3_t *uvzl = uvz02, *uvzr = uvz012;
  if (h01 < th && x012[h01] < x02[h01]) {
    xl = x012; xr = x02; zl = z012; zr = z02; uvzl = uvz012; uvzr = uvz02;
  }

  for (int y = (int)v0.y; y <= (int)v2.y; ++y) {
    if (y < 0 || y >= ctx->h) continue;
    int i = y - (int)v0.y;
    int xs = xl[i], xe = xr[i];
    float scan_w = (float)(xe - xs);
    if (scan_w <= 0) continue;
    for (int x = xs; x <= xe; x++) {
      if (x < 0 || x >= ctx->w) continue;
      float t = (float)(x - xs) / scan_w;
      float cz = zl[i] + (zr[i] - zl[i]) * t;
      int bi = y * ctx->w + x;
      if (cz < ctx->z_buffer[bi]) {
        sf_fvec3_t uvz = _sf_lerp_fvec3(uvzl[i], uvzr[i], t);
        float u = uvz.x / uvz.z, v = uvz.y / uvz.z;
        int tx = (int)(u * (float)tex->w) % tex->w;
        int ty = (int)(v * (float)tex->h) % tex->h;
        if (tx < 0) tx += tex->w; if (ty < 0) ty += tex->h;
        sf_fvec3_t texel = tex->px[ty * tex->w + tx];
        float r = texel.x * l_int.x, g = texel.y * l_int.y, b = texel.z * l_int.z;
        ctx->z_buffer[bi] = cz;
        ctx->buffer[bi] = sf_pack_color((sf_unpkd_clr_t){
          (uint8_t)(sqrtf(r) * 255.0f), (uint8_t)(sqrtf(g) * 255.0f), (uint8_t)(sqrtf(b) * 255.0f), 255
        });
      }
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

void sf_draw_debug_axes(sf_ctx_t *ctx) {
  int cx = 40;
  int cy = ctx->h - 40;
  float scale = 30.0f;

  sf_fvec3_t x_axis = { ctx->camera.V.m[0][0], ctx->camera.V.m[0][1], ctx->camera.V.m[0][2] };
  sf_fvec3_t y_axis = { ctx->camera.V.m[1][0], ctx->camera.V.m[1][1], ctx->camera.V.m[1][2] };
  sf_fvec3_t z_axis = { ctx->camera.V.m[2][0], ctx->camera.V.m[2][1], ctx->camera.V.m[2][2] };

  #define DRAW_AXIS(axis, color, label) do { \
    sf_ivec2_t end_pt = { cx + (int)(axis.x * scale), cy - (int)(axis.y * scale) }; \
    sf_line(ctx, color, (sf_ivec2_t){cx, cy}, end_pt); \
    int text_x = end_pt.x + (int)(axis.x * 12.0f) - 4; \
    int text_y = end_pt.y - (int)(axis.y * 12.0f) - 4; \
    sf_put_text(ctx, label, (sf_ivec2_t){text_x, text_y}, color, 1); \
  } while(0)

  DRAW_AXIS(x_axis, SF_CLR_RED, "X");
  DRAW_AXIS(y_axis, SF_CLR_GREEN, "Y");
  DRAW_AXIS(z_axis, SF_CLR_BLUE, "Z");

  #undef DRAW_AXIS
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
    m.m[2][2] = (near + far) / z_range;
    m.m[2][3] = -1.0f; 
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

  m.m[0][2] = -f.x;
  m.m[1][2] = -f.y;
  m.m[2][2] = -f.z;

  m.m[3][0] = -sf_fvec3_dot(r, eye);
  m.m[3][1] = -sf_fvec3_dot(u, eye);
  m.m[3][2] =  sf_fvec3_dot(f, eye);

  return m;
}

sf_fmat4_t sf_make_scale_fmat4(sf_fvec3_t s) {
  sf_fmat4_t m = sf_make_idn_fmat4();
  m.m[0][0] = s.x;
  m.m[1][1] = s.y;
  m.m[2][2] = s.z;
  return m;
}

/* SF_IMPLEMENTATION_HELPERS */
uint32_t _sf_vec_to_index(sf_ctx_t *ctx, sf_ivec2_t v) {
  return v.y * ctx->w + v.x;
}

void _sf_swap_svec2(sf_ivec2_t *v0, sf_ivec2_t *v1) {
  sf_ivec2_t t = *v0; *v0 = *v1; *v1 = t;
}

void _sf_swap_fvec3(sf_fvec3_t *v0, sf_fvec3_t *v1) {
    sf_fvec3_t t = *v0; *v0 = *v1; *v1 = t;
}

void _sf_interp_fvec3(sf_fvec3_t v0, sf_fvec3_t v1, int steps, sf_fvec3_t *out) {
  if (steps <= 0) return;
  float step_x = (v1.x - v0.x) / steps;
  float step_y = (v1.y - v0.y) / steps;
  float step_z = (v1.z - v0.z) / steps;
  for (int i = 0; i <= steps; ++i) {
    out[i].x = v0.x + (step_x * i);
    out[i].y = v0.y + (step_y * i);
    out[i].z = v0.z + (step_z * i);
  }
}

void _sf_interp_x(sf_ivec2_t v0, sf_ivec2_t v1, int *xs) {
  if (v0.y == v1.y) {
    xs[0] = v0.x;
    return;
  }
  for (int y = v0.y; y <= v1.y; ++y) {
    xs[y - v0.y] = (y - v0.y) * (v1.x - v0.x) / (v1.y - v0.y) + v0.x;
  }
}

void _sf_interp_y(sf_ivec2_t v0, sf_ivec2_t v1, int *ys) {
  if (v0.x == v1.x) {
    ys[0] = v0.y;
    return;
  }
  for (int x = v0.x; x <= v1.x; ++x) {
    ys[x - v0.x] = (x - v0.x) * (v1.y - v0.y) / (v1.x - v0.x) + v0.y;
  }
}

void _sf_interp_f(float v0, float v1, int steps, float *out) {
  if (steps == 0) return;
  float step = (v1 - v0) / steps;
  for (int i = 0; i <= steps; ++i) {
    out[i] = v0 + (step * i);
  }
}

float _sf_lerp_f(float a, float b, float t) {
  return a + (b - a) * t;
}

sf_fvec3_t _sf_lerp_fvec3(sf_fvec3_t a, sf_fvec3_t b, float t) {
  return (sf_fvec3_t){
    a.x + (b.x - a.x) * t,
    a.y + (b.y - a.y) * t,
    a.z + (b.z - a.z) * t
  };
}

sf_fvec3_t _sf_intersect_near(sf_fvec3_t v0, sf_fvec3_t v1, float near) {
  float t = (near - v0.z) / (v1.z - v0.z);
  return (sf_fvec3_t){
    v0.x + (v1.x - v0.x) * t,
    v0.y + (v1.y - v0.y) * t,
    near
  };
}

sf_fvec3_t _sf_project_vertex(sf_ctx_t *ctx, sf_fvec3_t v, sf_fmat4_t P) {
  sf_fvec3_t proj = sf_fmat4_mul_vec3(P, v);
  return (sf_fvec3_t){
    (proj.x + 1.0f) * 0.5f * (float)ctx->w,
    (1.0f - (proj.y + 1.0f) * 0.5f) * (float)ctx->h,
    proj.z
  };
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

bool _sf_resolve_asset(const char* filename, char* out_path, size_t max_len) {
    char dir_stack[32][512];
    int stack_head = 0;
    snprintf(dir_stack[stack_head++], 512, "%s", SF_ASSET_PATH);
    while (stack_head > 0) {
        char current_dir[512];
        snprintf(current_dir, sizeof(current_dir), "%s", dir_stack[--stack_head]);
        DIR *dir = opendir(current_dir);
        if (!dir) continue;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, entry->d_name);
            struct stat statbuf;
            if (stat(full_path, &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode)) {
                    if (stack_head < 32) {
                        snprintf(dir_stack[stack_head++], 512, "%s", full_path);
                    }
                } else if (strcmp(entry->d_name, filename) == 0) {
                    snprintf(out_path, max_len, "%s", full_path);
                    closedir(dir);
                    return true;
                }
            }
        }
        closedir(dir);
    }
    return false;
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
  size_t v_size  = obj->v_cnt * sizeof(sf_fvec3_t);
  size_t vt_size = obj->vt_cnt * sizeof(sf_fvec2_t);
  size_t vn_size = obj->vn_cnt * sizeof(sf_fvec3_t);
  size_t f_size  = obj->f_cnt * sizeof(sf_face_t);
  return v_size + vt_size + vn_size + f_size;
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
