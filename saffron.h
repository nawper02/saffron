/* saffron.h
 * Saffron is an stb-style graphics engine library.
 * Eventually it may become a game/simulation engine.
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
#define SF_ARENA_SIZE                 10485760
#define SF_MAX_OBJS                   10
#define SF_MAX_ENTITIES               100
#define SF_MAX_LIGHTS                 10
#define SF_MAX_TEXTURES               20
#define SF_MAX_CAMS                   5
#define SF_MAX_CB_PER_EVT             4
#define SF_MAX_UI_ELEMENTS            64
#define SF_MAX_FRAMES                 512
#define SF_MAX_SPRITES                20
#define SF_MAX_EMITRS                 10
#define SF_MAX_SPRITE_FRAMES          16
#define SF_LOG_INDENT                 "            "
#define SF_PI                         3.14159265359f
#define SF_NANOS_PER_SEC              1000000000ULL
#define SF_ASSET_PATH                 "/usr/local/share/saffron/sf_assets"

#define SF_LOG(ctx, level, fmt, ...)  sf_log_(ctx, level, __func__, fmt, ##__VA_ARGS__)
#define SF_ALIGN_SIZE(size)           (((size) + 7) & ~7)
#define SF_DEG2RAD(d)                 ((d) * (SF_PI / 180.0f))
#define SF_RAD2DEG(r)                 ((r) * (180.0f / SF_PI))
#define sf_get_obj(ctx, name)         sf_get_obj_(ctx, name, true)
#define sf_get_enti(ctx, name)        sf_get_enti_(ctx, name, true)
#define sf_get_cam(ctx, name)         sf_get_cam_(ctx, name, true)
#define sf_get_emitr(ctx, name)       sf_get_emitr_(ctx, name, true)
#define sf_get_sprite(ctx, name)      sf_get_sprite_(ctx, name, true)

#define SF_CLR_RED                    ((sf_pkd_clr_t)0xFFFF0000)
#define SF_CLR_GREEN                  ((sf_pkd_clr_t)0xFF00FF00)
#define SF_CLR_BLUE                   ((sf_pkd_clr_t)0xFF0000FF)
#define SF_CLR_BLACK                  ((sf_pkd_clr_t)0xFF000000)
#define SF_CLR_WHITE                  ((sf_pkd_clr_t)0xFFFFFFFF)

/* SF_TYPES */
typedef struct sf_ctx_t_ sf_ctx_t;

typedef enum {
  SF_RUN_STATE_INIT,
  SF_RUN_STATE_RUNNING,
  SF_RUN_STATE_ABORT,
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

typedef enum {
  SF_CONV_DEFAULT = 0,
  SF_CONV_NED,
  SF_CONV_FLU,
  SF_CONV_MAX
} sf_convention_t;

typedef struct sf_frame_t_ sf_frame_t;
struct sf_frame_t_ {
  sf_fvec3_t                       pos;
  sf_fvec3_t                       rot;
  sf_fvec3_t                       scale;

  sf_fmat4_t                       local_M;
  sf_fmat4_t                       global_M;
  bool                             is_dirty;
  bool                             is_root;

  sf_frame_t                      *parent;
  sf_frame_t                      *first_child;
  sf_frame_t                      *next_sibling;
};

typedef struct {
  int32_t                          id;
  const char                      *name;
  int                              w, h, buffer_size;
  sf_pkd_clr_t                    *buffer;
  float                           *z_buffer;
  float                            fov, near_plane, far_plane;
  bool                             is_proj_dirty;
  sf_fmat4_t                       V, P;
  sf_frame_t                      *frame;
} sf_cam_t;

typedef struct {
  sf_pkd_clr_t                    *px;
  int                              w;
  int                              h;
  int32_t                          id;
  const char                      *name;
} sf_tex_t;

typedef struct {
  int                              v;
  int                              vt;
  int                              vn;
} sf_vtx_idx_t;

typedef struct {
  sf_vtx_idx_t                     idx[3];
} sf_face_t;

typedef struct {
  sf_fvec3_t                      *v;
  sf_fvec2_t                      *vt;
  sf_fvec3_t                      *vn;
  sf_face_t                       *f;
  int32_t                          v_cnt;
  int32_t                          vt_cnt;
  int32_t                          vn_cnt;
  int32_t                          f_cnt;
  int32_t                          id;
  const char                      *name;
} sf_obj_t;

typedef struct {
  sf_obj_t                         obj;
  int32_t                          id;
  sf_tex_t                        *tex;
  const char                      *name;
  sf_frame_t                      *frame;
} sf_enti_t;

typedef enum {
  SF_LIGHT_DIR,
  SF_LIGHT_POINT
} sf_light_type_t;

typedef struct {
  sf_light_type_t                  type;
  sf_fvec3_t                       color;
  float                            intensity;
  sf_frame_t                      *frame;
} sf_light_t;

typedef struct {
  int32_t                          id;
  const char                      *name;
  sf_tex_t                        *frames[SF_MAX_SPRITE_FRAMES];
  int                              frame_count;
  float                            frame_duration;
  float                            base_scale;
} sf_sprite_t;

typedef struct {
  sf_fvec3_t                       pos;
  sf_fvec3_t                       vel;
  float                            life;
  float                            max_life;
  float                            anim_time;
  bool                             active;
} sf_particle_t;

typedef enum {
  SF_EMITR_DIR,
  SF_EMITR_OMNI,
  SF_EMITR_VOLUME
} sf_emitr_type_t;

typedef struct {
  int32_t                          id;
  const char                      *name;
  sf_emitr_type_t                  type;
  sf_sprite_t                     *sprite;
  sf_frame_t                      *frame;

  sf_particle_t                   *particles;
  int                              max_particles;
  float                            spawn_rate;
  float                            spawn_acc;
  float                            particle_life;
  float                            speed;

  sf_fvec3_t                       dir;
  float                            spread;
  sf_fvec3_t                       volume_size;
} sf_emitr_t;

typedef struct {
  size_t                           size;
  size_t                           offset;
  uint8_t                         *buffer;
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
  sf_event_cb                      cb;
  void                            *userdata;
} sf_callback_entry_t;

typedef struct {
  bool                             keys[SF_KEY_MAX];
  bool                             keys_prev[SF_KEY_MAX];
  int                              mouse_x, mouse_y;
  int                              mouse_dx, mouse_dy;
  bool                             mouse_btns[SF_MOUSE_MAX];
  bool                             mouse_btns_prev[SF_MOUSE_MAX];
} sf_input_state_t;

typedef void (*sf_ui_cb)(struct sf_ctx_t_ *ctx, void *userdata);

typedef enum {
  SF_UI_BUTTON,
  SF_UI_SLIDER,
  SF_UI_CHECKBOX
} sf_ui_type_t;

typedef struct {
  sf_pkd_clr_t                     color_base;
  sf_pkd_clr_t                     color_hover;
  sf_pkd_clr_t                     color_active;
  sf_pkd_clr_t                     color_text;
  bool                             draw_outline;
} sf_ui_style_t;

typedef struct {
  sf_ui_type_t                     type;
  sf_ui_style_t                    style;

  sf_ivec2_t                       v0, v1;
  bool                             is_hovered;
  bool                             is_pressed;
  bool                             is_visible;
  bool                             is_disabled;

  union {
    struct {
      const char                  *text;
      sf_ui_cb                     callback;
      void                        *userdata;
    } button;
    struct {
      float                        value;
      float                        min_val;
      float                        max_val;
      sf_ui_cb                     callback;
      void                        *userdata;
    } slider;
    struct {
      const char                  *text;
      bool                         is_checked;
      sf_ui_cb                     callback;
      void                        *userdata;
    } checkbox;

  };
} sf_ui_lmn_t;

typedef struct sf_ui_t_ {
  sf_ui_lmn_t                     *elements;
  int32_t                          count;
  sf_ui_style_t                    default_style;
} sf_ui_t;

struct sf_ctx_t_ {
  sf_run_state_t                   state;

  sf_arena_t                       arena;
  int                              arena_size;

  sf_frame_t                      *roots[SF_CONV_MAX];
  sf_frame_t                      *frames;
  int32_t                          frames_count;
  sf_obj_t                        *objs;
  int32_t                          obj_count;
  sf_enti_t                       *entities;
  int32_t                          enti_count;
  sf_tex_t                        *textures;
  int32_t                          tex_count;
  sf_cam_t                        *cameras;
  int32_t                          cam_count;
  sf_sprite_t                     *sprites;
  int32_t                          sprite_count;
  sf_emitr_t                      *emitrs;
  int32_t                          emitr_count;

  sf_light_t                      *lights;
  int32_t                          light_count;

  sf_ui_t                         *ui;

  sf_input_state_t                 input;
  sf_callback_entry_t              callbacks[SF_EVT_MAX][SF_MAX_CB_PER_EVT];

  sf_cam_t                         camera;

  float                            delta_time;
  float                            elapsed_time;
  float                            fps;
  uint32_t                         frame_count;
  uint64_t                         _start_ticks;
  uint64_t                         _last_ticks;

  sf_log_fn                        log_cb;
  void*                            log_user;
  sf_log_level_t                   log_min;
};

/* SF_CORE_FUNCTIONS */
void          sf_init              (sf_ctx_t *ctx, int w, int h);
void          sf_destroy           (sf_ctx_t *ctx);
bool          sf_running           (sf_ctx_t *ctx);
void          sf_stop              (sf_ctx_t *ctx);
void          sf_render_enti       (sf_ctx_t *ctx, sf_cam_t *cam, sf_enti_t *enti);
void          sf_render_ctx        (sf_ctx_t *ctx);
void          sf_render_cam        (sf_ctx_t *ctx, sf_cam_t *cam);
void          sf_render_emitrs     (sf_ctx_t *ctx, sf_cam_t *cam);
void          sf_update_emitrs     (sf_ctx_t *ctx);
void          sf_time_update       (sf_ctx_t *ctx);

/* SF_MEMORY_FUNCTIONS */
sf_arena_t    sf_arena_init        (sf_ctx_t *ctx, size_t size);
void*         sf_arena_alloc       (sf_ctx_t *ctx, sf_arena_t *arena, size_t size);
size_t        sf_arena_save        (sf_ctx_t *ctx, sf_arena_t *arena);
void          sf_arena_restore     (sf_ctx_t *ctx, sf_arena_t *arena, size_t mark);

/* SF_EVENT_FUNCTIONS */
void          sf_event_reg         (sf_ctx_t *ctx, sf_event_type_t type, sf_event_cb cb, void *userdata);
void          sf_event_trigger     (sf_ctx_t *ctx, const sf_event_t *event);
void          sf_input_cycle_state (sf_ctx_t *ctx);
void          sf_input_set_key     (sf_ctx_t *ctx, sf_key_t key, bool is_down);
void          sf_input_set_mouse_p (sf_ctx_t *ctx, int x, int y);
void          sf_input_set_mouse_b (sf_ctx_t *ctx, sf_mouse_btn_t btn, bool is_down);
bool          sf_key_down          (sf_ctx_t *ctx, sf_key_t key);
bool          sf_key_pressed       (sf_ctx_t *ctx, sf_key_t key);

/* SF_SCENE_FUNCTIONS */
sf_tex_t*     sf_load_texture_bmp  (sf_ctx_t *ctx, const char *filename, const char *texname);
sf_tex_t*     sf_get_texture_      (sf_ctx_t *ctx, const char *texname, bool should_log_failure);
sf_sprite_t*  sf_load_sprite       (sf_ctx_t *ctx, const char *spritename, float duration, float scale, int frame_count, ...);
sf_sprite_t*  sf_get_sprite_       (sf_ctx_t *ctx, const char *spritename, bool should_log_failure);
sf_emitr_t*   sf_add_emitr         (sf_ctx_t *ctx, const char *emitrname, sf_emitr_type_t type, sf_sprite_t *sprite, int max_p);
sf_emitr_t*   sf_get_emitr_        (sf_ctx_t *ctx, const char *emitrname, bool should_log_failure);
sf_obj_t*     sf_load_obj          (sf_ctx_t *ctx, const char *filename, const char *objname);
sf_obj_t*     sf_get_obj_          (sf_ctx_t *ctx, const char *objname, bool should_log_failure);
sf_enti_t*    sf_add_enti          (sf_ctx_t *ctx, sf_obj_t *obj, const char *entiname);
sf_enti_t*    sf_get_enti_         (sf_ctx_t *ctx, const char *entiname, bool should_log_failure);
sf_cam_t*     sf_add_cam           (sf_ctx_t *ctx, const char *camname, int w, int h, float fov);
sf_cam_t*     sf_get_cam_          (sf_ctx_t *ctx, const char *camname, bool should_log_failure);
void          sf_enti_set_pos      (sf_ctx_t *ctx, sf_enti_t *enti, float x, float y, float z);
void          sf_enti_move         (sf_ctx_t *ctx, sf_enti_t *enti, float dx, float dy, float dz);
void          sf_enti_set_rot      (sf_ctx_t *ctx, sf_enti_t *enti, float rx, float ry, float rz);
void          sf_enti_rotate       (sf_ctx_t *ctx, sf_enti_t *enti, float drx, float dry, float drz);
void          sf_enti_set_scale    (sf_ctx_t *ctx, sf_enti_t *enti, float sx, float sy, float sz);
void          sf_enti_set_tex      (sf_ctx_t *ctx, const char *entiname, const char *texname);
sf_light_t*   sf_add_light         (sf_ctx_t *ctx, sf_light_type_t type, sf_fvec3_t color, float intensity);
void          sf_load_world        (sf_ctx_t *ctx, const char *filename, const char *worldname);
void          sf_camera_set_psp    (sf_ctx_t *ctx, sf_cam_t *cam, float fov, float near_plane, float far_plane);
void          sf_camera_set_pos    (sf_ctx_t *ctx, sf_cam_t *cam, float x, float y, float z);
void          sf_camera_move_loc   (sf_ctx_t *ctx, sf_cam_t *cam, float fwd, float right, float up);
void          sf_camera_look_at    (sf_ctx_t *ctx, sf_cam_t *cam, sf_fvec3_t target);
void          sf_camera_add_yp     (sf_ctx_t *ctx, sf_cam_t *cam, float yaw_offset, float pitch_offset);

/* SF_FRAME_FUNCTIONS */
sf_frame_t*   sf_get_root          (sf_ctx_t *ctx, sf_convention_t conv);
sf_frame_t*   sf_add_frame         (sf_ctx_t *ctx, sf_frame_t *parent);
void          sf_update_frames     (sf_ctx_t *ctx);
void          sf_frame_look_at     (sf_frame_t *f, sf_fvec3_t target);
void          sf_frame_set_parent  (sf_frame_t *child, sf_frame_t *new_parent);

/* SF_DRAWING_FUNCTIONS */
void          sf_fill              (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c);
void          sf_pixel             (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0);
void          sf_pixel_depth       (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, float z);
void          sf_line              (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1);
void          sf_rect              (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1);
void          sf_tri               (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, bool use_depth);
void          sf_tri_tex           (sf_ctx_t *ctx, sf_cam_t *cam, sf_tex_t *tex, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, sf_fvec3_t uvz0, sf_fvec3_t uvz1, sf_fvec3_t uvz2, sf_fvec3_t l_int);
void          sf_put_text          (sf_ctx_t *ctx, sf_cam_t *cam, const char *text, sf_ivec2_t p, sf_pkd_clr_t c, int scale);
void          sf_clear_depth       (sf_ctx_t *ctx, sf_cam_t *cam);
void          sf_draw_cam_pip      (sf_ctx_t *ctx, sf_cam_t *dest, sf_cam_t *src, sf_ivec2_t pos);
void          sf_draw_debug_ovrlay (sf_ctx_t *ctx, sf_cam_t *cam);
void          sf_draw_debug_axes   (sf_ctx_t *ctx, sf_cam_t *cam);
void          sf_draw_debug_frames (sf_ctx_t *ctx, sf_cam_t *cam, float axis_size);
void          sf_draw_debug_lights (sf_ctx_t *ctx, sf_cam_t *cam, float size);
void          sf_draw_debug_cams   (sf_ctx_t *ctx, sf_cam_t *view_cam, float ray_len);
void          sf_draw_sprite       (sf_ctx_t *ctx, sf_cam_t *cam, sf_sprite_t *spr, sf_fvec3_t pos_w, float anim_time, float scale_mult);

/* SF_UI_FUNCTIONS */
sf_ui_t*      sf_create_ui         (sf_ctx_t *ctx);
void          sf_update_ui         (sf_ctx_t *ctx, sf_ui_t *ui);
void          sf_render_ui         (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_t *ui);
sf_ui_lmn_t*  sf_add_button        (sf_ctx_t *ctx, const char *text, sf_ivec2_t v0, sf_ivec2_t v1, sf_ui_cb cb, void *userdata);
sf_ui_lmn_t*  sf_add_slider        (sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, float min_val, float max_val, float init_val, sf_ui_cb cb, void *userdata);
sf_ui_lmn_t*  sf_add_checkbox      (sf_ctx_t *ctx, const char *text, sf_ivec2_t v0, sf_ivec2_t v1, bool init_state, sf_ui_cb cb, void *userdata);
void          draw_button          (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
void          draw_slider          (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
void          draw_checkbox        (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);

/* SF_LOG_FUNCTIONS */
void          sf_log_              (sf_ctx_t *ctx, sf_log_level_t level, const char* func, const char* fmt, ...);
void          sf_set_logger        (sf_ctx_t *ctx, sf_log_fn log_cb, void* userdata);
void          sf_logger_console    (const char* message, void* userdata);

/* SF_IMPLEMENTATION_HELPERS */
uint32_t       _sf_vec_to_index    (sf_ctx_t *ctx, sf_cam_t *cam, sf_ivec2_t v);
void           _sf_swap_svec2      (sf_ivec2_t *v0, sf_ivec2_t *v1);
void           _sf_swap_fvec3      (sf_fvec3_t *v0, sf_fvec3_t *v1);
void           _sf_interp_fvec3    (sf_fvec3_t  v0, sf_fvec3_t v1, int steps, sf_fvec3_t *out);
void           _sf_interp_x        (sf_ivec2_t  v0, sf_ivec2_t v1, int *xs);
void           _sf_interp_y        (sf_ivec2_t  v0, sf_ivec2_t v1, int *ys);
void           _sf_interp_f        (float v0, float v1, int steps, float *out);
float          _sf_lerp_f          (float a, float b, float t);
sf_fvec3_t     _sf_lerp_fvec3      (sf_fvec3_t a, sf_fvec3_t b, float t);
sf_fvec3_t     _sf_intersect_near  (sf_fvec3_t v0, sf_fvec3_t v1, float near);
sf_fvec3_t     _sf_project_vertex  (sf_ctx_t *ctx, sf_cam_t *cam, sf_fvec3_t v, sf_fmat4_t P);
const char*    _sf_log_lvl_to_str  (sf_log_level_t level);
bool           _sf_resolve_asset   (const char* filename, char* out_path, size_t max_len);
uint64_t       _sf_get_ticks       (void);
sf_pkd_clr_t   _sf_pack_color      (sf_unpkd_clr_t);
sf_unpkd_clr_t _sf_unpack_color    (sf_pkd_clr_t);
size_t         _sf_obj_memusg      (sf_obj_t *obj);
void           _sf_set_up_frames   (sf_ctx_t *ctx);
void           _sf_calc_frame_tree (sf_frame_t *f, sf_fmat4_t parent_global_M, bool force_dirty);

/* SF_LA_FUNCTIONS */
sf_fmat4_t     sf_fmat4_mul_fmat4  (sf_fmat4_t m0, sf_fmat4_t m1);
sf_fvec3_t     sf_fmat4_mul_vec3   (sf_fmat4_t m, sf_fvec3_t v);
sf_fvec3_t     sf_fvec3_sub        (sf_fvec3_t v0, sf_fvec3_t v1);
sf_fvec3_t     sf_fvec3_add        (sf_fvec3_t v0, sf_fvec3_t v1);
sf_fvec3_t     sf_fvec3_norm       (sf_fvec3_t v);
sf_fvec3_t     sf_fvec3_cross      (sf_fvec3_t v0, sf_fvec3_t v1);
float          sf_fvec3_dot        (sf_fvec3_t v0, sf_fvec3_t v1);
sf_fmat4_t     sf_make_tsl_fmat4   (float x, float y, float z);
sf_fmat4_t     sf_make_rot_fmat4   (sf_fvec3_t angles);
sf_fmat4_t     sf_make_psp_fmat4   (float fov_deg, float aspect, float near, float far);
sf_fmat4_t     sf_make_idn_fmat4   (void);
sf_fmat4_t     sf_make_view_fmat4  (sf_fvec3_t eye, sf_fvec3_t target, sf_fvec3_t up);
sf_fmat4_t     sf_make_scale_fmat4 (sf_fvec3_t scale);

/* SF_GAMMA_LUT */
static const uint8_t               _sf_gamma_lut[256];
static const uint8_t               _sf_degamma_lut[256];

/* SF_FONT_DATA */
static const uint8_t               _sf_font_8x8[];

#ifdef __cplusplus
}
#endif
#endif /* SAFFRON_H */

/* SF_IMPLEMENTATION */
#ifdef SAFFRON_IMPLEMENTATION

/* SF_CORE_FUNCTIONS */
void sf_init(sf_ctx_t *ctx, int w, int h) {
  memset(ctx, 0, sizeof(sf_ctx_t));
  ctx->state                       = SF_RUN_STATE_RUNNING;
  ctx->camera.w                    = w;
  ctx->camera.h                    = h;
  ctx->camera.buffer_size          = w * h;
  ctx->camera.buffer               = (sf_pkd_clr_t*) malloc(w*h*sizeof(sf_pkd_clr_t));
  ctx->camera.z_buffer             = (float*)        malloc(w*h*sizeof(float));
  ctx->camera.fov                  = 60.0f;
  ctx->camera.near_plane           = 0.1f;
  ctx->camera.far_plane            = 100.0f;
  ctx->camera.is_proj_dirty        = true;
  ctx->arena                       = sf_arena_init(ctx, SF_ARENA_SIZE);
  ctx->log_cb                      = sf_logger_console;
  ctx->log_user                    = NULL;
  ctx->log_min                     = SF_LOG_INFO;
  ctx->objs                        = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_OBJS     * sizeof(sf_obj_t));
  ctx->entities                    = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_ENTITIES * sizeof(sf_enti_t));
  ctx->lights                      = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_LIGHTS   * sizeof(sf_light_t));
  ctx->textures                    = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_TEXTURES * sizeof(sf_tex_t));
  ctx->cameras                     = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_CAMS     * sizeof(sf_cam_t));
  ctx->frames                      = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_FRAMES   * sizeof(sf_frame_t));
  ctx->sprites                     = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_SPRITES  * sizeof(sf_sprite_t));
  ctx->emitrs                      = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_EMITRS   * sizeof(sf_emitr_t));
  ctx->enti_count                  = 0;
  ctx->light_count                 = 0;
  ctx->tex_count                   = 0;
  ctx->cam_count                   = 0;
  ctx->frames_count                = 0;
  ctx->sprite_count                = 0;
  ctx->emitr_count                 = 0;
  ctx->_start_ticks                = _sf_get_ticks();
  ctx->_last_ticks                 = ctx->_start_ticks;
  ctx->delta_time                  = 0.0f;
  ctx->elapsed_time                = 0.0f;
  ctx->fps                         = 0.0f;
  ctx->frame_count                 = 0;
  ctx->ui                          = sf_create_ui(ctx);
  _sf_set_up_frames(ctx);
  ctx->camera.frame                = sf_add_frame(ctx, NULL);

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "buffer : %dx%d\n"
              SF_LOG_INDENT "memory : %d\n"
              SF_LOG_INDENT "mxobjs : %d\n" 
              SF_LOG_INDENT "mxents : %d\n", 
              ctx->camera.w, ctx->camera.h, SF_ARENA_SIZE, SF_MAX_OBJS, SF_MAX_ENTITIES);
}

