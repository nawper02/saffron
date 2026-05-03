/* saffron.h
 * Saffron is an stb-style graphics/game engine library.
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
#define SF_ARENA_SIZE                 67108864
#define SF_MAX_OBJS                   128
#define SF_MAX_ENTITIES               1024
#define SF_MAX_LIGHTS                 32
#define SF_MAX_TEXTURES               64
#define SF_MAX_CAMS                   8
#define SF_MAX_CB_PER_EVT             4
#define SF_MAX_UI_ELEMENTS            512
#define SF_MAX_TEXT_INPUT_LEN         128
#define SF_MAX_DROPDOWN_ITEMS         64
#define SF_DROPDOWN_MAX_VISIBLE       8
#define SF_MAX_FRAMES                 512
#define SF_MAX_SPRITES                20
#define SF_MAX_EMITRS                 10
#define SF_MAX_SKYBOXES               4
#define SF_SKYBOX_SPAN                128
#define SF_MAX_SPRITE_FRAMES          16
#define SF_MAX_SPRITE_3DS                 8192
#define SF_PERF_HIST_SIZE             64
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
#define sf_get_light(ctx, name)       sf_get_light_(ctx, name, true)
#define sf_get_skybox(ctx, name)      sf_get_skybox_(ctx, name, true)

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

typedef enum {
  SF_LOG_DEBUG,
  SF_LOG_INFO,
  SF_LOG_WARN,
  SF_LOG_ERROR
} sf_log_level_t;

typedef void  (*sf_log_fn    )(const char* message, void* userdata);

typedef uint32_t sf_pkd_clr_t;
typedef struct { uint8_t  r, g, b, a; } sf_unpkd_clr_t;
typedef struct { int      x, y;       } sf_ivec2_t;
typedef struct { float    x, y;       } sf_fvec2_t;
typedef struct { int      x, y, z;    } sf_svec3_t;
typedef struct { float    x, y, z;    } sf_fvec3_t;
typedef struct { int      x, y, z;    } sf_ivec3_t;
typedef struct { float m[4][4];       } sf_fmat4_t;
typedef struct { sf_fvec3_t o, d;     } sf_ray_t;

typedef enum {
  SF_CONV_DEFAULT = 0,
  SF_CONV_NED,
  SF_CONV_FLU,
  SF_CONV_MAX
} sf_convention_t;

typedef struct sf_frame_t_ sf_frame_t;
struct sf_frame_t_ {
  sf_fvec3_t                        pos;
  sf_fvec3_t                        rot;
  sf_fvec3_t                        scale;

  sf_fmat4_t                        local_M;
  sf_fmat4_t                        global_M;
  bool                              is_dirty;
  bool                              is_root;

  const char                       *name;

  sf_frame_t                       *parent;
  sf_frame_t                       *first_child;
  sf_frame_t                       *next_sibling;
};

typedef struct {
  int32_t                           id;
  const char                       *name;
  int                               w, h, buffer_size;
  sf_pkd_clr_t                     *buffer;
  float                            *z_buffer;
  float                             fov, near_plane, far_plane;
  bool                              is_proj_dirty;
  sf_fmat4_t                        V, P;
  sf_frame_t                       *frame;
} sf_cam_t;

typedef struct {
  sf_pkd_clr_t                     *px;
  int                               w;
  int                               h;
  int                               w_mask;
  int                               h_mask;
  int32_t                           id;
  const char                       *name;
} sf_tex_t;

typedef struct {
  int                               v;
  int                               vt;
  int                               vn;
} sf_vtx_idx_t;

typedef struct {
  sf_vtx_idx_t                      idx[3];
} sf_face_t;

typedef struct {
  sf_fvec3_t                       *v;
  sf_fvec2_t                       *vt;
  sf_fvec3_t                       *vn;
  sf_face_t                        *f;
  int32_t                           v_cnt;
  int32_t                           vt_cnt;
  int32_t                           vn_cnt;
  int32_t                           f_cnt;
  int32_t                           id;
  const char                       *name;
  sf_fvec3_t                        bs_center;
  float                             bs_radius;
  const char                       *src_path;
  int32_t                           v_cap;
  int32_t                           vt_cap;
  int32_t                           f_cap;
} sf_obj_t;

typedef struct {
  sf_obj_t                          obj;
  int32_t                           id;
  sf_tex_t                         *tex;
  sf_fvec2_t                        tex_scale;
  const char                       *name;
  sf_frame_t                       *frame;
} sf_enti_t;

typedef enum {
  SF_LIGHT_DIR,
  SF_LIGHT_POINT
} sf_light_type_t;

typedef struct {
  sf_light_type_t                   type;
  sf_fvec3_t                        color;
  float                             intensity;
  sf_frame_t                       *frame;
  const char                       *name;
  int32_t                           id;
} sf_light_t;

typedef struct {
  int32_t                           id;
  const char                       *name;
  sf_tex_t                         *frames[SF_MAX_SPRITE_FRAMES];
  int                               frame_count;
  float                             frame_duration;
  float                             base_scale;
  float                             opacity;
} sf_sprite_t;

typedef struct {
  char                              name[32];
  sf_sprite_t                      *sprite;
  sf_fvec3_t                        pos;
  float                             scale;
  float                             opacity;
  float                             angle;
  sf_fvec3_t                        normal;
  sf_frame_t                       *frame;
} sf_sprite_3d_t;

typedef struct {
  int32_t                           id;
  const char                       *name;
  sf_tex_t                         *tex;
} sf_skybox_t;

typedef struct {
  sf_fvec3_t                        pos;
  sf_fvec3_t                        vel;
  float                             life;
  float                             max_life;
  float                             anim_time;
  bool                              active;
} sf_particle_t;

typedef enum {
  SF_EMITR_DIR,
  SF_EMITR_OMNI,
  SF_EMITR_VOLUME
} sf_emitr_type_t;

typedef struct {
  int32_t                           id;
  const char                       *name;
  sf_emitr_type_t                   type;
  sf_sprite_t                      *sprite;
  sf_frame_t                       *frame;

  sf_particle_t                    *particles;
  int                               max_particles;
  float                             spawn_rate;
  float                             spawn_acc;
  float                             particle_life;
  float                             speed;

  sf_fvec3_t                        dir;
  float                             spread;
  sf_fvec3_t                        volume_size;
} sf_emitr_t;

typedef struct {
  size_t                            size;
  size_t                            offset;
  uint8_t                          *buffer;
} sf_arena_t;

typedef float (*sf_height_fn )(float x, float z, void *ud);

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
  SF_KEY_BACKSPACE, SF_KEY_RETURN, SF_KEY_ESC, SF_KEY_TAB,
  SF_KEY_DEL, SF_KEY_HOME, SF_KEY_END, SF_KEY_LCTRL,
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
  SF_EVT_MOUSE_WHEEL,
  SF_EVT_TEXT_INPUT,
  SF_EVT_MAX
} sf_event_type_t;

typedef struct {
  sf_event_type_t type;
  union {
    sf_key_t key;
    struct { int x, y, dx, dy; } mouse_move;
    struct { sf_mouse_btn_t btn; int x, y; } mouse_btn;
    struct { int dy; }             wheel;
    struct { char text[8]; }       text;
  };
} sf_event_t;

typedef void  (*sf_event_cb  )(struct sf_ctx_t_ *ctx, const sf_event_t *event, void *userdata);

typedef struct {
  sf_event_cb                       cb;
  void                             *userdata;
} sf_callback_entry_t;

typedef struct {
  bool                              keys[SF_KEY_MAX];
  bool                              keys_prev[SF_KEY_MAX];
  int                               mouse_x, mouse_y;
  int                               mouse_dx, mouse_dy;
  int                               wheel_dy;
  bool                              mouse_btns[SF_MOUSE_MAX];
  bool                              mouse_btns_prev[SF_MOUSE_MAX];
} sf_input_state_t;

typedef void (*sf_ui_cb)(struct sf_ctx_t_ *ctx, void *userdata);

typedef enum {
  SF_UI_BUTTON,
  SF_UI_SLIDER,
  SF_UI_CHECKBOX,
  SF_UI_LABEL,
  SF_UI_TEXT_INPUT,
  SF_UI_DRAG_FLOAT,
  SF_UI_DROPDOWN,
  SF_UI_PANEL,
  SF_UI_IMAGE
} sf_ui_type_t;

typedef struct {
  sf_pkd_clr_t                      color_base;
  sf_pkd_clr_t                      color_hover;
  sf_pkd_clr_t                      color_active;
  sf_pkd_clr_t                      color_text;
  bool                              draw_outline;
} sf_ui_style_t;

typedef struct sf_ui_lmn_t_ sf_ui_lmn_t;
struct sf_ui_lmn_t_ {
  sf_ui_type_t                      type;
  const char                       *name;
  sf_ui_style_t                     style;

  sf_ivec2_t                        v0, v1;
  bool                              is_hovered;
  bool                              is_pressed;
  bool                              is_visible;
  bool                              is_disabled;
  sf_ui_lmn_t                      *parent_panel;

  union {
    struct {
      const char                   *text;
      sf_ui_cb                      callback;
      void                         *userdata;
    } button;
    struct {
      float                         value;
      float                         min_val;
      float                         max_val;
      sf_ui_cb                      callback;
      void                         *userdata;
    } slider;
    struct {
      const char                   *text;
      bool                          is_checked;
      sf_ui_cb                      callback;
      void                         *userdata;
    } checkbox;
    struct {
      const char                   *text;
      sf_pkd_clr_t                  color;
    } label;
    struct {
      char                         *buf;
      int                           buflen;
      int                           caret;
      sf_ui_cb                      callback;
      void                         *userdata;
    } text_input;
    struct {
      float                        *target;
      float                         step;
      int                           drag_anchor_x;
      float                         drag_anchor_val;
      sf_ui_cb                      callback;
      void                         *userdata;
    } drag_float;
    struct {
      const char                  **items;
      int                           n_items;
      int                          *selected;
      bool                          is_open;
      int                           hover_item;
      int                           scroll_offset;
      int                           max_visible;
      sf_ui_cb                      callback;
      void                         *userdata;
    } dropdown;
    struct {
      const char                   *title;
      bool                          collapsed;
      int                           content_h;
    } panel;
    struct {
      sf_tex_t                     *tex;
      bool                          keyed; /* true = treat magenta as transparent */
    } image;
  };
};

typedef struct sf_ui_t_ {
  sf_ui_lmn_t                      *elements;
  int32_t                           count;
  sf_ui_style_t                     default_style;
  sf_ui_lmn_t                      *focused;
  sf_ui_lmn_t                      *active_panel;
} sf_ui_t;

typedef enum {
  SF_RENDER_NORMAL                  = 0,
  SF_RENDER_WIREFRAME,
  SF_RENDER_DEPTH,
  SF_RENDER_MODE_COUNT
} sf_render_mode_t;

struct sf_ctx_t_ {
  sf_run_state_t                    state;

  sf_arena_t                        arena;
  int                               arena_size;

  sf_frame_t                       *roots[SF_CONV_MAX];
  sf_frame_t                       *frames;
  int32_t                           frames_count;
  sf_frame_t                       *free_frames;
  sf_obj_t                         *objs;
  int32_t                           obj_count;
  sf_enti_t                        *entities;
  int32_t                           enti_count;
  sf_tex_t                         *textures;
  int32_t                           tex_count;
  sf_cam_t                         *cameras;
  int32_t                           cam_count;
  sf_sprite_t                      *sprites;
  int32_t                           sprite_count;
  sf_sprite_3d_t                        *sprite_3ds;
  int32_t                           sprite_3d_count;
  sf_emitr_t                       *emitrs;
  int32_t                           emitr_count;
  sf_skybox_t                      *skyboxes;
  int32_t                           skybox_count;
  sf_skybox_t                      *active_skybox;
  bool                              skybox_enabled;

  bool                              fog_enabled;
  sf_fvec3_t                        fog_color;
  float                             fog_start;
  float                             fog_end;
  sf_render_mode_t                  render_mode;

  sf_light_t                       *lights;
  int32_t                           light_count;

  sf_ui_t                          *ui;

  sf_input_state_t                  input;
  sf_callback_entry_t               callbacks[SF_EVT_MAX][SF_MAX_CB_PER_EVT];

  sf_cam_t                          main_camera;

  float                             delta_time;
  float                             elapsed_time;
  float                             fps;
  uint32_t                          frame_count;
  uint64_t                          _start_ticks;
  uint64_t                          _last_ticks;

  float                             _perf_dt_hist[SF_PERF_HIST_SIZE];
  int                               _perf_dt_idx;
  int                               _perf_tri_count;

  sf_log_fn                         log_cb;
  void*                             log_user;
  sf_log_level_t                    log_min;
};

/* SF_CORE_FUNCTIONS */
void           sf_init              (sf_ctx_t *ctx, int w, int h);
void           sf_destroy           (sf_ctx_t *ctx);
bool           sf_running           (sf_ctx_t *ctx);
void           sf_stop              (sf_ctx_t *ctx);
void           sf_render_enti       (sf_ctx_t *ctx, sf_cam_t *cam, sf_enti_t *enti);
void           sf_render_ctx        (sf_ctx_t *ctx);
void           sf_render_cam        (sf_ctx_t *ctx, sf_cam_t *cam);
void           sf_render_emitrs     (sf_ctx_t *ctx, sf_cam_t *cam);
void           sf_render_skybox     (sf_ctx_t *ctx, sf_cam_t *cam);
void           sf_render_fog        (sf_ctx_t *ctx, sf_cam_t *cam);
void           sf_render_depth      (sf_ctx_t *ctx, sf_cam_t *cam);
void           sf_update_emitrs     (sf_ctx_t *ctx);
void           sf_time_update       (sf_ctx_t *ctx);

/* SF_MEMORY_FUNCTIONS */
sf_arena_t     sf_arena_init        (sf_ctx_t *ctx, size_t size);
void*          sf_arena_alloc       (sf_ctx_t *ctx, sf_arena_t *arena, size_t size);
size_t         sf_arena_save        (sf_ctx_t *ctx, sf_arena_t *arena);
void           sf_arena_restore     (sf_ctx_t *ctx, sf_arena_t *arena, size_t mark);
size_t         _sf_obj_memusg       (sf_obj_t *obj);
char*          _sf_arena_strdup     (sf_ctx_t *ctx, const char *s);

/* SF_EVENT_FUNCTIONS */
void           sf_event_reg         (sf_ctx_t *ctx, sf_event_type_t type, sf_event_cb cb, void *userdata);
void           sf_event_trigger     (sf_ctx_t *ctx, const sf_event_t *event);
void           sf_input_cycle_state (sf_ctx_t *ctx);
void           sf_input_set_key     (sf_ctx_t *ctx, sf_key_t key, bool is_down);
void           sf_input_set_mouse_p (sf_ctx_t *ctx, int x, int y);
void           sf_input_set_mouse_b (sf_ctx_t *ctx, sf_mouse_btn_t btn, bool is_down);
void           sf_input_set_wheel   (sf_ctx_t *ctx, int dy);
void           sf_input_set_text    (sf_ctx_t *ctx, const char *txt);
bool           sf_key_down          (sf_ctx_t *ctx, sf_key_t key);
bool           sf_key_pressed       (sf_ctx_t *ctx, sf_key_t key);

/* SF_SCENE_FUNCTIONS */
sf_tex_t*      sf_load_texture_bmp  (sf_ctx_t *ctx, const char *filename, const char *texname);
sf_tex_t*      sf_get_texture_      (sf_ctx_t *ctx, const char *texname, bool should_log_failure);
sf_sprite_t*   sf_load_sprite       (sf_ctx_t *ctx, const char *spritename, float duration, float scale, int frame_count, ...);
sf_sprite_t*   sf_get_sprite_       (sf_ctx_t *ctx, const char *spritename, bool should_log_failure);
sf_emitr_t*    sf_add_emitr         (sf_ctx_t *ctx, const char *emitrname, sf_emitr_type_t type, sf_sprite_t *sprite, int max_p);
sf_emitr_t*    sf_get_emitr_        (sf_ctx_t *ctx, const char *emitrname, bool should_log_failure);
sf_obj_t*      sf_load_obj          (sf_ctx_t *ctx, const char *filename, const char *objname);
sf_obj_t*      sf_get_obj_          (sf_ctx_t *ctx, const char *objname, bool should_log_failure);
sf_enti_t*     sf_add_enti          (sf_ctx_t *ctx, sf_obj_t *obj, const char *entiname);
sf_enti_t*     sf_get_enti_         (sf_ctx_t *ctx, const char *entiname, bool should_log_failure);
sf_cam_t*      sf_add_cam           (sf_ctx_t *ctx, const char *camname, int w, int h, float fov);
sf_cam_t*      sf_get_cam_          (sf_ctx_t *ctx, const char *camname, bool should_log_failure);
sf_light_t*    sf_add_light         (sf_ctx_t *ctx, const char *lightname, sf_light_type_t type, sf_fvec3_t color, float intensity);
sf_light_t*    sf_get_light_        (sf_ctx_t *ctx, const char *lightname, bool should_log_failure);
void           sf_set_fog           (sf_ctx_t *ctx, sf_fvec3_t color, float start, float end);
sf_skybox_t*   sf_load_skybox       (sf_ctx_t *ctx, const char *filename, const char *skyboxname);
sf_skybox_t*   sf_get_skybox_       (sf_ctx_t *ctx, const char *skyboxname, bool should_log_failure);
void           sf_set_active_skybox (sf_ctx_t *ctx, sf_skybox_t *skybox);
void           sf_enti_set_pos      (sf_ctx_t *ctx, sf_enti_t *enti, float x, float y, float z);
void           sf_enti_move         (sf_ctx_t *ctx, sf_enti_t *enti, float dx, float dy, float dz);
void           sf_enti_set_rot      (sf_ctx_t *ctx, sf_enti_t *enti, float rx, float ry, float rz);
void           sf_enti_rotate       (sf_ctx_t *ctx, sf_enti_t *enti, float drx, float dry, float drz);
void           sf_enti_set_scale    (sf_ctx_t *ctx, sf_enti_t *enti, float sx, float sy, float sz);
void           sf_enti_set_tex      (sf_ctx_t *ctx, const char *entiname, const char *texname);
void           sf_obj_recenter      (sf_obj_t *obj);
void           sf_camera_set_psp    (sf_ctx_t *ctx, sf_cam_t *cam, float fov, float near_plane, float far_plane);
void           sf_camera_set_pos    (sf_ctx_t *ctx, sf_cam_t *cam, float x, float y, float z);
void           sf_camera_move_loc   (sf_ctx_t *ctx, sf_cam_t *cam, float fwd, float right, float up);
void           sf_camera_look_at    (sf_ctx_t *ctx, sf_cam_t *cam, sf_fvec3_t target);
void           sf_camera_add_yp     (sf_ctx_t *ctx, sf_cam_t *cam, float yaw_offset, float pitch_offset);
void           sf_load_sff          (sf_ctx_t *ctx, const char *filename, const char *worldname);
bool           sf_save_sff          (sf_ctx_t *ctx, const char *filepath);
bool           _sf_sff_read_kv      (FILE *f, char *key, size_t ksz, char *val, size_t vsz);
void           _sf_sff_trim         (char *s);
sf_fvec3_t     _sf_sff_prse_vec3    (const char *s);
int            _sf_sff_prse_list    (const char *s, char out[][64], int max);
sf_frame_t*    _sf_sff_get_frame_   (sf_ctx_t *ctx, const char *name);
void           _sf_sff_prse_frame   (sf_ctx_t *ctx, FILE *f, const char *name, int *frame_count);
void           _sf_sff_prse_cam     (sf_ctx_t *ctx, FILE *f, const char *name, int *cam_count);
void           _sf_sff_prse_enti    (sf_ctx_t *ctx, FILE *f, const char *name, int *enti_count);
void           _sf_sff_prse_light   (sf_ctx_t *ctx, FILE *f, const char *name, int *light_count);
void           _sf_sff_prse_sprit   (sf_ctx_t *ctx, FILE *f, const char *name, int *sprite_count);
void           _sf_sff_prse_emitr   (sf_ctx_t *ctx, FILE *f, const char *name, int *emitr_count);
void           _sf_sff_prse_sprit_3d(sf_ctx_t *ctx, FILE *f, const char *name, int *sprite_3d_count);

/* SF_FRAME_FUNCTIONS */
sf_frame_t*    sf_get_root          (sf_ctx_t *ctx, sf_convention_t conv);
sf_frame_t*    sf_add_frame         (sf_ctx_t *ctx, sf_frame_t *parent);
void           sf_update_frames     (sf_ctx_t *ctx);
void           sf_frame_look_at     (sf_frame_t *f, sf_fvec3_t target);
void           sf_frame_set_parent  (sf_frame_t *child, sf_frame_t *new_parent);
void           sf_remove_frame      (sf_ctx_t *ctx, sf_frame_t *f);
void           _sf_set_up_frames    (sf_ctx_t *ctx);
void           _sf_calc_frame_tree  (sf_frame_t *f, sf_fmat4_t parent_global_M, bool force_dirty);
void           _sf_write_frame_ref  (FILE *f, sf_frame_t *fr, sf_ctx_t *ctx);

/* SF_DRAWING_FUNCTIONS */
void           sf_fill              (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c);
void           sf_pixel             (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0);
void           sf_pixel_depth       (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, float z);
void           sf_line              (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1);
void           sf_rect              (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1);
void           sf_tri               (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, bool use_depth);
void           sf_tri_tex           (sf_ctx_t *ctx, sf_cam_t *cam, sf_tex_t *tex, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, sf_fvec3_t uvz0, sf_fvec3_t uvz1, sf_fvec3_t uvz2, sf_fvec3_t l_int, float opacity);
void           sf_put_text          (sf_ctx_t *ctx, sf_cam_t *cam, const char *text, sf_ivec2_t p, sf_pkd_clr_t c, int scale);
void           sf_clear_depth       (sf_ctx_t *ctx, sf_cam_t *cam);
void           sf_draw_cam_pip      (sf_ctx_t *ctx, sf_cam_t *dest, sf_cam_t *src, sf_ivec2_t pos);
void           sf_draw_debug_ovrlay (sf_ctx_t *ctx, sf_cam_t *cam);
void           sf_draw_debug_axes   (sf_ctx_t *ctx, sf_cam_t *cam);
void           sf_draw_debug_frames (sf_ctx_t *ctx, sf_cam_t *cam, float axis_size);
void           sf_draw_debug_lights (sf_ctx_t *ctx, sf_cam_t *cam, float size);
void           sf_draw_debug_cams   (sf_ctx_t *ctx, sf_cam_t *view_cam, float ray_len);
void           sf_draw_debug_perf   (sf_ctx_t *ctx, sf_cam_t *cam);
void           sf_draw_sprite       (sf_ctx_t *ctx, sf_cam_t *cam, sf_sprite_t *spr, sf_fvec3_t pos_w, float anim_time, float scale_mult);
void           sf_draw_sprite_3d    (sf_ctx_t *ctx, sf_cam_t *cam, sf_sprite_3d_t *bill, float anim_time);
sf_sprite_3d_t*     sf_add_sprite_3d(sf_ctx_t *ctx, sf_sprite_t *spr, const char *name, sf_fvec3_t pos, float scale, float opacity, float angle);
void           sf_clear_sprite_3ds  (sf_ctx_t *ctx);

/* SF_UI_FUNCTIONS */
sf_ui_t*       sf_ui_create         (sf_ctx_t *ctx);
void           sf_ui_update         (sf_ctx_t *ctx, sf_ui_t *ui);
void           sf_ui_render         (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_t *ui);
void           sf_ui_render_popups  (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_t *ui);
void           sf_ui_clear          (sf_ctx_t *ctx);
sf_ui_lmn_t*   sf_ui_get_by_name    (sf_ui_t *ui, const char *name);
void           sf_ui_set_callback   (sf_ui_lmn_t *el, sf_ui_cb cb, void *userdata);
sf_ui_lmn_t*   sf_ui_add_button     (sf_ctx_t *ctx, const char *text, sf_ivec2_t v0, sf_ivec2_t v1, sf_ui_cb cb, void *userdata);
sf_ui_lmn_t*   sf_ui_add_slider     (sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, float min_val, float max_val, float init_val, sf_ui_cb cb, void *userdata);
sf_ui_lmn_t*   sf_ui_add_checkbox   (sf_ctx_t *ctx, const char *text, sf_ivec2_t v0, sf_ivec2_t v1, bool init_state, sf_ui_cb cb, void *userdata);
sf_ui_lmn_t*   sf_ui_add_label      (sf_ctx_t *ctx, const char *text, sf_ivec2_t pos, sf_pkd_clr_t color);
sf_ui_lmn_t*   sf_ui_add_text_input (sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, char *buf, int buflen, sf_ui_cb cb, void *ud);
sf_ui_lmn_t*   sf_ui_add_drag_float (sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, float *target, float step, sf_ui_cb cb, void *ud);
sf_ui_lmn_t*   sf_ui_add_dropdown   (sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, const char **items, int n, int *selected, sf_ui_cb cb, void *ud);
sf_ui_lmn_t*   sf_ui_add_panel      (sf_ctx_t *ctx, const char *title, sf_ivec2_t v0, sf_ivec2_t v1);
sf_ui_lmn_t*   sf_ui_add_image     (sf_ctx_t *ctx, sf_tex_t *tex, sf_ivec2_t v0, sf_ivec2_t v1, bool keyed);
bool           sf_save_sfui         (sf_ctx_t *ctx, sf_ui_t *ui, const char *filepath);
sf_ui_t*       sf_load_sfui         (sf_ctx_t *ctx, const char *filepath);
sf_ui_lmn_t*   _sf_ui_find_prnt_pnl (sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1);
bool           _sf_ui_eff_visible   (sf_ui_lmn_t *el);
void           _sf_draw_button      (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
void           _sf_draw_slider      (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
void           _sf_draw_checkbox    (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
void           _sf_draw_label       (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
void           _sf_draw_text_input  (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
void           _sf_draw_drag_float  (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
void           _sf_draw_dropdown    (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
void           _sf_draw_drpdwn_popup(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
void           _sf_draw_panel       (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
void           _sf_update_text_input(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_pressed);
void           _sf_update_drag_float(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed);
void           _sf_update_dropdown  (sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_pressed);
void           _sf_update_panel     (sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_pressed);
void           _sf_update_button    (sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released);
void           _sf_update_checkbox  (sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released);
void           _sf_update_slider    (sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released);
const char*    _sf_ui_type_str      (sf_ui_type_t t);
int            _sf_ui_type_from_str (const char *s);

/* SF_MESH_AUTHORING_FUNCTIONS */
sf_obj_t*      sf_obj_create_empty  (sf_ctx_t *ctx, const char *objname, int max_v, int max_vt, int max_f);
int            sf_obj_add_vert      (sf_obj_t *obj, sf_fvec3_t p);
int            sf_obj_add_uv        (sf_obj_t *obj, sf_fvec2_t uv);
int            sf_obj_add_face      (sf_obj_t *obj, int i0, int i1, int i2);
int            sf_obj_add_face_uv   (sf_obj_t *obj, int v0, int v1, int v2, int t0, int t1, int t2);
void           sf_obj_recompute_bs  (sf_obj_t *obj);
sf_obj_t*      sf_obj_make_plane    (sf_ctx_t *ctx, const char *objname, float sx, float sz, int res);
sf_obj_t*      sf_obj_make_box      (sf_ctx_t *ctx, const char *objname, float sx, float sy, float sz);
sf_obj_t*      sf_obj_make_sphere   (sf_ctx_t *ctx, const char *objname, float radius, int segs);
sf_obj_t*      sf_obj_make_cyl      (sf_ctx_t *ctx, const char *objname, float radius, float height, int segs);
sf_obj_t*      sf_obj_make_heightmap(sf_ctx_t *ctx, const char *objname, float size_x, float size_z, int res, sf_height_fn fn, void *ud);
bool           sf_obj_save_obj      (sf_ctx_t *ctx, sf_obj_t *obj, const char *filepath);
float          sf_noise_fbm         (float x, float z, int oct, float lac, float gain, uint32_t seed);

/* SF_PICKING_FUNCTIONS */
sf_ray_t       sf_ray_from_screen   (sf_ctx_t *ctx, sf_cam_t *cam, int sx, int sy);
sf_enti_t*     sf_raycast_entities  (sf_ctx_t *ctx, sf_ray_t ray, float *out_t);
bool           sf_ray_triangle      (sf_ray_t r, sf_fvec3_t a, sf_fvec3_t b, sf_fvec3_t c, float *out_t);
bool           sf_ray_plane_y       (sf_ray_t r, float y, sf_fvec3_t *out);
bool           sf_ray_aabb          (sf_ray_t r, sf_fvec3_t bmin, sf_fvec3_t bmax, float *out_t);

/* SF_LOG_FUNCTIONS */
void           sf_log_              (sf_ctx_t *ctx, sf_log_level_t level, const char* func, const char* fmt, ...);
void           sf_set_logger        (sf_ctx_t *ctx, sf_log_fn log_cb, void* userdata);
void           sf_logger_console    (const char* message, void* userdata);
const char*    _sf_log_lvl_to_str   (sf_log_level_t level);

/* SF_MATH_FUNCTIONS */
sf_fmat4_t     sf_fmat4_mul_fmat4   (sf_fmat4_t m0, sf_fmat4_t m1);
sf_fvec3_t     sf_fmat4_mul_vec3    (sf_fmat4_t m, sf_fvec3_t v);
sf_fvec3_t     sf_fvec3_sub         (sf_fvec3_t v0, sf_fvec3_t v1);
sf_fvec3_t     sf_fvec3_add         (sf_fvec3_t v0, sf_fvec3_t v1);
sf_fvec3_t     sf_fvec3_norm        (sf_fvec3_t v);
sf_fvec3_t     sf_fvec3_cross       (sf_fvec3_t v0, sf_fvec3_t v1);
float          sf_fvec3_dot         (sf_fvec3_t v0, sf_fvec3_t v1);
sf_fmat4_t     sf_make_tsl_fmat4    (float x, float y, float z);
sf_fmat4_t     sf_make_rot_fmat4    (sf_fvec3_t angles);
sf_fmat4_t     sf_make_psp_fmat4    (float fov_deg, float aspect, float near, float far);
sf_fmat4_t     sf_make_idn_fmat4    (void);
sf_fmat4_t     sf_make_view_fmat4   (sf_fvec3_t eye, sf_fvec3_t target, sf_fvec3_t up);
sf_fmat4_t     sf_make_scale_fmat4  (sf_fvec3_t scale);
uint32_t       _sf_vec_to_index     (sf_ctx_t *ctx, sf_cam_t *cam, sf_ivec2_t v);
void           _sf_swap_svec2       (sf_ivec2_t *v0, sf_ivec2_t *v1);
void           _sf_swap_fvec3       (sf_fvec3_t *v0, sf_fvec3_t *v1);
float          _sf_lerp_f           (float a, float b, float t);
sf_fvec3_t     _sf_lerp_fvec3       (sf_fvec3_t a, sf_fvec3_t b, float t);
sf_fvec3_t     _sf_intersect_near   (sf_fvec3_t v0, sf_fvec3_t v1, float near);
sf_fvec3_t     _sf_project_vertex   (sf_ctx_t *ctx, sf_cam_t *cam, sf_fvec3_t v, sf_fmat4_t P);
float          _sf_hash_2d          (int x, int z, uint32_t seed);
float          _sf_smooth_noise     (float x, float z, uint32_t seed);

/* SF_IMPLEMENTATION_HELPERS */
bool           _sf_resolve_asset    (const char* filename, char* out_path, size_t max_len);
const char*    _sf_basename         (const char *path);
uint64_t       _sf_get_ticks        (void);
sf_pkd_clr_t   _sf_pack_color       (sf_unpkd_clr_t);
sf_unpkd_clr_t _sf_unpack_color     (sf_pkd_clr_t);
void           _sf_write_qstr       (FILE *f, const char *s);
bool           _sf_parse_qstr       (const char *line, const char *key, char *out, size_t outsz);

/* SF_GAMMA_LUT */
static const uint8_t                _sf_gamma_lut[256];
static const uint8_t                _sf_degamma_lut[256];

/* SF_FONT_DATA */
static const uint8_t                _sf_font_8x8[];

#ifdef __cplusplus
}
#endif
#endif /* SAFFRON_H */

/* SF_IMPLEMENTATION */
#ifdef SAFFRON_IMPLEMENTATION

/* SF_CORE_FUNCTIONS */
void sf_init(sf_ctx_t *ctx, int w, int h) {
  /* Initialize the engine context: allocate arena, all scene arrays, main camera pixel/z buffers, and default UI. */
  memset(ctx, 0, sizeof(sf_ctx_t));
  ctx->state                        = SF_RUN_STATE_RUNNING;
  ctx->main_camera.w                = w;
  ctx->main_camera.h                = h;
  ctx->main_camera.buffer_size      = w * h;
  ctx->main_camera.buffer           = (sf_pkd_clr_t*) malloc(w*h*sizeof(sf_pkd_clr_t));
  ctx->main_camera.z_buffer         = (float*)        malloc(w*h*sizeof(float));
  ctx->main_camera.fov              = 60.0f;
  ctx->main_camera.near_plane       = 0.1f;
  ctx->main_camera.far_plane        = 100.0f;
  ctx->main_camera.is_proj_dirty    = true;
  ctx->arena                        = sf_arena_init(ctx, SF_ARENA_SIZE);
  ctx->log_cb                       = sf_logger_console;
  ctx->log_user                     = NULL;
  ctx->log_min                      = SF_LOG_INFO;
  ctx->objs                         = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_OBJS     * sizeof(sf_obj_t));
  ctx->entities                     = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_ENTITIES * sizeof(sf_enti_t));
  ctx->lights                       = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_LIGHTS   * sizeof(sf_light_t));
  ctx->textures                     = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_TEXTURES * sizeof(sf_tex_t));
  ctx->cameras                      = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_CAMS     * sizeof(sf_cam_t));
  ctx->frames                       = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_FRAMES   * sizeof(sf_frame_t));
  ctx->sprites                      = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_SPRITES  * sizeof(sf_sprite_t));
  ctx->emitrs                       = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_EMITRS   * sizeof(sf_emitr_t));
  ctx->sprite_3ds                        = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_SPRITE_3DS    * sizeof(sf_sprite_3d_t));
  ctx->skyboxes                     = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_SKYBOXES * sizeof(sf_skybox_t));
  ctx->obj_count                    = 0;
  ctx->enti_count                   = 0;
  ctx->light_count                  = 0;
  ctx->tex_count                    = 0;
  ctx->cam_count                    = 0;
  ctx->frames_count                 = 0;
  ctx->free_frames                  = NULL;
  ctx->sprite_count                 = 0;
  ctx->sprite_3d_count                   = 0;
  ctx->emitr_count                  = 0;
  ctx->skybox_count                 = 0;
  ctx->active_skybox                = NULL;
  ctx->fog_enabled                  = false;
  ctx->skybox_enabled               = false;
  ctx->fog_color                    = (sf_fvec3_t){ 0.0, 0.0, 0.0 };
  ctx->fog_start                    = 12.0f;
  ctx->fog_end                      = 18.0f;
  ctx->render_mode                  = SF_RENDER_NORMAL;
  ctx->_start_ticks                 = _sf_get_ticks();
  ctx->_last_ticks                  = ctx->_start_ticks;
  ctx->delta_time                   = 0.0f;
  ctx->elapsed_time                 = 0.0f;
  ctx->fps                          = 0.0f;
  ctx->frame_count                  = 0;
  ctx->ui                           = sf_ui_create(ctx);
  _sf_set_up_frames(ctx);
  ctx->main_camera.frame                = sf_add_frame(ctx, NULL);

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "buffer : %dx%d\n"
              SF_LOG_INDENT "memory : %d\n"
              SF_LOG_INDENT "mxobjs : %d\n"
              SF_LOG_INDENT "mxents : %d\n",
              ctx->main_camera.w, ctx->main_camera.h, SF_ARENA_SIZE, SF_MAX_OBJS, SF_MAX_ENTITIES);
}

void sf_destroy(sf_ctx_t *ctx) {
  /* Free all heap allocations (pixel buffers, arena) and log session stats. */
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
  free(ctx->main_camera.buffer);
  free(ctx->main_camera.z_buffer);
  free(ctx->arena.buffer);

  ctx->state                        = SF_RUN_STATE_STOPPED;
  ctx->arena.offset                 = 0;
  ctx->main_camera.buffer_size      = 0;
  ctx->main_camera.w                = 0;
  ctx->main_camera.h                = 0;
  ctx->enti_count                   = 0;
  ctx->obj_count                    = 0;
  ctx->tex_count                    = 0;
  ctx->light_count                  = 0;
  ctx->cam_count                    = 0;
  ctx->sprite_count                 = 0;
  ctx->emitr_count                  = 0;
}

bool sf_running(sf_ctx_t *ctx) {
  /* Return true while the engine is in the running state; use as the main loop condition. */
  return (ctx->state == SF_RUN_STATE_RUNNING);
}

void sf_stop(sf_ctx_t *ctx) {
  /* Signal the engine to stop; sf_running() will return false on the next check. */
  ctx->state = SF_RUN_STATE_STOPPED;
}

void sf_render_enti(sf_ctx_t *ctx, sf_cam_t *cam, sf_enti_t *enti) {
  /* Rasterize one entity into cam: frustum-cull, light, near-clip, then draw textured or flat triangles. */
  if (!enti || !enti->frame) return;

  sf_fmat4_t M = enti->frame->global_M;
  sf_fmat4_t V = cam->V;
  sf_fmat4_t P = cam->P;
  sf_fmat4_t MV = sf_fmat4_mul_fmat4(M, V);

  float sx = sqrtf(M.m[0][0]*M.m[0][0] + M.m[0][1]*M.m[0][1] + M.m[0][2]*M.m[0][2]);
  float sy = sqrtf(M.m[1][0]*M.m[1][0] + M.m[1][1]*M.m[1][1] + M.m[1][2]*M.m[1][2]);
  float sz = sqrtf(M.m[2][0]*M.m[2][0] + M.m[2][1]*M.m[2][1] + M.m[2][2]*M.m[2][2]);
  float max_s = sx > sy ? (sx > sz ? sx : sz) : (sy > sz ? sy : sz);
  float r = enti->obj.bs_radius * max_s;
  sf_fvec3_t c = sf_fmat4_mul_vec3(MV, enti->obj.bs_center);
  float fov_r = cam->fov * 0.01745329f * 0.5f;
  float aspect = (float)cam->w / (float)cam->h;
  float cos_v = cosf(fov_r);
  float sin_v = sinf(fov_r);
  float cos_h = cosf(atanf(tanf(fov_r) * aspect));
  float sin_h = sinf(atanf(tanf(fov_r) * aspect));
  if (c.z - r > -cam->near_plane) { return; }
  if (c.z + r < -cam->far_plane)  { return; }
  if ( c.x * cos_h + c.z * sin_h > r) { return; }
  if (-c.x * cos_h + c.z * sin_h > r) { return; }
  if ( c.y * cos_v + c.z * sin_v > r) { return; }
  if (-c.y * cos_v + c.z * sin_v > r) { return; }

  ctx->_perf_tri_count += enti->obj.f_cnt;

  size_t mark = sf_arena_save(ctx, &ctx->arena);
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
        float dist_sq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
        float dist = sqrtf(dist_sq);
        float inv_dist = (dist > 0.0f) ? 1.0f / dist : 0.0f;
        light_dir = (sf_fvec3_t){ diff.x * inv_dist, diff.y * inv_dist, diff.z * inv_dist };
        atten = 1.0f / (1.0f + 0.09f * dist + 0.032f * dist_sq);
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
      for (int j = 0; j < 3; j++) {
        uvs[j].x *= enti->tex_scale.x;
        uvs[j].y *= enti->tex_scale.y;
      }
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
    if (ctx->render_mode == SF_RENDER_WIREFRAME) {
      if (inc > 0) {
        sf_pkd_clr_t wclr = 0xFF44FF44u;
        bool vis0 = (v_view[0].z <= -near), vis1 = (v_view[1].z <= -near), vis2 = (v_view[2].z <= -near);
        sf_fvec3_t sv0 = _sf_project_vertex(ctx, cam, v_view[0], P);
        sf_fvec3_t sv1 = _sf_project_vertex(ctx, cam, v_view[1], P);
        sf_fvec3_t sv2 = _sf_project_vertex(ctx, cam, v_view[2], P);
        if (vis0 && vis1) sf_line(ctx, cam, wclr, (sf_ivec2_t){(int)sv0.x,(int)sv0.y}, (sf_ivec2_t){(int)sv1.x,(int)sv1.y});
        if (vis1 && vis2) sf_line(ctx, cam, wclr, (sf_ivec2_t){(int)sv1.x,(int)sv1.y}, (sf_ivec2_t){(int)sv2.x,(int)sv2.y});
        if (vis2 && vis0) sf_line(ctx, cam, wclr, (sf_ivec2_t){(int)sv2.x,(int)sv2.y}, (sf_ivec2_t){(int)sv0.x,(int)sv0.y});
      }
    } else if (enti->tex && has_uvs) {
      if (inc == 3) {
        sf_tri_tex(ctx, cam, enti->tex, _sf_project_vertex(ctx, cam, in[0], P), _sf_project_vertex(ctx, cam, in[1], P), _sf_project_vertex(ctx, cam, in[2], P), in_uvz[0], in_uvz[1], in_uvz[2], l_int, 1.0f);
      } else if (inc == 1) {
        float t1 = ((-near) - in[0].z) / (out[0].z - in[0].z);
        float t2 = ((-near) - in[0].z) / (out[1].z - in[0].z);
        sf_fvec3_t v1 = { in[0].x + (out[0].x - in[0].x) * t1, in[0].y + (out[0].y - in[0].y) * t1, -near };
        sf_fvec3_t v2 = { in[0].x + (out[1].x - in[0].x) * t2, in[0].y + (out[1].y - in[0].y) * t2, -near };
        sf_fvec3_t uvz1 = { in_uvz[0].x + (out_uvz[0].x - in_uvz[0].x) * t1, in_uvz[0].y + (out_uvz[0].y - in_uvz[0].y) * t1, in_uvz[0].z + (out_uvz[0].z - in_uvz[0].z) * t1 };
        sf_fvec3_t uvz2 = { in_uvz[0].x + (out_uvz[1].x - in_uvz[0].x) * t2, in_uvz[0].y + (out_uvz[1].y - in_uvz[0].y) * t2, in_uvz[0].z + (out_uvz[1].z - in_uvz[0].z) * t2 };
        sf_tri_tex(ctx, cam, enti->tex, _sf_project_vertex(ctx, cam, in[0], P), _sf_project_vertex(ctx, cam, v1, P), _sf_project_vertex(ctx, cam, v2, P), in_uvz[0], uvz1, uvz2, l_int, 1.0f);
      } else if (inc == 2) {
        float t1 = ((-near) - in[0].z) / (out[0].z - in[0].z);
        float t2 = ((-near) - in[1].z) / (out[0].z - in[1].z);
        sf_fvec3_t v1 = { in[0].x + (out[0].x - in[0].x) * t1, in[0].y + (out[0].y - in[0].y) * t1, -near };
        sf_fvec3_t v2 = { in[1].x + (out[0].x - in[1].x) * t2, in[1].y + (out[0].y - in[1].y) * t2, -near };
        sf_fvec3_t uvz1 = { in_uvz[0].x + (out_uvz[0].x - in_uvz[0].x) * t1, in_uvz[0].y + (out_uvz[0].y - in_uvz[0].y) * t1, in_uvz[0].z + (out_uvz[0].z - in_uvz[0].z) * t1 };
        sf_fvec3_t uvz2 = { in_uvz[1].x + (out_uvz[0].x - in_uvz[1].x) * t2, in_uvz[1].y + (out_uvz[0].y - in_uvz[1].y) * t2, in_uvz[1].z + (out_uvz[0].z - in_uvz[1].z) * t2 };
        sf_tri_tex(ctx, cam, enti->tex, _sf_project_vertex(ctx, cam, in[0], P), _sf_project_vertex(ctx, cam, in[1], P), _sf_project_vertex(ctx, cam, v1, P), in_uvz[0], in_uvz[1], uvz1, l_int, 1.0f);
        sf_tri_tex(ctx, cam, enti->tex, _sf_project_vertex(ctx, cam, in[1], P), _sf_project_vertex(ctx, cam, v1, P), _sf_project_vertex(ctx, cam, v2, P), in_uvz[1], uvz1, uvz2, l_int, 1.0f);
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
  /* Update frames and emitters, then render all cameras including the main camera. */
  sf_update_frames(ctx);
  sf_update_emitrs(ctx);
  ctx->_perf_tri_count = 0;
  sf_render_cam(ctx, &ctx->main_camera);
  for (int i = 0; i < ctx->cam_count; ++i) {
    sf_render_cam(ctx, &ctx->cameras[i]);
  }
}

void sf_render_cam(sf_ctx_t *ctx, sf_cam_t *cam) {
  /* Clear and render a single camera: fire RENDER_START/END events, rebuild projection if dirty, draw all entities. */
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

  sf_clear_depth(ctx, cam);
  if (ctx->active_skybox && ctx->skybox_enabled) {
    sf_render_skybox(ctx, cam);
  } else {
    sf_fill(ctx, cam, SF_CLR_BLACK);
  }

  for (int i = 0; i < ctx->enti_count; i++) {
    sf_render_enti(ctx, cam, &ctx->entities[i]);
  }

  for (int i = 0; i < ctx->sprite_3d_count; i++) {
    sf_draw_sprite_3d(ctx, cam, &ctx->sprite_3ds[i], 0.0f);
  }

  sf_render_emitrs(ctx, cam);
  if (ctx->render_mode == SF_RENDER_DEPTH) {
    sf_render_depth(ctx, cam);
  } else if (ctx->render_mode == SF_RENDER_NORMAL && ctx->fog_enabled) {
    sf_render_fog(ctx, cam);
  }

  sf_event_t ev_end;
  ev_end.type = SF_EVT_RENDER_END;
  sf_event_trigger(ctx, &ev_end);
}

void sf_render_emitrs(sf_ctx_t *ctx, sf_cam_t *cam) {
  /* Draw all active particles from every emitter as sprites into cam. */
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

void sf_render_fog(sf_ctx_t *ctx, sf_cam_t *cam) {
  /* Post-process depth fog: blend each geometry pixel toward fog_color based on
   * linearised view-space depth.  Sky/unwritten pixels (z > 2.0) are skipped. */
  if (!ctx->fog_enabled) return;
  float near     = cam->near_plane;
  float far      = cam->far_plane;
  float A        = 2.0f * near * far;
  float B        = far + near;
  float C        = far - near;
  float inv_rng  = 1.0f / (ctx->fog_end - ctx->fog_start);
  uint32_t fr    = (uint32_t)(ctx->fog_color.x * 255.0f + 0.5f);
  uint32_t fg_   = (uint32_t)(ctx->fog_color.y * 255.0f + 0.5f);
  uint32_t fb    = (uint32_t)(ctx->fog_color.z * 255.0f + 0.5f);
  int n = cam->w * cam->h;
  for (int i = 0; i < n; i++) {
    float z = cam->z_buffer[i];
    float t;
    if (z > 2.0f) {
      t = 1.0f;
    } else {
      float depth = A / (B - z * C);
      t = (depth - ctx->fog_start) * inv_rng;
      if (t <= 0.0f) continue;
      if (t >  1.0f) t = 1.0f;
    }
    sf_pkd_clr_t px = cam->buffer[i];
    uint32_t r = (px >> 16) & 0xFF;
    uint32_t g = (px >>  8) & 0xFF;
    uint32_t b =  px        & 0xFF;
    uint32_t it = (uint32_t)(t * 256.0f);
    r = (r * (256u - it) + fr  * it) >> 8;
    g = (g * (256u - it) + fg_ * it) >> 8;
    b = (b * (256u - it) + fb  * it) >> 8;
    cam->buffer[i] = (0xFFu << 24) | (r << 16) | (g << 8) | b;
  }
}

void sf_render_depth(sf_ctx_t *ctx, sf_cam_t *cam) {
  /* Visualise the z-buffer as a heatmap: blue (near) → cyan → green → yellow → red (far/sky).
   * NDC z is highly nonlinear so we linearise to view-space depth first, then apply a power
   * curve to spread the colours usefully across the visible scene range. */
  static const float kr[5] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f};
  static const float kg[5] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f};
  static const float kb[5] = {1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
  float near    = cam->near_plane;
  float far     = cam->far_plane;
  float A       = 2.0f * near * far;
  float B       = far + near;
  float C       = far - near;
  float inv_rng = 1.0f / (far - near);
  int n = cam->w * cam->h;
  for (int i = 0; i < n; i++) {
    float z = cam->z_buffer[i];
    float t;
    if (z > 2.0f) {
      t = 1.0f;
    } else {
      float depth = A / (B - z * C);
      t = (depth - near) * inv_rng;
      if (t < 0.0f) t = 0.0f;
      if (t > 1.0f) t = 1.0f;
      t = powf(t, 0.2f);
    }
    float scaled = t * 4.0f;
    int seg = (int)scaled;
    if (seg >= 4) seg = 3;
    float f = scaled - (float)seg;
    uint8_t r = (uint8_t)((kr[seg] + (kr[seg+1] - kr[seg]) * f) * 255.0f + 0.5f);
    uint8_t g = (uint8_t)((kg[seg] + (kg[seg+1] - kg[seg]) * f) * 255.0f + 0.5f);
    uint8_t b = (uint8_t)((kb[seg] + (kb[seg+1] - kb[seg]) * f) * 255.0f + 0.5f);
    cam->buffer[i] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  }
}

void sf_set_fog(sf_ctx_t *ctx, sf_fvec3_t color, float start, float end) {
  /* Configure and enable fog.  Toggle off/on with ctx->fog_enabled = false/true. */
  ctx->fog_color   = color;
  ctx->fog_start   = start;
  ctx->fog_end     = end;
  ctx->fog_enabled = true;
}

void sf_render_skybox(sf_ctx_t *ctx, sf_cam_t *cam) {
  /* Fill the camera buffer with an equirectangular sky panorama using span interpolation.
   * Exact UVs are computed every SF_SKYBOX_SPAN pixels and linearly interpolated between. */
  if (!ctx->active_skybox || !ctx->active_skybox->tex) {
    sf_fill(ctx, cam, SF_CLR_BLACK);
    return;
  }
  sf_tex_t *tex = ctx->active_skybox->tex;
  float rx = cam->V.m[0][0], ry = cam->V.m[1][0], rz = cam->V.m[2][0];
  float ux = cam->V.m[0][1], uy = cam->V.m[1][1], uz = cam->V.m[2][1];
  float fx = -cam->V.m[0][2], fy = -cam->V.m[1][2], fz = -cam->V.m[2][2];
  float inv_px  = 1.0f / cam->P.m[0][0];
  float inv_py  = 1.0f / cam->P.m[1][1];
  float inv_w   = 1.0f / (float)cam->w;
  float inv_h   = 1.0f / (float)cam->h;
  float inv_2pi = 1.0f / (2.0f * SF_PI);
  float inv_pi  = 1.0f / SF_PI;
  float fw      = (float)tex->w;
  float fh      = (float)tex->h;
  float step_x  = rx * 2.0f * inv_px * inv_w;
  float step_y  = ry * 2.0f * inv_px * inv_w;
  float step_z  = rz * 2.0f * inv_px * inv_w;

  for (int py = 0; py < cam->h; py++) {
    float ndc_y = 1.0f - (2.0f * ((float)py + 0.5f)) * inv_h;
    float vd_y  = ndc_y * inv_py;
    float vd_x0 = (inv_w - 1.0f) * inv_px;
    float wd_x  = vd_x0 * rx + vd_y * ux + fx;
    float wd_y_ = vd_x0 * ry + vd_y * uy + fy;
    float wd_z  = vd_x0 * rz + vd_y * uz + fz;
    sf_pkd_clr_t *row = &cam->buffer[py * cam->w];
    for (int px = 0; px < cam->w; px += SF_SKYBOX_SPAN) {
      int span_len = cam->w - px;
      if (span_len > SF_SKYBOX_SPAN) span_len = SF_SKYBOX_SPAN;
      float xz0 = sqrtf(wd_x * wd_x + wd_z * wd_z);
      float u0  = atan2f(wd_x, wd_z) * inv_2pi + 0.5f;
      float v0  = atan2f(wd_y_, xz0)  * inv_pi  + 0.5f;
      float ex  = wd_x  + step_x * (float)span_len;
      float ey  = wd_y_ + step_y * (float)span_len;
      float ez  = wd_z  + step_z * (float)span_len;
      float xz1 = sqrtf(ex * ex + ez * ez);
      float u1  = atan2f(ex, ez) * inv_2pi + 0.5f;
      float v1  = atan2f(ey, xz1) * inv_pi  + 0.5f;
      if (u1 - u0 >  0.5f) u1 -= 1.0f;
      if (u0 - u1 >  0.5f) u1 += 1.0f;
      float inv_span = 1.0f / (float)span_len;
      float du = (u1 - u0) * inv_span;
      float dv = (v1 - v0) * inv_span;
      float u  = u0, v = v0;
      for (int i = 0; i < span_len; i++) {
        int tx = (int)(u * fw) & tex->w_mask;
        int ty = (int)((1.0f - v) * fh) & tex->h_mask;
        row[px + i] = tex->px[ty * tex->w + tx];
        u += du; v += dv;
      }
      wd_x = ex; wd_y_ = ey; wd_z = ez;
    }
  }
}

void sf_update_emitrs(sf_ctx_t *ctx) {
  /* Advance particle lifetimes, move them by velocity, and spawn new particles according to rate. */
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
  /* Sample the high-resolution clock to compute delta_time, elapsed_time, smoothed FPS, and frame count. */
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
  ctx->_perf_dt_hist[ctx->_perf_dt_idx] = ctx->delta_time;
  ctx->_perf_dt_idx = (ctx->_perf_dt_idx + 1) % SF_PERF_HIST_SIZE;
}

/* SF_MEMORY_FUNCTIONS */
sf_arena_t sf_arena_init(sf_ctx_t *ctx, size_t size) {
  /* Allocate a new arena of the given byte size; all subsequent allocs bump a single pointer. */
  sf_arena_t arena;
  arena.size = size;
  arena.offset = 0;
  arena.buffer = malloc(size);
  return arena;
}

void* sf_arena_alloc(sf_ctx_t *ctx, sf_arena_t *arena, size_t size) {
  /* Bump-allocate size bytes from arena with alignment; returns NULL and logs error if full. */
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
  /* Return the current arena offset as a rewind mark for temporary allocations. */
  return arena->offset;
}

void sf_arena_restore(sf_ctx_t *ctx, sf_arena_t *arena, size_t mark) {
  /* Rewind arena to a previously saved mark, freeing everything allocated since. */
  arena->offset = mark;
}

size_t _sf_obj_memusg(sf_obj_t *obj) {
  /* Return the total arena bytes consumed by an obj's vertex, UV, normal, and face arrays. */
  if (!obj) return 0;
  size_t v_size  = obj->v_cnt * sizeof(sf_fvec3_t);
  size_t vt_size = obj->vt_cnt * sizeof(sf_fvec2_t);
  size_t vn_size = obj->vn_cnt * sizeof(sf_fvec3_t);
  size_t f_size  = obj->f_cnt * sizeof(sf_face_t);
  return v_size + vt_size + vn_size + f_size;
}

char* _sf_arena_strdup(sf_ctx_t *ctx, const char *s) {
  /* Duplicate a C string into the arena; returns NULL for a NULL input. */
  if (!s) return NULL;
  size_t n = strlen(s) + 1;
  char *m = (char*)sf_arena_alloc(ctx, &ctx->arena, n);
  if (m) memcpy(m, s, n);
  return m;
}

/* SF_EVENT_FUNCTIONS */
void sf_event_reg(sf_ctx_t *ctx, sf_event_type_t type, sf_event_cb cb, void *userdata) {
  /* Register a callback to be fired whenever the given event type is triggered. */
  if (type >= SF_EVT_MAX) return;
  for (int i = 0; i < SF_MAX_CB_PER_EVT; ++i) {
    if (ctx->callbacks[type][i].cb == NULL) {
      ctx->callbacks[type][i].cb = cb;
      ctx->callbacks[type][i].userdata = userdata;
      return;
    }
  }
  SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "callback slots full for event type %d, max %d\n", type, SF_MAX_CB_PER_EVT);
}

void sf_event_trigger(sf_ctx_t *ctx, const sf_event_t *event) {
  /* Invoke all registered callbacks for the given event. */
  if (event->type >= SF_EVT_MAX) return;
  for (int i = 0; i < SF_MAX_CB_PER_EVT; ++i) {
    if (ctx->callbacks[event->type][i].cb) {
      ctx->callbacks[event->type][i].cb(ctx, event, ctx->callbacks[event->type][i].userdata);
    }
  }
}

void sf_input_cycle_state(sf_ctx_t *ctx) {
  /* Snapshot current key/mouse state to _prev arrays and reset per-frame deltas; call once per frame before polling. */
  memcpy(ctx->input.keys_prev, ctx->input.keys, sizeof(ctx->input.keys));
  memcpy(ctx->input.mouse_btns_prev, ctx->input.mouse_btns, sizeof(ctx->input.mouse_btns));
  ctx->input.mouse_dx = 0;
  ctx->input.mouse_dy = 0;
  ctx->input.wheel_dy = 0;
}

void sf_input_set_wheel(sf_ctx_t *ctx, int dy) {
  /* Record a scroll-wheel delta and fire SF_EVT_MOUSE_WHEEL. */
  ctx->input.wheel_dy += dy;
  sf_event_t ev;
  ev.type = SF_EVT_MOUSE_WHEEL;
  ev.wheel.dy = dy;
  sf_event_trigger(ctx, &ev);
}

void sf_input_set_text(sf_ctx_t *ctx, const char *txt) {
  /* Feed a text-input string into the engine: fire SF_EVT_TEXT_INPUT and insert printable chars into the focused text-input element. */
  sf_event_t ev;
  ev.type = SF_EVT_TEXT_INPUT;
  int i = 0;
  while (i < 7 && txt[i]) { ev.text.text[i] = txt[i]; i++; }
  ev.text.text[i] = '\0';
  sf_event_trigger(ctx, &ev);

  if (ctx->ui && ctx->ui->focused && ctx->ui->focused->type == SF_UI_TEXT_INPUT) {
    sf_ui_lmn_t *el = ctx->ui->focused;
    if (!el->text_input.buf || el->text_input.buflen <= 1) return;
    int len = (int)strlen(el->text_input.buf);
    int caret = el->text_input.caret;
    if (caret < 0) caret = 0;
    if (caret > len) caret = len;
    for (int k = 0; txt[k]; k++) {
      unsigned char c = (unsigned char)txt[k];
      if (c < 0x20 || c == 0x7F) continue;
      if (len + 1 >= el->text_input.buflen) break;
      memmove(&el->text_input.buf[caret + 1], &el->text_input.buf[caret], len - caret + 1);
      el->text_input.buf[caret] = (char)c;
      caret++;
      len++;
    }
    el->text_input.caret = caret;
    if (el->text_input.callback) el->text_input.callback(ctx, el->text_input.userdata);
  }
}

void sf_input_set_key(sf_ctx_t *ctx, sf_key_t key, bool is_down) {
  /* Update a key's pressed state and fire SF_EVT_KEY_DOWN or SF_EVT_KEY_UP if it changed. */
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
  /* Update the mouse position, compute deltas, and fire SF_EVT_MOUSE_MOVE if it moved. */
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
  /* Update a mouse button state and fire SF_EVT_MOUSE_DOWN or SF_EVT_MOUSE_UP if it changed. */
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
  /* Return true while the key is held down this frame. */
  return ctx->input.keys[key];
}

bool sf_key_pressed(sf_ctx_t *ctx, sf_key_t key) {
  /* Return true only on the frame the key transitions from up to down. */
  return ctx->input.keys[key] && !ctx->input.keys_prev[key];
}

/* SF_SCENE_FUNCTIONS */
sf_tex_t* sf_load_texture_bmp(sf_ctx_t *ctx, const char *filename, const char *texname) {
  /* Load a 24-bit BMP file into the texture pool, applying gamma correction and treating magenta as transparent. */
  if (ctx->tex_count >= SF_MAX_TEXTURES) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to load texture '%s', max (%d) reached\n", texname, SF_MAX_TEXTURES);
    return NULL;
  }
  if (sf_get_texture_(ctx, texname, false) != NULL) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to load texture '%s', name in use\n", texname);
    return NULL;
  }
  char path[512];
  if (!_sf_resolve_asset(filename, path, sizeof(path))) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "missing texture: %s\n", filename);
    return NULL;
  }
  FILE *file = fopen(path, "rb");
  if (!file) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "could not open: %s\n", path);
    return NULL;
  }
  uint8_t header[54];
  if (fread(header, 1, 54, file) != 54 || header[0] != 'B' || header[1] != 'M') {
    fclose(file);
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "invalid bmp: %s\n", filename);
    return NULL;
  }
  uint32_t data_offset = header[10] | (header[11]<<8) | (header[12]<<16) | (header[13]<<24);
  int32_t w = header[18] | (header[19]<<8) | (header[20]<<16) | (header[21]<<24);
  int32_t h = header[22] | (header[23]<<8) | (header[24]<<16) | (header[25]<<24);
  int32_t h_abs = abs(h);
  sf_tex_t *tex = &ctx->textures[ctx->tex_count++];
  tex->w = w;
  tex->h = h_abs;
  tex->w_mask = w - 1;
  tex->h_mask = h_abs - 1;
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
              SF_LOG_INDENT "h      : %d\n"
              SF_LOG_INDENT "used   : %d/%d\n",
              filename, texname, tex->id, w, h_abs, ctx->tex_count, SF_MAX_TEXTURES);
  return tex;
}