void sf_destroy(sf_ctx_t *ctx) {

  float avg_fps = (ctx->elapsed_time > 0.0f) ? ((float)ctx->frame_count / ctx->elapsed_time) : 0.0f;
  float mem_pct = ((float)ctx->arena.offset / (float)ctx->arena.size) * 100.0f;
  int ui_count  = (ctx->ui) ? ctx->ui->count : 0;

  SF_LOG(ctx, SF_LOG_INFO, 
              SF_LOG_INDENT "time   : %.2fs\n"
              SF_LOG_INDENT "frames : %u (avg %.1f fps)\n"
              SF_LOG_INDENT "memory : %zu / %zu bytes (%.1f%%)\n"
              SF_LOG_INDENT "assets : %d objs, %d texs\n"
              SF_LOG_INDENT "active : %d entis, %d ui_elems\n"
              SF_LOG_INDENT "thank you .....\n",
              ctx->elapsed_time,
              ctx->frame_count, avg_fps,
              ctx->arena.offset, ctx->arena.size, mem_pct,
              ctx->obj_count, ctx->tex_count,
              ctx->enti_count, ui_count);

  for (int i = 0; i < ctx->cam_count; ++i) {
    free(ctx->cameras[i].buffer);
    free(ctx->cameras[i].z_buffer);
  }
  free(ctx->camera.buffer);
  free(ctx->camera.z_buffer);
  free(ctx->arena.buffer);

  ctx->state                    = SF_RUN_STATE_STOPPED;
  ctx->arena.offset             = 0;
  ctx->camera.buffer_size       = 0;
  ctx->camera.w                 = 0;
  ctx->camera.h                 = 0;
  ctx->enti_count               = 0;
  ctx->obj_count                = 0;
  ctx->tex_count                = 0;
  ctx->light_count              = 0;
  ctx->cam_count                = 0;
  ctx->sprite_count             = 0;
  ctx->emitr_count              = 0;
}

bool sf_running(sf_ctx_t *ctx) {
  return (ctx->state == SF_RUN_STATE_RUNNING);
}

void sf_stop(sf_ctx_t *ctx) {
  ctx->state = SF_RUN_STATE_STOPPED;
}

void sf_render_enti(sf_ctx_t *ctx, sf_cam_t *cam, sf_enti_t *enti) {
  if (!enti || !enti->frame) return;

  size_t mark = sf_arena_save(ctx, &ctx->arena);
  sf_fmat4_t M = enti->frame->global_M;
  sf_fmat4_t V = cam->V;
  sf_fmat4_t P = cam->P;
  sf_fmat4_t MV = sf_fmat4_mul_fmat4(M, V);
  float near = 0.1f;
  sf_fvec3_t* vv = sf_arena_alloc(ctx, &ctx->arena, enti->obj.v_cnt * sizeof(sf_fvec3_t));
  if (!vv) return;

  for (int i = 0; i < enti->obj.v_cnt; i++) {
    vv[i] = sf_fmat4_mul_vec3(MV, enti->obj.v[i]);
  }

  struct { sf_fvec3_t pos_v, dir_v, color; float intensity; sf_light_type_t type; } lv[SF_MAX_LIGHTS];
  int lv_cnt = 0;
  for (int l = 0; l < ctx->light_count && l < SF_MAX_LIGHTS; l++) {
    sf_light_t *light = &ctx->lights[l];
    if (!light->frame) continue;
    sf_fmat4_t lM = light->frame->global_M;
    sf_fvec3_t lp_w = {lM.m[3][0], lM.m[3][1], lM.m[3][2]};
    lv[lv_cnt].pos_v = sf_fmat4_mul_vec3(V, lp_w);
    lv[lv_cnt].type = light->type;
    lv[lv_cnt].intensity = light->intensity;
    lv[lv_cnt].color = light->color;
    if (light->type == SF_LIGHT_DIR) {
      sf_fvec3_t dir_w = {-lM.m[2][0], -lM.m[2][1], -lM.m[2][2]};
      sf_fvec3_t end_v = sf_fmat4_mul_vec3(V, sf_fvec3_add(lp_w, dir_w));
      lv[lv_cnt].dir_v = sf_fvec3_norm(sf_fvec3_sub(end_v, lv[lv_cnt].pos_v));
    }
    lv_cnt++;
  }

  for (int i = 0; i < enti->obj.f_cnt; i++) {
    sf_face_t face = enti->obj.f[i];
    sf_fvec3_t v_view[3] = { vv[face.idx[0].v], vv[face.idx[1].v], vv[face.idx[2].v] };
    sf_fvec3_t a_v = sf_fvec3_sub(v_view[1], v_view[0]);
    sf_fvec3_t b_v = sf_fvec3_sub(v_view[2], v_view[0]);
    sf_fvec3_t n_v = sf_fvec3_cross(a_v, b_v);

    if (sf_fvec3_dot(n_v, v_view[0]) >= 0) continue;

    sf_fvec3_t n = sf_fvec3_norm(n_v);
    sf_fvec3_t centroid_v = {
      (v_view[0].x + v_view[1].x + v_view[2].x) * 0.333333f,
      (v_view[0].y + v_view[1].y + v_view[2].y) * 0.333333f,
      (v_view[0].z + v_view[1].z + v_view[2].z) * 0.333333f
    };

    sf_fvec3_t l_int = {0.1f, 0.1f, 0.1f};

    for (int l = 0; l < lv_cnt; l++) {
      sf_fvec3_t light_dir;
      float atten = 1.0f;

      if (lv[l].type == SF_LIGHT_DIR) {
        light_dir = lv[l].dir_v;
      } else {
        sf_fvec3_t diff = sf_fvec3_sub(lv[l].pos_v, centroid_v);
        float dist = sqrtf(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
        light_dir = sf_fvec3_norm(diff);
        atten = 1.0f / (1.0f + 0.09f * dist + 0.032f * (dist * dist));
      }

      float diff_factor = sf_fvec3_dot(n, light_dir);
      if (diff_factor > 0.0f) {
        l_int.x += lv[l].color.x * lv[l].intensity * diff_factor * atten;
        l_int.y += lv[l].color.y * lv[l].intensity * diff_factor * atten;
        l_int.z += lv[l].color.z * lv[l].intensity * diff_factor * atten;
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
        sf_tri_tex(ctx, cam, enti->tex, _sf_project_vertex(ctx, cam, in[0], P), _sf_project_vertex(ctx, cam, in[1], P), _sf_project_vertex(ctx, cam, in[2], P), in_uvz[0], in_uvz[1], in_uvz[2], l_int);
      } else if (inc == 1) {
        float t1 = ((-near) - in[0].z) / (out[0].z - in[0].z);
        float t2 = ((-near) - in[0].z) / (out[1].z - in[0].z);
        sf_fvec3_t v1 = { in[0].x + (out[0].x - in[0].x) * t1, in[0].y + (out[0].y - in[0].y) * t1, -near };
        sf_fvec3_t v2 = { in[0].x + (out[1].x - in[0].x) * t2, in[0].y + (out[1].y - in[0].y) * t2, -near };
        sf_fvec3_t uvz1 = { in_uvz[0].x + (out_uvz[0].x - in_uvz[0].x) * t1, in_uvz[0].y + (out_uvz[0].y - in_uvz[0].y) * t1, in_uvz[0].z + (out_uvz[0].z - in_uvz[0].z) * t1 };
        sf_fvec3_t uvz2 = { in_uvz[0].x + (out_uvz[1].x - in_uvz[0].x) * t2, in_uvz[0].y + (out_uvz[1].y - in_uvz[0].y) * t2, in_uvz[0].z + (out_uvz[1].z - in_uvz[0].z) * t2 };
        sf_tri_tex(ctx, cam, enti->tex, _sf_project_vertex(ctx, cam, in[0], P), _sf_project_vertex(ctx, cam, v1, P), _sf_project_vertex(ctx, cam, v2, P), in_uvz[0], uvz1, uvz2, l_int);
      } else if (inc == 2) {
        float t1 = ((-near) - in[0].z) / (out[0].z - in[0].z);
        float t2 = ((-near) - in[1].z) / (out[0].z - in[1].z);
        sf_fvec3_t v1 = { in[0].x + (out[0].x - in[0].x) * t1, in[0].y + (out[0].y - in[0].y) * t1, -near };
        sf_fvec3_t v2 = { in[1].x + (out[0].x - in[1].x) * t2, in[1].y + (out[0].y - in[1].y) * t2, -near };
        sf_fvec3_t uvz1 = { in_uvz[0].x + (out_uvz[0].x - in_uvz[0].x) * t1, in_uvz[0].y + (out_uvz[0].y - in_uvz[0].y) * t1, in_uvz[0].z + (out_uvz[0].z - in_uvz[0].z) * t1 };
        sf_fvec3_t uvz2 = { in_uvz[1].x + (out_uvz[0].x - in_uvz[1].x) * t2, in_uvz[1].y + (out_uvz[0].y - in_uvz[1].y) * t2, in_uvz[1].z + (out_uvz[0].z - in_uvz[1].z) * t2 };
        sf_tri_tex(ctx, cam, enti->tex, _sf_project_vertex(ctx, cam, in[0], P), _sf_project_vertex(ctx, cam, in[1], P), _sf_project_vertex(ctx, cam, v1, P), in_uvz[0], in_uvz[1], uvz1, l_int);
        sf_tri_tex(ctx, cam, enti->tex, _sf_project_vertex(ctx, cam, in[1], P), _sf_project_vertex(ctx, cam, v1, P), _sf_project_vertex(ctx, cam, v2, P), in_uvz[1], uvz1, uvz2, l_int);
      }
    } else {
      sf_pkd_clr_t shaded_color = _sf_pack_color((sf_unpkd_clr_t){(uint8_t)(l_int.x * 255), (uint8_t)(l_int.y * 255), (uint8_t)(l_int.z * 255), 255});
      if (inc == 3) {
        sf_tri(ctx, cam, shaded_color, _sf_project_vertex(ctx, cam, v_view[0], P), _sf_project_vertex(ctx, cam, v_view[1], P), _sf_project_vertex(ctx, cam, v_view[2], P), true);
      } else if (inc == 1) {
        sf_fvec3_t v1 = _sf_intersect_near(in[0], out[0], -near);
        sf_fvec3_t v2 = _sf_intersect_near(in[0], out[1], -near);
        sf_tri(ctx, cam, shaded_color, _sf_project_vertex(ctx, cam, in[0], P), _sf_project_vertex(ctx, cam, v1, P), _sf_project_vertex(ctx, cam, v2, P), true);
      } else if (inc == 2) {
        sf_fvec3_t v1 = _sf_intersect_near(in[0], out[0], -near);
        sf_fvec3_t v2 = _sf_intersect_near(in[1], out[0], -near);
        sf_tri(ctx, cam, shaded_color, _sf_project_vertex(ctx, cam, in[0], P), _sf_project_vertex(ctx, cam, in[1], P), _sf_project_vertex(ctx, cam, v1, P), true);
        sf_tri(ctx, cam, shaded_color, _sf_project_vertex(ctx, cam, in[1], P), _sf_project_vertex(ctx, cam, v1, P),    _sf_project_vertex(ctx, cam, v2, P), true);
      }
    }
  }
  sf_arena_restore(ctx, &ctx->arena, mark);
}

void sf_render_ctx(sf_ctx_t *ctx) {
  sf_update_frames(ctx);
  sf_update_emitrs(ctx);
  sf_render_cam(ctx, &ctx->camera);
  for (int i = 0; i < ctx->cam_count; ++i) {
    sf_render_cam(ctx, &ctx->cameras[i]);
  }
}

void sf_render_cam(sf_ctx_t *ctx, sf_cam_t *cam) {
  sf_event_t ev_start;
  ev_start.type = SF_EVT_RENDER_START;
  sf_event_trigger(ctx, &ev_start);

  if (cam->is_proj_dirty) {
    float aspect = (float)cam->w / (float)cam->h;
    cam->P = sf_make_psp_fmat4(cam->fov, aspect, cam->near_plane, cam->far_plane);
    cam->is_proj_dirty = false;
  }

  if (cam->frame) {
    sf_fmat4_t gM = cam->frame->global_M;

    sf_fvec3_t eye    = { gM.m[3][0],  gM.m[3][1],  gM.m[3][2] };
    sf_fvec3_t fwd    = {-gM.m[2][0], -gM.m[2][1], -gM.m[2][2] };
    sf_fvec3_t up     = { gM.m[1][0],  gM.m[1][1],  gM.m[1][2] };

    sf_fvec3_t target = sf_fvec3_add(eye, fwd);
    cam->V = sf_make_view_fmat4(eye, target, up);
  }

  sf_fill(ctx, cam, SF_CLR_BLACK);
  sf_clear_depth(ctx, cam); 

  for (int i = 0; i < ctx->enti_count; i++) {
    sf_render_enti(ctx, cam, &ctx->entities[i]);
  }

  sf_render_emitrs(ctx, cam);

  sf_event_t ev_end;
  ev_end.type = SF_EVT_RENDER_END;
  sf_event_trigger(ctx, &ev_end);
}

void sf_render_emitrs(sf_ctx_t *ctx, sf_cam_t *cam) {
  for (int i = 0; i < ctx->emitr_count; i++) {
    sf_emitr_t *em = &ctx->emitrs[i];
    for (int p = 0; p < em->max_particles; p++) {
      if (em->particles[p].active) {
        float scale_mult = em->particles[p].life / em->particles[p].max_life;
        sf_draw_sprite(ctx, cam, em->sprite, em->particles[p].pos, em->particles[p].anim_time, scale_mult);
      }
    }
  }
}

void sf_update_emitrs(sf_ctx_t *ctx) {
  float dt = ctx->delta_time;
  for (int i = 0; i < ctx->emitr_count; i++) {
    sf_emitr_t *em = &ctx->emitrs[i];
    if (!em->frame) continue;

    sf_fmat4_t M = em->frame->global_M;
    sf_fvec3_t pos_w = { M.m[3][0], M.m[3][1], M.m[3][2] };

    for (int p = 0; p < em->max_particles; p++) {
      if (em->particles[p].active) {
        em->particles[p].life -= dt;
        em->particles[p].anim_time += dt;
        if (em->particles[p].life <= 0.0f) {
          em->particles[p].active = false;
        } else if (em->type != SF_EMITR_VOLUME) {
          em->particles[p].pos.x += em->particles[p].vel.x * dt;
          em->particles[p].pos.y += em->particles[p].vel.y * dt;
          em->particles[p].pos.z += em->particles[p].vel.z * dt;
        }
      }
    }

    em->spawn_acc += em->spawn_rate * dt;
    while (em->spawn_acc >= 1.0f) {
      em->spawn_acc -= 1.0f;
      for (int p = 0; p < em->max_particles; p++) {
        if (!em->particles[p].active) {
          sf_particle_t *part = &em->particles[p];
          part->active = true;
          part->life = em->particle_life * (0.8f + 0.4f * ((float)rand() / (float)RAND_MAX)); 
          part->max_life = part->life;
          part->anim_time = 0.0f;

          if (em->type == SF_EMITR_VOLUME) {
            part->pos = sf_fvec3_add(pos_w, (sf_fvec3_t){
              (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * em->volume_size.x * 0.5f,
              (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * em->volume_size.y * 0.5f,
              (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * em->volume_size.z * 0.5f
            });
            part->vel = (sf_fvec3_t){0,0,0};
          }
          else if (em->type == SF_EMITR_OMNI) {
            part->pos = pos_w;
            sf_fvec3_t dir = sf_fvec3_norm((sf_fvec3_t){ 
              ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f, 
              ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f, 
              ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f 
            });
            part->vel = (sf_fvec3_t){dir.x * em->speed, dir.y * em->speed, dir.z * em->speed};
          } 
          else if (em->type == SF_EMITR_DIR) {
            part->pos = pos_w;
            sf_fvec3_t fuzzy_dir = sf_fvec3_norm((sf_fvec3_t){
              em->dir.x + (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * em->spread,
              em->dir.y + (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * em->spread,
              em->dir.z + (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * em->spread
            });
            part->vel = (sf_fvec3_t){fuzzy_dir.x * em->speed, fuzzy_dir.y * em->speed, fuzzy_dir.z * em->speed};
          }
          break; 
        }
      }
    }
  }
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

/* SF_MEMORY_FUNCTIONS */
sf_arena_t sf_arena_init(sf_ctx_t *ctx, size_t size) {
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

size_t sf_arena_save(sf_ctx_t *ctx, sf_arena_t *arena) {
  return arena->offset;
}

void sf_arena_restore(sf_ctx_t *ctx, sf_arena_t *arena, size_t mark) {
  arena->offset = mark;
}

/* SF_EVENT_FUNCTIONS */
void sf_event_reg(sf_ctx_t *ctx, sf_event_type_t type, sf_event_cb cb, void *userdata) {
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

void sf_event_trigger(sf_ctx_t *ctx, const sf_event_t *event) {
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
    sf_event_trigger(ctx, &ev);
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
    sf_event_trigger(ctx, &ev);
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
    sf_event_trigger(ctx, &ev);
  }
}

bool sf_key_down(sf_ctx_t *ctx, sf_key_t key) {
  return ctx->input.keys[key];
}

bool sf_key_pressed(sf_ctx_t *ctx, sf_key_t key) {
  return ctx->input.keys[key] && !ctx->input.keys_prev[key];
}

/* SF_SCENE_FUNCTIONS */
sf_tex_t* sf_load_texture_bmp(sf_ctx_t *ctx, const char *filename, const char *texname) {
  if (ctx->tex_count >= SF_MAX_TEXTURES) return NULL;
  if (sf_get_texture_(ctx, texname, false) != NULL) return NULL;
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
  tex->px = sf_arena_alloc(ctx, &ctx->arena, w * h_abs * sizeof(sf_pkd_clr_t));
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
      if (bgr[2] == 255 && bgr[1] == 0 && bgr[0] == 255) {
        tex->px[dest_y * w + x] = 0x00000000;
      } else {
        uint8_t lr = (uint8_t)(powf(bgr[2] / 255.0f, 2.2f) * 255.0f + 0.5f);
        uint8_t lg = (uint8_t)(powf(bgr[1] / 255.0f, 2.2f) * 255.0f + 0.5f);
        uint8_t lb = (uint8_t)(powf(bgr[0] / 255.0f, 2.2f) * 255.0f + 0.5f);
        tex->px[dest_y * w + x] = (0xFFu << 24) | ((uint32_t)lr << 16) | ((uint32_t)lg << 8) | lb;
      }
    }
    fseek(file, padding, SEEK_CUR);
  }
  fclose(file);
  SF_LOG(ctx, SF_LOG_INFO, 
              SF_LOG_INDENT "file   : %s\n"
              SF_LOG_INDENT "name   : %s\n"
              SF_LOG_INDENT "id     : %d\n"
              SF_LOG_INDENT "w      : %d\n"
              SF_LOG_INDENT "h      : %d\n",
              filename, texname, tex->id, w, h_abs);
  return tex;
}

sf_tex_t* sf_get_texture_(sf_ctx_t *ctx, const char *texname, bool should_log_failure) {
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

sf_sprite_t* sf_load_sprite(sf_ctx_t *ctx, const char *spritename, float duration, float scale, int frame_count, ...) {
  char auto_name[32];
  if (spritename == NULL) {
    snprintf(auto_name, sizeof(auto_name), "spr_%d", ctx->sprite_count);
    spritename = auto_name;
  }
  if (NULL != sf_get_sprite_(ctx, spritename, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to load sprite, name in use\n");
    return NULL;
  }
  if (ctx->sprite_count >= SF_MAX_SPRITES || frame_count > SF_MAX_SPRITE_FRAMES) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to load sprite, max reached or too many frames\n");
    return NULL;
  }

  sf_sprite_t *spr = &ctx->sprites[ctx->sprite_count++];
  spr->id = ctx->sprite_count - 1;
  spr->frame_count = frame_count;
  spr->frame_duration = duration;
  spr->base_scale = scale;

  size_t name_len = strlen(spritename) + 1;
  spr->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (spr->name) memcpy((void*)spr->name, spritename, name_len);

  va_list args;
  va_start(args, frame_count);
  for (int i = 0; i < frame_count; i++) {
    const char* tex_name = va_arg(args, const char*);
    spr->frames[i] = sf_get_texture_(ctx, tex_name, true);
  }
  va_end(args);

  return spr;
}

sf_sprite_t* sf_get_sprite_(sf_ctx_t *ctx, const char *spritename, bool should_log_failure) {
  for (int32_t i = 0; i < ctx->sprite_count; ++i) {
    if (ctx->sprites[i].name && strcmp(ctx->sprites[i].name, spritename) == 0) {
      return &ctx->sprites[i];
    }
  }
  if (should_log_failure) SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "sprite '%s' not found\n", spritename);
  return NULL;
}

sf_emitr_t* sf_add_emitr(sf_ctx_t *ctx, const char *emitrname, sf_emitr_type_t type, sf_sprite_t *sprite, int max_p) {
  char auto_name[32];
  if (emitrname == NULL) {
    snprintf(auto_name, sizeof(auto_name), "emitr_%d", ctx->emitr_count);
    emitrname = auto_name;
  }
  if (NULL != sf_get_emitr_(ctx, emitrname, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add emitter, name in use\n");
    return NULL;
  }
  if (ctx->emitr_count >= SF_MAX_EMITRS) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add emitter, max reached\n");
    return NULL;
  }

  sf_emitr_t *em = &ctx->emitrs[ctx->emitr_count++];
  memset(em, 0, sizeof(sf_emitr_t));
  em->id = ctx->emitr_count - 1;
  em->type = type;
  em->sprite = sprite;
  em->max_particles = max_p;
  em->particles = sf_arena_alloc(ctx, &ctx->arena, max_p * sizeof(sf_particle_t));
  em->frame = sf_add_frame(ctx, NULL);

  size_t name_len = strlen(emitrname) + 1;
  em->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (em->name) memcpy((void*)em->name, emitrname, name_len);

  em->spawn_rate = 10.0f; 
  em->particle_life = 2.0f;
  em->speed = 5.0f;
  em->dir = (sf_fvec3_t){0, 1, 0};
  em->spread = 0.5f;
  em->volume_size = (sf_fvec3_t){5, 5, 5};

  SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "emitr  : %s (id %d)\n", em->name, em->id);
  return em;
}

sf_emitr_t* sf_get_emitr_(sf_ctx_t *ctx, const char *emitrname, bool should_log_failure) {
  for (int32_t i = 0; i < ctx->emitr_count; ++i) {
    if (ctx->emitrs[i].name && strcmp(ctx->emitrs[i].name, emitrname) == 0) {
      return &ctx->emitrs[i];
    }
  }
  if (should_log_failure) SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "emitter '%s' not found\n", emitrname);
  return NULL;
}

sf_obj_t* sf_load_obj(sf_ctx_t *ctx, const char *filename, const char *objname) {
  char auto_name[32];
  if (objname == NULL) {
    snprintf(auto_name, sizeof(auto_name), "obj_%d", ctx->obj_count);
    objname = auto_name;
  }

  if (NULL != sf_get_obj_(ctx, objname, false)) {
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
              SF_LOG_INDENT "mem    : %.2f\n",
              filename, objname, obj->id, v_cnt, vt_cnt, vn_cnt, f_cnt, 
              _sf_obj_memusg(obj), 
              ((float)ctx->arena.offset / (float)ctx->arena.size) * 100.0f);

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

sf_obj_t* sf_get_obj_(sf_ctx_t *ctx, const char *objname, bool should_log_failure) {
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
  if (NULL != sf_get_enti_(ctx, entiname, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add entity, entity name in use\n");
    return NULL;
  }
  if (ctx->enti_count >= SF_MAX_ENTITIES) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add entity, max entities reached\n");
    return NULL;
  }

  sf_enti_t *enti = &ctx->entities[ctx->enti_count++];
  enti->obj       = *obj;
  enti->id        = ctx->enti_count - 1;
  enti->tex       = NULL;
  enti->frame     = sf_add_frame(ctx, NULL);

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

sf_enti_t* sf_get_enti_(sf_ctx_t *ctx, const char *entiname, bool should_log_failure) {
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

sf_cam_t* sf_add_cam(sf_ctx_t *ctx, const char *camname, int w, int h, float fov) {
  char auto_name[32];
  if (NULL == camname) {
    snprintf(auto_name, sizeof(auto_name), "cam_%d", ctx->cam_count);
    camname = auto_name;
  }
  if (NULL != sf_get_cam_(ctx, camname, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add camera, name in use\n");
    return NULL;
  }
  if (ctx->cam_count >= SF_MAX_CAMS) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "max cameras (%d) reached\n", SF_MAX_CAMS);
    return NULL;
  }

  sf_cam_t *cam = &ctx->cameras[ctx->cam_count++];
  memset(cam, 0, sizeof(sf_cam_t));
  cam->w                 = w;
  cam->h                 = h;
  cam->buffer_size       = w * h;
  cam->buffer            = (sf_pkd_clr_t*) malloc(w * h * sizeof(sf_pkd_clr_t));
  cam->z_buffer          = (float*)        malloc(w * h * sizeof(float));
  cam->fov               = fov;
  cam->near_plane        = 0.1f;
  cam->far_plane         = 100.0f;
  cam->is_proj_dirty     = true;
  cam->id                = ctx->cam_count - 1;
  cam->frame             = sf_add_frame(ctx, NULL);

  size_t name_len = strlen(camname) + 1;
  cam->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (cam->name) {
    memcpy((void*)cam->name, camname, name_len);
  }

  SF_LOG(ctx, SF_LOG_INFO, 
              SF_LOG_INDENT "name   : %s\n"
              SF_LOG_INDENT "id     : %d\n"
              SF_LOG_INDENT "w      : %d\n"
              SF_LOG_INDENT "h      : %d\n"
              SF_LOG_INDENT "fov    : %.2f\n",
              cam->name, cam->id, w, h, fov);
  return cam;
}

sf_cam_t* sf_get_cam_(sf_ctx_t *ctx, const char *camname, bool should_log_failure) {
  for (int32_t i = 0; i < ctx->cam_count; ++i) {
    if (ctx->cameras[i].name && strcmp(ctx->cameras[i].name, camname) == 0) {
      return &ctx->cameras[i];
    }
  }
  if (should_log_failure) {
    SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "camera '%s' not found\n", camname);
  }
  return NULL;
}

void sf_enti_set_pos(sf_ctx_t *ctx, sf_enti_t *enti, float x, float y, float z) {
  if (enti && enti->frame) {
      enti->frame->pos = (sf_fvec3_t){x, y, z};
      enti->frame->is_dirty = true;
  }
}

void sf_enti_move(sf_ctx_t *ctx, sf_enti_t *enti, float dx, float dy, float dz) {
  if (enti && enti->frame) {
      enti->frame->pos.x += dx;
      enti->frame->pos.y += dy;
      enti->frame->pos.z += dz;
      enti->frame->is_dirty = true;
  }
}

void sf_enti_set_rot(sf_ctx_t *ctx, sf_enti_t *enti, float rx, float ry, float rz) {
  if (enti && enti->frame) {
      enti->frame->rot = (sf_fvec3_t){rx, ry, rz};
      enti->frame->is_dirty = true;
  }
}

void sf_enti_rotate(sf_ctx_t *ctx, sf_enti_t *enti, float drx, float dry, float drz) {
  if (enti && enti->frame) {
      enti->frame->rot.x += drx;
      enti->frame->rot.y += dry;
      enti->frame->rot.z += drz;
      enti->frame->is_dirty = true;
  }
}

void sf_enti_set_scale(sf_ctx_t *ctx, sf_enti_t *enti, float sx, float sy, float sz) {
  if (enti && enti->frame) {
      enti->frame->scale = (sf_fvec3_t){sx, sy, sz};
      enti->frame->is_dirty = true;
  }
}

void sf_enti_set_tex(sf_ctx_t *ctx, const char *entiname, const char *texname) {
  sf_enti_t *enti = sf_get_enti_(ctx, entiname, true);
  sf_tex_t *tex = sf_get_texture_(ctx, texname, true);
  if (enti && tex) {
    enti->tex = tex;
  }
}

sf_light_t* sf_add_light(sf_ctx_t *ctx, sf_light_type_t type, sf_fvec3_t color, float intensity) {
  if (ctx->light_count >= SF_MAX_LIGHTS) return NULL;

  sf_light_t *l = &ctx->lights[ctx->light_count++];
  l->type      = type;
  l->color     = color;
  l->intensity = intensity;

  l->frame     = sf_add_frame(ctx, NULL);

  return l;
}

void sf_load_world(sf_ctx_t *ctx, const char *filename, const char *worldname) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "could not open %s\n", filename);
    return;
  }
  char line[512];
  int obj_count = 0, enti_count = 0, light_count = 0, cam_count = 0, tex_count = 0, sprite_count, emitr_count;
  while (fgets(line, sizeof(line), file)) {
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
    char cmd;
    sscanf(line, " %c", &cmd);
    if (cmd == 't') {
      char t_name[64], t_file[256];
      if (sscanf(line, "t %63s %255s", t_name, t_file) == 2) {
        sf_load_texture_bmp(ctx, t_file, t_name);
        tex_count++;
      }
    }
    else if (cmd == 'm') {
      char m_name[64], m_file[256], r_path[512];
      if (sscanf(line, "m %63s %255s", m_name, m_file) == 2) {
        if (_sf_resolve_asset(m_file, r_path, sizeof(r_path))) {
          sf_load_obj(ctx, r_path, m_name);
          obj_count++;
        }
      }
    } 
    else if (cmd == 'e') {
      char m_name[64], e_name[64], t_name[64] = {0};
      float px, py, pz, rx, ry, rz, sx, sy, sz;
      int res = sscanf(line, "e %63s %63s %f %f %f %f %f %f %f %f %f %63s", 
        m_name, e_name, &px, &py, &pz, &rx, &ry, &rz, &sx, &sy, &sz, t_name);
      if (res >= 11) {
        sf_obj_t *obj = sf_get_obj_(ctx, m_name, true);
        if (obj) {
          sf_enti_t *enti = sf_add_enti(ctx, obj, e_name);
          enti_count++;
          sf_enti_set_pos(ctx, enti, px, py, pz);
          sf_enti_set_rot(ctx,enti, rx, ry, rz);
          sf_enti_set_scale(ctx,enti, sx, sy, sz);
          if (res == 12) {
            enti->tex = sf_get_texture_(ctx, t_name, true);
          }
        }
      }
    }
    else if (cmd == 'l') {
      char l_type[16];
      float x, y, z, r, g, b, i;
      sscanf(line, "l %15s %f %f %f %f %f %f %f", l_type, &x, &y, &z, &r, &g, &b, &i);
      if (strcmp(l_type, "dir") == 0) {
        sf_light_t *l = sf_add_light(ctx, SF_LIGHT_DIR, (sf_fvec3_t){r, g, b}, i);
        sf_frame_look_at(l->frame, (sf_fvec3_t){x, y, z});
      } else if (strcmp(l_type, "point") == 0) {
        sf_light_t *l = sf_add_light(ctx, SF_LIGHT_POINT, (sf_fvec3_t){r, g, b}, i);
        l->frame->pos = (sf_fvec3_t){x, y, z};
      }
    }
    else if (cmd == 'M') { 
      float px, py, pz, tx, ty, tz, fov;
      if (sscanf(line, "M %f %f %f %f %f %f %f", &px, &py, &pz, &tx, &ty, &tz, &fov) == 7) {
        sf_camera_set_pos(ctx, &ctx->camera, px, py, pz);
        sf_camera_look_at(ctx, &ctx->camera, (sf_fvec3_t){tx, ty, tz});
        ctx->camera.fov = fov;
        ctx->camera.is_proj_dirty = true;
        cam_count++;
      }
    }
    else if (cmd == 'c') { 
      char c_name[64];
      int w, h;
      float fov, px, py, pz, tx, ty, tz;
      if (sscanf(line, "c %63s %d %d %f %f %f %f %f %f %f", c_name, &w, &h, &fov, &px, &py, &pz, &tx, &ty, &tz) == 10) {
        sf_cam_t *cam = sf_add_cam(ctx, c_name, w, h, fov);
        if (cam) {
          sf_camera_set_pos(ctx, cam, px, py, pz);
          sf_camera_look_at(ctx, cam, (sf_fvec3_t){tx, ty, tz});
          cam_count++;
        }
      }
    }
    else if (cmd == 's') {
      char s_name[64];
      float dur, scale;
      int n_frames;
      int offset = 0;
      if (sscanf(line, "s %63s %f %f %d %n", s_name, &dur, &scale, &n_frames, &offset) == 4) {
        char tn[16][64] = {0};
        char *ptr = line + offset;
        for (int i = 0; i < n_frames && i < 16; i++) {
          int read = 0;
          sscanf(ptr, " %63s %n", tn[i], &read);
          ptr += read;
        }
        if (n_frames > 16) n_frames = 16;
        switch (n_frames) {
          case 1:  sf_load_sprite(ctx, s_name, dur, scale, 1, tn[0]); break;
          case 2:  sf_load_sprite(ctx, s_name, dur, scale, 2, tn[0], tn[1]); break;
          case 3:  sf_load_sprite(ctx, s_name, dur, scale, 3, tn[0], tn[1], tn[2]); break;
          case 4:  sf_load_sprite(ctx, s_name, dur, scale, 4, tn[0], tn[1], tn[2], tn[3]); break;
          case 5:  sf_load_sprite(ctx, s_name, dur, scale, 5, tn[0], tn[1], tn[2], tn[3], tn[4]); break;
          case 6:  sf_load_sprite(ctx, s_name, dur, scale, 6, tn[0], tn[1], tn[2], tn[3], tn[4], tn[5]); break;
          case 7:  sf_load_sprite(ctx, s_name, dur, scale, 7, tn[0], tn[1], tn[2], tn[3], tn[4], tn[5], tn[6]); break;
          case 8:  sf_load_sprite(ctx, s_name, dur, scale, 8, tn[0], tn[1], tn[2], tn[3], tn[4], tn[5], tn[6], tn[7]); break;
          case 9:  sf_load_sprite(ctx, s_name, dur, scale, 9, tn[0], tn[1], tn[2], tn[3], tn[4], tn[5], tn[6], tn[7], tn[8]); break;
          case 10: sf_load_sprite(ctx, s_name, dur, scale, 10, tn[0], tn[1], tn[2], tn[3], tn[4], tn[5], tn[6], tn[7], tn[8], tn[9]); break;
          case 11: sf_load_sprite(ctx, s_name, dur, scale, 11, tn[0], tn[1], tn[2], tn[3], tn[4], tn[5], tn[6], tn[7], tn[8], tn[9], tn[10]); break;
          case 12: sf_load_sprite(ctx, s_name, dur, scale, 12, tn[0], tn[1], tn[2], tn[3], tn[4], tn[5], tn[6], tn[7], tn[8], tn[9], tn[10], tn[11]); break;
          case 13: sf_load_sprite(ctx, s_name, dur, scale, 13, tn[0], tn[1], tn[2], tn[3], tn[4], tn[5], tn[6], tn[7], tn[8], tn[9], tn[10], tn[11], tn[12]); break;
          case 14: sf_load_sprite(ctx, s_name, dur, scale, 14, tn[0], tn[1], tn[2], tn[3], tn[4], tn[5], tn[6], tn[7], tn[8], tn[9], tn[10], tn[11], tn[12], tn[13]); break;
          case 15: sf_load_sprite(ctx, s_name, dur, scale, 15, tn[0], tn[1], tn[2], tn[3], tn[4], tn[5], tn[6], tn[7], tn[8], tn[9], tn[10], tn[11], tn[12], tn[13], tn[14]); break;
          case 16: sf_load_sprite(ctx, s_name, dur, scale, 16, tn[0], tn[1], tn[2], tn[3], tn[4], tn[5], tn[6], tn[7], tn[8], tn[9], tn[10], tn[11], tn[12], tn[13], tn[14], tn[15]); break;
        }
        sprite_count++;
      }
    }
    else if (cmd == 'p') {
      char e_name[64], type_str[16], spr_name[64];
      int max_p;
      float px, py, pz, rate, life, speed;
      int offset = 0;
      if (sscanf(line, "p %63s %15s %63s %d %f %f %f %f %f %f %n", 
          e_name, type_str, spr_name, &max_p, &px, &py, &pz, &rate, &life, &speed, &offset) >= 10) {
        sf_emitr_type_t type = SF_EMITR_OMNI;
        if (strcmp(type_str, "dir") == 0) type = SF_EMITR_DIR;
        else if (strcmp(type_str, "vol") == 0) type = SF_EMITR_VOLUME;
        sf_sprite_t *spr = sf_get_sprite_(ctx, spr_name, true);
        if (spr) {
          sf_emitr_t *em = sf_add_emitr(ctx, e_name, type, spr, max_p);
          if (em) {
            em->frame->pos = (sf_fvec3_t){px, py, pz};
            em->spawn_rate = rate;
            em->particle_life = life;
            em->speed = speed;
            char *ptr = line + offset;
            if (type == SF_EMITR_DIR) {
              float dx, dy, dz, spread;
              if (sscanf(ptr, " %f %f %f %f", &dx, &dy, &dz, &spread) == 4) {
                em->dir = sf_fvec3_norm((sf_fvec3_t){dx, dy, dz});
                em->spread = spread;
              }
            } else if (type == SF_EMITR_VOLUME) {
              float vx, vy, vz;
              if (sscanf(ptr, " %f %f %f", &vx, &vy, &vz) == 3) {
                em->volume_size = (sf_fvec3_t){vx, vy, vz};
              }
            }
            emitr_count++;
          }
        }
      }
    }
  }
  fclose(file);
  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "file   : %s\n"
              SF_LOG_INDENT "objs   : %d\n"
              SF_LOG_INDENT "entis  : %d\n"
              SF_LOG_INDENT "lights : %d\n"
              SF_LOG_INDENT "cams   : %d\n"
              SF_LOG_INDENT "texs   : %d\n"
              SF_LOG_INDENT "sprits : %d\n"
              SF_LOG_INDENT "emitrs : %d\n",
              filename, obj_count, enti_count, light_count, cam_count, tex_count, sprite_count, emitr_count);
}

void sf_camera_set_psp(sf_ctx_t *ctx, sf_cam_t *cam, float fov, float near_plane, float far_plane) {
  cam->fov        = fov;
  cam->near_plane = near_plane;
  cam->far_plane  = far_plane;
  cam->is_proj_dirty = true;
}

void sf_camera_set_pos(sf_ctx_t *ctx, sf_cam_t *cam, float x, float y, float z) {
  cam->frame->pos = (sf_fvec3_t){x, y, z};
  cam->frame->is_dirty = true;
}

void sf_camera_move_loc(sf_ctx_t *ctx, sf_cam_t *cam, float fwd, float right, float up) {
  if (!cam || !cam->frame) return;
  sf_fmat4_t m = cam->frame->global_M;
  sf_fvec3_t m_fwd = {-m.m[2][0] * fwd, -m.m[2][1] * fwd, -m.m[2][2] * fwd};
  sf_fvec3_t m_rgt = { m.m[0][0] * right, m.m[0][1] * right, m.m[0][2] * right};
  sf_fvec3_t m_up  = { m.m[1][0] * up,  m.m[1][1] * up,  m.m[1][2] * up};
  cam->frame->pos = sf_fvec3_add(cam->frame->pos, m_fwd);
  cam->frame->pos = sf_fvec3_add(cam->frame->pos, m_rgt);
  cam->frame->pos = sf_fvec3_add(cam->frame->pos, m_up);
  cam->frame->is_dirty = true;
}

void sf_camera_look_at(sf_ctx_t *ctx, sf_cam_t *cam, sf_fvec3_t target) {
  if (cam && cam->frame) sf_frame_look_at(cam->frame, target);
}

void sf_camera_add_yp(sf_ctx_t *ctx, sf_cam_t *cam, float yaw_offset, float pitch_offset) {
  if (!cam || !cam->frame) return;
  cam->frame->rot.y -= SF_DEG2RAD(yaw_offset);
  cam->frame->rot.x += SF_DEG2RAD(pitch_offset);
  if (cam->frame->rot.x >  SF_DEG2RAD(89.0f)) cam->frame->rot.x =  SF_DEG2RAD(89.0f);
  if (cam->frame->rot.x < -SF_DEG2RAD(89.0f)) cam->frame->rot.x = -SF_DEG2RAD(89.0f);
  cam->frame->is_dirty = true;
}

/* SF_FRAME_FUNCTIONS */
sf_frame_t* sf_get_root(sf_ctx_t *ctx, sf_convention_t conv) {
  if (conv < 0 || conv >= SF_CONV_MAX) return NULL;
  return ctx->roots[conv];
}

sf_frame_t* sf_add_frame(sf_ctx_t *ctx, sf_frame_t *parent) {
  if (ctx->frame_count >= SF_MAX_FRAMES) return NULL;
  sf_frame_t *f = &ctx->frames[ctx->frames_count++];
  memset(f, 0, sizeof(sf_frame_t));
  f->scale = (sf_fvec3_t){1.0f, 1.0f, 1.0f};
  f->local_M = sf_make_idn_fmat4();
  f->global_M = sf_make_idn_fmat4();
  f->is_dirty = true;
  f->is_root = false;

  if (!parent) parent = ctx->roots[SF_CONV_DEFAULT];

  f->parent = parent;
  f->next_sibling = parent->first_child;
  parent->first_child = f;

  return f;
}

void sf_update_frames(sf_ctx_t *ctx) {
  for (int i = 0; i < SF_CONV_MAX; i++) {
    if (ctx->roots[i]) {
      _sf_calc_frame_tree(ctx->roots[i], sf_make_idn_fmat4(), false);
    }
  }
}

void sf_frame_look_at(sf_frame_t *f, sf_fvec3_t target) {
  if (!f) return;
  sf_fvec3_t diff = sf_fvec3_sub(target, f->pos);
  f->rot.y = atan2f(-diff.x, -diff.z);
  float xz_dist = sqrtf(diff.x*diff.x + diff.z*diff.z);
  f->rot.x = atan2f(diff.y, xz_dist);
  f->rot.z = 0.0f;
  f->is_dirty = true;
}

void sf_frame_set_parent(sf_frame_t *child, sf_frame_t *new_parent) {
  if (!child || child->parent == new_parent) return;

  if (child->parent) {
    sf_frame_t **curr = &child->parent->first_child;
    while (*curr) {
      if (*curr == child) {
        *curr = child->next_sibling;
        break;
      }
      curr = &((*curr)->next_sibling);
    }
  }

  child->parent = new_parent;
  child->next_sibling = new_parent->first_child;
  new_parent->first_child = child;
  child->is_dirty = true;
}

/* SF_DRAWING_FUNCTIONS */
void sf_fill(sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c) {
  if (c == 0) { memset(cam->buffer, 0, cam->buffer_size * sizeof(sf_pkd_clr_t)); return; }
  sf_pkd_clr_t *buf = cam->buffer;
  int n = cam->buffer_size;
  for (int i = 0; i < n; ++i) buf[i] = c;
}

void sf_pixel(sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0) {
  if (v0.x < 0 || v0.x >= cam->w || v0.y < 0 || v0.y >= cam->h) return;
  cam->buffer[_sf_vec_to_index(ctx, cam, v0)] = c;
}

void sf_pixel_depth(sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v, float z) {
  if (v.x >= cam->w || v.y >= cam->h) return;
  uint32_t idx = _sf_vec_to_index(ctx, cam, v);
  if (z < cam->z_buffer[idx]) {
    cam->z_buffer[idx] = z;
    cam->buffer[idx] = c;
  }
}

void sf_line(sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1) {
  int dx = abs(v1.x - v0.x);
  int sx = v0.x < v1.x ? 1 : -1;
  int dy = -abs(v1.y - v0.y);
  int sy = v0.y < v1.y ? 1 : -1;
  int err = dx + dy;
  int e2;

  while (1) {
    sf_pixel(ctx, cam, c, v0);
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

void sf_rect(sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1) {
  int l = (v0.x < v1.x) ? v0.x : v1.x;
  int r = (v0.x > v1.x) ? v0.x : v1.x;
  int t = (v0.y < v1.y) ? v0.y : v1.y;
  int b = (v0.y > v1.y) ? v0.y : v1.y;
  if (l < 0) l = 0; if (r >= cam->w) r = cam->w - 1;
  if (t < 0) t = 0; if (b >= cam->h) b = cam->h - 1;
  for (int y = t; y <= b; ++y) {
    int bi = y * cam->w + l;
    for (int x = l; x <= r; ++x) cam->buffer[bi++] = c;
  }
}

void sf_tri(sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, bool use_depth) {
  if (v1.y < v0.y) { sf_fvec3_t t = v0; v0 = v1; v1 = t; }
  if (v2.y < v0.y) { sf_fvec3_t t = v0; v0 = v2; v2 = t; }
  if (v2.y < v1.y) { sf_fvec3_t t = v1; v1 = v2; v2 = t; }
  if (v2.y < 0 || v0.y >= cam->h) return;
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
  int cam_w = cam->w, cam_h = cam->h;
  sf_pkd_clr_t *cam_buf = cam->buffer;
  float *z_buf = cam->z_buffer;
  int y_start = (int)v0.y < 0 ? 0 : (int)v0.y;
  int y_end = (int)v2.y >= cam_h ? cam_h - 1 : (int)v2.y;
  for (int y = y_start; y <= y_end; ++y) {
    int idx = y - (int)v0.y;
    int x_s = xl[idx], x_e = xr[idx];
    float z_s = zl[idx], z_e = zr[idx];
    float w = (float)(x_e - x_s);
    float dz = (w <= 0.0f) ? 0.0f : (z_e - z_s) / w;
    int ox = x_s;
    if (x_s < 0) x_s = 0;
    if (x_e >= cam_w) x_e = cam_w - 1;
    float cz = z_s + (dz * (float)(x_s - ox));
    int bi = y * cam_w + x_s;
    if (use_depth) {
      for (int x = x_s; x <= x_e; ++x, ++bi, cz += dz) {
        if (cz < z_buf[bi]) { z_buf[bi] = cz; cam_buf[bi] = c; }
      }
    } else {
      for (int x = x_s; x <= x_e; ++x, ++bi) cam_buf[bi] = c;
    }
  }
}

void sf_tri_tex(sf_ctx_t *ctx, sf_cam_t *cam, sf_tex_t *tex, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, sf_fvec3_t uvz0, sf_fvec3_t uvz1, sf_fvec3_t uvz2, sf_fvec3_t l_int) {
  if (v1.y < v0.y) { _sf_swap_fvec3(&v0, &v1); _sf_swap_fvec3(&uvz0, &uvz1); }
  if (v2.y < v0.y) { _sf_swap_fvec3(&v0, &v2); _sf_swap_fvec3(&uvz0, &uvz2); }
  if (v2.y < v1.y) { _sf_swap_fvec3(&v1, &v2); _sf_swap_fvec3(&uvz1, &uvz2); }
  int iy0 = (int)v0.y, iy2 = (int)v2.y;
  if (iy2 < 0 || iy0 >= cam->h) return;
  int th = iy2 - iy0 + 1;
  if (th <= 1 || th > 1024) return;

  int x02[th], x012[th];
  float z02[th], z012[th];
  sf_fvec3_t uvz02[th], uvz012[th];

  _sf_interp_x((sf_ivec2_t){(int)v0.x, iy0}, (sf_ivec2_t){(int)v2.x, iy2}, x02);
  _sf_interp_f(v0.z, v2.z, th - 1, z02);
  _sf_interp_fvec3(uvz0, uvz2, th - 1, uvz02);

  int h01 = (int)v1.y - iy0;
  int h12 = iy2 - (int)v1.y;

  if (h01 > 0) {
    _sf_interp_x((sf_ivec2_t){(int)v0.x, iy0}, (sf_ivec2_t){(int)v1.x, (int)v1.y}, x012);
    _sf_interp_f(v0.z, v1.z, h01, z012);
    _sf_interp_fvec3(uvz0, uvz1, h01, uvz012);
  }
  if (h12 > 0) {
    _sf_interp_x((sf_ivec2_t){(int)v1.x, (int)v1.y}, (sf_ivec2_t){(int)v2.x, iy2}, &x012[h01]);
    _sf_interp_f(v1.z, v2.z, h12, &z012[h01]);
    _sf_interp_fvec3(uvz1, uvz2, h12, &uvz012[h01]);
  }

  int *xl = x02, *xr = x012;
  float *zl = z02, *zr = z012;
  sf_fvec3_t *uvzl = uvz02, *uvzr = uvz012;
  if (h01 < th && x012[h01] < x02[h01]) {
    xl = x012; xr = x02; zl = z012; zr = z02; uvzl = uvz012; uvzr = uvz02;
  }

  int li_r = (int)(l_int.x * 256.0f + 0.5f); if (li_r > 256) li_r = 256;
  int li_g = (int)(l_int.y * 256.0f + 0.5f); if (li_g > 256) li_g = 256;
  int li_b = (int)(l_int.z * 256.0f + 0.5f); if (li_b > 256) li_b = 256;
  int tex_w = tex->w, tex_h = tex->h;
  sf_pkd_clr_t *tex_px = tex->px;
  int cam_w = cam->w, cam_h = cam->h;
  sf_pkd_clr_t *cam_buf = cam->buffer;
  float *z_buf = cam->z_buffer;

  int y_start = iy0 < 0 ? 0 : iy0;
  int y_end = iy2 >= cam_h ? cam_h - 1 : iy2;

  for (int y = y_start; y <= y_end; ++y) {
    int i = y - iy0;
    int xs = xl[i], xe = xr[i];
    float scan_w = (float)(xe - xs);
    if (scan_w <= 0.0f) continue;
    float inv_sw = 1.0f / scan_w;
    float dz = (zr[i] - zl[i]) * inv_sw;
    float dux = (uvzr[i].x - uvzl[i].x) * inv_sw;
    float duy = (uvzr[i].y - uvzl[i].y) * inv_sw;
    float duz = (uvzr[i].z - uvzl[i].z) * inv_sw;
    int x0 = xs < 0 ? 0 : xs;
    int x1 = xe >= cam_w ? cam_w - 1 : xe;
    float skip = (float)(x0 - xs);
    float cz = zl[i] + dz * skip;
    float cux = uvzl[i].x + dux * skip;
    float cuy = uvzl[i].y + duy * skip;
    float cuz = uvzl[i].z + duz * skip;
    int bi = y * cam_w + x0;
    for (int x = x0; x <= x1; ++x, ++bi, cz += dz, cux += dux, cuy += duy, cuz += duz) {
      if (cz >= z_buf[bi]) continue;
      float rz = 1.0f / cuz;
      int tx = (int)(cux * rz * tex_w) % tex_w;
      int ty = (int)(cuy * rz * tex_h) % tex_h;
      if (tx < 0) tx += tex_w; if (ty < 0) ty += tex_h;
      sf_pkd_clr_t texel = tex_px[ty * tex_w + tx];
      if ((texel >> 24) == 0) continue;
      uint32_t tr = (texel >> 16) & 0xFF;
      uint32_t tg = (texel >> 8) & 0xFF;
      uint32_t tb = texel & 0xFF;
      uint32_t lr = (tr * li_r) >> 8; if (lr > 255) lr = 255;
      uint32_t lg = (tg * li_g) >> 8; if (lg > 255) lg = 255;
      uint32_t lb = (tb * li_b) >> 8; if (lb > 255) lb = 255;
      z_buf[bi] = cz;
      cam_buf[bi] = 0xFF000000u | ((uint32_t)_sf_gamma_lut[lr] << 16) | ((uint32_t)_sf_gamma_lut[lg] << 8) | _sf_gamma_lut[lb];
    }
  }
}

void sf_put_text(sf_ctx_t *ctx, sf_cam_t *cam, const char *text, sf_ivec2_t p, sf_pkd_clr_t c, int scale) {
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
            sf_pixel(ctx, cam, c, (sf_ivec2_t){ (int)(p.x + x), (int)(p.y + y) });
          } else {
            sf_rect(ctx, cam, c,
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

void sf_clear_depth(sf_ctx_t *ctx, sf_cam_t *cam) {
  memset(cam->z_buffer, 0x7F, cam->buffer_size * sizeof(float));
}

void sf_draw_cam_pip(sf_ctx_t *ctx, sf_cam_t *dest, sf_cam_t *src, sf_ivec2_t pos) {
  if (!dest || !dest->buffer || !src || !src->buffer) return;
  int sx0 = 0, sy0 = 0;
  int dx0 = pos.x, dy0 = pos.y;
  int copy_w = src->w, copy_h = src->h;
  if (dx0 < 0) { sx0 = -dx0; copy_w += dx0; dx0 = 0; }
  if (dy0 < 0) { sy0 = -dy0; copy_h += dy0; dy0 = 0; }
  if (dx0 + copy_w > dest->w) copy_w = dest->w - dx0;
  if (dy0 + copy_h > dest->h) copy_h = dest->h - dy0;
  if (copy_w <= 0 || copy_h <= 0) return;
  for (int y = 0; y < copy_h; ++y) {
    sf_pkd_clr_t *src_row = &src->buffer[(sy0 + y) * src->w + sx0];
    sf_pkd_clr_t *dst_row = &dest->buffer[(dy0 + y) * dest->w + dx0];
    memcpy(dst_row, src_row, copy_w * sizeof(sf_pkd_clr_t));
  }
}

void sf_draw_debug_ovrlay(sf_ctx_t *ctx, sf_cam_t *cam) {
  sf_draw_debug_axes   (ctx, cam);
  sf_draw_debug_frames (ctx, cam, 1.0f);
  sf_draw_debug_lights (ctx, cam, 1.0f);
  sf_draw_debug_cams   (ctx, cam, 1.0f);
}


void sf_draw_debug_axes(sf_ctx_t *ctx, sf_cam_t *cam) {
  int cx = 40;
  int cy = cam->h - 40;
  float scale = 30.0f;

  sf_fvec3_t x_axis = { cam->V.m[0][0], cam->V.m[0][1], cam->V.m[0][2] };
  sf_fvec3_t y_axis = { cam->V.m[1][0], cam->V.m[1][1], cam->V.m[1][2] };
  sf_fvec3_t z_axis = { cam->V.m[2][0], cam->V.m[2][1], cam->V.m[2][2] };

  #define DRAW_AXIS(axis, color, label) do { \
    sf_ivec2_t end_pt = { cx + (int)(axis.x * scale), cy - (int)(axis.y * scale) }; \
    sf_line(ctx, cam, color, (sf_ivec2_t){cx, cy}, end_pt); \
    int text_x = end_pt.x + (int)(axis.x * 12.0f) - 4; \
    int text_y = end_pt.y - (int)(axis.y * 12.0f) - 4; \
    sf_put_text(ctx, cam, label, (sf_ivec2_t){text_x, text_y}, color, 1); \
  } while(0)

  DRAW_AXIS(x_axis, SF_CLR_RED, "X");
  DRAW_AXIS(y_axis, SF_CLR_GREEN, "Y");
  DRAW_AXIS(z_axis, SF_CLR_BLUE, "Z");

  #undef DRAW_AXIS
}

void sf_draw_debug_frames(sf_ctx_t *ctx, sf_cam_t *cam, float axis_size) {
  if (!ctx || !cam) return;
  sf_pkd_clr_t parent_link_clr = (sf_pkd_clr_t)0xFF555555;
  for (int i = 0; i < ctx->frames_count; ++i) {
    sf_frame_t *f = &ctx->frames[i];
    sf_fmat4_t M = f->global_M;
    sf_fvec3_t origin_w = { M.m[3][0], M.m[3][1], M.m[3][2] };
    sf_fvec3_t x_tip_w = sf_fvec3_add(origin_w, (sf_fvec3_t){ M.m[0][0] * axis_size, M.m[0][1] * axis_size, M.m[0][2] * axis_size });
    sf_fvec3_t y_tip_w = sf_fvec3_add(origin_w, (sf_fvec3_t){ M.m[1][0] * axis_size, M.m[1][1] * axis_size, M.m[1][2] * axis_size });
    sf_fvec3_t z_tip_w = sf_fvec3_add(origin_w, (sf_fvec3_t){ M.m[2][0] * axis_size, M.m[2][1] * axis_size, M.m[2][2] * axis_size });
    sf_fvec3_t v_origin = sf_fmat4_mul_vec3(cam->V, origin_w);
    sf_fvec3_t v_xtip   = sf_fmat4_mul_vec3(cam->V, x_tip_w);
    sf_fvec3_t v_ytip   = sf_fmat4_mul_vec3(cam->V, y_tip_w);
    sf_fvec3_t v_ztip   = sf_fmat4_mul_vec3(cam->V, z_tip_w);
    #define DRAW_CLIPPED_LINE(v0, v1, color) do { \
      sf_fvec3_t _a = v0, _b = v1; \
      bool a_in = _a.z <= -cam->near_plane; \
      bool b_in = _b.z <= -cam->near_plane; \
      if (a_in || b_in) { \
        if (!a_in) _a = _sf_intersect_near(_b, _a, -cam->near_plane); \
        if (!b_in) _b = _sf_intersect_near(_a, _b, -cam->near_plane); \
        sf_fvec3_t s0 = _sf_project_vertex(ctx, cam, _a, cam->P); \
        sf_fvec3_t s1 = _sf_project_vertex(ctx, cam, _b, cam->P); \
        sf_line(ctx, cam, color, (sf_ivec2_t){(int)s0.x, (int)s0.y}, (sf_ivec2_t){(int)s1.x, (int)s1.y}); \
      } \
    } while(0)
    DRAW_CLIPPED_LINE(v_origin, v_xtip, SF_CLR_RED);
    DRAW_CLIPPED_LINE(v_origin, v_ytip, SF_CLR_GREEN);
    DRAW_CLIPPED_LINE(v_origin, v_ztip, SF_CLR_BLUE);
    if (f->parent && !f->is_root) {
      sf_fvec3_t p_origin_w = { f->parent->global_M.m[3][0], f->parent->global_M.m[3][1], f->parent->global_M.m[3][2] };
      sf_fvec3_t v_p_origin = sf_fmat4_mul_vec3(cam->V, p_origin_w);
      DRAW_CLIPPED_LINE(v_origin, v_p_origin, parent_link_clr);
    }
    #undef DRAW_CLIPPED_LINE
    if (v_origin.z <= -cam->near_plane) {
      sf_fvec3_t s_origin = _sf_project_vertex(ctx, cam, v_origin, cam->P);
      char label[32];
      snprintf(label, sizeof(label), "F:%d", i);
      sf_put_text(ctx, cam, label, (sf_ivec2_t){(int)s_origin.x + 5, (int)s_origin.y + 5}, SF_CLR_WHITE, 1);
    }
  }
}

void sf_draw_debug_lights(sf_ctx_t *ctx, sf_cam_t *cam, float size) {
  if (!ctx || !cam) return;
  for (int i = 0; i < ctx->light_count; ++i) {
    sf_light_t *l = &ctx->lights[i];
    if (!l->frame) continue;
    sf_fmat4_t M = l->frame->global_M;
    sf_fvec3_t pos_w = { M.m[3][0], M.m[3][1], M.m[3][2] };
    sf_pkd_clr_t clr = _sf_pack_color((sf_unpkd_clr_t){
      (uint8_t)(l->color.x * 255), (uint8_t)(l->color.y * 255), 
      (uint8_t)(l->color.z * 255), 255
    });
    if (l->type == SF_LIGHT_POINT) {
      sf_fvec3_t offsets[6] = {
        {size,0,0}, {-size,0,0}, {0,size,0}, {0,-size,0}, {0,0,size}, {0,0,-size}
      };
      for (int j = 0; j < 6; j++) {
        sf_fvec3_t p_w = sf_fvec3_add(pos_w, offsets[j]);
        sf_fvec3_t v0 = sf_fmat4_mul_vec3(cam->V, pos_w);
        sf_fvec3_t v1 = sf_fmat4_mul_vec3(cam->V, p_w);
        if (v0.z <= -cam->near_plane && v1.z <= -cam->near_plane) {
          sf_fvec3_t s0 = _sf_project_vertex(ctx, cam, v0, cam->P);
          sf_fvec3_t s1 = _sf_project_vertex(ctx, cam, v1, cam->P);
          sf_line(ctx, cam, clr, (sf_ivec2_t){(int)s0.x, (int)s0.y}, (sf_ivec2_t){(int)s1.x, (int)s1.y});
        }
      }
    } else if (l->type == SF_LIGHT_DIR) {
      sf_fvec3_t dir_w = {-M.m[2][0] * size * 2, -M.m[2][1] * size * 2, -M.m[2][2] * size * 2};
      sf_fvec3_t end_w = sf_fvec3_add(pos_w, dir_w);
      sf_fvec3_t v0 = sf_fmat4_mul_vec3(cam->V, pos_w);
      sf_fvec3_t v1 = sf_fmat4_mul_vec3(cam->V, end_w);
      if (v0.z <= -cam->near_plane && v1.z <= -cam->near_plane) {
        sf_fvec3_t s0 = _sf_project_vertex(ctx, cam, v0, cam->P);
        sf_fvec3_t s1 = _sf_project_vertex(ctx, cam, v1, cam->P);
        sf_line(ctx, cam, clr, (sf_ivec2_t){(int)s0.x, (int)s0.y}, (sf_ivec2_t){(int)s1.x, (int)s1.y});
      }
    }
  }
}

void sf_draw_debug_cams(sf_ctx_t *ctx, sf_cam_t *view_cam, float ray_len) {
  if (!ctx || !view_cam) return;
  for (int i = 0; i < ctx->cam_count; ++i) {
    sf_cam_t *c = &ctx->cameras[i];
    if (!c->frame || c == view_cam) continue;
    sf_fmat4_t M = c->frame->global_M;
    sf_fvec3_t origin_w = { M.m[3][0], M.m[3][1], M.m[3][2] };
    float aspect = (float)c->w / (float)c->h;
    float h_half = tanf(SF_DEG2RAD(c->fov) * 0.5f) * ray_len;
    float w_half = h_half * aspect;
    sf_fvec3_t corners[4] = {
      { w_half,  h_half, -ray_len},
      {-w_half,  h_half, -ray_len},
      {-w_half, -h_half, -ray_len},
      { w_half, -h_half, -ray_len}
    };
    sf_fvec3_t corners_w[4];
    for (int j = 0; j < 4; j++) {
      corners_w[j] = sf_fmat4_mul_vec3(M, corners[j]);
      sf_fvec3_t v0 = sf_fmat4_mul_vec3(view_cam->V, origin_w);
      sf_fvec3_t v1 = sf_fmat4_mul_vec3(view_cam->V, corners_w[j]);
      if (v0.z <= -view_cam->near_plane && v1.z <= -view_cam->near_plane) {
        sf_fvec3_t s0 = _sf_project_vertex(ctx, view_cam, v0, view_cam->P);
        sf_fvec3_t s1 = _sf_project_vertex(ctx, view_cam, v1, view_cam->P);
        sf_line(ctx, view_cam, SF_CLR_WHITE, (sf_ivec2_t){(int)s0.x, (int)s0.y}, (sf_ivec2_t){(int)s1.x, (int)s1.y});
      }
    }
    for (int j = 0; j < 4; j++) {
      sf_fvec3_t v0 = sf_fmat4_mul_vec3(view_cam->V, corners_w[j]);
      sf_fvec3_t v1 = sf_fmat4_mul_vec3(view_cam->V, corners_w[(j+1)%4]);
      if (v0.z <= -view_cam->near_plane && v1.z <= -view_cam->near_plane) {
        sf_fvec3_t s0 = _sf_project_vertex(ctx, view_cam, v0, view_cam->P);
        sf_fvec3_t s1 = _sf_project_vertex(ctx, view_cam, v1, view_cam->P);
        sf_line(ctx, view_cam, SF_CLR_GREEN, (sf_ivec2_t){(int)s0.x, (int)s0.y}, (sf_ivec2_t){(int)s1.x, (int)s1.y});
      }
    }
  }
}

void sf_draw_sprite(sf_ctx_t *ctx, sf_cam_t *cam, sf_sprite_t *spr, sf_fvec3_t pos_w, float anim_time, float scale_mult) {
  if (!spr || spr->frame_count == 0) return;
  int frame_idx = (int)(anim_time / spr->frame_duration) % spr->frame_count;
  sf_tex_t *tex = spr->frames[frame_idx];
  if (!tex) return;
  sf_fvec3_t v_view = sf_fmat4_mul_vec3(cam->V, pos_w);
  if (v_view.z >= -cam->near_plane) return;
  sf_fvec3_t center_scr = _sf_project_vertex(ctx, cam, v_view, cam->P);
  float actual_scale = spr->base_scale * scale_mult;
  sf_fvec3_t edge_view = { v_view.x + actual_scale, v_view.y + actual_scale, v_view.z };
  sf_fvec3_t edge_scr = _sf_project_vertex(ctx, cam, edge_view, cam->P);
  int half_w = abs((int)(edge_scr.x - center_scr.x));
  int half_h = abs((int)(edge_scr.y - center_scr.y));
  if (center_scr.x + half_w < 0 || center_scr.x - half_w >= cam->w ||
      center_scr.y + half_h < 0 || center_scr.y - half_h >= cam->h) return;
  float cz = center_scr.z;
  int xs = (int)center_scr.x - half_w;
  int xe = (int)center_scr.x + half_w;
  int ys = (int)center_scr.y - half_h;
  int ye = (int)center_scr.y + half_h;
  int span_w = xe - xs, span_h = ye - ys;
  if (span_w <= 0 || span_h <= 0) return;
  int tex_w = tex->w, tex_h = tex->h;
  sf_pkd_clr_t *tex_px = tex->px;
  int cam_w = cam->w, cam_h = cam->h;
  sf_pkd_clr_t *cam_buf = cam->buffer;
  float *z_buf = cam->z_buffer;
  int y0 = ys < 0 ? 0 : ys;
  int y1 = ye >= cam_h ? cam_h - 1 : ye - 1;
  int x0 = xs < 0 ? 0 : xs;
  int x1 = xe >= cam_w ? cam_w - 1 : xe - 1;
  uint8_t a8 = (uint8_t)(scale_mult * 255.0f + 0.5f);
  bool opaque = (a8 >= 252);
  uint8_t inv_a8 = 255 - a8;
  for (int y = y0; y <= y1; y++) {
    int ty = ((y - ys) * tex_h / span_h) % tex_h;
    int row = y * cam_w;
    for (int x = x0; x <= x1; x++) {
      int bi = row + x;
      if (cz >= z_buf[bi]) continue;
      int tx = ((x - xs) * tex_w / span_w) % tex_w;
      sf_pkd_clr_t texel = tex_px[ty * tex_w + tx];
      if ((texel >> 24) == 0) continue;
      uint32_t tr = (texel >> 16) & 0xFF;
      uint32_t tg = (texel >> 8) & 0xFF;
      uint32_t tb = texel & 0xFF;
      if (opaque) {
        z_buf[bi] = cz;
        cam_buf[bi] = 0xFF000000u | ((uint32_t)_sf_gamma_lut[tr] << 16) | ((uint32_t)_sf_gamma_lut[tg] << 8) | _sf_gamma_lut[tb];
      } else {
        uint32_t bg = cam_buf[bi];
        uint32_t bg_lr = _sf_degamma_lut[(bg >> 16) & 0xFF];
        uint32_t bg_lg = _sf_degamma_lut[(bg >> 8) & 0xFF];
        uint32_t bg_lb = _sf_degamma_lut[bg & 0xFF];
        uint32_t or_ = (tr * a8 + bg_lr * inv_a8) >> 8; if (or_ > 255) or_ = 255;
        uint32_t og  = (tg * a8 + bg_lg * inv_a8) >> 8; if (og > 255) og = 255;
        uint32_t ob  = (tb * a8 + bg_lb * inv_a8) >> 8; if (ob > 255) ob = 255;
        cam_buf[bi] = 0xFF000000u | ((uint32_t)_sf_gamma_lut[or_] << 16) | ((uint32_t)_sf_gamma_lut[og] << 8) | _sf_gamma_lut[ob];
      }
    }
  }
}

/* SF_UI_FUNCTIONS */
sf_ui_t* sf_create_ui (sf_ctx_t *ctx) {
  sf_ui_t *ui = (sf_ui_t*)sf_arena_alloc(ctx, &ctx->arena, sizeof(sf_ui_t));
  ui->elements = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_UI_ELEMENTS * sizeof(sf_ui_lmn_t));
  ui->count = 0;
  ui->default_style.color_base   = (sf_pkd_clr_t)0xFF404040;
  ui->default_style.color_hover  = (sf_pkd_clr_t)0xFF555555;
  ui->default_style.color_active = (sf_pkd_clr_t)0xFF707070;
  ui->default_style.color_text   = (sf_pkd_clr_t)0xFFEEEEEE;
  ui->default_style.draw_outline = false;
  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "ui inited\n"
              SF_LOG_INDENT "cpcti : %d \n",
              SF_MAX_UI_ELEMENTS);
  return ui;
}

sf_ui_lmn_t* sf_add_button(sf_ctx_t *ctx, const char *text, sf_ivec2_t v0, sf_ivec2_t v1, void (*cb)(sf_ctx_t*, void*), void *userdata) {
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;

  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));

  el->type            = SF_UI_BUTTON;
  el->style           = ctx->ui->default_style;

  el->v0              = v0;
  el->v1              = v1;
  el->is_visible      = true;

  el->button.text     = text;
  el->button.callback = cb;
  el->button.userdata = userdata;

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "text   : %s\n"
              SF_LOG_INDENT "cb     : %p\n",
              text, (void*)cb);
  return el;
}