sf_tex_t* sf_get_texture_(sf_ctx_t *ctx, const char *texname, bool should_log_failure) {
  /* Linear search for a texture by name; use the sf_get_texture() macro instead. */
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
  /* Create a sprite from a variadic list of texture names with per-frame duration and world scale. */
  char auto_name[32];
  if (spritename == NULL) {
    snprintf(auto_name, sizeof(auto_name), "spr_%d", ctx->sprite_count);
    spritename = auto_name;
  }
  if (NULL != sf_get_sprite_(ctx, spritename, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to load sprite '%s', name in use\n", spritename);
    return NULL;
  }
  if (ctx->sprite_count >= SF_MAX_SPRITES || frame_count > SF_MAX_SPRITE_FRAMES) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to load sprite '%s', max reached or too many frames\n", spritename);
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

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "name   : %s\n"
              SF_LOG_INDENT "id     : %d\n"
              SF_LOG_INDENT "frames : %d\n"
              SF_LOG_INDENT "dur    : %.2fs\n"
              SF_LOG_INDENT "scale  : %.2f\n"
              SF_LOG_INDENT "used   : %d/%d\n",
              spr->name, spr->id, frame_count, duration, scale, ctx->sprite_count, SF_MAX_SPRITES);
  return spr;
}

sf_sprite_t* sf_get_sprite_(sf_ctx_t *ctx, const char *spritename, bool should_log_failure) {
  /* Linear search for a sprite by name; use the sf_get_sprite() macro instead. */
  for (int32_t i = 0; i < ctx->sprite_count; ++i) {
    if (ctx->sprites[i].name && strcmp(ctx->sprites[i].name, spritename) == 0) {
      return &ctx->sprites[i];
    }
  }
  if (should_log_failure) SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "sprite '%s' not found\n", spritename);
  return NULL;
}

sf_emitr_t* sf_add_emitr(sf_ctx_t *ctx, const char *emitrname, sf_emitr_type_t type, sf_sprite_t *sprite, int max_p) {
  /* Add a particle emitter of the given type with a pre-allocated particle pool; returns its pointer. */
  char auto_name[32];
  if (emitrname == NULL) {
    snprintf(auto_name, sizeof(auto_name), "emitr_%d", ctx->emitr_count);
    emitrname = auto_name;
  }
  if (NULL != sf_get_emitr_(ctx, emitrname, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add emitter '%s', name in use\n", emitrname);
    return NULL;
  }
  if (ctx->emitr_count >= SF_MAX_EMITRS) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add emitter '%s', max (%d) reached\n", emitrname, SF_MAX_EMITRS);
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
  if (em->name) {
    memcpy((void*)em->name, emitrname, name_len);
    if (em->frame) em->frame->name = em->name;
  }

  em->spawn_rate = 10.0f; 
  em->particle_life = 2.0f;
  em->speed = 5.0f;
  em->dir = (sf_fvec3_t){0, 1, 0};
  em->spread = 0.5f;
  em->volume_size = (sf_fvec3_t){5, 5, 5};

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "name   : %s\n"
              SF_LOG_INDENT "id     : %d\n"
              SF_LOG_INDENT "type   : %s\n"
              SF_LOG_INDENT "max_p  : %d\n"
              SF_LOG_INDENT "used   : %d/%d\n",
              em->name, em->id, type == SF_EMITR_DIR ? "dir" : type == SF_EMITR_VOLUME ? "volume" : "omni", max_p, ctx->emitr_count, SF_MAX_EMITRS);
  return em;
}

sf_emitr_t* sf_get_emitr_(sf_ctx_t *ctx, const char *emitrname, bool should_log_failure) {
  /* Linear search for an emitter by name; use the sf_get_emitr() macro instead. */
  for (int32_t i = 0; i < ctx->emitr_count; ++i) {
    if (ctx->emitrs[i].name && strcmp(ctx->emitrs[i].name, emitrname) == 0) {
      return &ctx->emitrs[i];
    }
  }
  if (should_log_failure) SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "emitter '%s' not found\n", emitrname);
  return NULL;
}

sf_obj_t* sf_load_obj(sf_ctx_t *ctx, const char *filename, const char *objname) {
  /* Parse a Wavefront .obj file and store verts, UVs, normals, and faces in the arena. */
  char auto_name[32];
  if (objname == NULL) {
    snprintf(auto_name, sizeof(auto_name), "obj_%d", ctx->obj_count);
    objname = auto_name;
  }

  if (NULL != sf_get_obj_(ctx, objname, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to load obj '%s', name in use\n", objname);
    return NULL;
  }

  if (ctx->obj_count >= SF_MAX_OBJS) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to load obj '%s', max (%d) reached\n", objname, SF_MAX_OBJS);
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
  memset(obj, 0, sizeof(sf_obj_t));
  obj->v_cnt = v_cnt; obj->vt_cnt = vt_cnt; obj->vn_cnt = vn_cnt; obj->f_cnt = f_cnt;
  obj->v_cap = v_cnt; obj->vt_cap = vt_cnt; obj->f_cap = f_cnt;

  size_t path_len = strlen(filename) + 1;
  obj->src_path = (const char*)sf_arena_alloc(ctx, &ctx->arena, path_len);
  if (obj->src_path) memcpy((void*)obj->src_path, filename, path_len);

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
              SF_LOG_INDENT "used   : %d/%d\n",
              filename, objname, obj->id, v_cnt, vt_cnt, vn_cnt, f_cnt,
              _sf_obj_memusg(obj), ctx->obj_count, SF_MAX_OBJS);

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

  sf_fvec3_t bs_c = {0.0f, 0.0f, 0.0f};
  for (int i = 0; i < v_cnt; i++) {
    bs_c.x += obj->v[i].x;
    bs_c.y += obj->v[i].y;
    bs_c.z += obj->v[i].z;
  }
  float inv_v = 1.0f / (float)v_cnt;
  bs_c.x *= inv_v; bs_c.y *= inv_v; bs_c.z *= inv_v;
  float bs_r2 = 0.0f;
  for (int i = 0; i < v_cnt; i++) {
    float dx = obj->v[i].x - bs_c.x;
    float dy = obj->v[i].y - bs_c.y;
    float dz = obj->v[i].z - bs_c.z;
    float d2 = dx*dx + dy*dy + dz*dz;
    if (d2 > bs_r2) bs_r2 = d2;
  }
  obj->bs_center = bs_c;
  obj->bs_radius = sqrtf(bs_r2);

  fclose(file);
  return obj;
}

void sf_obj_recenter(sf_obj_t *obj) {
  /* Shift all vertices so the bounding-sphere center is at the origin. */
  if (!obj || obj->v_cnt == 0) return;
  sf_fvec3_t c = obj->bs_center;
  for (int i = 0; i < obj->v_cnt; i++) {
    obj->v[i].x -= c.x;
    obj->v[i].y -= c.y;
    obj->v[i].z -= c.z;
  }
  obj->bs_center = (sf_fvec3_t){0.0f, 0.0f, 0.0f};
}

sf_obj_t* sf_get_obj_(sf_ctx_t *ctx, const char *objname, bool should_log_failure) {
  /* Linear search for a mesh by name; use the sf_get_obj() macro instead. */
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
  /* Add a renderable entity referencing obj; creates its own scene-graph frame. */
  char auto_name[32];
  if (NULL == entiname) {
    snprintf(auto_name, sizeof(auto_name), "enti_%d", ctx->enti_count);
    entiname = auto_name;
  }

  if (obj == NULL) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add entity, obj is NULL\n");
    return NULL;
  }
  if (NULL != sf_get_enti_(ctx, entiname, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add entity '%s', name in use\n", entiname);
    return NULL;
  }
  if (ctx->enti_count >= SF_MAX_ENTITIES) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add entity, max (%d) reached\n", SF_MAX_ENTITIES);
    return NULL;
  }

  sf_enti_t *enti = &ctx->entities[ctx->enti_count++];
  enti->obj       = *obj;
  enti->id        = ctx->enti_count - 1;
  enti->tex       = NULL;
  enti->tex_scale = (sf_fvec2_t){1.0f, 1.0f};
  enti->frame     = sf_add_frame(ctx, NULL);

  size_t name_len = strlen(entiname) + 1;
  enti->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (enti->name) {
    memcpy((void*)enti->name, entiname, name_len);
    if (enti->frame) enti->frame->name = enti->name;
  }

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "enti   : %s (id %d)\n"
              SF_LOG_INDENT "obj    : %s (id %d)\n"
              SF_LOG_INDENT "used   : %d/%d\n",
              enti->name, enti->id, enti->obj.name, enti->obj.id, ctx->enti_count, SF_MAX_ENTITIES);

  return enti;
}

sf_enti_t* sf_get_enti_(sf_ctx_t *ctx, const char *entiname, bool should_log_failure) {
  /* Linear search for an entity by name; use the sf_get_enti() macro instead. */
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
  /* Add a secondary camera with its own pixel and z-buffers; useful for picture-in-picture views. */
  char auto_name[32];
  if (NULL == camname) {
    snprintf(auto_name, sizeof(auto_name), "cam_%d", ctx->cam_count);
    camname = auto_name;
  }
  if (NULL != sf_get_cam_(ctx, camname, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add camera '%s', name in use\n", camname);
    return NULL;
  }
  if (ctx->cam_count >= SF_MAX_CAMS) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add camera '%s', max (%d) reached\n", camname, SF_MAX_CAMS);
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
    if (cam->frame) cam->frame->name = cam->name;
  }

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "name   : %s\n"
              SF_LOG_INDENT "id     : %d\n"
              SF_LOG_INDENT "w      : %d\n"
              SF_LOG_INDENT "h      : %d\n"
              SF_LOG_INDENT "fov    : %.2f\n"
              SF_LOG_INDENT "near   : %.2f\n"
              SF_LOG_INDENT "far    : %.2f\n"
              SF_LOG_INDENT "used   : %d/%d\n",
              cam->name, cam->id, w, h, fov, cam->near_plane, cam->far_plane, ctx->cam_count, SF_MAX_CAMS);
  return cam;
}

sf_cam_t* sf_get_cam_(sf_ctx_t *ctx, const char *camname, bool should_log_failure) {
  /* Linear search for a camera by name; use the sf_get_cam() macro instead. */
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

sf_light_t* sf_add_light(sf_ctx_t *ctx, const char *lightname, sf_light_type_t type, sf_fvec3_t color, float intensity) {
  /* Add a point or directional light with a scene-graph frame for positioning. */
  char auto_name[32];
  if (lightname == NULL) {
    snprintf(auto_name, sizeof(auto_name), "light_%d", ctx->light_count);
    lightname = auto_name;
  }
  if (NULL != sf_get_light_(ctx, lightname, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add light '%s', name in use\n", lightname);
    return NULL;
  }
  if (ctx->light_count >= SF_MAX_LIGHTS) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to add light '%s', max (%d) reached\n", lightname, SF_MAX_LIGHTS);
    return NULL;
  }

  sf_light_t *l = &ctx->lights[ctx->light_count++];
  l->type      = type;
  l->color     = color;
  l->intensity = intensity;
  l->id        = ctx->light_count - 1;
  l->frame     = sf_add_frame(ctx, NULL);

  size_t name_len = strlen(lightname) + 1;
  l->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (l->name) {
    memcpy((void*)l->name, lightname, name_len);
    if (l->frame) l->frame->name = l->name;
  }

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "name   : %s\n"
              SF_LOG_INDENT "id     : %d\n"
              SF_LOG_INDENT "type   : %s\n"
              SF_LOG_INDENT "color  : %.2f %.2f %.2f\n"
              SF_LOG_INDENT "intens : %.2f\n"
              SF_LOG_INDENT "used   : %d/%d\n",
              l->name, l->id, type == SF_LIGHT_DIR ? "dir" : "point",
              color.x, color.y, color.z, intensity, ctx->light_count, SF_MAX_LIGHTS);
  return l;
}

sf_light_t* sf_get_light_(sf_ctx_t *ctx, const char *lightname, bool should_log_failure) {
  /* Linear search for a light by name; use the sf_get_light() macro instead. */
  for (int32_t i = 0; i < ctx->light_count; ++i) {
    if (ctx->lights[i].name && strcmp(ctx->lights[i].name, lightname) == 0) {
      return &ctx->lights[i];
    }
  }
  if (should_log_failure) SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "light '%s' not found\n", lightname);
  return NULL;
}

sf_skybox_t* sf_load_skybox(sf_ctx_t *ctx, const char *filename, const char *skyboxname) {
  /* Load an equirectangular BMP panorama as a skybox. The texture dimensions must be powers of two. */
  if (ctx->skybox_count >= SF_MAX_SKYBOXES) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to load skybox '%s', max (%d) reached\n", skyboxname, SF_MAX_SKYBOXES);
    return NULL;
  }
  if (sf_get_skybox_(ctx, skyboxname, false) != NULL) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "failed to load skybox '%s', name in use\n", skyboxname);
    return NULL;
  }
  sf_tex_t *tex = sf_load_texture_bmp(ctx, filename, skyboxname);
  if (!tex) return NULL;
  sf_skybox_t *sb   = &ctx->skyboxes[ctx->skybox_count++];
  sb->id            = ctx->skybox_count - 1;
  sb->tex           = tex;
  size_t name_len   = strlen(skyboxname) + 1;
  sb->name          = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (sb->name) memcpy((void*)sb->name, skyboxname, name_len);
  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "file   : %s\n"
              SF_LOG_INDENT "name   : %s\n"
              SF_LOG_INDENT "id     : %d\n"
              SF_LOG_INDENT "used   : %d/%d\n",
              filename, skyboxname, sb->id, ctx->skybox_count, SF_MAX_SKYBOXES);
  return sb;
}

sf_skybox_t* sf_get_skybox_(sf_ctx_t *ctx, const char *skyboxname, bool should_log_failure) {
  /* Linear search for a skybox by name; use the sf_get_skybox() macro instead. */
  for (int32_t i = 0; i < ctx->skybox_count; ++i) {
    if (ctx->skyboxes[i].name && strcmp(ctx->skyboxes[i].name, skyboxname) == 0) {
      return &ctx->skyboxes[i];
    }
  }
  if (should_log_failure) {
    SF_LOG(ctx, SF_LOG_WARN, SF_LOG_INDENT "skybox '%s' not found\n", skyboxname);
  }
  return NULL;
}

void sf_set_active_skybox(sf_ctx_t *ctx, sf_skybox_t *skybox) {
  /* Set or clear the active skybox rendered behind all scene geometry. Pass NULL to disable. */
  ctx->active_skybox = skybox;
}

void sf_enti_set_pos(sf_ctx_t *ctx, sf_enti_t *enti, float x, float y, float z) {
  /* Set an entity's world position directly. */
  if (enti && enti->frame) {
      enti->frame->pos = (sf_fvec3_t){x, y, z};
      enti->frame->is_dirty = true;
  }
}

void sf_enti_move(sf_ctx_t *ctx, sf_enti_t *enti, float dx, float dy, float dz) {
  /* Translate an entity by a world-space delta. */
  if (enti && enti->frame) {
      enti->frame->pos.x += dx;
      enti->frame->pos.y += dy;
      enti->frame->pos.z += dz;
      enti->frame->is_dirty = true;
  }
}

void sf_enti_set_rot(sf_ctx_t *ctx, sf_enti_t *enti, float rx, float ry, float rz) {
  /* Set an entity's Euler rotation (radians) directly. */
  if (enti && enti->frame) {
      enti->frame->rot = (sf_fvec3_t){rx, ry, rz};
      enti->frame->is_dirty = true;
  }
}

void sf_enti_rotate(sf_ctx_t *ctx, sf_enti_t *enti, float drx, float dry, float drz) {
  /* Add delta Euler angles (radians) to an entity's rotation. */
  if (enti && enti->frame) {
      enti->frame->rot.x += drx;
      enti->frame->rot.y += dry;
      enti->frame->rot.z += drz;
      enti->frame->is_dirty = true;
  }
}

void sf_enti_set_scale(sf_ctx_t *ctx, sf_enti_t *enti, float sx, float sy, float sz) {
  /* Set an entity's per-axis scale. */
  if (enti && enti->frame) {
      enti->frame->scale = (sf_fvec3_t){sx, sy, sz};
      enti->frame->is_dirty = true;
  }
}

void sf_enti_set_tex(sf_ctx_t *ctx, const char *entiname, const char *texname) {
  /* Assign a texture to an entity by name. */
  sf_enti_t *enti = sf_get_enti_(ctx, entiname, true);
  sf_tex_t *tex = sf_get_texture_(ctx, texname, true);
  if (enti && tex) {
    enti->tex = tex;
  }
}

void sf_camera_set_psp(sf_ctx_t *ctx, sf_cam_t *cam, float fov, float near_plane, float far_plane) {
  /* Update a camera's perspective parameters and mark the projection matrix dirty. */
  cam->fov        = fov;
  cam->near_plane = near_plane;
  cam->far_plane  = far_plane;
  cam->is_proj_dirty = true;
}

void sf_camera_set_pos(sf_ctx_t *ctx, sf_cam_t *cam, float x, float y, float z) {
  /* Set a camera's world position directly. */
  cam->frame->pos = (sf_fvec3_t){x, y, z};
  cam->frame->is_dirty = true;
}

void sf_camera_move_loc(sf_ctx_t *ctx, sf_cam_t *cam, float fwd, float right, float up) {
  /* Move a camera along its own local axes (forward, right, up). */
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
  /* Orient the camera to face a world-space target point. */
  if (cam && cam->frame) sf_frame_look_at(cam->frame, target);
}

void sf_camera_add_yp(sf_ctx_t *ctx, sf_cam_t *cam, float yaw_offset, float pitch_offset) {
  /* Add yaw and pitch (degrees) to a camera, clamping pitch to ±89°. */
  if (!cam || !cam->frame) return;
  cam->frame->rot.y -= SF_DEG2RAD(yaw_offset);
  cam->frame->rot.x += SF_DEG2RAD(pitch_offset);
  if (cam->frame->rot.x >  SF_DEG2RAD(89.0f)) cam->frame->rot.x =  SF_DEG2RAD(89.0f);
  if (cam->frame->rot.x < -SF_DEG2RAD(89.0f)) cam->frame->rot.x = -SF_DEG2RAD(89.0f);
  cam->frame->is_dirty = true;
}

void sf_load_sff(sf_ctx_t *ctx, const char *filename, const char *worldname) {
  /* Load a .sff world file, populating textures, objects, entities, cameras, lights, emitters, and skybox */
  char r_path[512];
  const char *open_path = filename;
  if (_sf_resolve_asset(filename, r_path, sizeof(r_path))) open_path = r_path;
  FILE *file = fopen(open_path, "r");
  if (!file) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "could not open %s\n", filename);
    return;
  }
  char line[512];
  int obj_count = 0, enti_count = 0, light_count = 0, cam_count = 0, tex_count = 0, sprite_count = 0, emitr_count = 0, frame_count = 0, skybox_count = 0, sprite_3d_count = 0;
  while (fgets(line, sizeof(line), file)) {
    _sf_sff_trim(line);
    if (line[0] == '#' || line[0] == '\0') continue;

    char keyword[32] = {0}, name[64] = {0}, filepath[256] = {0};

    if (sscanf(line, "%31s %63[^ \t{] {", keyword, name) == 2 && strchr(line, '{')) {
      _sf_sff_trim(name);
      if      (strcmp(keyword, "frame")   == 0)   _sf_sff_prse_frame(ctx, file, name, &frame_count);
      else if (strcmp(keyword, "camera")  == 0)   _sf_sff_prse_cam(ctx, file, name, &cam_count);
      else if (strcmp(keyword, "entity")  == 0)   _sf_sff_prse_enti(ctx, file, name, &enti_count);
      else if (strcmp(keyword, "light")   == 0)   _sf_sff_prse_light(ctx, file, name, &light_count);
      else if (strcmp(keyword, "sprite")  == 0)   _sf_sff_prse_sprit(ctx, file, name, &sprite_count);
      else if (strcmp(keyword, "emitter") == 0)   _sf_sff_prse_emitr(ctx, file, name, &emitr_count);
      else if (strcmp(keyword, "billboard") == 0) _sf_sff_prse_sprit_3d(ctx, file, name, &sprite_3d_count);
    }
    else if (sscanf(line, "mesh %63s \"%255[^\"]\"", name, filepath) == 2) {
      char r_path[512];
      if (_sf_resolve_asset(filepath, r_path, sizeof(r_path))) {
        sf_obj_t *obj = sf_load_obj(ctx, r_path, name);
        if (obj && strstr(line, "recenter")) sf_obj_recenter(obj);
        obj_count++;
      }
    }
    else if (sscanf(line, "texture %63s \"%255[^\"]\"", name, filepath) == 2) {
      sf_load_texture_bmp(ctx, filepath, name);
      tex_count++;
    }
    else if (sscanf(line, "skybox %63s \"%255[^\"]\"", name, filepath) == 2) {
      sf_skybox_t *sb = sf_load_skybox(ctx, filepath, name);
      if (sb) { sf_set_active_skybox(ctx, sb); skybox_count++; }
    }
    else if (sscanf(line, "include \"%255[^\"]\"", filepath) == 1) {
      sf_load_sff(ctx, filepath, filepath);
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
              SF_LOG_INDENT "emitrs : %d\n"
              SF_LOG_INDENT "frames : %d\n"
              SF_LOG_INDENT "skyboxs: %d\n",
              filename, obj_count, enti_count, light_count, cam_count, tex_count, sprite_count, emitr_count, frame_count, skybox_count);
}

bool sf_save_sff(sf_ctx_t *ctx, const char *filepath) {
  /* Serialize the current scene (cameras, objects, entities, lights) to a .sff file. */
  if (!ctx || !filepath) return false;
  FILE *f = fopen(filepath, "w");
  if (!f) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "could not write %s\n", filepath);
    return false;
  }
  char dir[512]; snprintf(dir, sizeof(dir), "%s", filepath);
  char *slash = strrchr(dir, '/');
  if (slash) *slash = '\0'; else dir[0] = '.' , dir[1] = '\0';

  fprintf(f, "# Saffron World File (exported)\n\n");

  fprintf(f, "camera main {\n");
  if (ctx->main_camera.frame) {
    sf_fvec3_t p = ctx->main_camera.frame->pos;
    fprintf(f, "    pos    = (%.3f, %.3f, %.3f)\n", p.x, p.y, p.z);
  }
  fprintf(f, "    fov    = %.2f\n", ctx->main_camera.fov);
  fprintf(f, "}\n\n");

  for (int i = 0; i < ctx->cam_count; i++) {
    sf_cam_t *c = &ctx->cameras[i];
    if (!c->name) continue;
    fprintf(f, "camera %s {\n", c->name);
    if (c->frame) {
      sf_fvec3_t p = c->frame->pos;
      fprintf(f, "    pos    = (%.3f, %.3f, %.3f)\n", p.x, p.y, p.z);
    }
    fprintf(f, "    fov    = %.2f\n", c->fov);
    fprintf(f, "    size   = (%d, %d, 0)\n", c->w, c->h);
    fprintf(f, "}\n\n");
  }

  for (int i = 0; i < ctx->obj_count; i++) {
    sf_obj_t *o = &ctx->objs[i];
    if (!o->name) continue;
    const char *path = o->src_path;
    char gen_path[512];
    if (!path) {
      snprintf(gen_path, sizeof(gen_path), "%s/%s.obj", dir, o->name);
      sf_obj_save_obj(ctx, o, gen_path);
      size_t plen = strlen(gen_path) + 1;
      o->src_path = (const char*)sf_arena_alloc(ctx, &ctx->arena, plen);
      if (o->src_path) memcpy((void*)o->src_path, gen_path, plen);
      path = o->src_path;
    }
    fprintf(f, "mesh %s \"%s\"\n", o->name, _sf_basename(path));
  }
  if (ctx->obj_count) fprintf(f, "\n");

  for (int i = 0; i < ctx->tex_count; i++) {
    sf_tex_t *t = &ctx->textures[i];
    if (!t->name) continue;
    fprintf(f, "texture %s \"%s.bmp\"\n", t->name, t->name);
  }
  if (ctx->tex_count) fprintf(f, "\n");

  for (int i = 0; i < ctx->sprite_count; i++) {
    sf_sprite_t *s = &ctx->sprites[i];
    if (!s->name || s->frame_count == 0) continue;
    fprintf(f, "sprite %s {\n", s->name);
    fprintf(f, "    duration = %.2f\n", s->frame_duration);
    fprintf(f, "    scale    = %.3f\n", s->base_scale);
    fprintf(f, "    frames   = [");
    for (int j = 0; j < s->frame_count; j++) {
      if (j > 0) fprintf(f, ", ");
      fprintf(f, "%s", (s->frames[j] && s->frames[j]->name) ? s->frames[j]->name : "");
    }
    fprintf(f, "]\n}\n\n");
  }

  if (ctx->active_skybox && ctx->active_skybox->name) {
    fprintf(f, "skybox %s \"%s.bmp\"\n\n", ctx->active_skybox->name, ctx->active_skybox->name);
  }

  for (int i = 0; i < ctx->enti_count; i++) {
    sf_enti_t *e = &ctx->entities[i];
    if (!e->name || !e->frame) continue;
    fprintf(f, "entity %s {\n", e->name);
    fprintf(f, "    mesh    = %s\n", e->obj.name ? e->obj.name : "");
    sf_fvec3_t p = e->frame->pos, r = e->frame->rot, s = e->frame->scale;
    fprintf(f, "    pos     = (%.3f, %.3f, %.3f)\n", p.x, p.y, p.z);
    fprintf(f, "    rot     = (%.3f, %.3f, %.3f)\n", r.x, r.y, r.z);
    fprintf(f, "    scale   = (%.3f, %.3f, %.3f)\n", s.x, s.y, s.z);
    if (e->tex && e->tex->name) fprintf(f, "    texture = %s\n", e->tex->name);
    if (e->tex_scale.x != 1.0f || e->tex_scale.y != 1.0f)
      fprintf(f, "    tex_scale = (%.3f, %.3f)\n", e->tex_scale.x, e->tex_scale.y);
    _sf_write_frame_ref(f, e->frame, ctx);
    fprintf(f, "}\n\n");
  }

  for (int i = 0; i < ctx->light_count; i++) {
    sf_light_t *l = &ctx->lights[i];
    if (!l->name || !l->frame) continue;
    fprintf(f, "light %s {\n", l->name);
    fprintf(f, "    type      = %s\n", l->type == SF_LIGHT_DIR ? "dir" : "point");
    sf_fvec3_t p = l->frame->pos;
    fprintf(f, "    pos       = (%.3f, %.3f, %.3f)\n", p.x, p.y, p.z);
    fprintf(f, "    color     = (%.3f, %.3f, %.3f)\n", l->color.x, l->color.y, l->color.z);
    fprintf(f, "    intensity = %.3f\n", l->intensity);
    _sf_write_frame_ref(f, l->frame, ctx);
    fprintf(f, "}\n\n");
  }

  for (int i = 0; i < ctx->sprite_3d_count; i++) {
    sf_sprite_3d_t *b = &ctx->sprite_3ds[i];
    if (!b->sprite || !b->sprite->name) continue;
    const char *bname = b->name[0] ? b->name : "bill";
    fprintf(f, "billboard %s {\n", bname);
    fprintf(f, "    sprite  = %s\n", b->sprite->name);
    fprintf(f, "    pos     = (%.4f, %.4f, %.4f)\n", b->pos.x, b->pos.y, b->pos.z);
    fprintf(f, "    scale   = %.4f\n", b->scale);
    fprintf(f, "    opacity = %.4f\n", b->opacity);
    fprintf(f, "    angle   = %.4f\n", b->angle);
    if (b->normal.x != 0.f || b->normal.y != 0.f || b->normal.z != 0.f)
      fprintf(f, "    normal  = (%.4f, %.4f, %.4f)\n", b->normal.x, b->normal.y, b->normal.z);
    if (b->frame && b->frame->name)
      fprintf(f, "    frame   = %s\n", b->frame->name);
    fprintf(f, "}\n\n");
  }

  fclose(f);
  SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "wrote %s\n", filepath);
  return true;
}

bool _sf_sff_read_kv(FILE *f, char *key, size_t ksz, char *val, size_t vsz) {
  /* Read the next "key = value" line from a .sff file block; returns false on '}' or EOF. */
  char line[512];
  while (fgets(line, sizeof(line), f)) {
    _sf_sff_trim(line);
    if (line[0] == '#' || line[0] == '\0') continue;
    if (line[0] == '}') return false;
    char *eq = strchr(line, '=');
    if (!eq) continue;
    *eq = '\0';
    char *k = line;  _sf_sff_trim(k);
    char *v = eq + 1; _sf_sff_trim(v);
    snprintf(key, ksz, "%s", k);
    snprintf(val, vsz, "%s", v);
    return true;
  }
  return false;
}

void _sf_sff_trim(char *s) {
  /* Trim leading and trailing whitespace (including newlines) from a string in place. */
  while (*s == ' ' || *s == '\t') { char *d = s; while ((*d = *(d+1))) d++; }
  size_t len = strlen(s);
  while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\n' || s[len-1] == '\r')) s[--len] = '\0';
}

sf_fvec3_t _sf_sff_prse_vec3(const char *s) {
  /* Parse a "(x, y, z)" string from a .sff value into an sf_fvec3_t. */
  sf_fvec3_t v = {0, 0, 0};
  sscanf(s, " ( %f , %f , %f )", &v.x, &v.y, &v.z);
  return v;
}

int _sf_sff_prse_list(const char *s, char out[][64], int max) {
  /* Parse a "[a, b, c]" list from a .sff value into a fixed array of strings; returns item count. */
  const char *p = strchr(s, '[');
  if (!p) return 0;
  p++;
  int count = 0;
  while (count < max) {
    while (*p == ' ' || *p == '\t') p++;
    if (*p == ']' || *p == '\0') break;
    int i = 0;
    while (*p && *p != ',' && *p != ']' && *p != ' ' && *p != '\t' && i < 63) out[count][i++] = *p++;
    out[count][i] = '\0';
    count++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == ',') p++;
  }
  return count;
}

sf_frame_t* _sf_sff_get_frame_(sf_ctx_t *ctx, const char *name) {
  /* Find a frame by name during .sff loading; use the sf_get_frame() macro at runtime. */
  for (int i = 0; i < ctx->frames_count; i++) {
    sf_frame_t *f = &ctx->frames[i];
    if (!f->parent && !f->is_root) continue; /* skip freed frames */
    if (f->name && strcmp(f->name, name) == 0)
      return f;
  }
  return NULL;
}

void _sf_sff_prse_frame(sf_ctx_t *ctx, FILE *f, const char *name, int *frame_count) {
  /* Parse a "frame name { ... }" block from a .sff file and add it to the scene graph. */
  char key[64], val[256];
  char parent_name[64] = {0};
  sf_fvec3_t pos = {0,0,0}, rot = {0,0,0}, scale = {1,1,1};
  while (_sf_sff_read_kv(f, key, sizeof(key), val, sizeof(val))) {
    if      (strcmp(key, "pos")    == 0) pos   = _sf_sff_prse_vec3(val);
    else if (strcmp(key, "rot")    == 0) rot   = _sf_sff_prse_vec3(val);
    else if (strcmp(key, "scale")  == 0) scale = _sf_sff_prse_vec3(val);
    else if (strcmp(key, "parent") == 0) snprintf(parent_name, sizeof(parent_name), "%s", val);
  }
  sf_frame_t *parent = NULL;
  if (parent_name[0]) parent = _sf_sff_get_frame_(ctx, parent_name);
  sf_frame_t *fr = sf_add_frame(ctx, parent);
  if (!fr) return;
  fr->pos   = pos;
  fr->rot   = rot;
  fr->scale = scale;
  fr->is_dirty = true;
  size_t name_len = strlen(name) + 1;
  fr->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (fr->name) memcpy((void*)fr->name, name, name_len);
  (*frame_count)++;
}

void _sf_sff_prse_cam(sf_ctx_t *ctx, FILE *f, const char *name, int *cam_count) {
  /* Parse a "camera name { ... }" block from a .sff file, creating or configuring a camera. */
  char key[64], val[256];
  char parent_frame[64] = {0};
  sf_fvec3_t pos = {0,0,0}, target = {0,0,0};
  float fov = 60.0f;
  int w = 0, h = 0;
  bool has_target = false;
  while (_sf_sff_read_kv(f, key, sizeof(key), val, sizeof(val))) {
    if      (strcmp(key, "pos")    == 0) pos    = _sf_sff_prse_vec3(val);
    else if (strcmp(key, "target") == 0) { target = _sf_sff_prse_vec3(val); has_target = true; }
    else if (strcmp(key, "fov")    == 0) sscanf(val, "%f", &fov);
    else if (strcmp(key, "frame")  == 0) snprintf(parent_frame, sizeof(parent_frame), "%s", val);
    else if (strcmp(key, "size")   == 0) { sf_fvec3_t s = _sf_sff_prse_vec3(val); w = (int)s.x; h = (int)s.y; }
  }
  if (strcmp(name, "main") == 0) {
    sf_camera_set_pos(ctx, &ctx->main_camera, pos.x, pos.y, pos.z);
    if (has_target) sf_camera_look_at(ctx, &ctx->main_camera, target);
    ctx->main_camera.fov = fov;
    ctx->main_camera.is_proj_dirty = true;
    if (parent_frame[0]) {
      sf_frame_t *pf = _sf_sff_get_frame_(ctx, parent_frame);
      if (pf) sf_frame_set_parent(ctx->main_camera.frame, pf);
    }
  } else {
    if (w == 0) w = ctx->main_camera.w;
    if (h == 0) h = ctx->main_camera.h;
    sf_cam_t *cam = sf_add_cam(ctx, name, w, h, fov);
    if (cam) {
      sf_camera_set_pos(ctx, cam, pos.x, pos.y, pos.z);
      if (has_target) sf_camera_look_at(ctx, cam, target);
      if (parent_frame[0]) {
        sf_frame_t *pf = _sf_sff_get_frame_(ctx, parent_frame);
        if (pf) sf_frame_set_parent(cam->frame, pf);
      }
    }
  }
  (*cam_count)++;
}

void _sf_sff_prse_enti(sf_ctx_t *ctx, FILE *f, const char *name, int *enti_count) {
  /* Parse an "entity name { ... }" block from a .sff file, creating a renderable entity. */
  char key[64], val[256];
  char mesh_name[64] = {0}, tex_name[64] = {0}, parent_frame[64] = {0};
  sf_fvec3_t pos = {0,0,0}, rot = {0,0,0}, scale = {1,1,1};
  sf_fvec2_t tex_scale = {1.0f, 1.0f};
  bool has_tex_scale = false;
  while (_sf_sff_read_kv(f, key, sizeof(key), val, sizeof(val))) {
    if      (strcmp(key, "mesh")      == 0) snprintf(mesh_name, sizeof(mesh_name), "%s", val);
    else if (strcmp(key, "texture")   == 0) snprintf(tex_name, sizeof(tex_name), "%s", val);
    else if (strcmp(key, "tex_scale") == 0) { sf_fvec3_t v = _sf_sff_prse_vec3(val); tex_scale = (sf_fvec2_t){v.x, v.y}; has_tex_scale = true; }
    else if (strcmp(key, "frame")     == 0) snprintf(parent_frame, sizeof(parent_frame), "%s", val);
    else if (strcmp(key, "pos")       == 0) pos   = _sf_sff_prse_vec3(val);
    else if (strcmp(key, "rot")       == 0) rot   = _sf_sff_prse_vec3(val);
    else if (strcmp(key, "scale")     == 0) scale = _sf_sff_prse_vec3(val);
  }
  sf_obj_t *obj = sf_get_obj_(ctx, mesh_name, true);
  if (!obj) return;
  char enti_name[80]; snprintf(enti_name, sizeof(enti_name), "%s", name);
  { int sfx = 1; while (sf_get_enti_(ctx, enti_name, false)) snprintf(enti_name, sizeof(enti_name), "%s_%d", name, sfx++); }
  sf_enti_t *enti = sf_add_enti(ctx, obj, enti_name);
  if (!enti) return;
  sf_enti_set_pos(ctx, enti, pos.x, pos.y, pos.z);
  sf_enti_set_rot(ctx, enti, rot.x, rot.y, rot.z);
  sf_enti_set_scale(ctx, enti, scale.x, scale.y, scale.z);
  if (tex_name[0]) enti->tex = sf_get_texture_(ctx, tex_name, true);
  if (has_tex_scale) enti->tex_scale = tex_scale;
  if (parent_frame[0]) {
    sf_frame_t *pf = _sf_sff_get_frame_(ctx, parent_frame);
    if (pf) sf_frame_set_parent(enti->frame, pf);
  }
  (*enti_count)++;
}

void _sf_sff_prse_light(sf_ctx_t *ctx, FILE *f, const char *name, int *light_count) {
  /* Parse a "light name { ... }" block from a .sff file and add a point or directional light. */
  char key[64], val[256];
  char type_str[16] = "point", parent_frame[64] = {0};
  sf_fvec3_t pos = {0,0,0}, color = {1,1,1};
  float intensity = 1.0f;
  while (_sf_sff_read_kv(f, key, sizeof(key), val, sizeof(val))) {
    if      (strcmp(key, "type")      == 0) snprintf(type_str, sizeof(type_str), "%s", val);
    else if (strcmp(key, "frame")     == 0) snprintf(parent_frame, sizeof(parent_frame), "%s", val);
    else if (strcmp(key, "pos")       == 0) pos       = _sf_sff_prse_vec3(val);
    else if (strcmp(key, "color")     == 0) color     = _sf_sff_prse_vec3(val);
    else if (strcmp(key, "intensity") == 0) sscanf(val, "%f", &intensity);
  }
  sf_light_t *l = NULL;
  if (strcmp(type_str, "dir") == 0) {
    l = sf_add_light(ctx, name, SF_LIGHT_DIR, color, intensity);
    if (l) sf_frame_look_at(l->frame, pos);
  } else {
    l = sf_add_light(ctx, name, SF_LIGHT_POINT, color, intensity);
    if (l) l->frame->pos = pos;
  }
  if (l && parent_frame[0]) {
    sf_frame_t *pf = _sf_sff_get_frame_(ctx, parent_frame);
    if (pf) sf_frame_set_parent(l->frame, pf);
  }
  (*light_count)++;
}

void _sf_sff_prse_sprit(sf_ctx_t *ctx, FILE *f, const char *name, int *sprite_count) {
  /* Parse a "sprite name { ... }" block from a .sff file and register a sprite animation. */
  char key[64], val[256];
  float duration = 1.0f, scale = 1.0f;
  char frame_names[SF_MAX_SPRITE_FRAMES][64] = {{0}};
  int frame_count = 0;
  while (_sf_sff_read_kv(f, key, sizeof(key), val, sizeof(val))) {
    if      (strcmp(key, "duration") == 0) sscanf(val, "%f", &duration);
    else if (strcmp(key, "scale")    == 0) sscanf(val, "%f", &scale);
    else if (strcmp(key, "frames")   == 0) frame_count = _sf_sff_prse_list(val, frame_names, SF_MAX_SPRITE_FRAMES);
  }
  if (frame_count == 0) return;
  if (ctx->sprite_count >= SF_MAX_SPRITES) return;
  if (NULL != sf_get_sprite_(ctx, name, false)) return;
  sf_sprite_t *spr = &ctx->sprites[ctx->sprite_count++];
  spr->id = ctx->sprite_count - 1;
  spr->frame_count = frame_count;
  spr->frame_duration = duration;
  spr->base_scale = scale;
  size_t name_len = strlen(name) + 1;
  spr->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, name_len);
  if (spr->name) memcpy((void*)spr->name, name, name_len);
  for (int i = 0; i < frame_count; i++)
    spr->frames[i] = sf_get_texture_(ctx, frame_names[i], true);
  (*sprite_count)++;
}