sf_ui_lmn_t* sf_add_slider(sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, float min_val, float max_val, float init_val, sf_ui_cb cb, void *userdata) {
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;

  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));

  el->type            = SF_UI_SLIDER;
  el->style           = ctx->ui->default_style;
  el->v0              = v0;
  el->v1              = v1;
  el->is_visible      = true;

  el->slider.min_val  = min_val;
  el->slider.max_val  = max_val;
  el->slider.value    = init_val;
  el->slider.callback = cb;
  el->slider.userdata = userdata;

  return el;
}

sf_ui_lmn_t* sf_add_checkbox(sf_ctx_t *ctx, const char *text, sf_ivec2_t v0, sf_ivec2_t v1, bool init_state, sf_ui_cb cb, void *userdata) {
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;

  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));

  el->type                = SF_UI_CHECKBOX;
  el->style               = ctx->ui->default_style;
  el->v0                  = v0;
  el->v1                  = v1;
  el->is_visible          = true;

  el->checkbox.text       = text;
  el->checkbox.is_checked = init_state;
  el->checkbox.callback   = cb;
  el->checkbox.userdata   = userdata;

  return el;
}

void draw_button(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *btn) {
  if (!btn->is_visible) return;
  sf_pkd_clr_t bg_color = btn->style.color_base; 

  if (btn->is_disabled) {
    bg_color = (sf_pkd_clr_t)0xFF555555;
  } else if (btn->is_pressed) {
    bg_color = btn->style.color_active;
  } else if (btn->is_hovered) {
    bg_color = btn->style.color_hover;
  }

  sf_rect(ctx, cam, bg_color, btn->v0, btn->v1);

  if (btn->button.text) {
    int text_len = strlen(btn->button.text);
    int text_w = text_len * 8; 
    int text_h = 8;
    int box_w = btn->v1.x - btn->v0.x;
    int box_h = btn->v1.y - btn->v0.y;
    sf_ivec2_t text_pos = {
      btn->v0.x + (box_w - text_w) / 2,
      btn->v0.y + (box_h - text_h) / 2
    };
    sf_put_text(ctx, cam, btn->button.text, text_pos, btn->style.color_text, 1);
  }
}