void _sf_sff_prse_emitr(sf_ctx_t *ctx, FILE *f, const char *name, int *emitr_count) {
  /* Parse an "emitter name { ... }" block from a .sff file and create a particle emitter. */
  char key[64], val[256];
  char type_str[16] = "omni", spr_name[64] = {0}, parent_frame[64] = {0};
  int max_p = 100;
  float spawn_rate = 10.0f, life = 1.0f, speed = 1.0f, spread = 0.0f;
  sf_fvec3_t pos = {0,0,0}, dir = {0,1,0}, volume = {1,1,1};
  bool has_dir = false, has_vol = false;
  while (_sf_sff_read_kv(f, key, sizeof(key), val, sizeof(val))) {
    if      (strcmp(key, "type")       == 0) snprintf(type_str, sizeof(type_str), "%s", val);
    else if (strcmp(key, "sprite")     == 0) snprintf(spr_name, sizeof(spr_name), "%s", val);
    else if (strcmp(key, "max")        == 0) sscanf(val, "%d", &max_p);
    else if (strcmp(key, "frame")      == 0) snprintf(parent_frame, sizeof(parent_frame), "%s", val);
    else if (strcmp(key, "pos")        == 0) pos        = _sf_sff_prse_vec3(val);
    else if (strcmp(key, "spawn_rate") == 0) sscanf(val, "%f", &spawn_rate);
    else if (strcmp(key, "life")       == 0) sscanf(val, "%f", &life);
    else if (strcmp(key, "speed")      == 0) sscanf(val, "%f", &speed);
    else if (strcmp(key, "dir")        == 0) { dir    = _sf_sff_prse_vec3(val); has_dir = true; }
    else if (strcmp(key, "spread")     == 0) sscanf(val, "%f", &spread);
    else if (strcmp(key, "volume")     == 0) { volume = _sf_sff_prse_vec3(val); has_vol = true; }
  }
  sf_emitr_type_t type = SF_EMITR_OMNI;
  if      (strcmp(type_str, "dir") == 0) type = SF_EMITR_DIR;
  else if (strcmp(type_str, "vol") == 0) type = SF_EMITR_VOLUME;
  sf_sprite_t *spr = sf_get_sprite_(ctx, spr_name, true);
  if (!spr) return;
  sf_emitr_t *em = sf_add_emitr(ctx, name, type, spr, max_p);
  if (!em) return;
  em->frame->pos = pos;
  em->spawn_rate = spawn_rate;
  em->particle_life = life;
  em->speed = speed;
  if (type == SF_EMITR_DIR || has_dir) { em->dir = sf_fvec3_norm(dir); em->spread = spread; }
  if (type == SF_EMITR_VOLUME || has_vol) em->volume_size = volume;
  if (parent_frame[0]) {
    sf_frame_t *pf = _sf_sff_get_frame_(ctx, parent_frame);
    if (pf) sf_frame_set_parent(em->frame, pf);
  }
  (*emitr_count)++;
}

void _sf_sff_prse_sprit_3d(sf_ctx_t *ctx, FILE *f, const char *name, int *sprite_3d_count) {
  /* Parse a "billboard name { ... }" block from a .sff file. */
  char key[64], val[256];
  char spr_name[64] = {0}, frame_name[64] = {0};
  sf_fvec3_t pos = {0,0,0}, normal = {0,0,0};
  float scale = 1.0f, opacity = 1.0f, angle = 0.0f;
  while (_sf_sff_read_kv(f, key, sizeof(key), val, sizeof(val))) {
    if      (strcmp(key, "sprite")  == 0) snprintf(spr_name,   sizeof(spr_name),   "%s", val);
    else if (strcmp(key, "frame")   == 0) snprintf(frame_name, sizeof(frame_name), "%s", val);
    else if (strcmp(key, "pos")     == 0) pos     = _sf_sff_prse_vec3(val);
    else if (strcmp(key, "scale")   == 0) sscanf(val, "%f", &scale);
    else if (strcmp(key, "opacity") == 0) sscanf(val, "%f", &opacity);
    else if (strcmp(key, "angle")   == 0) sscanf(val, "%f", &angle);
    else if (strcmp(key, "normal")  == 0) normal  = _sf_sff_prse_vec3(val);
  }
  if (!spr_name[0]) return;
  sf_sprite_t *spr = sf_get_sprite_(ctx, spr_name, true);
  if (!spr) return;
  sf_sprite_3d_t *b = sf_add_sprite_3d(ctx, spr, name, pos, scale, opacity, angle);
  if (b) {
    b->normal = normal;
    if (frame_name[0]) b->frame = _sf_sff_get_frame_(ctx, frame_name);
  }
  (*sprite_3d_count)++;
}

/* SF_FRAME_FUNCTIONS */
sf_frame_t* sf_get_root(sf_ctx_t *ctx, sf_convention_t conv) {
  /* Return the root frame for the given coordinate convention (DEFAULT, NED, FLU). */
  if (conv < 0 || conv >= SF_CONV_MAX) return NULL;
  return ctx->roots[conv];
}

sf_frame_t* sf_add_frame(sf_ctx_t *ctx, sf_frame_t *parent) {
  /* Allocate a new scene-graph frame (from the free list or pool) and attach it to parent. */
  sf_frame_t *f;
  if (ctx->free_frames) {
    f = ctx->free_frames;
    ctx->free_frames = f->next_sibling;
  } else {
    if (ctx->frames_count >= SF_MAX_FRAMES) return NULL;
    f = &ctx->frames[ctx->frames_count++];
  }
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
  /* Walk every root's tree and recompute global_M for dirty frames. */
  for (int i = 0; i < SF_CONV_MAX; i++) {
    if (ctx->roots[i]) {
      _sf_calc_frame_tree(ctx->roots[i], sf_make_idn_fmat4(), false);
    }
  }
}

void sf_frame_look_at(sf_frame_t *f, sf_fvec3_t target) {
  /* Orient a frame to face a world-space target point (sets yaw/pitch; rolls to zero). */
  if (!f) return;
  sf_fvec3_t diff = sf_fvec3_sub(target, f->pos);
  f->rot.y = atan2f(-diff.x, -diff.z);
  float xz_dist = sqrtf(diff.x*diff.x + diff.z*diff.z);
  f->rot.x = atan2f(diff.y, xz_dist);
  f->rot.z = 0.0f;
  f->is_dirty = true;
}

void sf_frame_set_parent(sf_frame_t *child, sf_frame_t *new_parent) {
  /* Re-parent a frame, unlinking it from its old parent and inserting it under new_parent. */
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

void sf_remove_frame(sf_ctx_t *ctx, sf_frame_t *f) {
  /* Recursively unlink a frame and all its children, returning them to the free list. */
  if (!f || f->is_root) return;
  sf_frame_t *c = f->first_child;
  while (c) {
    sf_frame_t *next = c->next_sibling;
    sf_remove_frame(ctx, c);
    c = next;
  }
  if (f->parent) {
    sf_frame_t **curr = &f->parent->first_child;
    while (*curr) {
      if (*curr == f) { *curr = f->next_sibling; break; }
      curr = &((*curr)->next_sibling);
    }
  }
  f->parent = NULL;
  f->first_child = NULL;
  f->name = NULL;
  f->next_sibling = ctx->free_frames;
  ctx->free_frames = f;
}

void _sf_set_up_frames(sf_ctx_t *ctx) {
  /* Create the three convention roots (DEFAULT, NED, FLU) with their fixed basis matrices. */
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
  /* Recursively recompute global_M for this frame and its children if dirty. */
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

void _sf_write_frame_ref(FILE *f, sf_frame_t *fr, sf_ctx_t *ctx) {
  /* Write a "frame = name" line to a .sff file if the frame has a named non-root parent. */
  (void)ctx;
  if (!fr || !fr->parent || fr->parent->is_root || !fr->parent->name) return;
  fprintf(f, "    frame     = %s\n", fr->parent->name);
}

/* SF_DRAWING_FUNCTIONS */
void sf_fill(sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c) {
  /* Fill the entire camera pixel buffer with a solid color. */
  if (c == 0) { memset(cam->buffer, 0, cam->buffer_size * sizeof(sf_pkd_clr_t)); return; }
  sf_pkd_clr_t *buf = cam->buffer;
  int n = cam->buffer_size;
  for (int i = 0; i < n; ++i) buf[i] = c;
}

void sf_pixel(sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0) {
  /* Write a single pixel, clipping to the camera bounds. */
  if (v0.x < 0 || v0.x >= cam->w || v0.y < 0 || v0.y >= cam->h) return;
  cam->buffer[_sf_vec_to_index(ctx, cam, v0)] = c;
}

void sf_pixel_depth(sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v, float z) {
  /* Write a pixel only if z is closer than the current z-buffer value. */
  if (v.x >= cam->w || v.y >= cam->h) return;
  uint32_t idx = _sf_vec_to_index(ctx, cam, v);
  if (z < cam->z_buffer[idx]) {
    cam->z_buffer[idx] = z;
    cam->buffer[idx] = c;
  }
}

void sf_line(sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1) {
  /* Draw a Bresenham line between two screen-space points. */
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
  /* Fill a screen-space axis-aligned rectangle with a solid color. */
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
  /* Rasterize a flat-shaded triangle; z is the projected depth used for optional depth testing. */
  if (v1.y < v0.y) { sf_fvec3_t t = v0; v0 = v1; v1 = t; }
  if (v2.y < v0.y) { sf_fvec3_t t = v0; v0 = v2; v2 = t; }
  if (v2.y < v1.y) { sf_fvec3_t t = v1; v1 = v2; v2 = t; }
  if (v2.y < 0 || v0.y >= cam->h) return;
  int iy0 = (int)v0.y, iy1 = (int)v1.y, iy2 = (int)v2.y;
  if (iy2 == iy0) return;
  float inv_h02 = 1.0f / (float)(iy2 - iy0);
  float dxa = (v2.x - v0.x) * inv_h02;
  float dza = (v2.z - v0.z) * inv_h02;
  int h01 = iy1 - iy0;
  bool swap = (h01 > 0) ? (v1.x < v0.x + dxa * h01) : (v1.x < v0.x);
  int cam_w = cam->w, cam_h = cam->h;
  sf_pkd_clr_t *cam_buf = cam->buffer;
  float *z_buf = cam->z_buffer;
  for (int half = 0; half < 2; half++) {
    int yb, ye_raw;
    float bx0, bz0, dxb, dzb;
    if (half == 0) {
      if (h01 <= 0) continue;
      yb = iy0; ye_raw = iy1 - 1;
      float inv_h01 = 1.0f / (float)h01;
      dxb = (v1.x - v0.x) * inv_h01;
      dzb = (v1.z - v0.z) * inv_h01;
      bx0 = v0.x; bz0 = v0.z;
    } else {
      int h12 = iy2 - iy1;
      if (h12 <= 0) continue;
      yb = iy1; ye_raw = iy2;
      float inv_h12 = 1.0f / (float)h12;
      dxb = (v2.x - v1.x) * inv_h12;
      dzb = (v2.z - v1.z) * inv_h12;
      bx0 = v1.x; bz0 = v1.z;
    }
    int ys = yb < 0 ? 0 : yb;
    int ye = ye_raw >= cam_h ? cam_h - 1 : ye_raw;
    float ax = v0.x + dxa * (ys - iy0), az = v0.z + dza * (ys - iy0);
    float bx = bx0 + dxb * (ys - yb), bz = bz0 + dzb * (ys - yb);
    for (int y = ys; y <= ye; ++y) {
      float lx, lz, rx, rz;
      if (swap) { lx = bx; lz = bz; rx = ax; rz = az; }
      else      { lx = ax; lz = az; rx = bx; rz = bz; }
      int x_s = (int)lx, x_e = (int)rx;
      float w = (float)(x_e - x_s);
      float dz = (w <= 0.0f) ? 0.0f : (rz - lz) / w;
      int ox = x_s;
      if (x_s < 0) x_s = 0;
      if (x_e >= cam_w) x_e = cam_w - 1;
      float cz = lz + dz * (float)(x_s - ox);
      int bi = y * cam_w + x_s;
      if (use_depth) {
        for (int x = x_s; x <= x_e; ++x, ++bi, cz += dz) {
          if (cz < z_buf[bi]) { z_buf[bi] = cz; cam_buf[bi] = c; }
        }
      } else {
        for (int x = x_s; x <= x_e; ++x, ++bi) cam_buf[bi] = c;
      }
      ax += dxa; az += dza;
      bx += dxb; bz += dzb;
    }
  }
}

void sf_tri_tex(sf_ctx_t *ctx, sf_cam_t *cam, sf_tex_t *tex, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, sf_fvec3_t uvz0, sf_fvec3_t uvz1, sf_fvec3_t uvz2, sf_fvec3_t l_int, float opacity) {
  /* Rasterize a perspective-correct textured and lit triangle; uvz encodes u/z, v/z, 1/z per vertex. */
  if (v1.y < v0.y) { _sf_swap_fvec3(&v0, &v1); _sf_swap_fvec3(&uvz0, &uvz1); }
  if (v2.y < v0.y) { _sf_swap_fvec3(&v0, &v2); _sf_swap_fvec3(&uvz0, &uvz2); }
  if (v2.y < v1.y) { _sf_swap_fvec3(&v1, &v2); _sf_swap_fvec3(&uvz1, &uvz2); }
  int iy0 = (int)v0.y, iy1 = (int)v1.y, iy2 = (int)v2.y;
  if (iy2 < 0 || iy0 >= cam->h) return;
  if (iy2 == iy0) return;
  float inv_h02 = 1.0f / (float)(iy2 - iy0);
  float dxa = (v2.x - v0.x) * inv_h02;
  float dza = (v2.z - v0.z) * inv_h02;
  float duxa = (uvz2.x - uvz0.x) * inv_h02;
  float duya = (uvz2.y - uvz0.y) * inv_h02;
  float duza = (uvz2.z - uvz0.z) * inv_h02;
  int h01 = iy1 - iy0;
  bool swap = (h01 > 0) ? (v1.x < v0.x + dxa * h01) : (v1.x < v0.x);
  int li_r = (int)(l_int.x * 256.0f + 0.5f); if (li_r > 256) li_r = 256;
  int li_g = (int)(l_int.y * 256.0f + 0.5f); if (li_g > 256) li_g = 256;
  int li_b = (int)(l_int.z * 256.0f + 0.5f); if (li_b > 256) li_b = 256;
  uint8_t opa8 = (uint8_t)(opacity * 255.f + 0.5f);
  bool opa_full = (opa8 >= 252);
  uint8_t inv_opa8 = 255 - opa8;
  int tex_w = tex->w, tex_h = tex->h;
  int tex_wm = tex->w_mask, tex_hm = tex->h_mask;
  sf_pkd_clr_t *tex_px = tex->px;
  int cam_w = cam->w, cam_h = cam->h;
  sf_pkd_clr_t *cam_buf = cam->buffer;
  float *z_buf = cam->z_buffer;
  for (int half = 0; half < 2; half++) {
    int yb, ye_raw;
    float bx0, bz0, bux0, buy0, buz0, dxb, dzb, duxb, duyb, duzb;
    if (half == 0) {
      if (h01 <= 0) continue;
      yb = iy0; ye_raw = iy1 - 1;
      float inv_h01 = 1.0f / (float)h01;
      dxb = (v1.x - v0.x) * inv_h01;
      dzb = (v1.z - v0.z) * inv_h01;
      duxb = (uvz1.x - uvz0.x) * inv_h01;
      duyb = (uvz1.y - uvz0.y) * inv_h01;
      duzb = (uvz1.z - uvz0.z) * inv_h01;
      bx0 = v0.x; bz0 = v0.z;
      bux0 = uvz0.x; buy0 = uvz0.y; buz0 = uvz0.z;
    } else {
      int h12 = iy2 - iy1;
      if (h12 <= 0) continue;
      yb = iy1; ye_raw = iy2;
      float inv_h12 = 1.0f / (float)h12;
      dxb = (v2.x - v1.x) * inv_h12;
      dzb = (v2.z - v1.z) * inv_h12;
      duxb = (uvz2.x - uvz1.x) * inv_h12;
      duyb = (uvz2.y - uvz1.y) * inv_h12;
      duzb = (uvz2.z - uvz1.z) * inv_h12;
      bx0 = v1.x; bz0 = v1.z;
      bux0 = uvz1.x; buy0 = uvz1.y; buz0 = uvz1.z;
    }
    int ys = yb < 0 ? 0 : yb;
    int ye = ye_raw >= cam_h ? cam_h - 1 : ye_raw;
    float sk_a = (float)(ys - iy0), sk_b = (float)(ys - yb);
    float ax = v0.x + dxa * sk_a, az = v0.z + dza * sk_a;
    float aux = uvz0.x + duxa * sk_a, auy = uvz0.y + duya * sk_a, auz = uvz0.z + duza * sk_a;
    float bx = bx0 + dxb * sk_b, bz = bz0 + dzb * sk_b;
    float bux = bux0 + duxb * sk_b, buy = buy0 + duyb * sk_b, buz = buz0 + duzb * sk_b;
    for (int y = ys; y <= ye; ++y) {
      float lx, lz, lux, luy, luz, rx, rz, rux, ruy, ruz;
      if (swap) {
        lx = bx; lz = bz; lux = bux; luy = buy; luz = buz;
        rx = ax; rz = az; rux = aux; ruy = auy; ruz = auz;
      } else {
        lx = ax; lz = az; lux = aux; luy = auy; luz = auz;
        rx = bx; rz = bz; rux = bux; ruy = buy; ruz = buz;
      }
      int xs = (int)lx, xe = (int)rx;
      float scan_w = (float)(xe - xs);
      if (scan_w > 0.0f) {
        float inv_sw = 1.0f / scan_w;
        float dz = (rz - lz) * inv_sw;
        float dux = (rux - lux) * inv_sw;
        float duy = (ruy - luy) * inv_sw;
        float duz = (ruz - luz) * inv_sw;
        int x0 = xs < 0 ? 0 : xs;
        int x1 = xe >= cam_w ? cam_w - 1 : xe;
        float skip = (float)(x0 - xs);
        float cz = lz + dz * skip;
        float cux = lux + dux * skip;
        float cuy = luy + duy * skip;
        float cuz = luz + duz * skip;
        int bi = y * cam_w + x0;
        for (int x = x0; x <= x1; ++x, ++bi, cz += dz, cux += dux, cuy += duy, cuz += duz) {
          if (cz >= z_buf[bi]) continue;
          float inv_z = 1.0f / cuz;
          int tx = (int)(cux * inv_z * tex_w) & tex_wm;
          int ty = (int)(cuy * inv_z * tex_h) & tex_hm;
          sf_pkd_clr_t texel = tex_px[ty * tex_w + tx];
          if ((texel >> 24) == 0) continue;
          uint32_t tr = (texel >> 16) & 0xFF;
          uint32_t tg = (texel >> 8) & 0xFF;
          uint32_t tb = texel & 0xFF;
          uint32_t lr = (tr * li_r) >> 8; if (lr > 255) lr = 255;
          uint32_t lg = (tg * li_g) >> 8; if (lg > 255) lg = 255;
          uint32_t lb = (tb * li_b) >> 8; if (lb > 255) lb = 255;
          if (opa_full) {
            z_buf[bi] = cz;
            cam_buf[bi] = 0xFF000000u | ((uint32_t)_sf_gamma_lut[lr] << 16) | ((uint32_t)_sf_gamma_lut[lg] << 8) | _sf_gamma_lut[lb];
          } else {
            uint32_t bg = cam_buf[bi];
            uint32_t bg_r = _sf_degamma_lut[(bg >> 16) & 0xFF];
            uint32_t bg_g = _sf_degamma_lut[(bg >> 8)  & 0xFF];
            uint32_t bg_b = _sf_degamma_lut[ bg         & 0xFF];
            uint32_t fr = (lr * opa8 + bg_r * inv_opa8) >> 8; if (fr > 255) fr = 255;
            uint32_t fg = (lg * opa8 + bg_g * inv_opa8) >> 8; if (fg > 255) fg = 255;
            uint32_t fb = (lb * opa8 + bg_b * inv_opa8) >> 8; if (fb > 255) fb = 255;
            cam_buf[bi] = 0xFF000000u | ((uint32_t)_sf_gamma_lut[fr] << 16) | ((uint32_t)_sf_gamma_lut[fg] << 8) | _sf_gamma_lut[fb];
          }
        }
      }
      ax += dxa; az += dza; aux += duxa; auy += duya; auz += duza;
      bx += dxb; bz += dzb; bux += duxb; buy += duyb; buz += duzb;
    }
  }
}

void sf_put_text(sf_ctx_t *ctx, sf_cam_t *cam, const char *text, sf_ivec2_t p, sf_pkd_clr_t c, int scale) {
  /* Render a null-terminated string using the built-in 8×8 bitmap font at integer scale. */
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
  /* Reset the z-buffer to maximum depth (0x7F7F7F7F ≈ far). */
  memset(cam->z_buffer, 0x7F, cam->buffer_size * sizeof(float));
}

void sf_draw_cam_pip(sf_ctx_t *ctx, sf_cam_t *dest, sf_cam_t *src, sf_ivec2_t pos) {
  /* Blit a secondary camera's buffer into dest at screen-space pos (picture-in-picture). */
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
  /* Draw the full debug overlay: world axes, frames, lights, cameras, and performance stats. */
  sf_draw_debug_axes   (ctx, cam);
  sf_draw_debug_frames (ctx, cam, 1.0f);
  sf_draw_debug_lights (ctx, cam, 1.0f);
  sf_draw_debug_cams   (ctx, cam, 1.0f);
  sf_draw_debug_perf   (ctx, cam);
}


void sf_draw_debug_axes(sf_ctx_t *ctx, sf_cam_t *cam) {
  /* Draw the world-space X/Y/Z axes as RGB lines in a corner compass widget. */
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
  /* Project and draw every scene-graph frame's local axes and parent-link lines. */
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
  /* Draw a star (point) or arrow (dir) gizmo for each light in the scene. */
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
  /* Draw frustum wireframes for every camera except view_cam itself. */
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

void sf_draw_debug_perf(sf_ctx_t *ctx, sf_cam_t *cam) {
  /* Render a HUD bar showing FPS, delta time, triangle count, and arena usage. */
  if (!ctx || !cam) return;

  int bh = 18;
  char buf[64];
  sf_pkd_clr_t dim = (sf_pkd_clr_t)0xFF666666;
  sf_pkd_clr_t val = SF_CLR_WHITE;

  sf_rect(ctx, cam, (sf_pkd_clr_t)0xFF111111, (sf_ivec2_t){0, 0}, (sf_ivec2_t){cam->w - 1, bh - 1});
  sf_line(ctx, cam, (sf_pkd_clr_t)0xFF333333, (sf_ivec2_t){0, bh - 1}, (sf_ivec2_t){cam->w - 1, bh - 1});

  int ty = 5;
  int col = 8;

  sf_pkd_clr_t saf_clr[7] = {
    (sf_pkd_clr_t)0xFFFF4444, (sf_pkd_clr_t)0xFFFF8800,
    (sf_pkd_clr_t)0xFFFFEE00, (sf_pkd_clr_t)0xFF00FF44,
    (sf_pkd_clr_t)0xFF00DDFF, (sf_pkd_clr_t)0xFF4488FF,
    (sf_pkd_clr_t)0xFFDD44FF
  };
  int saf_yoff[7] = { -1, 0, -1, 0, -1, 1, 0 };
  const char *saf_str = "SAFFRON";
  for (int i = 0; i < 7; i++) {
    char ch[2] = { saf_str[i], '\0' };
    sf_put_text(ctx, cam, ch, (sf_ivec2_t){col + i * 8, ty + saf_yoff[i]}, saf_clr[i], 1);
  }
  int perf_total_w = 112 + 120 + 120 + 136;
  col = cam->w - perf_total_w - 8;

  sf_put_text(ctx, cam, "fps", (sf_ivec2_t){col, ty}, dim, 1);
  snprintf(buf, sizeof(buf), "%6.1f", ctx->fps);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){col + 32, ty}, val, 1);
  col += 112;

  sf_put_text(ctx, cam, "dt", (sf_ivec2_t){col, ty}, dim, 1);
  snprintf(buf, sizeof(buf), "%6.2fms", ctx->delta_time * 1000.0f);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){col + 24, ty}, val, 1);
  col += 120;

  sf_put_text(ctx, cam, "tri", (sf_ivec2_t){col, ty}, dim, 1);
  snprintf(buf, sizeof(buf), "%7d", ctx->_perf_tri_count);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){col + 32, ty}, val, 1);
  col += 120;

  float mem_used = (float)ctx->arena.offset / (1024.0f * 1024.0f);
  float mem_pct = ((float)ctx->arena.offset / (float)ctx->arena.size) * 100.0f;
  sf_put_text(ctx, cam, "mem", (sf_ivec2_t){col, ty}, dim, 1);
  snprintf(buf, sizeof(buf), "%5.1fMB %3.0f%%", mem_used, mem_pct);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){col + 32, ty}, val, 1);
}

void sf_draw_sprite(sf_ctx_t *ctx, sf_cam_t *cam, sf_sprite_t *spr, sf_fvec3_t pos_w, float anim_time, float scale_mult) {
  /* Billboard a sprite frame at a world position with depth testing and alpha blending. */
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

sf_sprite_3d_t* sf_add_sprite_3d(sf_ctx_t *ctx, sf_sprite_t *spr, const char *name, sf_fvec3_t pos, float scale, float opacity, float angle) {
  /* Add a billboard instance to the scene's bill pool. */
  if (!ctx || ctx->sprite_3d_count >= SF_MAX_SPRITE_3DS) return NULL;
  sf_sprite_3d_t *b = &ctx->sprite_3ds[ctx->sprite_3d_count++];
  if (name) { int i; for (i = 0; i < 31 && name[i]; i++) b->name[i] = name[i]; b->name[i] = '\0'; }
  else { b->name[0] = '\0'; }
  b->sprite  = spr;
  b->pos     = pos;
  b->scale   = scale;
  b->opacity = opacity;
  b->angle   = angle;
  b->normal  = (sf_fvec3_t){0.f, 0.f, 0.f};
  b->frame   = NULL;
  return b;
}

void sf_clear_sprite_3ds(sf_ctx_t *ctx) {
  /* Remove all billboard instances from the scene. */
  if (ctx) ctx->sprite_3d_count = 0;
}

void sf_draw_sprite_3d(sf_ctx_t *ctx, sf_cam_t *cam, sf_sprite_3d_t *bill, float anim_time) {
  /* Billboard a sprite instance with per-instance scale, opacity, and screen-space rotation. */
  if (!bill || !bill->sprite || bill->sprite->frame_count == 0) return;
  int frame_idx = bill->sprite->frame_duration > 0.f
      ? (int)(anim_time / bill->sprite->frame_duration) % bill->sprite->frame_count : 0;
  sf_tex_t *tex = bill->sprite->frames[frame_idx];
  if (!tex) return;

  sf_fvec3_t world_pos = bill->pos;
  sf_fvec3_t world_normal = bill->normal;
  if (bill->frame) {
    sf_fmat4_t gM = bill->frame->global_M;
    world_pos = sf_fmat4_mul_vec3(gM, bill->pos);
    world_normal.x = bill->normal.x*gM.m[0][0] + bill->normal.y*gM.m[1][0] + bill->normal.z*gM.m[2][0];
    world_normal.y = bill->normal.x*gM.m[0][1] + bill->normal.y*gM.m[1][1] + bill->normal.z*gM.m[2][1];
    world_normal.z = bill->normal.x*gM.m[0][2] + bill->normal.y*gM.m[1][2] + bill->normal.z*gM.m[2][2];
  }

  sf_fvec3_t v_view = sf_fmat4_mul_vec3(cam->V, world_pos);
  if (v_view.z >= -cam->near_plane) return;

  float nl2 = world_normal.x*world_normal.x + world_normal.y*world_normal.y + world_normal.z*world_normal.z;
  if (nl2 > 0.001f) {
    float inv_nl = 1.f / sqrtf(nl2);
    sf_fvec3_t N = {world_normal.x*inv_nl, world_normal.y*inv_nl, world_normal.z*inv_nl};
    sf_fvec3_t up_ref = {0.f, 1.f, 0.f};
    if (fabsf(N.y) > 0.98f) up_ref = (sf_fvec3_t){1.f, 0.f, 0.f};
    sf_fvec3_t T = sf_fvec3_norm(sf_fvec3_cross(up_ref, N));
    sf_fvec3_t B3 = sf_fvec3_cross(N, T);
    float hs = bill->sprite->base_scale * bill->scale * 0.5f;
    sf_fvec3_t corners[4] = {
      {world_pos.x+(-T.x-B3.x)*hs, world_pos.y+(-T.y-B3.y)*hs, world_pos.z+(-T.z-B3.z)*hs},
      {world_pos.x+( T.x-B3.x)*hs, world_pos.y+( T.y-B3.y)*hs, world_pos.z+( T.z-B3.z)*hs},
      {world_pos.x+( T.x+B3.x)*hs, world_pos.y+( T.y+B3.y)*hs, world_pos.z+( T.z+B3.z)*hs},
      {world_pos.x+(-T.x+B3.x)*hs, world_pos.y+(-T.y+B3.y)*hs, world_pos.z+(-T.z+B3.z)*hs},
    };
    float uvals[4][2] = {{0.f,0.f},{0.999f,0.f},{0.999f,0.999f},{0.f,0.999f}};
    sf_fvec3_t vv[4], uvz[4], sv[4];
    for (int i = 0; i < 4; i++) {
      vv[i] = sf_fmat4_mul_vec3(cam->V, corners[i]);
      if (vv[i].z >= -cam->near_plane) return;
      float iz = 1.f / vv[i].z;
      uvz[i] = (sf_fvec3_t){uvals[i][0]*iz, uvals[i][1]*iz, iz};
      sv[i] = _sf_project_vertex(ctx, cam, vv[i], cam->P);
    }
    sf_fvec3_t l_int = {1.f, 1.f, 1.f};
    sf_tri_tex(ctx, cam, tex, sv[0], sv[1], sv[2], uvz[0], uvz[1], uvz[2], l_int, bill->opacity);
    sf_tri_tex(ctx, cam, tex, sv[0], sv[2], sv[3], uvz[0], uvz[2], uvz[3], l_int, bill->opacity);
    return;
  }

  sf_fvec3_t center_scr = _sf_project_vertex(ctx, cam, v_view, cam->P);
  float actual_scale = bill->sprite->base_scale * bill->scale;
  sf_fvec3_t edge_view = { v_view.x + actual_scale, v_view.y + actual_scale, v_view.z };
  sf_fvec3_t edge_scr  = _sf_project_vertex(ctx, cam, edge_view, cam->P);
  float hw = fabsf(edge_scr.x - center_scr.x);
  float hh = fabsf(edge_scr.y - center_scr.y);
  if (hw < 1.f) hw = 1.f;
  if (hh < 1.f) hh = 1.f;
  float ca = cosf(bill->angle), sa = sinf(bill->angle);
  float bb_hw = fabsf(hw * ca) + fabsf(hh * sa);
  float bb_hh = fabsf(hw * sa) + fabsf(hh * ca);
  float cx = center_scr.x, cy = center_scr.y, cz = center_scr.z;
  int x0 = (int)(cx - bb_hw) - 1, x1 = (int)(cx + bb_hw) + 1;
  int y0 = (int)(cy - bb_hh) - 1, y1 = (int)(cy + bb_hh) + 1;
  if (x1 < 0 || x0 >= cam->w || y1 < 0 || y0 >= cam->h) return;
  if (x0 < 0) x0 = 0; if (x1 >= cam->w) x1 = cam->w - 1;
  if (y0 < 0) y0 = 0; if (y1 >= cam->h) y1 = cam->h - 1;
  uint8_t a8 = (uint8_t)(bill->opacity * 255.f + 0.5f);
  bool opaque = (a8 >= 252);
  uint8_t inv_a8 = 255 - a8;
  int tex_w = tex->w, tex_h = tex->h;
  sf_pkd_clr_t *tex_px = tex->px;
  for (int y = y0; y <= y1; y++) {
    float ry = (float)y - cy;
    int row = y * cam->w;
    for (int x = x0; x <= x1; x++) {
      float rx = (float)x - cx;
      float lx =  rx * ca + ry * sa;
      float ly = -rx * sa + ry * ca;
      if (lx < -hw || lx > hw || ly < -hh || ly > hh) continue;
      int bi = row + x;
      if (cz >= cam->z_buffer[bi]) continue;
      int tx = (int)((lx / hw + 1.f) * 0.5f * (float)(tex_w - 1) + 0.5f);
      int ty = (int)((ly / hh + 1.f) * 0.5f * (float)(tex_h - 1) + 0.5f);
      if (tx < 0) tx = 0; if (tx >= tex_w) tx = tex_w - 1;
      if (ty < 0) ty = 0; if (ty >= tex_h) ty = tex_h - 1;
      sf_pkd_clr_t texel = tex_px[ty * tex_w + tx];
      if ((texel >> 24) == 0) continue;
      uint32_t tr = (texel >> 16) & 0xFF;
      uint32_t tg = (texel >> 8)  & 0xFF;
      uint32_t tb =  texel         & 0xFF;
      if (opaque) {
        cam->z_buffer[bi] = cz;
        cam->buffer[bi] = 0xFF000000u | ((uint32_t)_sf_gamma_lut[tr] << 16) | ((uint32_t)_sf_gamma_lut[tg] << 8) | _sf_gamma_lut[tb];
      } else {
        cam->z_buffer[bi] = cz;
        uint32_t bg = cam->buffer[bi];
        uint32_t bg_lr = _sf_degamma_lut[(bg >> 16) & 0xFF];
        uint32_t bg_lg = _sf_degamma_lut[(bg >> 8)  & 0xFF];
        uint32_t bg_lb = _sf_degamma_lut[ bg         & 0xFF];
        uint32_t or_ = (tr * a8 + bg_lr * inv_a8) >> 8; if (or_ > 255) or_ = 255;
        uint32_t og  = (tg * a8 + bg_lg * inv_a8) >> 8; if (og  > 255) og  = 255;
        uint32_t ob  = (tb * a8 + bg_lb * inv_a8) >> 8; if (ob  > 255) ob  = 255;
        cam->buffer[bi] = 0xFF000000u | ((uint32_t)_sf_gamma_lut[or_] << 16) | ((uint32_t)_sf_gamma_lut[og] << 8) | _sf_gamma_lut[ob];
      }
    }
  }
}

/* SF_UI_FUNCTIONS */
sf_ui_t* sf_ui_create (sf_ctx_t *ctx) {
  /* Allocate and initialize a UI context with a default style from the arena. */
  sf_ui_t *ui = (sf_ui_t*)sf_arena_alloc(ctx, &ctx->arena, sizeof(sf_ui_t));
  ui->elements = sf_arena_alloc(ctx, &ctx->arena, SF_MAX_UI_ELEMENTS * sizeof(sf_ui_lmn_t));
  ui->count = 0;
  ui->focused = NULL;
  ui->default_style.color_base   = (sf_pkd_clr_t)0xFF404040;
  ui->default_style.color_hover  = (sf_pkd_clr_t)0xFF555555;
  ui->default_style.color_active = (sf_pkd_clr_t)0xFF707070;
  ui->default_style.color_text   = (sf_pkd_clr_t)0xFFEEEEEE;
  ui->default_style.draw_outline = false;
  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "capct  : %d\n",
              SF_MAX_UI_ELEMENTS);
  return ui;
}

sf_ui_lmn_t* sf_ui_add_button(sf_ctx_t *ctx, const char *text, sf_ivec2_t v0, sf_ivec2_t v1, void (*cb)(sf_ctx_t*, void*), void *userdata) {
  /* Add a clickable button with a label and optional click callback. */
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;

  sf_ui_lmn_t *parent = _sf_ui_find_prnt_pnl(ctx, v0, v1);
  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));
  el->parent_panel    = parent;

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
              SF_LOG_INDENT "pos    : (%d,%d)-(%d,%d)\n"
              SF_LOG_INDENT "used   : %d/%d\n",
              text, v0.x, v0.y, v1.x, v1.y, ctx->ui->count, SF_MAX_UI_ELEMENTS);
  return el;
}

sf_ui_lmn_t* sf_ui_add_slider(sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, float min_val, float max_val, float init_val, sf_ui_cb cb, void *userdata) {
  /* Add a horizontal slider with a float range and optional on-change callback. */
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;

  sf_ui_lmn_t *parent = _sf_ui_find_prnt_pnl(ctx, v0, v1);
  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));
  el->parent_panel    = parent;

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

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "pos    : (%d,%d)-(%d,%d)\n"
              SF_LOG_INDENT "range  : %.2f-%.2f\n"
              SF_LOG_INDENT "init   : %.2f\n"
              SF_LOG_INDENT "used   : %d/%d\n",
              v0.x, v0.y, v1.x, v1.y, min_val, max_val, init_val, ctx->ui->count, SF_MAX_UI_ELEMENTS);
  return el;
}

sf_ui_lmn_t* sf_ui_add_checkbox(sf_ctx_t *ctx, const char *text, sf_ivec2_t v0, sf_ivec2_t v1, bool init_state, sf_ui_cb cb, void *userdata) {
  /* Add a checkbox with a label, initial checked state, and optional on-toggle callback. */
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;

  sf_ui_lmn_t *parent = _sf_ui_find_prnt_pnl(ctx, v0, v1);
  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));
  el->parent_panel        = parent;

  el->type                = SF_UI_CHECKBOX;
  el->style               = ctx->ui->default_style;
  el->v0                  = v0;
  el->v1                  = v1;
  el->is_visible          = true;

  el->checkbox.text       = text;
  el->checkbox.is_checked = init_state;
  el->checkbox.callback   = cb;
  el->checkbox.userdata   = userdata;

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "text   : %s\n"
              SF_LOG_INDENT "pos    : (%d,%d)-(%d,%d)\n"
              SF_LOG_INDENT "init   : %s\n"
              SF_LOG_INDENT "used   : %d/%d\n",
              text, v0.x, v0.y, v1.x, v1.y, init_state ? "checked" : "unchecked", ctx->ui->count, SF_MAX_UI_ELEMENTS);
  return el;
}

void _sf_draw_image(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el) {
  /* Scale-blit el->image.tex into el's rect; skip alpha=0 pixels when keyed. */
  (void)ctx;
  sf_tex_t *tex = el->image.tex;
  if (!tex || !tex->px) return;
  int dw = el->v1.x - el->v0.x, dh = el->v1.y - el->v0.y;
  if (dw <= 0 || dh <= 0) return;
  bool keyed = el->image.keyed;
  for (int y = 0; y < dh; y++) {
    int py = el->v0.y + y; if (py < 0 || py >= cam->h) continue;
    int sy = (y * tex->h) / dh;
    for (int x = 0; x < dw; x++) {
      int px_ = el->v0.x + x; if (px_ < 0 || px_ >= cam->w) continue;
      int sx = (x * tex->w) / dw;
      sf_pkd_clr_t c = tex->px[sy * tex->w + sx];
      if (keyed && (c >> 24) == 0) continue;
      cam->buffer[py * cam->w + px_] = c;
    }
  }
}

void sf_ui_render(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_t *ui) {
  /* Draw all visible UI elements; dropdown popups are drawn last to appear on top. */
  if (!ui) return;
  for (int i = 0; i < ui->count; ++i) {
    sf_ui_lmn_t *el = &ui->elements[i];
    if (!_sf_ui_eff_visible(el)) continue;
    switch (el->type) {
      case SF_UI_BUTTON:      _sf_draw_button(ctx, cam, el);          break;
      case SF_UI_SLIDER:      _sf_draw_slider(ctx, cam, el);          break;
      case SF_UI_CHECKBOX:    _sf_draw_checkbox(ctx, cam, el);        break;
      case SF_UI_LABEL:       _sf_draw_label(ctx, cam, el);       break;
      case SF_UI_TEXT_INPUT:  _sf_draw_text_input(ctx, cam, el);  break;
      case SF_UI_DRAG_FLOAT:  _sf_draw_drag_float(ctx, cam, el);  break;
      case SF_UI_DROPDOWN:    _sf_draw_dropdown(ctx, cam, el);    break;
      case SF_UI_PANEL:       _sf_draw_panel(ctx, cam, el);       break;
      case SF_UI_IMAGE:       _sf_draw_image(ctx, cam, el);       break;
    }
  }
  for (int i = 0; i < ui->count; ++i) {
    sf_ui_lmn_t *el = &ui->elements[i];
    if (!_sf_ui_eff_visible(el)) continue;
    if (el->type == SF_UI_DROPDOWN && el->dropdown.is_open) {
      _sf_draw_drpdwn_popup(ctx, cam, el);
    }
  }
}

void sf_ui_render_popups(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_t *ui) {
  /* Re-draw only open dropdown popups — call after any custom blitting to keep them on top. */
  if (!ui) return;
  for (int i = 0; i < ui->count; ++i) {
    sf_ui_lmn_t *el = &ui->elements[i];
    if (!_sf_ui_eff_visible(el)) continue;
    if (el->type == SF_UI_DROPDOWN && el->dropdown.is_open)
      _sf_draw_drpdwn_popup(ctx, cam, el);
  }
}

void sf_ui_update(sf_ctx_t *ctx, sf_ui_t *ui) {
  /* Process mouse hover/click input for every UI element and fire their callbacks. */
  if (!ui) return;

  int mx = ctx->input.mouse_x;
  int my = ctx->input.mouse_y;
  bool m_down = ctx->input.mouse_btns[SF_MOUSE_LEFT];
  bool m_pressed = m_down && !ctx->input.mouse_btns_prev[SF_MOUSE_LEFT];
  bool m_released = !m_down && ctx->input.mouse_btns_prev[SF_MOUSE_LEFT];

  /* Check if an open dropdown popup area is consuming this click */
  bool click_blocked = false;
  for (int i = 0; i < ui->count; ++i) {
    sf_ui_lmn_t *el = &ui->elements[i];
    if (el->type != SF_UI_DROPDOWN || !el->dropdown.is_open) continue;
    int h = el->v1.y - el->v0.y, w = el->v1.x - el->v0.x;
    int n = el->dropdown.n_items;
    int mv = (el->dropdown.max_visible > 0 && n > el->dropdown.max_visible)
             ? el->dropdown.max_visible : n;
    int py0 = el->v1.y, py1 = el->v1.y + mv * h;
    if (mx >= el->v0.x && mx <= el->v0.x + w && my >= py0 && my <= py1) {
      click_blocked = true; break;
    }
  }

  for (int i = 0; i < ui->count; ++i) {
    sf_ui_lmn_t *el = &ui->elements[i];

    if (!_sf_ui_eff_visible(el) || el->is_disabled) {
      el->is_hovered = false;
      el->is_pressed = false;
      continue;
    }

    el->is_hovered = (mx >= el->v0.x && mx <= el->v1.x &&
                      my >= el->v0.y && my <= el->v1.y);

    bool eff_pressed  = m_pressed  && (el->type == SF_UI_DROPDOWN || !click_blocked);
    bool eff_released = m_released && (el->type == SF_UI_DROPDOWN || !click_blocked);

    if (el->is_hovered && eff_pressed) {
      el->is_pressed = true;
    }

    switch (el->type) {
      case SF_UI_BUTTON:      _sf_update_button(ctx, el, m_down, eff_pressed, eff_released);   break;
      case SF_UI_CHECKBOX:    _sf_update_checkbox(ctx, el, m_down, eff_pressed, eff_released); break;
      case SF_UI_SLIDER:      _sf_update_slider(ctx, el, m_down, eff_pressed, eff_released);   break;
      case SF_UI_TEXT_INPUT:  _sf_update_text_input(ctx, el, eff_pressed);                     break;
      case SF_UI_DRAG_FLOAT:  _sf_update_drag_float(ctx, el, m_down, eff_pressed);             break;
      case SF_UI_DROPDOWN:    _sf_update_dropdown(ctx, el, m_pressed);                         break;
      case SF_UI_PANEL:       _sf_update_panel(ctx, el, eff_pressed);                          break;
      case SF_UI_LABEL:       break;
    }

    if (!m_down) {
      el->is_pressed = false;
    }
  }
}

void sf_ui_clear(sf_ctx_t *ctx) {
  /* Remove all UI elements and clear focus without freeing arena memory. */
  if (!ctx->ui) return;
  ctx->ui->count = 0;
  ctx->ui->focused = NULL;
}

void sf_ui_set_callback(sf_ui_lmn_t *el, sf_ui_cb cb, void *userdata) {
  /* Assign a callback and userdata to any UI element type. */
  if (!el) return;
  switch (el->type) {
    case SF_UI_BUTTON:     el->button.callback = cb;     el->button.userdata = userdata;     break;
    case SF_UI_SLIDER:     el->slider.callback = cb;     el->slider.userdata = userdata;     break;
    case SF_UI_CHECKBOX:   el->checkbox.callback = cb;   el->checkbox.userdata = userdata;   break;
    case SF_UI_TEXT_INPUT: el->text_input.callback = cb; el->text_input.userdata = userdata; break;
    case SF_UI_DRAG_FLOAT: el->drag_float.callback = cb; el->drag_float.userdata = userdata; break;
    case SF_UI_DROPDOWN:   el->dropdown.callback = cb;   el->dropdown.userdata = userdata;   break;
    case SF_UI_LABEL:
    case SF_UI_PANEL: break;
  }
}

sf_ui_lmn_t* sf_ui_get_by_name(sf_ui_t *ui, const char *name) {
  /* Look up a UI element by its name string; returns NULL if not found. */
  if (!ui || !name) return NULL;
  for (int i = 0; i < ui->count; i++) {
    sf_ui_lmn_t *el = &ui->elements[i];
    if (el->name && strcmp(el->name, name) == 0) return el;
  }
  return NULL;
}

sf_ui_lmn_t* sf_ui_add_label(sf_ctx_t *ctx, const char *text, sf_ivec2_t pos, sf_pkd_clr_t color) {
  /* Add a non-interactive text label at a pixel position. */
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;
  sf_ivec2_t v1 = { pos.x + (int)strlen(text) * 8, pos.y + 8 };
  sf_ui_lmn_t *parent = _sf_ui_find_prnt_pnl(ctx, pos, v1);
  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));
  el->parent_panel = parent;
  el->type       = SF_UI_LABEL;
  el->style      = ctx->ui->default_style;
  el->v0         = pos;
  el->v1         = v1;
  el->is_visible = true;
  el->label.text  = text;
  el->label.color = color ? color : el->style.color_text;
  return el;
}

sf_ui_lmn_t* sf_ui_add_text_input(sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, char *buf, int buflen, sf_ui_cb cb, void *ud) {
  /* Add a single-line text-input field backed by a caller-supplied buffer. */
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;
  sf_ui_lmn_t *parent = _sf_ui_find_prnt_pnl(ctx, v0, v1);
  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));
  el->parent_panel = parent;
  el->type       = SF_UI_TEXT_INPUT;
  el->style      = ctx->ui->default_style;
  el->v0         = v0;
  el->v1         = v1;
  el->is_visible = true;
  el->text_input.buf      = buf;
  el->text_input.buflen   = buflen;
  el->text_input.caret    = (int)strlen(buf);
  el->text_input.callback = cb;
  el->text_input.userdata = ud;
  return el;
}

sf_ui_lmn_t* sf_ui_add_drag_float(sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, float *target, float step, sf_ui_cb cb, void *ud) {
  /* Add a drag-to-change float widget that directly modifies a float pointer. */
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;
  sf_ui_lmn_t *parent = _sf_ui_find_prnt_pnl(ctx, v0, v1);
  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));
  el->parent_panel = parent;
  el->type       = SF_UI_DRAG_FLOAT;
  el->style      = ctx->ui->default_style;
  el->v0         = v0;
  el->v1         = v1;
  el->is_visible = true;
  el->drag_float.target    = target;
  el->drag_float.step      = step;
  el->drag_float.callback  = cb;
  el->drag_float.userdata  = ud;
  return el;
}

sf_ui_lmn_t* sf_ui_add_dropdown(sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, const char **items, int n, int *selected, sf_ui_cb cb, void *ud) {
  /* Add a dropdown selector from a string array; writes the chosen index to *selected. */
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;
  sf_ui_lmn_t *parent = _sf_ui_find_prnt_pnl(ctx, v0, v1);
  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));
  el->parent_panel = parent;
  el->type       = SF_UI_DROPDOWN;
  el->style      = ctx->ui->default_style;
  el->v0         = v0;
  el->v1         = v1;
  el->is_visible = true;
  el->dropdown.items      = items;
  el->dropdown.n_items    = n;
  el->dropdown.selected   = selected;
  el->dropdown.is_open      = false;
  el->dropdown.hover_item   = -1;
  el->dropdown.scroll_offset = 0;
  el->dropdown.max_visible   = SF_DROPDOWN_MAX_VISIBLE;
  el->dropdown.callback      = cb;
  el->dropdown.userdata      = ud;
  return el;
}

sf_ui_lmn_t* sf_ui_add_panel(sf_ctx_t *ctx, const char *title, sf_ivec2_t v0, sf_ivec2_t v1) {
  /* Add a collapsible panel container; child elements positioned within it are hidden when collapsed. */
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;
  sf_ui_lmn_t *parent = _sf_ui_find_prnt_pnl(ctx, v0, v1);
  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));
  el->parent_panel = parent;
  el->type       = SF_UI_PANEL;
  el->style      = ctx->ui->default_style;
  el->v0         = v0;
  el->v1         = v1;
  el->is_visible = true;
  el->panel.title     = title;
  el->panel.collapsed = false;
  el->panel.content_h = v1.y - v0.y - 16;
  return el;
}