void draw_slider(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el) {
  if (!el->is_visible) return;

  sf_pkd_clr_t fill_color = el->style.color_base; 
  if (el->is_disabled) {
    fill_color = (sf_pkd_clr_t)0xFF555555;
  } else if (el->is_pressed) {
    fill_color = el->style.color_active;
  } else if (el->is_hovered) {
    fill_color = el->style.color_hover;
  }

  sf_pkd_clr_t track_color = (sf_pkd_clr_t)0xFF222222;
  sf_rect(ctx, cam, track_color, el->v0, el->v1);

  float range = el->slider.max_val - el->slider.min_val;
  float t = (range > 0.0f) ? (el->slider.value - el->slider.min_val) / range : 0.0f;
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;

  int total_w = el->v1.x - el->v0.x;
  int fill_w = (int)(total_w * t);

  if (fill_w > 0) {
    sf_ivec2_t fill_v1 = { el->v0.x + fill_w, el->v1.y };
    sf_rect(ctx, cam, fill_color, el->v0, fill_v1);
  }

  char val_str[32];
  snprintf(val_str, sizeof(val_str), "%.2f", el->slider.value);
  int text_w = strlen(val_str) * 8;
  sf_ivec2_t text_pos = {
    el->v0.x + (total_w - text_w) / 2,
    el->v0.y + ((el->v1.y - el->v0.y) - 8) / 2
  };
  sf_put_text(ctx, cam, val_str, text_pos, el->style.color_text, 1);
}