sf_ui_lmn_t* sf_ui_add_image(sf_ctx_t *ctx, sf_tex_t *tex, sf_ivec2_t v0, sf_ivec2_t v1, bool keyed) {
  /* Add an image element; rendered as a scaled blit that hides when its parent panel collapses. */
  if (!ctx->ui || ctx->ui->count >= SF_MAX_UI_ELEMENTS) return NULL;
  sf_ui_lmn_t *parent = _sf_ui_find_prnt_pnl(ctx, v0, v1);
  sf_ui_lmn_t *el = &ctx->ui->elements[ctx->ui->count++];
  memset(el, 0, sizeof(sf_ui_lmn_t));
  el->parent_panel = parent;
  el->type       = SF_UI_IMAGE;
  el->v0         = v0;
  el->v1         = v1;
  el->is_visible = true;
  el->image.tex   = tex;
  el->image.keyed = keyed;
  return el;
}

bool sf_save_sfui(sf_ctx_t *ctx, sf_ui_t *ui, const char *filepath) {
  /* Serialize all UI elements to a .sfui text file for later loading. */
  if (!ctx || !ui || !filepath) return false;
  FILE *f = fopen(filepath, "w");
  if (!f) { SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "could not write %s\n", filepath); return false; }
  fprintf(f, "# Saffron UI File\n\n");
  for (int i = 0; i < ui->count; i++) {
    sf_ui_lmn_t *el = &ui->elements[i];
    fprintf(f, "ui %s {\n", _sf_ui_type_str(el->type));
    if (el->name) { fprintf(f, "    name = "); _sf_write_qstr(f, el->name); fprintf(f, "\n"); }
    fprintf(f, "    v0 = (%d, %d)\n", el->v0.x, el->v0.y);
    fprintf(f, "    v1 = (%d, %d)\n", el->v1.x, el->v1.y);
    fprintf(f, "    visible = %d\n", el->is_visible ? 1 : 0);
    switch (el->type) {
      case SF_UI_BUTTON:
        fprintf(f, "    text = "); _sf_write_qstr(f, el->button.text); fprintf(f, "\n");
        break;
      case SF_UI_CHECKBOX:
        fprintf(f, "    text = "); _sf_write_qstr(f, el->checkbox.text); fprintf(f, "\n");
        fprintf(f, "    checked = %d\n", el->checkbox.is_checked ? 1 : 0);
        break;
      case SF_UI_LABEL:
        fprintf(f, "    text = "); _sf_write_qstr(f, el->label.text); fprintf(f, "\n");
        fprintf(f, "    color = 0x%08X\n", (unsigned)el->label.color);
        break;
      case SF_UI_SLIDER:
        fprintf(f, "    min = %.4f\n", el->slider.min_val);
        fprintf(f, "    max = %.4f\n", el->slider.max_val);
        fprintf(f, "    value = %.4f\n", el->slider.value);
        break;
      case SF_UI_TEXT_INPUT:
        fprintf(f, "    buflen = %d\n", el->text_input.buflen);
        fprintf(f, "    text = "); _sf_write_qstr(f, el->text_input.buf ? el->text_input.buf : ""); fprintf(f, "\n");
        break;
      case SF_UI_DRAG_FLOAT:
        fprintf(f, "    step = %.4f\n", el->drag_float.step);
        break;
      case SF_UI_DROPDOWN:
        fprintf(f, "    n_items = %d\n", el->dropdown.n_items);
        for (int k = 0; k < el->dropdown.n_items; k++) {
          fprintf(f, "    item = "); _sf_write_qstr(f, el->dropdown.items[k] ? el->dropdown.items[k] : ""); fprintf(f, "\n");
        }
        fprintf(f, "    selected = %d\n", el->dropdown.selected ? *el->dropdown.selected : 0);
        break;
      case SF_UI_PANEL:
        fprintf(f, "    title = "); _sf_write_qstr(f, el->panel.title); fprintf(f, "\n");
        fprintf(f, "    collapsed = %d\n", el->panel.collapsed ? 1 : 0);
        break;
    }
    fprintf(f, "}\n\n");
  }
  fclose(f);
  SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "wrote %s\n", filepath);
  return true;
}

sf_ui_t* sf_load_sfui(sf_ctx_t *ctx, const char *filepath) {
  /* Parse a .sfui file and reconstruct all UI elements, returning the populated sf_ui_t. */
  if (!ctx || !filepath) return NULL;
  FILE *f = fopen(filepath, "r");
  if (!f) { SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "could not open %s\n", filepath); return NULL; }

  sf_ui_t *saved_ui = ctx->ui;
  sf_ui_t *ui = sf_ui_create(ctx);
  ctx->ui = ui;

  char line[1024];
  int cur_type = -1;
  sf_ivec2_t v0 = {0,0}, v1 = {0,0};
  bool visible = true;
  char text[512] = "", title[128] = "", name[128] = "";
  float slmin = 0.0f, slmax = 1.0f, slval = 0.0f, df_step = 0.05f;
  int checked = 0, collapsed = 0, buflen = 64;
  unsigned color = 0xFFEEEEEE;
  const char *dd_items[32];
  int dd_count = 0;
  int dd_sel = 0;

  while (fgets(line, sizeof(line), f)) {
    char *s = line;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '#' || *s == '\n' || *s == '\0') continue;
    if (strncmp(s, "ui ", 3) == 0) {
      char tname[32] = "";
      sscanf(s + 3, "%31s", tname);
      cur_type = _sf_ui_type_from_str(tname);
      v0 = (sf_ivec2_t){0,0}; v1 = (sf_ivec2_t){100,20};
      visible = true; checked = 0; collapsed = 0; buflen = 64;
      slmin = 0.0f; slmax = 1.0f; slval = 0.0f; df_step = 0.05f;
      color = 0xFFEEEEEE;
      text[0] = '\0'; title[0] = '\0'; name[0] = '\0';
      dd_count = 0; dd_sel = 0;
      continue;
    }
    if (*s == '}') {
      if (cur_type < 0) continue;
      switch (cur_type) {
        case SF_UI_BUTTON:     sf_ui_add_button(ctx, _sf_arena_strdup(ctx, text), v0, v1, NULL, NULL); break;
        case SF_UI_SLIDER:     sf_ui_add_slider(ctx, v0, v1, slmin, slmax, slval, NULL, NULL); break;
        case SF_UI_CHECKBOX:   sf_ui_add_checkbox(ctx, _sf_arena_strdup(ctx, text), v0, v1, checked != 0, NULL, NULL); break;
        case SF_UI_LABEL:      {
          sf_ui_lmn_t *el = sf_ui_add_label(ctx, _sf_arena_strdup(ctx, text), v0, (sf_pkd_clr_t)color);
          if (el) el->v1 = v1;
          break;
        }
        case SF_UI_TEXT_INPUT: {
          char *buf = (char*)sf_arena_alloc(ctx, &ctx->arena, buflen);
          if (buf) { snprintf(buf, buflen, "%s", text); sf_ui_add_text_input(ctx, v0, v1, buf, buflen, NULL, NULL); }
          break;
        }
        case SF_UI_DRAG_FLOAT: {
          float *tgt = (float*)sf_arena_alloc(ctx, &ctx->arena, sizeof(float));
          if (tgt) { *tgt = 0.0f; sf_ui_add_drag_float(ctx, v0, v1, tgt, df_step, NULL, NULL); }
          break;
        }
        case SF_UI_DROPDOWN: {
          const char **items = (const char**)sf_arena_alloc(ctx, &ctx->arena, sizeof(char*) * (dd_count > 0 ? dd_count : 1));
          for (int k = 0; k < dd_count; k++) items[k] = dd_items[k];
          int *sel = (int*)sf_arena_alloc(ctx, &ctx->arena, sizeof(int));
          if (sel) *sel = dd_sel;
          sf_ui_add_dropdown(ctx, v0, v1, items, dd_count, sel, NULL, NULL);
          break;
        }
        case SF_UI_PANEL: {
          sf_ui_lmn_t *p = sf_ui_add_panel(ctx, _sf_arena_strdup(ctx, title), v0, v1);
          if (p) p->panel.collapsed = (collapsed != 0);
          break;
        }
      }
      if (ui->count > 0) {
        ui->elements[ui->count - 1].is_visible = visible;
        if (name[0]) ui->elements[ui->count - 1].name = _sf_arena_strdup(ctx, name);
      }
      cur_type = -1;
      continue;
    }
    int a, b;
    if      (sscanf(s, "v0 = (%d, %d)", &a, &b) == 2) v0 = (sf_ivec2_t){a, b};
    else if (sscanf(s, "v1 = (%d, %d)", &a, &b) == 2) v1 = (sf_ivec2_t){a, b};
    else if (sscanf(s, "visible = %d", &a) == 1) visible = (a != 0);
    else if (sscanf(s, "checked = %d", &a) == 1) checked = a;
    else if (sscanf(s, "collapsed = %d", &a) == 1) collapsed = a;
    else if (sscanf(s, "buflen = %d", &a) == 1) buflen = a;
    else if (sscanf(s, "selected = %d", &a) == 1) dd_sel = a;
    else if (sscanf(s, "n_items = %d", &a) == 1) {}
    else if (sscanf(s, "color = 0x%x", &color) == 1) {}
    else if (sscanf(s, "min = %f", &slmin) == 1) {}
    else if (sscanf(s, "max = %f", &slmax) == 1) {}
    else if (sscanf(s, "value = %f", &slval) == 1) {}
    else if (sscanf(s, "step = %f", &df_step) == 1) {}
    else if (_sf_parse_qstr(s, "name", name, sizeof(name))) {}
    else if (_sf_parse_qstr(s, "text", text, sizeof(text))) {}
    else if (_sf_parse_qstr(s, "title", title, sizeof(title))) {}
    else {
      char tmp[256];
      if (_sf_parse_qstr(s, "item", tmp, sizeof(tmp))) {
        if (dd_count < 32) dd_items[dd_count++] = _sf_arena_strdup(ctx, tmp);
      }
    }
  }

  fclose(f);
  ctx->ui = saved_ui;
  SF_LOG(ctx, SF_LOG_INFO, SF_LOG_INDENT "loaded %s (%d elems)\n", filepath, ui->count);
  return ui;
}

sf_ui_lmn_t* _sf_ui_find_prnt_pnl(sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1) {
  /* Find the innermost panel that fully contains the given rect; returns NULL if none. */
  if (!ctx->ui) return NULL;
  for (int i = ctx->ui->count - 1; i >= 0; i--) {
    sf_ui_lmn_t *p = &ctx->ui->elements[i];
    if (p->type != SF_UI_PANEL) continue;
    if (v0.x >= p->v0.x && v1.x <= p->v1.x && v0.y >= p->v0.y && v1.y <= p->v1.y) return p;
  }
  return NULL;
}

bool _sf_ui_eff_visible(sf_ui_lmn_t *el) {
  /* Return true if el and all its ancestor panels are visible and not collapsed. */
  if (!el->is_visible) return false;
  sf_ui_lmn_t *p = el->parent_panel;
  while (p) {
    if (!p->is_visible || p->panel.collapsed) return false;
    p = p->parent_panel;
  }
  return true;
}

void _sf_update_button(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released) {
  /* Fire the button callback on mouse-up while hovered. */
  if (el->is_hovered && el->is_pressed && m_released) {
    if (el->button.callback) {
      el->button.callback(ctx, el->button.userdata);
    }
  }
}

void _sf_draw_button(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *btn) {
  /* Render a button: fill rect with hover/pressed/disabled tint and center the label. */
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

void _sf_draw_slider(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el) {
  /* Render a slider: dark track, filled portion proportional to value, centered value text. */
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

void _sf_draw_checkbox(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el) {
  /* Render a checkbox: square box with a check mark when checked, plus label text. */
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

void _sf_draw_label(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el) {
  /* Render a static text label at its position with its assigned color. */
  if (!el->is_visible || !el->label.text) return;
  sf_put_text(ctx, cam, el->label.text, el->v0, el->label.color, 1);
}

void _sf_draw_text_input(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el) {
  /* Render a text-input box: background, buffer contents, and blinking caret when focused. */
  if (!el->is_visible) return;
  bool focused = (ctx->ui && ctx->ui->focused == el);
  sf_pkd_clr_t bg = focused ? el->style.color_active : (el->is_hovered ? el->style.color_hover : el->style.color_base);
  sf_rect(ctx, cam, bg, el->v0, el->v1);
  sf_rect(ctx, cam, (sf_pkd_clr_t)0xFF222222, (sf_ivec2_t){el->v0.x+1, el->v0.y+1}, (sf_ivec2_t){el->v1.x-1, el->v1.y-1});
  int h = el->v1.y - el->v0.y;
  sf_ivec2_t tp = { el->v0.x + 4, el->v0.y + (h - 8) / 2 };
  if (el->text_input.buf) sf_put_text(ctx, cam, el->text_input.buf, tp, el->style.color_text, 1);
  if (focused) {
    int cx = tp.x + el->text_input.caret * 8;
    sf_line(ctx, cam, el->style.color_text, (sf_ivec2_t){cx, tp.y}, (sf_ivec2_t){cx, tp.y + 8});
  }
}

void _sf_draw_drag_float(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el) {
  /* Render a drag-float widget showing the current float value centered in its box. */
  if (!el->is_visible) return;
  sf_pkd_clr_t bg = el->is_pressed ? el->style.color_active : (el->is_hovered ? el->style.color_hover : el->style.color_base);
  sf_rect(ctx, cam, bg, el->v0, el->v1);
  char buf[32];
  float v = el->drag_float.target ? *el->drag_float.target : 0.0f;
  snprintf(buf, sizeof(buf), "%.3f", v);
  int tw = (int)strlen(buf) * 8;
  int h = el->v1.y - el->v0.y;
  int w = el->v1.x - el->v0.x;
  sf_ivec2_t tp = { el->v0.x + (w - tw)/2, el->v0.y + (h - 8)/2 };
  sf_put_text(ctx, cam, buf, tp, el->style.color_text, 1);
}

void _sf_draw_dropdown(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el) {
  /* Render the collapsed dropdown header showing the selected item label and an arrow. */
  if (!el->is_visible) return;
  sf_pkd_clr_t bg = el->is_hovered ? el->style.color_hover : el->style.color_base;
  sf_rect(ctx, cam, bg, el->v0, el->v1);
  const char *lbl = "---";
  int sel = el->dropdown.selected ? *el->dropdown.selected : -1;
  if (sel >= 0 && sel < el->dropdown.n_items && el->dropdown.items) lbl = el->dropdown.items[sel];
  int h = el->v1.y - el->v0.y;
  sf_ivec2_t tp = { el->v0.x + 4, el->v0.y + (h - 8)/2 };
  sf_put_text(ctx, cam, lbl, tp, el->style.color_text, 1);
  sf_put_text(ctx, cam, "v", (sf_ivec2_t){el->v1.x - 12, el->v0.y + (h-8)/2}, el->style.color_text, 1);
}

void _sf_draw_drpdwn_popup(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el) {
  /* Render the open dropdown item list below the header, highlighting the hovered row.
     Only max_visible rows are shown at once; scroll_offset controls the first visible item. */
  if (!el->is_visible || !el->dropdown.is_open) return;
  int h = el->v1.y - el->v0.y;
  int w = el->v1.x - el->v0.x;
  int n = el->dropdown.n_items;
  int mv = (el->dropdown.max_visible > 0 && n > el->dropdown.max_visible)
           ? el->dropdown.max_visible : n;
  int scroll = el->dropdown.scroll_offset;
  if (scroll < 0) scroll = 0;
  if (scroll > n - mv) scroll = n - mv;
  if (scroll < 0) scroll = 0;
  for (int i = 0; i < mv; i++) {
    int item_idx = scroll + i;
    if (item_idx >= n) break;
    sf_ivec2_t iv0 = { el->v0.x, el->v1.y + i * h };
    sf_ivec2_t iv1 = { el->v0.x + w, el->v1.y + (i + 1) * h };
    sf_pkd_clr_t bg = (item_idx == el->dropdown.hover_item) ? el->style.color_hover : el->style.color_base;
    sf_rect(ctx, cam, bg, iv0, iv1);
    sf_put_text(ctx, cam, el->dropdown.items[item_idx], (sf_ivec2_t){iv0.x + 4, iv0.y + (h-8)/2}, el->style.color_text, 1);
  }
  if (scroll > 0)
    sf_put_text(ctx, cam, "^", (sf_ivec2_t){el->v0.x + w - 10, el->v1.y + (h-8)/2}, el->style.color_text, 1);
  if (scroll + mv < n)
    sf_put_text(ctx, cam, "v", (sf_ivec2_t){el->v0.x + w - 10, el->v1.y + (mv-1)*h + (h-8)/2}, el->style.color_text, 1);
}

void _sf_draw_panel(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el) {
  /* Render a panel: dark background, header bar, and collapse indicator prefix. */
  if (!el->is_visible) return;
  sf_ivec2_t full_v1 = el->panel.collapsed ? (sf_ivec2_t){ el->v1.x, el->v0.y + 16 } : el->v1;
  sf_rect(ctx, cam, (sf_pkd_clr_t)0xFF2A2A2A, el->v0, full_v1);
  sf_rect(ctx, cam, el->style.color_base, el->v0, (sf_ivec2_t){ el->v1.x, el->v0.y + 16 });
  if (el->panel.title) {
    const char *pfx = el->panel.collapsed ? "+ " : "- ";
    char buf[96];
    snprintf(buf, sizeof(buf), "%s%s", pfx, el->panel.title);
    sf_put_text(ctx, cam, buf, (sf_ivec2_t){el->v0.x + 4, el->v0.y + 4}, el->style.color_text, 1);
  }
}

void _sf_update_text_input(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_pressed) {
  /* Handle focus, character insertion/deletion, and caret movement for a text-input element. */
  if (!ctx->ui) return;
  if (el->is_hovered && m_pressed) ctx->ui->focused = el;
  if (ctx->ui->focused != el || !el->text_input.buf) return;
  int len = (int)strlen(el->text_input.buf);
  if (el->text_input.caret > len) el->text_input.caret = len;
  if (sf_key_pressed(ctx, SF_KEY_BACKSPACE) && el->text_input.caret > 0) {
    memmove(&el->text_input.buf[el->text_input.caret - 1], &el->text_input.buf[el->text_input.caret], len - el->text_input.caret + 1);
    el->text_input.caret--;
    if (el->text_input.callback) el->text_input.callback(ctx, el->text_input.userdata);
  }
  if (sf_key_pressed(ctx, SF_KEY_LEFT)  && el->text_input.caret > 0)   el->text_input.caret--;
  if (sf_key_pressed(ctx, SF_KEY_RIGHT) && el->text_input.caret < len) el->text_input.caret++;
  if (sf_key_pressed(ctx, SF_KEY_HOME))   el->text_input.caret = 0;
  if (sf_key_pressed(ctx, SF_KEY_END))    el->text_input.caret = len;
  if (sf_key_pressed(ctx, SF_KEY_RETURN) || sf_key_pressed(ctx, SF_KEY_ESC)) ctx->ui->focused = NULL;
}

void _sf_update_drag_float(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed) {
  /* Update a drag-float: record anchor on click, modify the target float by horizontal delta. */
  if (!el->drag_float.target) return;
  if (el->is_hovered && m_pressed) {
    el->drag_float.drag_anchor_x  = ctx->input.mouse_x;
    el->drag_float.drag_anchor_val = *el->drag_float.target;
  }
  if (el->is_pressed && m_down) {
    int dx = ctx->input.mouse_x - el->drag_float.drag_anchor_x;
    float nv = el->drag_float.drag_anchor_val + (float)dx * el->drag_float.step;
    if (nv != *el->drag_float.target) {
      *el->drag_float.target = nv;
      if (el->drag_float.callback) el->drag_float.callback(ctx, el->drag_float.userdata);
    }
  }
}

void _sf_update_dropdown(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_pressed) {
  /* Toggle open/closed, track hovered item, scroll with mouse wheel, and write selection on click. */
  int mx = ctx->input.mouse_x, my = ctx->input.mouse_y;
  int h = el->v1.y - el->v0.y;
  int w = el->v1.x - el->v0.x;
  int n = el->dropdown.n_items;
  int mv = (el->dropdown.max_visible > 0 && n > el->dropdown.max_visible)
           ? el->dropdown.max_visible : n;
  int scroll = el->dropdown.scroll_offset;
  if (scroll < 0) scroll = 0;
  if (scroll > n - mv) scroll = n - mv;
  if (scroll < 0) scroll = 0;
  el->dropdown.hover_item = -1;
  if (el->dropdown.is_open) {
    int popup_y0 = el->v1.y;
    int popup_y1 = el->v1.y + mv * h;
    bool over_popup = (mx >= el->v0.x && mx <= el->v0.x + w && my >= popup_y0 && my <= popup_y1);
    if (over_popup && ctx->input.wheel_dy != 0) {
      scroll -= ctx->input.wheel_dy;
      if (scroll < 0) scroll = 0;
      if (scroll > n - mv) scroll = n - mv;
      if (scroll < 0) scroll = 0;
      el->dropdown.scroll_offset = scroll;
    }
    for (int i = 0; i < mv; i++) {
      int item_idx = scroll + i;
      if (item_idx >= n) break;
      int iy0 = el->v1.y + i * h;
      int iy1 = el->v1.y + (i + 1) * h;
      if (mx >= el->v0.x && mx <= el->v0.x + w && my >= iy0 && my <= iy1) {
        el->dropdown.hover_item = item_idx;
        if (m_pressed) {
          if (el->dropdown.selected) *el->dropdown.selected = item_idx;
          el->dropdown.is_open = false;
          if (el->dropdown.callback) el->dropdown.callback(ctx, el->dropdown.userdata);
        }
      }
    }
    if (m_pressed && el->dropdown.hover_item < 0 && !el->is_hovered) el->dropdown.is_open = false;
  }
  if (el->is_hovered && m_pressed) {
    el->dropdown.is_open = !el->dropdown.is_open;
    if (el->dropdown.is_open && el->dropdown.selected) {
      int sel = *el->dropdown.selected;
      if (sel < scroll) scroll = sel;
      else if (sel >= scroll + mv) scroll = sel - mv + 1;
      if (scroll < 0) scroll = 0;
      if (scroll > n - mv) scroll = n - mv;
      if (scroll < 0) scroll = 0;
      el->dropdown.scroll_offset = scroll;
    }
  }
}

void _sf_update_panel(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_pressed) {
  int mx = ctx->input.mouse_x, my = ctx->input.mouse_y;
  bool header = (mx >= el->v0.x && mx <= el->v1.x && my >= el->v0.y && my <= el->v0.y + 16);
  if (header && m_pressed) el->panel.collapsed = !el->panel.collapsed;
}

void _sf_update_checkbox(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released) {
  if (el->is_hovered && el->is_pressed && m_released) {
    el->checkbox.is_checked = !el->checkbox.is_checked;
    if (el->checkbox.callback) {
      el->checkbox.callback(ctx, el->checkbox.userdata);
    }
  }
}

void _sf_update_slider(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released) {
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

const char* _sf_ui_type_str(sf_ui_type_t t) {
  switch (t) {
    case SF_UI_BUTTON:     return "button";
    case SF_UI_SLIDER:     return "slider";
    case SF_UI_CHECKBOX:   return "checkbox";
    case SF_UI_LABEL:      return "label";
    case SF_UI_TEXT_INPUT: return "text_input";
    case SF_UI_DRAG_FLOAT: return "drag_float";
    case SF_UI_DROPDOWN:   return "dropdown";
    case SF_UI_PANEL:      return "panel";
  }
  return "button";
}

int _sf_ui_type_from_str(const char *s) {
  if (strcmp(s, "button")     == 0) return SF_UI_BUTTON;
  if (strcmp(s, "slider")     == 0) return SF_UI_SLIDER;
  if (strcmp(s, "checkbox")   == 0) return SF_UI_CHECKBOX;
  if (strcmp(s, "label")      == 0) return SF_UI_LABEL;
  if (strcmp(s, "text_input") == 0) return SF_UI_TEXT_INPUT;
  if (strcmp(s, "drag_float") == 0) return SF_UI_DRAG_FLOAT;
  if (strcmp(s, "dropdown")   == 0) return SF_UI_DROPDOWN;
  if (strcmp(s, "panel")      == 0) return SF_UI_PANEL;
  return -1;
}

/* SF_MESH_AUTHORING_FUNCTIONS */
sf_obj_t* sf_obj_create_empty(sf_ctx_t *ctx, const char *objname, int max_v, int max_vt, int max_f) {
  /* Allocate a named mesh with pre-reserved vertex, UV, and face arrays in the arena. */
  char auto_name[32];
  if (objname == NULL) {
    snprintf(auto_name, sizeof(auto_name), "obj_%d", ctx->obj_count);
    objname = auto_name;
  }
  if (NULL != sf_get_obj_(ctx, objname, false)) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "obj '%s' name in use\n", objname);
    return NULL;
  }
  if (ctx->obj_count >= SF_MAX_OBJS) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "obj '%s' max reached\n", objname);
    return NULL;
  }
  sf_obj_t *obj = &ctx->objs[ctx->obj_count++];
  memset(obj, 0, sizeof(sf_obj_t));
  obj->v_cap  = max_v;
  obj->vt_cap = max_vt;
  obj->f_cap  = max_f;
  obj->v  = sf_arena_alloc(ctx, &ctx->arena, max_v  * sizeof(sf_fvec3_t));
  obj->vt = max_vt > 0 ? sf_arena_alloc(ctx, &ctx->arena, max_vt * sizeof(sf_fvec2_t)) : NULL;
  obj->f  = sf_arena_alloc(ctx, &ctx->arena, max_f  * sizeof(sf_face_t));
  obj->id = ctx->obj_count - 1;

  size_t nlen = strlen(objname) + 1;
  obj->name = (const char*)sf_arena_alloc(ctx, &ctx->arena, nlen);
  if (obj->name) memcpy((void*)obj->name, objname, nlen);

  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "name   : %s\n"
              SF_LOG_INDENT "cap_v  : %d\n"
              SF_LOG_INDENT "cap_vt : %d\n"
              SF_LOG_INDENT "cap_f  : %d\n",
              obj->name, max_v, max_vt, max_f);
  return obj;
}