void draw_checkbox(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el) {
  if (!el->is_visible) return;

  sf_pkd_clr_t box_color = el->style.color_base; 
  if (el->is_disabled) {
    box_color = (sf_pkd_clr_t)0xFF555555;
  } else if (el->is_pressed) {
    box_color = el->style.color_active;
  } else if (el->is_hovered) {
    box_color = el->style.color_hover;
  }

  int h = el->v1.y - el->v0.y;
  int box_size = h < 16 ? h : 16; 
  sf_ivec2_t box_v0 = { el->v0.x, el->v0.y + (h - box_size)/2 };
  sf_ivec2_t box_v1 = { box_v0.x + box_size, box_v0.y + box_size };

  sf_rect(ctx, cam, box_color, box_v0, box_v1);

  if (el->checkbox.is_checked) {
    sf_ivec2_t inner_v0 = { box_v0.x + 3, box_v0.y + 3 };
    sf_ivec2_t inner_v1 = { box_v1.x - 3, box_v1.y - 3 };
    sf_rect(ctx, cam, el->style.color_text, inner_v0, inner_v1);
  }

  if (el->checkbox.text) {
    sf_ivec2_t text_pos = {
      box_v1.x + 8,
      el->v0.y + (h - 8) / 2
    };
    sf_put_text(ctx, cam, el->checkbox.text, text_pos, el->style.color_text, 1);
  }
}

void sf_render_ui(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_t *ui) {
  if (!ui) return;
  for (int i = 0; i < ui->count; ++i) {
    sf_ui_lmn_t *el = &ui->elements[i];
    switch (el->type) {
      case SF_UI_BUTTON:
        draw_button(ctx, cam, el);
        break;
      case SF_UI_SLIDER:
        draw_slider(ctx, cam, el);
        break;
      case SF_UI_CHECKBOX:
        draw_checkbox(ctx, cam, el);
        break;
    }
  }
}

static void _sf_update_button(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released) {
  if (el->is_hovered && el->is_pressed && m_released) {
    if (el->button.callback) {
      el->button.callback(ctx, el->button.userdata);
    }
  }
}

static void _sf_update_checkbox(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released) {
  if (el->is_hovered && el->is_pressed && m_released) {
    el->checkbox.is_checked = !el->checkbox.is_checked;
    if (el->checkbox.callback) {
      el->checkbox.callback(ctx, el->checkbox.userdata);
    }
  }
}

static void _sf_update_slider(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released) {
  if (el->is_pressed && m_down) {
    int mx = ctx->input.mouse_x;
    float width = (float)(el->v1.x - el->v0.x);
    float offset = (float)(mx - el->v0.x);
    float t = offset / width;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float new_val = el->slider.min_val + t * (el->slider.max_val - el->slider.min_val);
    if (new_val != el->slider.value) {
      el->slider.value = new_val;
      if (el->slider.callback) {
        el->slider.callback(ctx, el->slider.userdata);
      }
    }
  }
}