int sf_obj_add_vert(sf_obj_t *obj, sf_fvec3_t p) {
  /* Append a vertex position; returns its index or -1 if the array is full. */
  if (!obj || obj->v_cnt >= obj->v_cap) return -1;
  obj->v[obj->v_cnt] = p;
  return obj->v_cnt++;
}

int sf_obj_add_uv(sf_obj_t *obj, sf_fvec2_t uv) {
  /* Append a UV coordinate; returns its index or -1 if the array is full. */
  if (!obj || obj->vt_cnt >= obj->vt_cap) return -1;
  obj->vt[obj->vt_cnt] = uv;
  return obj->vt_cnt++;
}

int sf_obj_add_face(sf_obj_t *obj, int i0, int i1, int i2) {
  /* Append a triangle face by vertex indices only (no UV mapping). */
  if (!obj || obj->f_cnt >= obj->f_cap) return -1;
  sf_face_t *f = &obj->f[obj->f_cnt];
  f->idx[0] = (sf_vtx_idx_t){i0, -1, -1};
  f->idx[1] = (sf_vtx_idx_t){i1, -1, -1};
  f->idx[2] = (sf_vtx_idx_t){i2, -1, -1};
  return obj->f_cnt++;
}

int sf_obj_add_face_uv(sf_obj_t *obj, int v0, int v1, int v2, int t0, int t1, int t2) {
  /* Append a triangle face with per-corner UV indices. */
  if (!obj || obj->f_cnt >= obj->f_cap) return -1;
  sf_face_t *f = &obj->f[obj->f_cnt];
  f->idx[0] = (sf_vtx_idx_t){v0, t0, -1};
  f->idx[1] = (sf_vtx_idx_t){v1, t1, -1};
  f->idx[2] = (sf_vtx_idx_t){v2, t2, -1};
  return obj->f_cnt++;
}

void sf_obj_recompute_bs(sf_obj_t *obj) {
  /* Recompute the bounding-sphere center (centroid) and radius from the current vertex set. */
  if (!obj || obj->v_cnt == 0) return;
  sf_fvec3_t c = {0, 0, 0};
  for (int i = 0; i < obj->v_cnt; i++) { c.x += obj->v[i].x; c.y += obj->v[i].y; c.z += obj->v[i].z; }
  float inv = 1.0f / (float)obj->v_cnt;
  c.x *= inv; c.y *= inv; c.z *= inv;
  float r2 = 0.0f;
  for (int i = 0; i < obj->v_cnt; i++) {
    float dx = obj->v[i].x - c.x, dy = obj->v[i].y - c.y, dz = obj->v[i].z - c.z;
    float d2 = dx*dx + dy*dy + dz*dz;
    if (d2 > r2) r2 = d2;
  }
  obj->bs_center = c;
  obj->bs_radius = sqrtf(r2);
}

sf_obj_t* sf_obj_make_plane(sf_ctx_t *ctx, const char *objname, float sx, float sz, int res) {
  /* Generate a flat XZ-plane mesh of size sx×sz subdivided into res×res quads. */
  if (res < 1) res = 1;
  int nv = (res + 1) * (res + 1);
  int nvt = nv;
  int nf = res * res * 2;
  sf_obj_t *obj = sf_obj_create_empty(ctx, objname, nv, nvt, nf);
  if (!obj) return NULL;
  float hx = sx * 0.5f, hz = sz * 0.5f;
  for (int z = 0; z <= res; z++) {
    for (int x = 0; x <= res; x++) {
      float u = (float)x / (float)res, v = (float)z / (float)res;
      sf_obj_add_vert(obj, (sf_fvec3_t){ -hx + u * sx, 0.0f, -hz + v * sz });
      sf_obj_add_uv  (obj, (sf_fvec2_t){ u, v });
    }
  }
  for (int z = 0; z < res; z++) {
    for (int x = 0; x < res; x++) {
      int a = z * (res + 1) + x;
      int b = a + 1;
      int c = a + (res + 1);
      int d = c + 1;
      sf_obj_add_face_uv(obj, a, c, b, a, c, b);
      sf_obj_add_face_uv(obj, b, c, d, b, c, d);
    }
  }
  sf_obj_recompute_bs(obj);
  return obj;
}

sf_obj_t* sf_obj_make_box(sf_ctx_t *ctx, const char *objname, float sx, float sy, float sz) {
  /* Generate a UV-mapped box mesh with the given half-extents. */
  sf_obj_t *obj = sf_obj_create_empty(ctx, objname, 24, 24, 12);
  if (!obj) return NULL;
  float x = sx*0.5f, y = sy*0.5f, z = sz*0.5f;
  sf_fvec3_t p[8] = {
    {-x,-y,-z},{ x,-y,-z},{ x, y,-z},{-x, y,-z},
    {-x,-y, z},{ x,-y, z},{ x, y, z},{-x, y, z}
  };
  int faces[6][4] = {
    {0,1,2,3}, {5,4,7,6}, {4,0,3,7}, {1,5,6,2}, {4,5,1,0}, {3,2,6,7}
  };
  sf_fvec2_t uvs[4] = {{0,0},{1,0},{1,1},{0,1}};
  for (int f = 0; f < 6; f++) {
    int base_v = obj->v_cnt;
    for (int i = 0; i < 4; i++) {
      sf_obj_add_vert(obj, p[faces[f][i]]);
      sf_obj_add_uv  (obj, uvs[i]);
    }
    sf_obj_add_face_uv(obj, base_v+0, base_v+2, base_v+1, base_v+0, base_v+2, base_v+1);
    sf_obj_add_face_uv(obj, base_v+0, base_v+3, base_v+2, base_v+0, base_v+3, base_v+2);
  }
  sf_obj_recompute_bs(obj);
  return obj;
}

sf_obj_t* sf_obj_make_sphere(sf_ctx_t *ctx, const char *objname, float radius, int segs) {
  /* Generate a UV-sphere mesh with the given radius and segment count. */
  if (segs < 3) segs = 3;
  int rings = segs;
  int nv  = (rings + 1) * (segs + 1);
  int nvt = nv;
  int nf  = rings * segs * 2;
  sf_obj_t *obj = sf_obj_create_empty(ctx, objname, nv, nvt, nf);
  if (!obj) return NULL;
  for (int i = 0; i <= rings; i++) {
    float v = (float)i / (float)rings;
    float phi = v * SF_PI;
    for (int j = 0; j <= segs; j++) {
      float u = (float)j / (float)segs;
      float theta = u * SF_PI * 2.0f;
      sf_fvec3_t p = { radius * sinf(phi) * cosf(theta), radius * cosf(phi), radius * sinf(phi) * sinf(theta) };
      sf_obj_add_vert(obj, p);
      sf_obj_add_uv  (obj, (sf_fvec2_t){u, v});
    }
  }
  int row = segs + 1;
  for (int i = 0; i < rings; i++) {
    for (int j = 0; j < segs; j++) {
      int a = i * row + j;
      int b = a + 1;
      int c = a + row;
      int d = c + 1;
      sf_obj_add_face_uv(obj, a, b, c, a, b, c);
      sf_obj_add_face_uv(obj, b, d, c, b, d, c);
    }
  }
  sf_obj_recompute_bs(obj);
  return obj;
}

sf_obj_t* sf_obj_make_cyl(sf_ctx_t *ctx, const char *objname, float radius, float height, int segs) {
  /* Generate a capped cylinder mesh with the given radius, height, and segment count. */
  if (segs < 3) segs = 3;
  int nv  = (segs + 1) * 2 + 2;
  int nvt = nv;
  int nf  = segs * 4;
  sf_obj_t *obj = sf_obj_create_empty(ctx, objname, nv, nvt, nf);
  if (!obj) return NULL;
  float hy = height * 0.5f;
  int top_c = sf_obj_add_vert(obj, (sf_fvec3_t){0,  hy, 0}); sf_obj_add_uv(obj, (sf_fvec2_t){0.5f,0.5f});
  int bot_c = sf_obj_add_vert(obj, (sf_fvec3_t){0, -hy, 0}); sf_obj_add_uv(obj, (sf_fvec2_t){0.5f,0.5f});
  int top_start = obj->v_cnt;
  for (int i = 0; i <= segs; i++) {
    float t = (float)i / (float)segs * SF_PI * 2.0f;
    float x = cosf(t) * radius, z = sinf(t) * radius;
    sf_obj_add_vert(obj, (sf_fvec3_t){x,  hy, z}); sf_obj_add_uv(obj, (sf_fvec2_t){(float)i/(float)segs, 0.0f});
  }
  int bot_start = obj->v_cnt;
  for (int i = 0; i <= segs; i++) {
    float t = (float)i / (float)segs * SF_PI * 2.0f;
    float x = cosf(t) * radius, z = sinf(t) * radius;
    sf_obj_add_vert(obj, (sf_fvec3_t){x, -hy, z}); sf_obj_add_uv(obj, (sf_fvec2_t){(float)i/(float)segs, 1.0f});
  }
  for (int i = 0; i < segs; i++) {
    int t0 = top_start + i, t1 = top_start + i + 1;
    int b0 = bot_start + i, b1 = bot_start + i + 1;
    sf_obj_add_face_uv(obj, t0, t1, b0, t0, t1, b0);
    sf_obj_add_face_uv(obj, t1, b1, b0, t1, b1, b0);
    sf_obj_add_face_uv(obj, top_c, t1, t0, top_c, t1, t0);
    sf_obj_add_face_uv(obj, bot_c, b0, b1, bot_c, b0, b1);
  }
  sf_obj_recompute_bs(obj);
  return obj;
}

sf_obj_t* sf_obj_make_heightmap(sf_ctx_t *ctx, const char *objname, float size_x, float size_z, int res, sf_height_fn fn, void *ud) {
  /* Generate a terrain mesh by sampling a height callback function on a res×res grid. */
  if (res < 1) res = 1;
  int nv = (res + 1) * (res + 1);
  int nvt = nv;
  int nf = res * res * 2;
  sf_obj_t *obj = sf_obj_create_empty(ctx, objname, nv, nvt, nf);
  if (!obj) return NULL;
  float hx = size_x * 0.5f, hz = size_z * 0.5f;
  for (int z = 0; z <= res; z++) {
    for (int x = 0; x <= res; x++) {
      float wx = -hx + (float)x / (float)res * size_x;
      float wz = -hz + (float)z / (float)res * size_z;
      float wy = fn ? fn(wx, wz, ud) : 0.0f;
      sf_obj_add_vert(obj, (sf_fvec3_t){ wx, wy, wz });
      sf_obj_add_uv  (obj, (sf_fvec2_t){ (float)x/(float)res, (float)z/(float)res });
    }
  }
  for (int z = 0; z < res; z++) {
    for (int x = 0; x < res; x++) {
      int a = z * (res + 1) + x;
      int b = a + 1;
      int c = a + (res + 1);
      int d = c + 1;
      sf_obj_add_face_uv(obj, a, c, b, a, c, b);
      sf_obj_add_face_uv(obj, b, c, d, b, c, d);
    }
  }
  sf_obj_recompute_bs(obj);
  return obj;
}

bool sf_obj_save_obj(sf_ctx_t *ctx, sf_obj_t *obj, const char *filepath) {
  /* Export a mesh to a Wavefront .obj file. */
  if (!obj || !filepath) return false;
  FILE *f = fopen(filepath, "w");
  if (!f) {
    SF_LOG(ctx, SF_LOG_ERROR, SF_LOG_INDENT "could not write %s\n", filepath);
    return false;
  }
  fprintf(f, "# saffron export: %s\n", obj->name ? obj->name : "unnamed");
  for (int i = 0; i < obj->v_cnt; i++)  fprintf(f, "v %.6f %.6f %.6f\n", obj->v[i].x, obj->v[i].y, obj->v[i].z);
  for (int i = 0; i < obj->vt_cnt; i++) fprintf(f, "vt %.6f %.6f\n", obj->vt[i].x, obj->vt[i].y);
  for (int i = 0; i < obj->f_cnt; i++) {
    sf_face_t *fc = &obj->f[i];
    bool has_uv = (fc->idx[0].vt >= 0);
    if (has_uv) {
      fprintf(f, "f %d/%d %d/%d %d/%d\n",
              fc->idx[0].v+1, fc->idx[0].vt+1,
              fc->idx[1].v+1, fc->idx[1].vt+1,
              fc->idx[2].v+1, fc->idx[2].vt+1);
    } else {
      fprintf(f, "f %d %d %d\n", fc->idx[0].v+1, fc->idx[1].v+1, fc->idx[2].v+1);
    }
  }
  fclose(f);
  SF_LOG(ctx, SF_LOG_INFO,
              SF_LOG_INDENT "file   : %s\n"
              SF_LOG_INDENT "verts  : %d\n"
              SF_LOG_INDENT "faces  : %d\n",
              filepath, obj->v_cnt, obj->f_cnt);
  return true;
}

float sf_noise_fbm(float x, float z, int oct, float lac, float gain, uint32_t seed) {
  /* Fractional Brownian motion: sum of smooth noise octaves for natural-looking terrain. */
  float sum = 0.0f, amp = 1.0f, freq = 1.0f, norm = 0.0f;
  for (int i = 0; i < oct; i++) {
    sum  += _sf_smooth_noise(x * freq, z * freq, seed + (uint32_t)i) * amp;
    norm += amp;
    amp  *= gain;
    freq *= lac;
  }
  return (norm > 0.0f) ? (sum / norm) : 0.0f;
}

/* SF_PICKING_FUNCTIONS */
sf_ray_t sf_ray_from_screen(sf_ctx_t *ctx, sf_cam_t *cam, int sx, int sy) {
  /* Unproject a screen pixel into a world-space ray origin and direction. */
  sf_ray_t r = {{0,0,0},{0,0,-1}};
  if (!cam || !cam->frame) return r;
  float ndc_x = (2.0f * (float)sx / (float)cam->w) - 1.0f;
  float ndc_y = 1.0f - (2.0f * (float)sy / (float)cam->h);
  float aspect = (float)cam->w / (float)cam->h;
  float t = tanf(SF_DEG2RAD(cam->fov) * 0.5f);
  sf_fvec3_t dir_v = { ndc_x * aspect * t, ndc_y * t, -1.0f };
  sf_fmat4_t M = cam->frame->global_M;
  sf_fvec3_t right = {M.m[0][0], M.m[0][1], M.m[0][2]};
  sf_fvec3_t up    = {M.m[1][0], M.m[1][1], M.m[1][2]};
  sf_fvec3_t fwd   = {-M.m[2][0], -M.m[2][1], -M.m[2][2]};
  sf_fvec3_t dir_w = {
    right.x * dir_v.x + up.x * dir_v.y - fwd.x * dir_v.z,
    right.y * dir_v.x + up.y * dir_v.y - fwd.y * dir_v.z,
    right.z * dir_v.x + up.z * dir_v.y - fwd.z * dir_v.z
  };
  r.o = (sf_fvec3_t){ M.m[3][0], M.m[3][1], M.m[3][2] };
  r.d = sf_fvec3_norm(dir_w);
  return r;
}

bool sf_ray_triangle(sf_ray_t r, sf_fvec3_t a, sf_fvec3_t b, sf_fvec3_t c, float *out_t) {
  /* Möller–Trumbore ray-triangle intersection; writes hit distance to out_t if non-NULL. */
  sf_fvec3_t e1 = sf_fvec3_sub(b, a);
  sf_fvec3_t e2 = sf_fvec3_sub(c, a);
  sf_fvec3_t p  = sf_fvec3_cross(r.d, e2);
  float det = sf_fvec3_dot(e1, p);
  if (det > -1e-6f && det < 1e-6f) return false;
  float inv = 1.0f / det;
  sf_fvec3_t s = sf_fvec3_sub(r.o, a);
  float u = sf_fvec3_dot(s, p) * inv;
  if (u < 0.0f || u > 1.0f) return false;
  sf_fvec3_t q = sf_fvec3_cross(s, e1);
  float v = sf_fvec3_dot(r.d, q) * inv;
  if (v < 0.0f || u + v > 1.0f) return false;
  float t = sf_fvec3_dot(e2, q) * inv;
  if (t <= 0.0f) return false;
  if (out_t) *out_t = t;
  return true;
}

bool sf_ray_plane_y(sf_ray_t r, float y, sf_fvec3_t *out) {
  /* Intersect a ray with the horizontal plane at height y; writes the hit point to out if non-NULL. */
  if (fabsf(r.d.y) < 1e-6f) return false;
  float t = (y - r.o.y) / r.d.y;
  if (t < 0.0f) return false;
  if (out) *out = (sf_fvec3_t){ r.o.x + r.d.x * t, y, r.o.z + r.d.z * t };
  return true;
}

bool sf_ray_aabb(sf_ray_t r, sf_fvec3_t bmin, sf_fvec3_t bmax, float *out_t) {
  /* Slab-method AABB intersection; writes nearest hit distance to out_t if non-NULL. */
  float tmin = 0.0f, tmax = 1e30f;
  float o[3] = {r.o.x, r.o.y, r.o.z}, d[3] = {r.d.x, r.d.y, r.d.z};
  float mn[3] = {bmin.x, bmin.y, bmin.z}, mx[3] = {bmax.x, bmax.y, bmax.z};
  for (int i = 0; i < 3; i++) {
    if (fabsf(d[i]) < 1e-6f) {
      if (o[i] < mn[i] || o[i] > mx[i]) return false;
    } else {
      float inv = 1.0f / d[i];
      float t1 = (mn[i] - o[i]) * inv, t2 = (mx[i] - o[i]) * inv;
      if (t1 > t2) { float s = t1; t1 = t2; t2 = s; }
      if (t1 > tmin) tmin = t1;
      if (t2 < tmax) tmax = t2;
      if (tmin > tmax) return false;
    }
  }
  if (out_t) *out_t = tmin;
  return true;
}

sf_enti_t* sf_raycast_entities(sf_ctx_t *ctx, sf_ray_t ray, float *out_t) {
  /* Test a ray against every entity's triangles and return the closest hit entity. */
  sf_enti_t *hit = NULL;
  float best = 1e30f;
  for (int i = 0; i < ctx->enti_count; i++) {
    sf_enti_t *e = &ctx->entities[i];
    if (!e->frame) continue;
    sf_fmat4_t M = e->frame->global_M;
    for (int fi = 0; fi < e->obj.f_cnt; fi++) {
      sf_face_t *fc = &e->obj.f[fi];
      sf_fvec3_t a = sf_fmat4_mul_vec3(M, e->obj.v[fc->idx[0].v]);
      sf_fvec3_t b = sf_fmat4_mul_vec3(M, e->obj.v[fc->idx[1].v]);
      sf_fvec3_t c = sf_fmat4_mul_vec3(M, e->obj.v[fc->idx[2].v]);
      float t;
      if (sf_ray_triangle(ray, a, b, c, &t) && t < best) {
        best = t;
        hit = e;
      }
    }
  }
  if (hit && out_t) *out_t = best;
  return hit;
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
  /* Override the engine's log sink with a custom callback and optional userdata pointer. */
  ctx->log_cb = callback;
  ctx->log_user = userdata;
}

void sf_logger_console(const char* message, void* userdata) {
  /* Default log sink: write the formatted message to stdout. */
  fprintf(stdout, "%s", message);
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

/* SF_MATH_FUNCTIONS */
sf_fmat4_t sf_fmat4_mul_fmat4(sf_fmat4_t m0, sf_fmat4_t m1) {
  /* Multiply two 4×4 matrices and return the result. */
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
  /* Transform a vec3 by a 4×4 matrix with perspective divide. */
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
  /* Subtract v1 from v0 component-wise. */
  return (sf_fvec3_t){ v0.x - v1.x, v0.y - v1.y, v0.z - v1.z };
}

sf_fvec3_t sf_fvec3_add(sf_fvec3_t v0, sf_fvec3_t v1) {
  /* Add two vec3s component-wise. */
  return (sf_fvec3_t){ v0.x + v1.x, v0.y + v1.y, v0.z + v1.z };
}

sf_fvec3_t sf_fvec3_norm(sf_fvec3_t v) {
  /* Return a unit-length copy of v, or {0,0,0} for the zero vector. */
  float sq = v.x*v.x + v.y*v.y + v.z*v.z;
  if (sq == 0.0f) return (sf_fvec3_t){0,0,0};
  float inv_len = 1.0f / sqrtf(sq);
  return (sf_fvec3_t){ v.x * inv_len, v.y * inv_len, v.z * inv_len };
}

sf_fvec3_t sf_fvec3_cross(sf_fvec3_t v0, sf_fvec3_t v1) {
  /* Compute the cross product of v0 and v1. */
  return (sf_fvec3_t){
    v0.y * v1.z - v0.z * v1.y,
    v0.z * v1.x - v0.x * v1.z,
    v0.x * v1.y - v0.y * v1.x
  };
}

float sf_fvec3_dot(sf_fvec3_t v0, sf_fvec3_t v1) {
  /* Compute the scalar dot product of v0 and v1. */
  return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

sf_fmat4_t sf_make_tsl_fmat4(float x, float y, float z) {
  /* Build a 4×4 translation matrix for (x, y, z). */
  sf_fmat4_t m = sf_make_idn_fmat4();
  m.m[3][0] = x;
  m.m[3][1] = y;
  m.m[3][2] = z;
  return m;
}

sf_fmat4_t sf_make_rot_fmat4(sf_fvec3_t angles) {
  /* Build a 4×4 rotation matrix from Euler XYZ angles (radians), applied as Y→X→Z. */
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
  /* Build a right-handed perspective projection matrix from FOV (degrees), aspect ratio, and clip planes. */
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
  /* Return a 4×4 identity matrix. */
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
  /* Build a look-at view matrix from eye position, target point, and up vector. */
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
  /* Build a 4×4 non-uniform scale matrix from a vec3 of scale factors. */
  sf_fmat4_t m = sf_make_idn_fmat4();
  m.m[0][0] = s.x;
  m.m[1][1] = s.y;
  m.m[2][2] = s.z;
  return m;
}

uint32_t _sf_vec_to_index(sf_ctx_t *ctx, sf_cam_t *cam, sf_ivec2_t v) {
  return v.y * cam->w + v.x;
}

void _sf_swap_svec2(sf_ivec2_t *v0, sf_ivec2_t *v1) {
  sf_ivec2_t t = *v0; *v0 = *v1; *v1 = t;
}

void _sf_swap_fvec3(sf_fvec3_t *v0, sf_fvec3_t *v1) {
    sf_fvec3_t t = *v0; *v0 = *v1; *v1 = t;
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

float _sf_hash_2d(int x, int z, uint32_t seed) {
  uint32_t h = (uint32_t)x * 374761393u + (uint32_t)z * 668265263u + seed * 362437u;
  h = (h ^ (h >> 13)) * 1274126177u;
  h = h ^ (h >> 16);
  return (float)h / (float)0xFFFFFFFFu;
}

float _sf_smooth_noise(float x, float z, uint32_t seed) {
  int ix = (int)floorf(x), iz = (int)floorf(z);
  float fx = x - (float)ix, fz = z - (float)iz;
  float u = fx * fx * (3.0f - 2.0f * fx);
  float v = fz * fz * (3.0f - 2.0f * fz);
  float a = _sf_hash_2d(ix,     iz,     seed);
  float b = _sf_hash_2d(ix + 1, iz,     seed);
  float c = _sf_hash_2d(ix,     iz + 1, seed);
  float d = _sf_hash_2d(ix + 1, iz + 1, seed);
  float ab = a + (b - a) * u;
  float cd = c + (d - c) * u;
  return ab + (cd - ab) * v;
}

/* SF_IMPLEMENTATION_HELPERS */

bool _sf_resolve_asset(const char* filename, char* out_path, size_t max_len) {
  char dir_stack[32][512];
  int stack_head = 0;
  snprintf(dir_stack[stack_head++], 512, "%s", SF_ASSET_PATH);
#ifdef SF_SRC_ASSET_PATH
  if (stack_head < 32) snprintf(dir_stack[stack_head++], 512, "%s", SF_SRC_ASSET_PATH);
#endif
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

const char* _sf_basename(const char *path) {
  const char *s = strrchr(path, '/');
  return s ? s + 1 : path;
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

void _sf_write_qstr(FILE *f, const char *s) {
  fputc('"', f);
  if (s) for (const char *p = s; *p; p++) {
    if (*p == '"' || *p == '\\') fputc('\\', f);
    fputc(*p, f);
  }
  fputc('"', f);
}

bool _sf_parse_qstr(const char *line, const char *key, char *out, size_t outsz) {
  const char *p = strstr(line, key);
  if (!p) return false;
  p = strchr(p, '=');
  if (!p) return false;
  p++;
  while (*p == ' ' || *p == '\t') p++;
  if (*p != '"') return false;
  p++;
  size_t n = 0;
  while (*p && *p != '"' && n + 1 < outsz) {
    if (*p == '\\' && p[1]) p++;
    out[n++] = *p++;
  }
  out[n] = '\0';
  return true;
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