void sf_update_ui(sf_ctx_t *ctx, sf_ui_t *ui) {
  if (!ui) return;

  int mx = ctx->input.mouse_x;
  int my = ctx->input.mouse_y;
  bool m_down = ctx->input.mouse_btns[SF_MOUSE_LEFT];
  bool m_pressed = m_down && !ctx->input.mouse_btns_prev[SF_MOUSE_LEFT];
  bool m_released = !m_down && ctx->input.mouse_btns_prev[SF_MOUSE_LEFT];

  for (int i = 0; i < ui->count; ++i) {
    sf_ui_lmn_t *el = &ui->elements[i];

    if (!el->is_visible || el->is_disabled) {
      el->is_hovered = false;
      el->is_pressed = false;
      continue;
    }

    el->is_hovered = (mx >= el->v0.x && mx <= el->v1.x && 
                      my >= el->v0.y && my <= el->v1.y);

    if (el->is_hovered && m_pressed) {
      el->is_pressed = true;
    }

    switch (el->type) {
      case SF_UI_BUTTON:
        _sf_update_button(ctx, el, m_down, m_pressed, m_released); 
        break;
      case SF_UI_CHECKBOX:
        _sf_update_checkbox(ctx, el, m_down, m_pressed, m_released); 
        break;
      case SF_UI_SLIDER:
        _sf_update_slider(ctx, el, m_down, m_pressed, m_released); 
        break;
    }

    if (!m_down) {
      el->is_pressed = false;
    }
  }
}

/* SF_LOG_FUNCTIONS */
void sf_log_(sf_ctx_t *ctx, sf_log_level_t level, const char* func, const char* fmt, ...) {
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

void sf_set_logger(sf_ctx_t *ctx, sf_log_fn callback, void* userdata) {
  ctx->log_cb = callback;
  ctx->log_user = userdata;
}

void sf_logger_console(const char* message, void* userdata) {
  fprintf(stdout, "%s", message);
}

/* SF_IMPLEMENTATION_HELPERS */
uint32_t _sf_vec_to_index(sf_ctx_t *ctx, sf_cam_t *cam, sf_ivec2_t v) {
  return v.y * cam->w + v.x;
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

sf_fvec3_t _sf_project_vertex(sf_ctx_t *ctx, sf_cam_t *cam, sf_fvec3_t v, sf_fmat4_t P) {
  sf_fvec3_t proj = sf_fmat4_mul_vec3(P, v);
  return (sf_fvec3_t){
    (proj.x + 1.0f) * 0.5f * (float)cam->w,
    (1.0f - (proj.y + 1.0f) * 0.5f) * (float)cam->h,
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

uint64_t _sf_get_ticks(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * SF_NANOS_PER_SEC + (uint64_t)ts.tv_nsec;
}

sf_pkd_clr_t _sf_pack_color(sf_unpkd_clr_t c) {
  return ((uint32_t)c.a << 24) | 
         ((uint32_t)c.r << 16) | 
         ((uint32_t)c.g << 8 ) |
          (uint32_t)c.b;
}

sf_unpkd_clr_t _sf_unpack_color(sf_pkd_clr_t c) {
  sf_unpkd_clr_t unpkd;
  unpkd.a = (uint8_t)((c >> 24) & 0xFF);
  unpkd.r = (uint8_t)((c >> 16) & 0xFF);
  unpkd.g = (uint8_t)((c >> 8)  & 0xFF);
  unpkd.b = (uint8_t)( c        & 0xFF);
  return unpkd;
}

size_t _sf_obj_memusg(sf_obj_t *obj) {
  if (!obj) return 0;
  size_t v_size  = obj->v_cnt * sizeof(sf_fvec3_t);
  size_t vt_size = obj->vt_cnt * sizeof(sf_fvec2_t);
  size_t vn_size = obj->vn_cnt * sizeof(sf_fvec3_t);
  size_t f_size  = obj->f_cnt * sizeof(sf_face_t);
  return v_size + vt_size + vn_size + f_size;
}

void _sf_set_up_frames(sf_ctx_t *ctx) {
  ctx->roots[SF_CONV_DEFAULT] = &ctx->frames[ctx->frames_count++];
  memset(ctx->roots[SF_CONV_DEFAULT], 0, sizeof(sf_frame_t));
  ctx->roots[SF_CONV_DEFAULT]->local_M  = sf_make_idn_fmat4();
  ctx->roots[SF_CONV_DEFAULT]->global_M = sf_make_idn_fmat4();
  ctx->roots[SF_CONV_DEFAULT]->is_root  = true;

  ctx->roots[SF_CONV_NED] = &ctx->frames[ctx->frames_count++];
  memset(ctx->roots[SF_CONV_NED], 0, sizeof(sf_frame_t));
  sf_fmat4_t ned_M = sf_make_idn_fmat4();
  ned_M.m[0][0]= 0; ned_M.m[0][1]= 1; ned_M.m[0][2]= 0; 
  ned_M.m[1][0]= 0; ned_M.m[1][1]= 0; ned_M.m[1][2]=-1; 
  ned_M.m[2][0]=-1; ned_M.m[2][1]= 0; ned_M.m[2][2]= 0; 
  ctx->roots[SF_CONV_NED]->local_M  = ned_M;
  ctx->roots[SF_CONV_NED]->global_M = ned_M;
  ctx->roots[SF_CONV_NED]->is_root  = true;

  ctx->roots[SF_CONV_FLU] = &ctx->frames[ctx->frames_count++];
  memset(ctx->roots[SF_CONV_FLU], 0, sizeof(sf_frame_t));
  sf_fmat4_t flu_M = sf_make_idn_fmat4();
  flu_M.m[0][0]= 0; flu_M.m[0][1]=-1; flu_M.m[0][2]= 0; 
  flu_M.m[1][0]= 0; flu_M.m[1][1]= 0; flu_M.m[1][2]= 1; 
  flu_M.m[2][0]=-1; flu_M.m[2][1]= 0; flu_M.m[2][2]= 0; 
  ctx->roots[SF_CONV_FLU]->local_M  = flu_M;
  ctx->roots[SF_CONV_FLU]->global_M = flu_M;
  ctx->roots[SF_CONV_FLU]->is_root  = true;
}

void _sf_calc_frame_tree(sf_frame_t *f, sf_fmat4_t parent_global_M, bool force_dirty) {
  if (!f) return;
  bool current_dirty = f->is_dirty || force_dirty;

  if (current_dirty) {
    if (!f->is_root) {
      sf_fmat4_t t_mat  = sf_make_tsl_fmat4(f->pos.x, f->pos.y, f->pos.z);
      sf_fmat4_t r_mat  = sf_make_rot_fmat4(f->rot);
      sf_fmat4_t s_mat  = sf_make_scale_fmat4(f->scale);
      sf_fmat4_t rs_mat = sf_fmat4_mul_fmat4(r_mat, s_mat);
      f->local_M = sf_fmat4_mul_fmat4(rs_mat, t_mat);
    }
    f->global_M = sf_fmat4_mul_fmat4(f->local_M, parent_global_M);
    f->is_dirty = false;
  }
 
  _sf_calc_frame_tree(f->first_child, f->global_M, current_dirty);
  _sf_calc_frame_tree(f->next_sibling, parent_global_M, force_dirty);
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

/* SF_GAMMA_LUT - sqrtf(i/255.0)*255 for linear->sRGB approx */
static const uint8_t _sf_gamma_lut[256] = {
    0, 16, 23, 28, 32, 36, 39, 42, 45, 48, 50, 53, 55, 58, 60, 62,
   64, 66, 68, 70, 71, 73, 75, 77, 78, 80, 81, 83, 84, 86, 87, 89,
   90, 92, 93, 94, 96, 97, 98,100,101,102,103,105,106,107,108,109,
  111,112,113,114,115,116,117,118,119,121,122,123,124,125,126,127,
  128,129,130,131,132,133,134,135,135,136,137,138,139,140,141,142,
  143,144,145,145,146,147,148,149,150,151,151,152,153,154,155,156,
  156,157,158,159,160,160,161,162,163,164,164,165,166,167,167,168,
  169,170,170,171,172,173,173,174,175,176,176,177,178,179,179,180,
  181,181,182,183,183,184,185,186,186,187,188,188,189,190,190,191,
  192,192,193,194,194,195,196,196,197,198,198,199,199,200,201,201,
  202,203,203,204,204,205,206,206,207,208,208,209,209,210,211,211,
  212,212,213,214,214,215,215,216,217,217,218,218,219,220,220,221,
  221,222,222,223,224,224,225,225,226,226,227,228,228,229,229,230,
  230,231,231,232,233,233,234,234,235,235,236,236,237,237,238,238,
  239,240,240,241,241,242,242,243,243,244,244,245,245,246,246,247,
  247,248,248,249,249,250,250,251,251,252,252,253,253,254,254,255
};

/* SF_DEGAMMA_LUT - (i/255.0)^2*255 for sRGB->linear approx */
static const uint8_t _sf_degamma_lut[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  4,  4,
    4,  4,  5,  5,  5,  5,  6,  6,  6,  7,  7,  7,  8,  8,  8,  9,
    9,  9, 10, 10, 11, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16,
   16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 23, 23, 24, 24,
   25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 35, 35,
   36, 37, 38, 38, 39, 40, 41, 42, 42, 43, 44, 45, 46, 47, 47, 48,
   49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59, 60, 61, 62, 63,
   64, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 77, 78, 79, 80,
   81, 82, 84, 85, 86, 87, 88, 89, 91, 92, 93, 94, 95, 97, 98, 99,
  100,102,103,104,105,107,108,109,111,112,113,115,116,117,119,120,
  121,123,124,126,127,128,130,131,133,134,136,137,139,140,142,143,
  145,146,148,149,151,152,154,155,157,158,160,162,163,165,166,168,
  170,171,173,175,176,178,180,181,183,185,186,188,190,192,193,195,
  197,199,200,202,204,206,207,209,211,213,215,217,218,220,222,224,
  226,228,230,232,233,235,237,239,241,243,245,247,249,251,253,255
};

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
