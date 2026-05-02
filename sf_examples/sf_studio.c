/* sf_studio - interactive editor for saffron files */
#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>

typedef enum { SEL_NONE = 0, SEL_ENTI, SEL_LIGHT, SEL_CAM, SEL_EMITR } sel_kind_t;
typedef enum { TAB_SFF = 0, TAB_SFUI } tab_t;

static sf_ctx_t     sf_ctx;
static sel_kind_t   g_sel_kind      = SEL_NONE;
static sf_enti_t   *g_sel           = NULL;
static sf_light_t  *g_sel_light     = NULL;
static sf_cam_t    *g_sel_cam       = NULL;
static sf_emitr_t  *g_sel_emitr     = NULL;
static int          g_new_prim      = 0;
static float        g_orbit_yaw     = 0.8f;
static float        g_orbit_pitch   = 0.5f;
static float        g_orbit_dist    = 12.0f;
static sf_fvec3_t   g_orbit_target  = {0.0f, 0.0f, 0.0f};
static bool         g_ui_dirty      = true;
static bool         g_dbg_axes      = true;
static bool         g_dbg_frames    = true;
static bool         g_dbg_lights    = true;
static bool         g_dbg_cams      = true;
static int          g_w             = 1920;
static int          g_h             = 1080;
static char         g_save_path[SF_MAX_TEXT_INPUT_LEN] = "studio_out.sff";
static char         g_sfui_path[SF_MAX_TEXT_INPUT_LEN] = "studio_out.sfui";
static tab_t        g_tab = TAB_SFF;

/* Translate gizmo state (SFF tab, selected entity/light/cam/emitter) */
typedef enum { GZ_AX_NONE = -1, GZ_AX_X = 0, GZ_AX_Y = 1, GZ_AX_Z = 2 } gz_axis_t;
static gz_axis_t  g_gz_hover          = GZ_AX_NONE;
static gz_axis_t  g_gz_drag           = GZ_AX_NONE;
static bool       g_gz_active         = false;           /* geometry valid this frame */
static sf_ivec2_t g_gz_screen_origin  = {0, 0};
static sf_ivec2_t g_gz_screen_tip[3];                    /* tip per axis */
static float      g_gz_pixel_per_unit[3] = {0, 0, 0};    /* projected px per world unit */
static sf_fvec3_t g_gz_drag_start_pos;                   /* pos at mouse-down */
static int        g_gz_drag_start_mx  = 0;
static int        g_gz_drag_start_my  = 0;

/* Outliner scroll state */
static int          g_outl_scroll = 0;

/* mouse latest (updated in SDL loop) */
static int          g_mouse_x = 0, g_mouse_y = 0;

static sf_ui_t     *g_designer_ui   = NULL;
static sf_ui_lmn_t *g_design_sel    = NULL;
static bool         g_design_dragging = false;
static bool         g_design_resizing = false;
static sf_ivec2_t   g_design_drag_off = {0, 0};
static char         g_design_text_buf[128] = "";
static void        *g_design_text_last = NULL;
static float        g_design_v0x, g_design_v0y, g_design_v1x, g_design_v1y;
static float        g_design_canvas_w = 800.0f, g_design_canvas_h = 600.0f;
static float        g_design_slider_min = 0.0f, g_design_slider_max = 1.0f, g_design_slider_val = 0.5f;
static float        g_design_df_step = 0.05f;
static int          g_design_buflen = 64;
static bool         g_design_checked = false;
static int          g_design_dd_selected = 0;
static char         g_design_dd_item_buf[64] = "";
static bool         g_design_panel_collapsed = false;
static char         g_design_name_buf[64] = "";
static float        g_design_zoom = 1.0f;
static float        g_design_pan_x = 0.0f, g_design_pan_y = 0.0f;
static bool         g_design_panning = false;
static int          g_design_pan_last_mx = 0, g_design_pan_last_my = 0;
#define DESIGN_HANDLE 10

static int          g_parent_sel    = 0;
static const char  *g_parent_items[SF_MAX_FRAMES];
static int          g_parent_count  = 0;

/* primitive generation parameters */
static float g_plane_sx    = 2.0f,  g_plane_sz    = 2.0f;
static float g_plane_res   = 4.0f;
static float g_box_sx      = 1.0f,  g_box_sy      = 1.0f, g_box_sz = 1.0f;
static float g_sphere_r    = 1.0f;
static float g_sphere_segs = 16.0f;
static float g_cyl_r       = 0.5f,  g_cyl_h       = 1.5f;
static float g_cyl_segs    = 16.0f;
static float g_ter_sx      = 20.0f, g_ter_sz      = 20.0f;
static float g_ter_res     = 48.0f;
static float g_ter_amp     = 2.0f,  g_ter_freq    = 0.15f;
static float g_ter_oct     = 4.0f;
static float g_ter_seed    = 1337.0f;

#define STUDIO_MAX_MODELS 64
static int          g_model_sel     = 0;
static char         g_model_files[STUDIO_MAX_MODELS][64];
static const char  *g_model_items[STUDIO_MAX_MODELS];
static int          g_model_count   = 0;

#define STUDIO_MAX_TEXS 128
static int          g_tex_sel       = 0;
static char         g_tex_files[STUDIO_MAX_TEXS][64];
static const char  *g_tex_items[STUDIO_MAX_TEXS];
static int          g_tex_count     = 0;

static const char *k_emitr_items[3] = { "dir", "omni", "vol" };

static const char *k_spawn_labels[9]    = { "Pln", "Box", "Sph", "Cyl", "Ter", "Lt", "Cam", "Em", "Mdl" };
static const char *k_spawn_icon_bmp[9]  = { "plane.bmp", "box.bmp", "sphere.bmp", "cylinder.bmp", "terrain.bmp", "light.bmp", "camera.bmp", "emitter.bmp", "model.bmp" };
static sf_tex_t   *g_spawn_icon_tex[9]  = { 0 };
static sf_ivec2_t  g_spawn_btn_pos[9];  /* filled in build_spawn_buttons, read post-render */
static bool        g_spawn_btn_has_pos  = false;
static sf_ui_lmn_t *g_spawn_btn_el[9]   = { 0 };

/* Generic icon registry: per-frame list of (button, icon) to overlay after ui render */
typedef enum {
  ICN_SAVE, ICN_OPEN, ICN_DELETE, ICN_PARENT, ICN_UNGROUP,
  ICN_PREV, ICN_NEXT, ICN_APPLY, ICN_CLEAR, ICN_TEXTURE,
  ICN_GRID, ICN_LAYER, ICN_NEW, ICN_COUNT
} icon_id_t;
static const char *k_icon_bmp[ICN_COUNT] = {
  "save.bmp", "open.bmp", "delete.bmp", "parent.bmp", "ungroup.bmp",
  "step_back.bmp", "step_forward.bmp", "play.bmp", "cut.bmp", "texture.bmp",
  "grid.bmp", "layer.bmp", "new.bmp"
};
static sf_tex_t *g_icon_tex[ICN_COUNT] = { 0 };

#define STUDIO_MAX_ICON_REG 64
static sf_ui_lmn_t *g_icon_reg_el[STUDIO_MAX_ICON_REG];
static int          g_icon_reg_id[STUDIO_MAX_ICON_REG];
static int          g_icon_reg_n = 0;

static sf_ui_lmn_t *icon_btn(icon_id_t id, const char *label, sf_ivec2_t v0, sf_ivec2_t v1, sf_ui_cb cb, void *ud) {
  sf_ui_lmn_t *e = sf_ui_add_button(&sf_ctx, label, v0, v1, cb, ud);
  if (g_icon_reg_n < STUDIO_MAX_ICON_REG) {
    g_icon_reg_el[g_icon_reg_n] = e;
    g_icon_reg_id[g_icon_reg_n] = (int)id;
    g_icon_reg_n++;
  }
  return e;
}

static sf_ivec2_t  g_cam_pip_pos  = {0, 0};
static sf_ivec2_t  g_cam_pip_size = {0, 0};
static bool        g_cam_pip_visible = false;
static int   g_emitr_type_sel = 1;   /* default omni */
static float g_emitr_rate     = 20.0f;
static float g_emitr_life     = 2.0f;
static float g_emitr_speed    = 3.0f;
static float g_emitr_max      = 100.0f;

#define STUDIO_TEX_PER_PAGE 16
static int          g_tex_page      = 0;
static const char  *g_tex_page_items[STUDIO_TEX_PER_PAGE];
static int          g_tex_page_sel  = 0;

static char         g_rename_buf[64] = "";
static void        *g_rename_last    = NULL;
static bool         g_scale_lock     = false;

typedef enum { PM_NONE=0, PM_PLANE, PM_BOX, PM_SPHERE, PM_CYL, PM_TERRAIN, PM_MODEL } prim_kind_t;
typedef struct {
  prim_kind_t kind;
  float       p[8];
  int         model_idx;
} prim_meta_t;
static prim_meta_t g_enti_meta[SF_MAX_ENTITIES];

static prim_meta_t* sel_meta(void) {
  if (g_sel_kind != SEL_ENTI || !g_sel) return NULL;
  int idx = (int)(g_sel - sf_ctx.entities);
  if (idx < 0 || idx >= SF_MAX_ENTITIES) return NULL;
  return &g_enti_meta[idx];
}

/* --- HELPERS --- */

static sf_frame_t* sel_frame(void) {
  switch (g_sel_kind) {
    case SEL_ENTI:  return g_sel        ? g_sel->frame        : NULL;
    case SEL_LIGHT: return g_sel_light  ? g_sel_light->frame  : NULL;
    case SEL_CAM:   return g_sel_cam    ? g_sel_cam->frame    : NULL;
    case SEL_EMITR: return g_sel_emitr  ? g_sel_emitr->frame  : NULL;
    default:        return NULL;
  }
}

static const char* sel_name(void) {
  switch (g_sel_kind) {
    case SEL_ENTI:  return g_sel       && g_sel->name        ? g_sel->name        : "?";
    case SEL_LIGHT: return g_sel_light && g_sel_light->name  ? g_sel_light->name  : "?";
    case SEL_CAM:   return g_sel_cam   && g_sel_cam->name    ? g_sel_cam->name    : "?";
    case SEL_EMITR: return g_sel_emitr && g_sel_emitr->name  ? g_sel_emitr->name  : "?";
    default:        return "?";
  }
}

static void sel_clear(void) {
  g_sel_kind = SEL_NONE;
  g_sel = NULL; g_sel_light = NULL; g_sel_cam = NULL; g_sel_emitr = NULL;
}

static void scan_models(void) {
  g_model_count = 0;
  const char *paths[2] = { SF_ASSET_PATH "/sf_objs", "./sf_assets/sf_objs" };
  for (int p = 0; p < 2 && g_model_count < STUDIO_MAX_MODELS; p++) {
    DIR *d = opendir(paths[p]);
    if (!d) continue;
    struct dirent *e;
    while ((e = readdir(d)) != NULL && g_model_count < STUDIO_MAX_MODELS) {
      const char *dot = strrchr(e->d_name, '.');
      if (!dot || strcmp(dot, ".obj") != 0) continue;
      /* dedupe */
      bool dup = false;
      for (int i = 0; i < g_model_count; i++) if (strcmp(g_model_files[i], e->d_name) == 0) { dup = true; break; }
      if (dup) continue;
      snprintf(g_model_files[g_model_count], sizeof(g_model_files[0]), "%s", e->d_name);
      g_model_items[g_model_count] = g_model_files[g_model_count];
      g_model_count++;
    }
    closedir(d);
  }
}

static void scan_textures_dir(const char *root) {
  char stack[16][512];
  int top = 0;
  snprintf(stack[top++], 512, "%s", root);
  while (top > 0 && g_tex_count < STUDIO_MAX_TEXS) {
    char cur[512];
    snprintf(cur, sizeof(cur), "%s", stack[--top]);
    DIR *d = opendir(cur);
    if (!d) continue;
    struct dirent *e;
    while ((e = readdir(d)) != NULL && g_tex_count < STUDIO_MAX_TEXS) {
      if (e->d_name[0] == '.') continue;
      char full[512];
      snprintf(full, sizeof(full), "%s/%s", cur, e->d_name);
      struct stat st;
      if (stat(full, &st) != 0) continue;
      if (S_ISDIR(st.st_mode)) {
        if (top < 16) snprintf(stack[top++], 512, "%s", full);
      } else {
        const char *dot = strrchr(e->d_name, '.');
        if (!dot || strcmp(dot, ".bmp") != 0) continue;
        bool dup = false;
        for (int i = 0; i < g_tex_count; i++) if (strcmp(g_tex_files[i], e->d_name) == 0) { dup = true; break; }
        if (dup) continue;
        snprintf(g_tex_files[g_tex_count], sizeof(g_tex_files[0]), "%s", e->d_name);
        g_tex_items[g_tex_count] = g_tex_files[g_tex_count];
        g_tex_count++;
      }
    }
    closedir(d);
  }
}

static void scan_textures(void) {
  g_tex_count = 0;
  scan_textures_dir(SF_ASSET_PATH "/sf_textures");
  scan_textures_dir("./sf_assets/sf_textures");
}

static void orbit_apply(sf_cam_t *cam) {
  float cy = cosf(g_orbit_yaw), sy = sinf(g_orbit_yaw);
  float cp = cosf(g_orbit_pitch), sp = sinf(g_orbit_pitch);
  sf_fvec3_t off = { g_orbit_dist * cp * cy, g_orbit_dist * sp, g_orbit_dist * cp * sy };
  sf_fvec3_t p   = sf_fvec3_add(g_orbit_target, off);
  sf_camera_set_pos(&sf_ctx, cam, p.x, p.y, p.z);
  sf_camera_look_at(&sf_ctx, cam, g_orbit_target);
}

/* --- ENTITY SPAWNING --- */

static void spawn_camera(void);
static void spawn_emitter(void);

static prim_meta_t g_regen_pending_meta;

static float _terrain_fn(float x, float z, void *ud) {
  prim_meta_t *m = (prim_meta_t*)ud;
  float freq = m ? m->p[4] : g_ter_freq;
  int   oct  = m ? (int)m->p[5] : (int)g_ter_oct;
  float amp  = m ? m->p[3] : g_ter_amp;
  unsigned seed = m ? (unsigned)m->p[6] : (unsigned)g_ter_seed;
  return sf_noise_fbm(x * freq, z * freq, oct, 2.0f, 0.5f, seed) * amp;
}

static void meta_set_defaults(prim_meta_t *m, prim_kind_t k) {
  memset(m, 0, sizeof(*m));
  m->kind = k;
  switch (k) {
    case PM_PLANE:   m->p[0]=g_plane_sx;  m->p[1]=g_plane_sz;  m->p[2]=g_plane_res;   break;
    case PM_BOX:     m->p[0]=g_box_sx;    m->p[1]=g_box_sy;    m->p[2]=g_box_sz;      break;
    case PM_SPHERE:  m->p[0]=g_sphere_r;  m->p[1]=g_sphere_segs;                      break;
    case PM_CYL:     m->p[0]=g_cyl_r;     m->p[1]=g_cyl_h;     m->p[2]=g_cyl_segs;    break;
    case PM_TERRAIN: m->p[0]=g_ter_sx; m->p[1]=g_ter_sz; m->p[2]=g_ter_res;
                     m->p[3]=g_ter_amp; m->p[4]=g_ter_freq; m->p[5]=g_ter_oct; m->p[6]=g_ter_seed; break;
    case PM_MODEL:   m->model_idx = g_model_sel; break;
    default: break;
  }
}

static sf_obj_t* build_obj_from_meta(const char *name, prim_meta_t *m) {
  switch (m->kind) {
    case PM_PLANE:   return sf_obj_make_plane(&sf_ctx, name, m->p[0], m->p[1], (int)m->p[2]);
    case PM_BOX:     return sf_obj_make_box  (&sf_ctx, name, m->p[0], m->p[1], m->p[2]);
    case PM_SPHERE:  return sf_obj_make_sphere(&sf_ctx, name, m->p[0], (int)m->p[1]);
    case PM_CYL:     return sf_obj_make_cyl  (&sf_ctx, name, m->p[0], m->p[1], (int)m->p[2]);
    case PM_TERRAIN: g_regen_pending_meta = *m;
                     return sf_obj_make_heightmap(&sf_ctx, name, m->p[0], m->p[1], (int)m->p[2], _terrain_fn, &g_regen_pending_meta);
    case PM_MODEL: {
      if (m->model_idx < 0 || m->model_idx >= g_model_count) return NULL;
      const char *fname = g_model_files[m->model_idx];
      char r_path[512];
      if (!_sf_resolve_asset(fname, r_path, sizeof(r_path))) return NULL;
      return sf_load_obj(&sf_ctx, r_path, name);
    }
    default: return NULL;
  }
}

static void cb_regen_sel(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  prim_meta_t *m = sel_meta();
  if (!m || m->kind == PM_NONE) return;
  int before = sf_ctx.obj_count;
  char name[32];
  snprintf(name, sizeof(name), "mesh_%d", before);
  sf_obj_t *o = build_obj_from_meta(name, m);
  if (o) { g_sel->obj = *o; sf_ctx.obj_count = before; }
}

static void spawn_primitive(int kind) {
  char objname[32], entiname[32];
  int seq = sf_ctx.enti_count;
  snprintf(entiname, sizeof(entiname), "enti_%d", seq);
  snprintf(objname, sizeof(objname), "mesh_%d", sf_ctx.obj_count);

  switch (kind) {
    case 5: {
      char ln[32];
      snprintf(ln, sizeof(ln), "light_%d", sf_ctx.light_count);
      sf_light_t *l = sf_add_light(&sf_ctx, ln, SF_LIGHT_POINT, (sf_fvec3_t){1.0f, 1.0f, 1.0f}, 2.0f);
      if (l && l->frame) { l->frame->pos = g_orbit_target; l->frame->is_dirty = true; }
      sel_clear();
      if (l) { g_sel_kind = SEL_LIGHT; g_sel_light = l; }
      g_ui_dirty = true;
      return;
    }
    case 6: spawn_camera();  return;
    case 7: spawn_emitter(); return;
    default: break;
  }

  prim_kind_t pk = PM_NONE;
  switch (kind) {
    case 0: pk = PM_PLANE;   break;
    case 1: pk = PM_BOX;     break;
    case 2: pk = PM_SPHERE;  break;
    case 3: pk = PM_CYL;     break;
    case 4: pk = PM_TERRAIN; break;
    case 8: pk = PM_MODEL;   break;
    default: return;
  }
  prim_meta_t tmp; meta_set_defaults(&tmp, pk);
  sf_obj_t *o = build_obj_from_meta(objname, &tmp);
  if (!o) return;
  sf_enti_t *e = sf_add_enti(&sf_ctx, o, entiname);
  if (!e) return;
  if (e->frame) { e->frame->pos = g_orbit_target; e->frame->is_dirty = true; }
  int idx = (int)(e - sf_ctx.entities);
  if (idx >= 0 && idx < SF_MAX_ENTITIES) g_enti_meta[idx] = tmp;
  sel_clear();
  g_sel_kind = SEL_ENTI;
  g_sel = e;
  g_ui_dirty = true;
}

static void spawn_emitter(void) {
  /* default sprite: try Stars.bmp, else fall back to first loaded texture */
  sf_sprite_t *spr = sf_get_sprite_(&sf_ctx, "spr_Stars", false);
  if (!spr) {
    sf_tex_t *stars = NULL;
    for (int i = 0; i < sf_ctx.tex_count; i++) {
      if (sf_ctx.textures[i].name && strcmp(sf_ctx.textures[i].name, "Stars") == 0) { stars = &sf_ctx.textures[i]; break; }
    }
    if (!stars) stars = sf_load_texture_bmp(&sf_ctx, "Stars.bmp", "Stars");
    if (stars) spr = sf_load_sprite(&sf_ctx, "spr_Stars", 1.0f, 0.3f, 1, stars->name);
  }
  if (!spr && sf_ctx.sprite_count > 0) spr = &sf_ctx.sprites[0];
  if (!spr) {
    if (sf_ctx.tex_count == 0) {
      SF_LOG(&sf_ctx, SF_LOG_WARN, "Emitter needs a sprite; load a texture first (Textures panel).\n");
      return;
    }
    spr = sf_load_sprite(&sf_ctx, "studio_spr", 1.0f, 0.3f, 1, sf_ctx.textures[0].name);
    if (!spr) return;
  }
  char name[32];
  snprintf(name, sizeof(name), "em_%d", sf_ctx.emitr_count);
  sf_emitr_type_t type = (g_emitr_type_sel == 0) ? SF_EMITR_DIR : (g_emitr_type_sel == 2) ? SF_EMITR_VOLUME : SF_EMITR_OMNI;
  int max_n = (int)g_emitr_max; if (max_n < 1) max_n = 1;
  sf_emitr_t *em = sf_add_emitr(&sf_ctx, name, type, spr, max_n);
  if (!em) return;
  em->spawn_rate    = g_emitr_rate;
  em->particle_life = g_emitr_life;
  em->speed         = g_emitr_speed;
  em->dir           = (sf_fvec3_t){0.0f, 1.0f, 0.0f};
  em->spread        = 0.2f;
  em->volume_size   = (sf_fvec3_t){5.0f, 5.0f, 5.0f};
  if (em->frame) {
    em->frame->pos = g_orbit_target;
    em->frame->is_dirty = true;
  }
  sel_clear();
  g_sel_kind = SEL_EMITR;
  g_sel_emitr = em;
  g_ui_dirty = true;
}

static void spawn_camera(void) {
  char name[32];
  snprintf(name, sizeof(name), "cam_%d", sf_ctx.cam_count);
  sf_cam_t *c = sf_add_cam(&sf_ctx, name, 240, 160, 60.0f);
  if (!c) return;
  if (c->frame) {
    c->frame->pos = g_orbit_target;
    c->frame->is_dirty = true;
  }
  sel_clear();
  g_sel_kind = SEL_CAM;
  g_sel_cam = c;
  g_ui_dirty = true;
}

/* --- CALLBACKS --- */

static void cb_spawn_kind(sf_ctx_t *ctx, void *ud) {
  (void)ctx;
  spawn_primitive((int)(intptr_t)ud);
}

static void cb_delete(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_sel_kind == SEL_ENTI && g_sel) {
    int idx = (int)(g_sel - sf_ctx.entities);
    if (idx < 0 || idx >= sf_ctx.enti_count) return;
    sf_remove_frame(&sf_ctx, g_sel->frame);
    for (int i = idx; i < sf_ctx.enti_count - 1; i++) sf_ctx.entities[i] = sf_ctx.entities[i + 1];
    for (int i = idx; i < sf_ctx.enti_count - 1 && i < SF_MAX_ENTITIES - 1; i++) g_enti_meta[i] = g_enti_meta[i + 1];
    if (sf_ctx.enti_count - 1 >= 0 && sf_ctx.enti_count - 1 < SF_MAX_ENTITIES) {
      memset(&g_enti_meta[sf_ctx.enti_count - 1], 0, sizeof(prim_meta_t));
    }
    sf_ctx.enti_count--;
  } else if (g_sel_kind == SEL_LIGHT && g_sel_light) {
    int idx = (int)(g_sel_light - sf_ctx.lights);
    if (idx < 0 || idx >= sf_ctx.light_count) return;
    sf_remove_frame(&sf_ctx, g_sel_light->frame);
    for (int i = idx; i < sf_ctx.light_count - 1; i++) sf_ctx.lights[i] = sf_ctx.lights[i + 1];
    sf_ctx.light_count--;
  } else if (g_sel_kind == SEL_CAM && g_sel_cam) {
    int idx = (int)(g_sel_cam - sf_ctx.cameras);
    if (idx < 0 || idx >= sf_ctx.cam_count) return;
    sf_remove_frame(&sf_ctx, g_sel_cam->frame);
    for (int i = idx; i < sf_ctx.cam_count - 1; i++) sf_ctx.cameras[i] = sf_ctx.cameras[i + 1];
    sf_ctx.cam_count--;
  } else if (g_sel_kind == SEL_EMITR && g_sel_emitr) {
    int idx = (int)(g_sel_emitr - sf_ctx.emitrs);
    if (idx < 0 || idx >= sf_ctx.emitr_count) return;
    sf_remove_frame(&sf_ctx, g_sel_emitr->frame);
    for (int i = idx; i < sf_ctx.emitr_count - 1; i++) sf_ctx.emitrs[i] = sf_ctx.emitrs[i + 1];
    sf_ctx.emitr_count--;
  } else {
    return;
  }
  sel_clear();
  g_ui_dirty = true;
}

static void cb_save_sff(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (sf_save_sff(&sf_ctx, g_save_path)) {
    SF_LOG(&sf_ctx, SF_LOG_INFO, "Saved to %s\n", g_save_path);
  } else {
    SF_LOG(&sf_ctx, SF_LOG_ERROR, "Save failed: %s\n", g_save_path);
  }
}

static void cb_load_sff(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  g_sel = NULL; g_sel_light = NULL; g_sel_cam = NULL; g_sel_emitr = NULL;
  g_sel_kind = SEL_NONE;
  sf_load_sff(&sf_ctx, g_save_path, "Loaded World");
  g_ui_dirty = true;
}

/* ---- SFUI DESIGNER ---- */

static void _design_remove_at(int idx) {
  if (!g_designer_ui || idx < 0 || idx >= g_designer_ui->count) return;
  for (int i = idx; i < g_designer_ui->count - 1; i++) g_designer_ui->elements[i] = g_designer_ui->elements[i + 1];
  g_designer_ui->count--;
}

static sf_ivec2_t canvas_origin(void) {
  int dw = (int)(g_design_canvas_w * g_design_zoom);
  int dh = (int)(g_design_canvas_h * g_design_zoom);
  int ox = (g_w - 10 - dw) / 2 + (int)g_design_pan_x;
  int oy = (g_h + 30 - dh) / 2 + (int)g_design_pan_y;
  if (ox < 230) ox = 230;
  if (oy < 18) oy = 18;
  return (sf_ivec2_t){ox, oy};
}

static void _design_shift_elements(int dx, int dy) {
  if (!g_designer_ui || (dx == 0 && dy == 0)) return;
  for (int i = 0; i < g_designer_ui->count; i++) {
    g_designer_ui->elements[i].v0.x += dx; g_designer_ui->elements[i].v0.y += dy;
    g_designer_ui->elements[i].v1.x += dx; g_designer_ui->elements[i].v1.y += dy;
  }
}

static void _design_apply_zoom(float new_zoom) {
  if (new_zoom < 0.25f) new_zoom = 0.25f;
  if (new_zoom > 4.0f)  new_zoom = 4.0f;
  if (!g_designer_ui) { g_design_zoom = new_zoom; return; }
  sf_ivec2_t old_org = canvas_origin();
  float ratio = new_zoom / g_design_zoom;
  float old_zoom = g_design_zoom;
  g_design_zoom = new_zoom;
  sf_ivec2_t new_org = canvas_origin();
  for (int i = 0; i < g_designer_ui->count; i++) {
    sf_ui_lmn_t *el = &g_designer_ui->elements[i];
    float lx0 = (el->v0.x - old_org.x) * ratio;
    float ly0 = (el->v0.y - old_org.y) * ratio;
    float lx1 = (el->v1.x - old_org.x) * ratio;
    float ly1 = (el->v1.y - old_org.y) * ratio;
    el->v0 = (sf_ivec2_t){new_org.x + (int)lx0, new_org.y + (int)ly0};
    el->v1 = (sf_ivec2_t){new_org.x + (int)lx1, new_org.y + (int)ly1};
  }
  (void)old_zoom;
}

static sf_ui_lmn_t* _design_add_element(int type) {
  if (!g_designer_ui) return NULL;
  sf_ui_t *saved = sf_ctx.ui;
  sf_ctx.ui = g_designer_ui;
  sf_ivec2_t org = canvas_origin();
  int cx = org.x + (int)g_design_canvas_w / 2;
  int cy = org.y + (int)g_design_canvas_h / 2;
  sf_ivec2_t v0 = {cx - 60, cy - 10};
  sf_ivec2_t v1 = {cx + 60, cy + 10};
  sf_ui_lmn_t *el = NULL;
  switch (type) {
    case SF_UI_BUTTON:   el = sf_ui_add_button(&sf_ctx, "Button", v0, v1, NULL, NULL); break;
    case SF_UI_LABEL:    el = sf_ui_add_label(&sf_ctx, "Label", v0, 0xFFEEEEEE);
                         if (el) el->v1 = v1; break;
    case SF_UI_PANEL:    el = sf_ui_add_panel(&sf_ctx, "Panel", (sf_ivec2_t){cx - 100, cy - 60}, (sf_ivec2_t){cx + 100, cy + 60}); break;
    case SF_UI_CHECKBOX: el = sf_ui_add_checkbox(&sf_ctx, "Check", v0, v1, false, NULL, NULL); break;
    case SF_UI_SLIDER:   el = sf_ui_add_slider(&sf_ctx, v0, v1, 0.0f, 1.0f, 0.5f, NULL, NULL); break;
    case SF_UI_DRAG_FLOAT: {
      float *tgt = (float*)sf_arena_alloc(&sf_ctx, &sf_ctx.arena, sizeof(float));
      if (tgt) { *tgt = 0.0f; el = sf_ui_add_drag_float(&sf_ctx, v0, v1, tgt, 0.05f, NULL, NULL); }
      break;
    }
    case SF_UI_TEXT_INPUT: {
      char *buf = (char*)sf_arena_alloc(&sf_ctx, &sf_ctx.arena, 64);
      if (buf) { buf[0] = '\0'; el = sf_ui_add_text_input(&sf_ctx, v0, v1, buf, 64, NULL, NULL); }
      break;
    }
    case SF_UI_DROPDOWN: {
      const char **items = (const char**)sf_arena_alloc(&sf_ctx, &sf_ctx.arena, sizeof(char*) * 2);
      items[0] = "option A"; items[1] = "option B";
      int *sel = (int*)sf_arena_alloc(&sf_ctx, &sf_ctx.arena, sizeof(int));
      if (sel) *sel = 0;
      el = sf_ui_add_dropdown(&sf_ctx, v0, v1, items, 2, sel, NULL, NULL);
      break;
    }
  }
  sf_ctx.ui = saved;
  if (el) g_design_sel = el;
  g_ui_dirty = true;
  return el;
}

static void cb_design_add(sf_ctx_t *c, void *ud)  { (void)c; _design_add_element((int)(intptr_t)ud); }
static void cb_design_del(sf_ctx_t *c, void *ud)  {
  (void)c; (void)ud;
  if (!g_designer_ui || !g_design_sel) return;
  int idx = (int)(g_design_sel - g_designer_ui->elements);
  _design_remove_at(idx);
  g_design_sel = NULL;
  g_ui_dirty = true;
}
static void cb_design_save(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  if (!g_designer_ui) return;
  sf_ivec2_t org = canvas_origin();
  float z = g_design_zoom;
  sf_ivec2_t *backup = (sf_ivec2_t*)malloc(sizeof(sf_ivec2_t) * 2 * g_designer_ui->count);
  for (int i = 0; i < g_designer_ui->count; i++) {
    backup[i*2+0] = g_designer_ui->elements[i].v0;
    backup[i*2+1] = g_designer_ui->elements[i].v1;
    g_designer_ui->elements[i].v0 = (sf_ivec2_t){(int)((g_designer_ui->elements[i].v0.x - org.x) / z),
                                                  (int)((g_designer_ui->elements[i].v0.y - org.y) / z)};
    g_designer_ui->elements[i].v1 = (sf_ivec2_t){(int)((g_designer_ui->elements[i].v1.x - org.x) / z),
                                                  (int)((g_designer_ui->elements[i].v1.y - org.y) / z)};
  }
  sf_save_sfui(&sf_ctx, g_designer_ui, g_sfui_path);
  for (int i = 0; i < g_designer_ui->count; i++) {
    g_designer_ui->elements[i].v0 = backup[i*2+0];
    g_designer_ui->elements[i].v1 = backup[i*2+1];
  }
  free(backup);
}
static void cb_design_load(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  sf_ui_t *loaded = sf_load_sfui(&sf_ctx, g_sfui_path);
  if (loaded) {
    sf_ivec2_t org = canvas_origin();
    float z = g_design_zoom;
    for (int i = 0; i < loaded->count; i++) {
      loaded->elements[i].v0 = (sf_ivec2_t){org.x + (int)(loaded->elements[i].v0.x * z),
                                             org.y + (int)(loaded->elements[i].v0.y * z)};
      loaded->elements[i].v1 = (sf_ivec2_t){org.x + (int)(loaded->elements[i].v1.x * z),
                                             org.y + (int)(loaded->elements[i].v1.y * z)};
    }
    g_designer_ui = loaded;
    g_design_sel = NULL;
    g_ui_dirty = true;
  }
}
static void cb_design_apply_bounds(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  if (!g_design_sel) return;
  sf_ivec2_t org = canvas_origin();
  float z = g_design_zoom;
  g_design_sel->v0 = (sf_ivec2_t){org.x + (int)(g_design_v0x * z), org.y + (int)(g_design_v0y * z)};
  g_design_sel->v1 = (sf_ivec2_t){org.x + (int)(g_design_v1x * z), org.y + (int)(g_design_v1y * z)};
}

static void cb_canvas_resized(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  static float prev_w = 800.0f, prev_h = 600.0f;
  sf_ivec2_t old_org = {
    (g_w - 10 - (int)(prev_w * g_design_zoom)) / 2 + (int)g_design_pan_x,
    (g_h + 30 - (int)(prev_h * g_design_zoom)) / 2 + (int)g_design_pan_y
  };
  if (old_org.x < 230) old_org.x = 230;
  if (old_org.y < 18)  old_org.y = 18;
  sf_ivec2_t new_org = canvas_origin();
  int dx = new_org.x - old_org.x, dy = new_org.y - old_org.y;
  if (g_designer_ui && (dx || dy)) {
    for (int i = 0; i < g_designer_ui->count; i++) {
      g_designer_ui->elements[i].v0.x += dx; g_designer_ui->elements[i].v0.y += dy;
      g_designer_ui->elements[i].v1.x += dx; g_designer_ui->elements[i].v1.y += dy;
    }
  }
  prev_w = g_design_canvas_w; prev_h = g_design_canvas_h;
}

static void cb_design_apply_slider(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  if (!g_design_sel || g_design_sel->type != SF_UI_SLIDER) return;
  g_design_sel->slider.min_val = g_design_slider_min;
  g_design_sel->slider.max_val = g_design_slider_max;
  g_design_sel->slider.value   = g_design_slider_val;
}
static void cb_design_apply_df(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  if (!g_design_sel || g_design_sel->type != SF_UI_DRAG_FLOAT) return;
  g_design_sel->drag_float.step = g_design_df_step;
}
static void cb_design_apply_checked(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  if (!g_design_sel || g_design_sel->type != SF_UI_CHECKBOX) return;
  g_design_sel->checkbox.is_checked = g_design_checked;
}
static void cb_design_apply_panel(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  if (!g_design_sel || g_design_sel->type != SF_UI_PANEL) return;
  g_design_sel->panel.collapsed = g_design_panel_collapsed;
}
static void cb_design_dd_add(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  if (!g_design_sel || g_design_sel->type != SF_UI_DROPDOWN) return;
  if (g_design_dd_item_buf[0] == '\0') return;
  int n = g_design_sel->dropdown.n_items;
  const char **old = g_design_sel->dropdown.items;
  const char **ni = (const char**)sf_arena_alloc(&sf_ctx, &sf_ctx.arena, sizeof(char*) * (n + 1));
  for (int i = 0; i < n; i++) ni[i] = old[i];
  ni[n] = _sf_arena_strdup(&sf_ctx, g_design_dd_item_buf);
  g_design_sel->dropdown.items = ni;
  g_design_sel->dropdown.n_items = n + 1;
  g_design_dd_item_buf[0] = '\0';
  g_ui_dirty = true;
}
static void cb_design_dd_pop(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  if (!g_design_sel || g_design_sel->type != SF_UI_DROPDOWN) return;
  if (g_design_sel->dropdown.n_items > 0) g_design_sel->dropdown.n_items--;
  if (g_design_sel->dropdown.selected && *g_design_sel->dropdown.selected >= g_design_sel->dropdown.n_items) {
    *g_design_sel->dropdown.selected = 0;
  }
  g_ui_dirty = true;
}
static void cb_design_apply_dd_sel(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  if (!g_design_sel || g_design_sel->type != SF_UI_DROPDOWN) return;
  if (g_design_sel->dropdown.selected) *g_design_sel->dropdown.selected = g_design_dd_selected;
}
static void cb_design_apply_name(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  if (!g_design_sel) return;
  g_design_sel->name = g_design_name_buf[0] ? _sf_arena_strdup(&sf_ctx, g_design_name_buf) : NULL;
}
static void cb_design_apply_text(sf_ctx_t *c, void *ud) {
  (void)c; (void)ud;
  if (!g_design_sel) return;
  char *mem = _sf_arena_strdup(&sf_ctx, g_design_text_buf);
  if (!mem) return;
  switch (g_design_sel->type) {
    case SF_UI_BUTTON:   g_design_sel->button.text   = mem; break;
    case SF_UI_LABEL:    g_design_sel->label.text    = mem; break;
    case SF_UI_CHECKBOX: g_design_sel->checkbox.text = mem; break;
    case SF_UI_PANEL:    g_design_sel->panel.title   = mem; break;
    default: break;
  }
}

static void cb_mark_dirty(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  sf_frame_t *f = sel_frame();
  if (f) f->is_dirty = true;
}

static void cb_scale_lock_tog(sf_ctx_t *ctx, void *ud) { (void)ctx; (void)ud; g_scale_lock = !g_scale_lock; }

static void cb_scale_x(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  sf_frame_t *f = sel_frame();
  if (!f) return;
  if (g_scale_lock) { f->scale.y = f->scale.x; f->scale.z = f->scale.x; }
  f->is_dirty = true;
}
static void cb_scale_y(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  sf_frame_t *f = sel_frame();
  if (!f) return;
  if (g_scale_lock) { f->scale.x = f->scale.y; f->scale.z = f->scale.y; }
  f->is_dirty = true;
}
static void cb_scale_z(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  sf_frame_t *f = sel_frame();
  if (!f) return;
  if (g_scale_lock) { f->scale.x = f->scale.z; f->scale.y = f->scale.z; }
  f->is_dirty = true;
}

static void cb_cam_proj_dirty(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_sel_kind == SEL_CAM && g_sel_cam) g_sel_cam->is_proj_dirty = true;
}

static void cb_spawn_cam(sf_ctx_t *ctx, void *ud) { (void)ctx; (void)ud; spawn_camera(); }

static void cb_apply_tex(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_tex_count == 0 || g_tex_sel < 0 || g_tex_sel >= g_tex_count) return;
  const char *fname = g_tex_files[g_tex_sel];
  char texname[64];
  const char *dot = strrchr(fname, '.');
  int stem = dot ? (int)(dot - fname) : (int)strlen(fname);
  if (stem > 60) stem = 60;
  snprintf(texname, sizeof(texname), "%.*s", stem, fname);

  sf_tex_t *tex = NULL;
  for (int i = 0; i < sf_ctx.tex_count; i++) {
    if (sf_ctx.textures[i].name && strcmp(sf_ctx.textures[i].name, texname) == 0) { tex = &sf_ctx.textures[i]; break; }
  }
  if (!tex) tex = sf_load_texture_bmp(&sf_ctx, fname, texname);
  if (tex && g_sel_kind == SEL_ENTI && g_sel) g_sel->tex = tex;
}

static void cb_tex_page_prev(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_tex_page > 0) g_tex_page--;
  g_ui_dirty = true;
}
static void cb_tex_page_next(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  int max_page = (g_tex_count + STUDIO_TEX_PER_PAGE - 1) / STUDIO_TEX_PER_PAGE - 1;
  if (g_tex_page < max_page) g_tex_page++;
  g_ui_dirty = true;
}
static void cb_apply_tex_page(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  int base = g_tex_page * STUDIO_TEX_PER_PAGE;
  g_tex_sel = base + g_tex_page_sel;
  cb_apply_tex(ctx, ud);
}

static void cb_apply_emitr_sprite(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_sel_kind != SEL_EMITR || !g_sel_emitr) return;
  int base = g_tex_page * STUDIO_TEX_PER_PAGE;
  int idx = base + g_tex_page_sel;
  if (idx < 0 || idx >= g_tex_count) return;
  const char *fname = g_tex_files[idx];
  char stemname[64];
  const char *dot = strrchr(fname, '.');
  int stem = dot ? (int)(dot - fname) : (int)strlen(fname);
  if (stem > 60) stem = 60;
  snprintf(stemname, sizeof(stemname), "%.*s", stem, fname);

  sf_tex_t *tex = NULL;
  for (int i = 0; i < sf_ctx.tex_count; i++) {
    if (sf_ctx.textures[i].name && strcmp(sf_ctx.textures[i].name, stemname) == 0) { tex = &sf_ctx.textures[i]; break; }
  }
  if (!tex) tex = sf_load_texture_bmp(&sf_ctx, fname, stemname);
  if (!tex) return;

  char sname[80];
  snprintf(sname, sizeof(sname), "spr_%s", stemname);
  sf_sprite_t *spr = sf_get_sprite_(&sf_ctx, sname, false);
  if (!spr) spr = sf_load_sprite(&sf_ctx, sname, 1.0f, 0.3f, 1, tex->name);
  if (spr) g_sel_emitr->sprite = spr;
}

static void cb_clear_tex(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_sel_kind == SEL_ENTI && g_sel) g_sel->tex = NULL;
}

static void rebuild_parent_list(void) {
  g_parent_count = 0;
  g_parent_items[g_parent_count++] = "(none)";
  sf_frame_t *sf = sel_frame();
  if (!sf) { g_parent_sel = 0; return; }
  for (int i = 0; i < sf_ctx.frames_count && g_parent_count < SF_MAX_FRAMES; i++) {
    sf_frame_t *f = &sf_ctx.frames[i];
    if (!f->name || f == sf) continue;
    bool cycle = false;
    for (sf_frame_t *p = f; p; p = p->parent) if (p == sf) { cycle = true; break; }
    if (cycle) continue;
    g_parent_items[g_parent_count++] = f->name;
  }
  g_parent_sel = 0;
  if (sf->parent && sf->parent->name) {
    for (int i = 1; i < g_parent_count; i++) {
      if (strcmp(g_parent_items[i], sf->parent->name) == 0) { g_parent_sel = i; break; }
    }
  }
}

static void cb_set_parent(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  sf_frame_t *sf = sel_frame();
  if (!sf) return;
  if (g_parent_sel <= 0 || g_parent_sel >= g_parent_count) {
    sf_frame_set_parent(sf, NULL);
  } else {
    const char *nm = g_parent_items[g_parent_sel];
    for (int i = 0; i < sf_ctx.frames_count; i++) {
      if (sf_ctx.frames[i].name && strcmp(sf_ctx.frames[i].name, nm) == 0) {
        sf_frame_set_parent(sf, &sf_ctx.frames[i]);
        break;
      }
    }
  }
  sf->is_dirty = true;
  g_ui_dirty = true;
}

static void cb_apply_rename(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_sel_kind == SEL_NONE) return;
  if (g_rename_buf[0] == '\0') return;
  size_t n = strlen(g_rename_buf) + 1;
  char *mem = (char*)sf_arena_alloc(&sf_ctx, &sf_ctx.arena, n);
  if (!mem) return;
  memcpy(mem, g_rename_buf, n);
  sf_frame_t *f = sel_frame();
  if (f) f->name = mem;
  switch (g_sel_kind) {
    case SEL_ENTI:  if (g_sel)       g_sel->name       = mem; break;
    case SEL_LIGHT: if (g_sel_light) g_sel_light->name = mem; break;
    case SEL_CAM:   if (g_sel_cam)   g_sel_cam->name   = mem; break;
    case SEL_EMITR: if (g_sel_emitr) g_sel_emitr->name = mem; break;
    default: break;
  }
  g_ui_dirty = true;
}

static void cb_unparent(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  sf_frame_t *sf = sel_frame();
  if (!sf) return;
  sf_frame_set_parent(sf, NULL);
  sf->is_dirty = true;
  g_ui_dirty = true;
}

static void cb_dbg_axes  (sf_ctx_t *c, void *ud) { (void)c; (void)ud; g_dbg_axes   = !g_dbg_axes; }
static void cb_dbg_frames(sf_ctx_t *c, void *ud) { (void)c; (void)ud; g_dbg_frames = !g_dbg_frames; }
static void cb_dbg_lights(sf_ctx_t *c, void *ud) { (void)c; (void)ud; g_dbg_lights = !g_dbg_lights; }
static void cb_dbg_cams  (sf_ctx_t *c, void *ud) { (void)c; (void)ud; g_dbg_cams   = !g_dbg_cams; }

/* --- UI REBUILD --- */

static void cb_tab_sff(sf_ctx_t *c, void *ud)  { (void)c; (void)ud; g_tab = TAB_SFF;  g_ui_dirty = true; }
static void cb_tab_sfui(sf_ctx_t *c, void *ud) { (void)c; (void)ud; g_tab = TAB_SFUI; g_ui_dirty = true; }

static void build_tab_bar(void) {
  int bw = 56, bh = 16, bx = 80, by = 0;
  const char *sff_lbl  = (g_tab == TAB_SFF)  ? "[SFF]"  : "SFF";
  const char *sfui_lbl = (g_tab == TAB_SFUI) ? "[SFUI]" : "SFUI";
  sf_ui_lmn_t *b0 = sf_ui_add_button(&sf_ctx, sff_lbl,  (sf_ivec2_t){bx,          by}, (sf_ivec2_t){bx + bw,     by + bh}, cb_tab_sff,  NULL);
  sf_ui_lmn_t *b1 = sf_ui_add_button(&sf_ctx, sfui_lbl, (sf_ivec2_t){bx + bw + 4, by}, (sf_ivec2_t){bx + 2*bw+4, by + bh}, cb_tab_sfui, NULL);
  sf_pkd_clr_t transp = (sf_pkd_clr_t)0xFF111111;
  sf_pkd_clr_t blue   = (sf_pkd_clr_t)0xFF88AAFF;
  if (b0) { b0->style.color_base = transp; b0->style.color_hover = (sf_pkd_clr_t)0xFF222233; b0->style.color_active = (sf_pkd_clr_t)0xFF222233; if (g_tab == TAB_SFF)  b0->style.color_text = blue; }
  if (b1) { b1->style.color_base = transp; b1->style.color_hover = (sf_pkd_clr_t)0xFF222233; b1->style.color_active = (sf_pkd_clr_t)0xFF222233; if (g_tab == TAB_SFUI) b1->style.color_text = blue; }
}

static void build_section_sep(int y, const char *title) {
  sf_ui_add_label(&sf_ctx, title, (sf_ivec2_t){20, y}, 0xFF88AAFF);
}

/* --- Outliner --- */
#define OUTL_MAX 256
typedef struct {
  sel_kind_t kind;
  void      *ptr;
  sf_frame_t *frame;
  int         depth;
} outl_item_t;
static outl_item_t g_outl_items[OUTL_MAX];
static int         g_outl_count = 0;

static sel_kind_t _outl_kind_of_frame(sf_frame_t *f, void **out_ptr) {
  for (int i = 0; i < sf_ctx.enti_count; i++) if (sf_ctx.entities[i].frame == f) { *out_ptr = &sf_ctx.entities[i]; return SEL_ENTI; }
  for (int i = 0; i < sf_ctx.light_count; i++) if (sf_ctx.lights[i].frame   == f) { *out_ptr = &sf_ctx.lights[i];   return SEL_LIGHT; }
  for (int i = 0; i < sf_ctx.cam_count;   i++) if (sf_ctx.cameras[i].frame  == f) { *out_ptr = &sf_ctx.cameras[i];  return SEL_CAM; }
  for (int i = 0; i < sf_ctx.emitr_count; i++) if (sf_ctx.emitrs[i].frame   == f) { *out_ptr = &sf_ctx.emitrs[i];   return SEL_EMITR; }
  *out_ptr = NULL;
  return SEL_NONE;
}

static void _outl_walk(sf_frame_t *f, int depth) {
  if (!f || g_outl_count >= OUTL_MAX) return;
  void *p = NULL;
  sel_kind_t k = _outl_kind_of_frame(f, &p);
  if (k != SEL_NONE) {
    g_outl_items[g_outl_count++] = (outl_item_t){ k, p, f, depth };
  }
  for (sf_frame_t *c = f->first_child; c; c = c->next_sibling) {
    _outl_walk(c, depth + 1);
  }
}

static void rebuild_outliner_items(void) {
  g_outl_count = 0;
  /* Walk from the root frame: find it by taking any frame's parent chain. */
  sf_frame_t *root = NULL;
  if (sf_ctx.enti_count > 0) {
    sf_frame_t *f = sf_ctx.entities[0].frame;
    while (f && f->parent) f = f->parent;
    root = f;
  } else if (sf_ctx.light_count > 0) {
    sf_frame_t *f = sf_ctx.lights[0].frame;
    while (f && f->parent) f = f->parent;
    root = f;
  }
  if (root) {
    for (sf_frame_t *c = root->first_child; c; c = c->next_sibling) _outl_walk(c, 0);
  }
  else {
    /* Fallback: list items in order even if no root found */
    for (int i = 0; i < sf_ctx.enti_count && g_outl_count < OUTL_MAX; i++)
      g_outl_items[g_outl_count++] = (outl_item_t){ SEL_ENTI, &sf_ctx.entities[i], sf_ctx.entities[i].frame, 0 };
    for (int i = 0; i < sf_ctx.light_count && g_outl_count < OUTL_MAX; i++)
      g_outl_items[g_outl_count++] = (outl_item_t){ SEL_LIGHT, &sf_ctx.lights[i], sf_ctx.lights[i].frame, 0 };
    for (int i = 0; i < sf_ctx.cam_count && g_outl_count < OUTL_MAX; i++)
      g_outl_items[g_outl_count++] = (outl_item_t){ SEL_CAM, &sf_ctx.cameras[i], sf_ctx.cameras[i].frame, 0 };
    for (int i = 0; i < sf_ctx.emitr_count && g_outl_count < OUTL_MAX; i++)
      g_outl_items[g_outl_count++] = (outl_item_t){ SEL_EMITR, &sf_ctx.emitrs[i], sf_ctx.emitrs[i].frame, 0 };
  }
}

static void cb_outliner_pick(sf_ctx_t *c, void *ud) {
  (void)c;
  int idx = (int)(intptr_t)ud;
  if (idx < 0 || idx >= g_outl_count) return;
  outl_item_t *it = &g_outl_items[idx];
  sel_clear();
  g_sel_kind = it->kind;
  switch (it->kind) {
    case SEL_ENTI:  g_sel       = (sf_enti_t*) it->ptr; break;
    case SEL_LIGHT: g_sel_light = (sf_light_t*)it->ptr; break;
    case SEL_CAM:   g_sel_cam   = (sf_cam_t*)  it->ptr; break;
    case SEL_EMITR: g_sel_emitr = (sf_emitr_t*)it->ptr; break;
    default: break;
  }
  g_ui_dirty = true;
}

static void cb_outl_scroll_up  (sf_ctx_t *c, void *ud) { (void)c; (void)ud; g_outl_scroll -= 5; if (g_outl_scroll < 0) g_outl_scroll = 0; g_ui_dirty = true; }
static void cb_outl_scroll_dn  (sf_ctx_t *c, void *ud) { (void)c; (void)ud; g_outl_scroll += 5; g_ui_dirty = true; }

static void build_outliner_panel(int rx0, int rx1, int top, int bottom) {
  static char s_rowbuf[OUTL_MAX][72];
  rebuild_outliner_items();
  int panel_h = bottom - top;
  sf_ui_add_panel(&sf_ctx, "Outliner", (sf_ivec2_t){rx0, top}, (sf_ivec2_t){rx1, top + panel_h});
  int content_y0 = top + 22;
  int content_y1 = top + panel_h - 24;
  int row_h = 16;
  int visible = (content_y1 - content_y0) / row_h;
  if (visible < 1) visible = 1;
  int max_scroll = g_outl_count - visible; if (max_scroll < 0) max_scroll = 0;
  if (g_outl_scroll > max_scroll) g_outl_scroll = max_scroll;

  int y = content_y0;
  for (int i = 0; i < visible && (g_outl_scroll + i) < g_outl_count; i++) {
    int idx = g_outl_scroll + i;
    outl_item_t *it = &g_outl_items[idx];
    const char *kind_glyph = (it->kind == SEL_ENTI) ? "[E]" : (it->kind == SEL_LIGHT) ? "[L]"
                           : (it->kind == SEL_CAM)  ? "[C]" : "[M]";
    /* sanitize name: only printable ASCII, cap length */
    char safe_nm[32] = "?";
    if (it->frame && it->frame->name) {
      int k = 0;
      for (const char *p = it->frame->name; *p && k < (int)sizeof(safe_nm)-1; p++) {
        unsigned char c = (unsigned char)*p;
        safe_nm[k++] = (c >= 32 && c < 127) ? (char)c : '?';
      }
      safe_nm[k] = '\0';
      if (k == 0) { safe_nm[0] = '?'; safe_nm[1] = '\0'; }
    }
    int depth = it->depth < 0 ? 0 : it->depth;
    if (depth > 6) depth = 6;
    /* tree branch prefix: "  " per ancestor, then "|- " at this node (root has no branch) */
    char indent[32] = {0};
    int ii = 0;
    for (int d = 0; d < depth; d++) { indent[ii++] = ' '; indent[ii++] = ' '; }
    if (depth > 0) { indent[ii++] = '|'; indent[ii++] = '-'; indent[ii++] = ' '; }
    indent[ii] = '\0';
    bool is_sel = (it->kind == g_sel_kind) && (
      (it->kind == SEL_ENTI  && it->ptr == g_sel)       ||
      (it->kind == SEL_LIGHT && it->ptr == g_sel_light) ||
      (it->kind == SEL_CAM   && it->ptr == g_sel_cam)   ||
      (it->kind == SEL_EMITR && it->ptr == g_sel_emitr));
    snprintf(s_rowbuf[idx], sizeof(s_rowbuf[0]), "%s%s%s %s", indent, is_sel ? "* " : "", kind_glyph, safe_nm);
    /* button centers text; pad right with spaces so content visually left-aligns */
    int row_chars = (rx1 - rx0 - 12) / 8;
    if (row_chars > (int)sizeof(s_rowbuf[0]) - 1) row_chars = (int)sizeof(s_rowbuf[0]) - 1;
    int cur = (int)strlen(s_rowbuf[idx]);
    while (cur < row_chars) s_rowbuf[idx][cur++] = ' ';
    s_rowbuf[idx][cur] = '\0';
    sf_ui_lmn_t *b = sf_ui_add_button(&sf_ctx, s_rowbuf[idx],
                                   (sf_ivec2_t){rx0 + 6,  y},
                                   (sf_ivec2_t){rx1 - 6,  y + row_h - 2},
                                   cb_outliner_pick, (void*)(intptr_t)idx);
    if (b && is_sel) { b->style.color_text = 0xFF88AAFF; }
    y += row_h;
  }

  /* scroll buttons at bottom of panel */
  int sy = top + panel_h - 22;
  static char pageinfo[32];
  snprintf(pageinfo, sizeof(pageinfo), "%d/%d", g_outl_count > 0 ? (g_outl_scroll + 1) : 0, g_outl_count);
  sf_ui_add_button(&sf_ctx, "<", (sf_ivec2_t){rx0 + 6,  sy}, (sf_ivec2_t){rx0 + 34, sy + 18}, cb_outl_scroll_up, NULL);
  sf_ui_add_label (&sf_ctx, pageinfo, (sf_ivec2_t){rx0 + 40, sy + 4}, 0xFFAAAAAA);
  sf_ui_add_button(&sf_ctx, ">", (sf_ivec2_t){rx1 - 34, sy}, (sf_ivec2_t){rx1 - 6,  sy + 18}, cb_outl_scroll_dn, NULL);
}

static void build_debug_panel(int rx0, int rx1, int r) {
  sf_ui_add_panel(&sf_ctx, "Overlay", (sf_ivec2_t){rx0, r}, (sf_ivec2_t){rx1, r + 94});
  sf_ui_add_checkbox(&sf_ctx, "frames",     (sf_ivec2_t){rx0 + 10, r + 22}, (sf_ivec2_t){rx1 - 10, r + 40},  g_dbg_frames, cb_dbg_frames, NULL);
  sf_ui_add_checkbox(&sf_ctx, "lights",     (sf_ivec2_t){rx0 + 10, r + 44}, (sf_ivec2_t){rx1 - 10, r + 62},  g_dbg_lights, cb_dbg_lights, NULL);
  sf_ui_add_checkbox(&sf_ctx, "cameras",    (sf_ivec2_t){rx0 + 10, r + 66}, (sf_ivec2_t){rx1 - 10, r + 84},  g_dbg_cams,   cb_dbg_cams,   NULL);
}

static const char *k_light_items[2] = { "point", "dir" };
static int g_light_type_sel = 0;

static void cb_light_type(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_sel_kind == SEL_LIGHT && g_sel_light) {
    g_sel_light->type = (g_light_type_sel == 0) ? SF_LIGHT_POINT : SF_LIGHT_DIR;
  }
}

/* Create: icon-button row. Two rows of up to 5 buttons each. Returns y past last row. */
static int build_spawn_buttons(int y) {
  const int BTN = 34, GAP = 2;
  int cols = 5;
  int y0 = y;
  for (int i = 0; i < 9; i++) {
    int row = i / cols;
    int col = i % cols;
    int x0 = 22 + col * (BTN + GAP);
    int yy = y0 + row * (BTN + GAP);
    g_spawn_btn_pos[i] = (sf_ivec2_t){x0, yy};
    const char *label = g_spawn_icon_tex[i] ? "" : k_spawn_labels[i];
    g_spawn_btn_el[i] = sf_ui_add_button(&sf_ctx, label, (sf_ivec2_t){x0, yy}, (sf_ivec2_t){x0 + BTN, yy + BTN}, cb_spawn_kind, (void*)(intptr_t)i);
  }
  g_spawn_btn_has_pos = true;
  return y0 + 2 * (BTN + GAP);
}

/* nearest-neighbor blit of a texture into the main camera buffer at dest rect */
static void blit_tex_scaled(sf_tex_t *tex, int dx, int dy, int dw, int dh) {
  if (!tex || !tex->px || dw <= 0 || dh <= 0) return;
  sf_cam_t *cam = &sf_ctx.main_camera;
  for (int y = 0; y < dh; y++) {
    int py = dy + y; if (py < 0 || py >= cam->h) continue;
    int sy = (y * tex->h) / dh;
    for (int x = 0; x < dw; x++) {
      int px_ = dx + x; if (px_ < 0 || px_ >= cam->w) continue;
      int sx = (x * tex->w) / dw;
      cam->buffer[py * cam->w + px_] = tex->px[sy * tex->w + sx];
    }
  }
}

/* blit with magic-pink (0xFF00FF) treated as transparent — matches sf_png2bmp key */
static void blit_tex_keyed(sf_tex_t *tex, int dx, int dy, int dw, int dh) {
  if (!tex || !tex->px || dw <= 0 || dh <= 0) return;
  sf_cam_t *cam = &sf_ctx.main_camera;
  for (int y = 0; y < dh; y++) {
    int py = dy + y; if (py < 0 || py >= cam->h) continue;
    int sy = (y * tex->h) / dh;
    for (int x = 0; x < dw; x++) {
      int px_ = dx + x; if (px_ < 0 || px_ >= cam->w) continue;
      int sx = (x * tex->w) / dw;
      sf_pkd_clr_t c = tex->px[sy * tex->w + sx];
      if ((c >> 24) == 0) continue;   /* loader already converted pink key to alpha=0 */
      cam->buffer[py * cam->w + px_] = c;
    }
  }
}

/* scale-blit an arbitrary sf_pkd_clr_t buffer (src w*h) into main cam at dest rect */
static void blit_buf_scaled(sf_pkd_clr_t *src, int sw, int sh, int dx, int dy, int dw, int dh) {
  if (!src || dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0) return;
  sf_cam_t *cam = &sf_ctx.main_camera;
  for (int y = 0; y < dh; y++) {
    int py = dy + y; if (py < 0 || py >= cam->h) continue;
    int sy = (y * sh) / dh;
    for (int x = 0; x < dw; x++) {
      int px_ = dx + x; if (px_ < 0 || px_ >= cam->w) continue;
      int sx = (x * sw) / dw;
      cam->buffer[py * cam->w + px_] = src[sy * sw + sx];
    }
  }
}

static void draw_cam_pip_overlay(void) {
  if (!g_cam_pip_visible || g_sel_kind != SEL_CAM || !g_sel_cam || !g_sel_cam->buffer) return;
  blit_buf_scaled(g_sel_cam->buffer, g_sel_cam->w, g_sel_cam->h,
                  g_cam_pip_pos.x, g_cam_pip_pos.y, g_cam_pip_size.x, g_cam_pip_size.y);
}

/* --- translate gizmo --- */

static float _gz_world_len(void) {
  /* scale gizmo length with camera distance so it stays ~constant on screen */
  sf_frame_t *f = sel_frame();
  if (!f) return 1.0f;
  sf_cam_t *cam = &sf_ctx.main_camera;
  sf_fvec3_t w = { f->global_M.m[3][0], f->global_M.m[3][1], f->global_M.m[3][2] };
  sf_fvec3_t v = sf_fmat4_mul_vec3(cam->V, w);
  float dist = -v.z; if (dist < 0.5f) dist = 0.5f;
  return dist * 0.12f;
}

static bool _gz_project(sf_fvec3_t w, sf_ivec2_t *out) {
  sf_cam_t *cam = &sf_ctx.main_camera;
  sf_fvec3_t v = sf_fmat4_mul_vec3(cam->V, w);
  if (v.z > -cam->near_plane) return false;
  sf_fvec3_t s = _sf_project_vertex(&sf_ctx, cam, v, cam->P);
  out->x = (int)s.x;
  out->y = (int)s.y;
  return true;
}

static void update_gizmo_geometry(void) {
  g_gz_active = false;
  sf_frame_t *f = sel_frame();
  if (!f) return;
  sf_fvec3_t origin_w = { f->global_M.m[3][0], f->global_M.m[3][1], f->global_M.m[3][2] };
  float L = _gz_world_len();
  sf_fvec3_t tips_w[3] = {
    { origin_w.x + L, origin_w.y,     origin_w.z     },
    { origin_w.x,     origin_w.y + L, origin_w.z     },
    { origin_w.x,     origin_w.y,     origin_w.z + L },
  };
  sf_ivec2_t so;
  if (!_gz_project(origin_w, &so)) return;
  g_gz_screen_origin = so;
  for (int i = 0; i < 3; i++) {
    if (!_gz_project(tips_w[i], &g_gz_screen_tip[i])) return;
    float dx = (float)(g_gz_screen_tip[i].x - so.x);
    float dy = (float)(g_gz_screen_tip[i].y - so.y);
    float len = sqrtf(dx*dx + dy*dy);
    g_gz_pixel_per_unit[i] = len / L;
  }
  g_gz_active = true;
}

/* distance from point p to segment a..b (squared), plus parametric t */
static float _seg_dist2(sf_ivec2_t a, sf_ivec2_t b, int px, int py, float *out_t) {
  float ax = (float)a.x, ay = (float)a.y;
  float bx = (float)b.x, by = (float)b.y;
  float dx = bx - ax, dy = by - ay;
  float len2 = dx*dx + dy*dy;
  float t = len2 > 0.0f ? (((float)px - ax) * dx + ((float)py - ay) * dy) / len2 : 0.0f;
  if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
  float cx = ax + dx * t, cy = ay + dy * t;
  float ex = (float)px - cx, ey = (float)py - cy;
  if (out_t) *out_t = t;
  return ex*ex + ey*ey;
}

static gz_axis_t gizmo_hit(int mx, int my) {
  if (!g_gz_active) return GZ_AX_NONE;
  const float R2 = 8.0f * 8.0f;
  float best = R2;
  gz_axis_t hit = GZ_AX_NONE;
  for (int i = 0; i < 3; i++) {
    float d2 = _seg_dist2(g_gz_screen_origin, g_gz_screen_tip[i], mx, my, NULL);
    if (d2 < best) { best = d2; hit = (gz_axis_t)i; }
  }
  return hit;
}

static void draw_gizmo(void) {
  if (!g_gz_active) return;
  sf_pkd_clr_t base[3]  = { 0xFFFF4040, 0xFF40FF40, 0xFF4080FF };
  sf_pkd_clr_t hi[3]    = { 0xFFFFFF80, 0xFFFFFF80, 0xFFFFFF80 };
  const char  *lbl[3]   = { "X", "Y", "Z" };
  sf_cam_t *cam = &sf_ctx.main_camera;
  for (int i = 0; i < 3; i++) {
    bool active = (g_gz_drag == (gz_axis_t)i) || (g_gz_drag == GZ_AX_NONE && g_gz_hover == (gz_axis_t)i);
    sf_pkd_clr_t c = active ? hi[i] : base[i];
    sf_line(&sf_ctx, cam, c, g_gz_screen_origin, g_gz_screen_tip[i]);
    /* arrowhead box */
    sf_ivec2_t t = g_gz_screen_tip[i];
    sf_rect(&sf_ctx, cam, c, (sf_ivec2_t){t.x - 3, t.y - 3}, (sf_ivec2_t){t.x + 3, t.y + 3});
    sf_put_text(&sf_ctx, cam, lbl[i], (sf_ivec2_t){t.x + 5, t.y - 4}, c, 1);
  }
}

/* axis unit vector in world */
static sf_fvec3_t _gz_axis_w(gz_axis_t a) {
  if (a == GZ_AX_X) return (sf_fvec3_t){1.0f, 0.0f, 0.0f};
  if (a == GZ_AX_Y) return (sf_fvec3_t){0.0f, 1.0f, 0.0f};
  return (sf_fvec3_t){0.0f, 0.0f, 1.0f};
}

static bool gizmo_begin_drag(int mx, int my) {
  gz_axis_t a = gizmo_hit(mx, my);
  if (a == GZ_AX_NONE) return false;
  sf_frame_t *f = sel_frame();
  if (!f) return false;
  g_gz_drag = a;
  g_gz_drag_start_pos = f->pos;
  g_gz_drag_start_mx = mx;
  g_gz_drag_start_my = my;
  return true;
}

static void gizmo_drag_update(int mx, int my) {
  if (g_gz_drag == GZ_AX_NONE || !g_gz_active) return;
  sf_frame_t *f = sel_frame();
  if (!f) { g_gz_drag = GZ_AX_NONE; return; }
  int axis = (int)g_gz_drag;
  float dx = (float)(g_gz_screen_tip[axis].x - g_gz_screen_origin.x);
  float dy = (float)(g_gz_screen_tip[axis].y - g_gz_screen_origin.y);
  float len = sqrtf(dx*dx + dy*dy);
  if (len < 1.0f) return;
  float ux = dx / len, uy = dy / len;
  float mdx = (float)(mx - g_gz_drag_start_mx);
  float mdy = (float)(my - g_gz_drag_start_my);
  float projected_px = mdx * ux + mdy * uy;
  float ppu = g_gz_pixel_per_unit[axis];
  if (ppu < 0.001f) return;
  float world_delta = projected_px / ppu;
  sf_fvec3_t ax = _gz_axis_w(g_gz_drag);
  /* if the frame has a parent, pos is local — convert world-axis delta to parent-local */
  if (f->parent && !f->is_root) {
    /* inverse of parent's rotation would be ideal, but for simplicity we drag in world;
       set pos from start_pos + world axis — works when parent has identity rotation.
       For rotated parents, this is approximate. */
  }
  f->pos.x = g_gz_drag_start_pos.x + ax.x * world_delta;
  f->pos.y = g_gz_drag_start_pos.y + ax.y * world_delta;
  f->pos.z = g_gz_drag_start_pos.z + ax.z * world_delta;
  f->is_dirty = true;
  g_ui_dirty = true;  /* rebuild inspector drag_float display */
}

static void gizmo_end_drag(void) {
  g_gz_drag = GZ_AX_NONE;
}

/* draw icons over the spawn buttons (called after sf_render_ui) */
static bool _spawn_btn_visible(int i) {
  sf_ui_lmn_t *el = g_spawn_btn_el[i];
  if (!el || !el->is_visible) return false;
  for (sf_ui_lmn_t *p = el->parent_panel; p; p = p->parent_panel) {
    if (!p->is_visible || p->panel.collapsed) return false;
  }
  return true;
}

static void draw_spawn_icons(void) {
  if (!g_spawn_btn_has_pos) return;
  const int BTN = 34;
  for (int i = 0; i < 9; i++) {
    if (!g_spawn_icon_tex[i]) continue;
    if (!_spawn_btn_visible(i)) continue;
    int x = g_spawn_btn_pos[i].x + 1;
    int y = g_spawn_btn_pos[i].y + 1;
    blit_tex_keyed(g_spawn_icon_tex[i], x, y, BTN - 2, BTN - 2);
  }
}

static bool _ui_el_visible(sf_ui_lmn_t *el) {
  if (!el || !el->is_visible) return false;
  for (sf_ui_lmn_t *p = el->parent_panel; p; p = p->parent_panel) {
    if (!p->is_visible || p->panel.collapsed) return false;
  }
  return true;
}

static void draw_ui_icons(void) {
  for (int i = 0; i < g_icon_reg_n; i++) {
    int id = g_icon_reg_id[i];
    sf_ui_lmn_t *el = g_icon_reg_el[i];
    if (!el || id < 0 || id >= ICN_COUNT || !g_icon_tex[id]) continue;
    if (!_ui_el_visible(el)) continue;
    int bw = el->v1.x - el->v0.x;
    int bh = el->v1.y - el->v0.y;
    int sz = bh - 2; if (sz > 18) sz = 18; if (sz < 10) sz = 10;
    int ix, iy;
    const char *lbl = el->button.text ? el->button.text : "";
    int txt_w = (int)strlen(lbl) * 8;
    if (txt_w == 0) {
      /* icon-only: center */
      ix = el->v0.x + (bw - sz) / 2;
    } else {
      /* wide button: text is centered; the "   " (3-space) prefix in the label
         reserves a 24px slot at the left; icon sits there */
      int txt_x = el->v0.x + (bw - txt_w) / 2;
      ix = txt_x + 3;
    }
    iy = el->v0.y + (bh - sz) / 2;
    blit_tex_keyed(g_icon_tex[id], ix, iy, sz, sz);
  }
}

/* adds texture picker + scale for the selected entity at y. returns y past last row. */
static int build_texture_section(int y) {
  static char s_pageinfo[32];
  int yy = y;
  if (g_tex_count > 0) {
    int total_pages = (g_tex_count + STUDIO_TEX_PER_PAGE - 1) / STUDIO_TEX_PER_PAGE;
    if (g_tex_page >= total_pages) g_tex_page = total_pages - 1;
    int base = g_tex_page * STUDIO_TEX_PER_PAGE;
    int page_n = g_tex_count - base;
    if (page_n > STUDIO_TEX_PER_PAGE) page_n = STUDIO_TEX_PER_PAGE;
    for (int i = 0; i < page_n; i++) g_tex_page_items[i] = g_tex_items[base + i];
    if (g_tex_page_sel >= page_n) g_tex_page_sel = 0;
    sf_ui_add_dropdown(&sf_ctx, (sf_ivec2_t){20, yy}, (sf_ivec2_t){210, yy + 20}, g_tex_page_items, page_n, &g_tex_page_sel, NULL, NULL);
    yy += 26;
    icon_btn(ICN_PREV, "", (sf_ivec2_t){20, yy}, (sf_ivec2_t){50, yy + 20}, cb_tex_page_prev, NULL);
    snprintf(s_pageinfo, sizeof(s_pageinfo), "%d/%d (%d)", g_tex_page + 1, total_pages, g_tex_count);
    sf_ui_add_label(&sf_ctx, s_pageinfo, (sf_ivec2_t){58, yy + 4}, 0xFFAAAAAA);
    icon_btn(ICN_NEXT, "", (sf_ivec2_t){180, yy}, (sf_ivec2_t){210, yy + 20}, cb_tex_page_next, NULL);
    yy += 26;
    icon_btn(ICN_APPLY, "   Apply", (sf_ivec2_t){20,  yy}, (sf_ivec2_t){112, yy + 20}, cb_apply_tex_page, NULL);
    icon_btn(ICN_CLEAR, "   Clear", (sf_ivec2_t){116, yy}, (sf_ivec2_t){210, yy + 20}, cb_clear_tex, NULL);
    yy += 26;
    sf_ui_add_label(&sf_ctx, (g_sel_kind == SEL_ENTI && g_sel && g_sel->tex && g_sel->tex->name) ? g_sel->tex->name : "(none)",
                 (sf_ivec2_t){20, yy}, 0xFFAAAAAA);
    yy += 14;
    if (g_sel_kind == SEL_ENTI && g_sel) {
      sf_ui_add_label(&sf_ctx, "scale u v", (sf_ivec2_t){20, yy}, SF_CLR_WHITE); yy += 14;
      sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  yy}, (sf_ivec2_t){112, yy + 20}, &g_sel->tex_scale.x, 0.05f, NULL, NULL);
      sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){116, yy}, (sf_ivec2_t){210, yy + 20}, &g_sel->tex_scale.y, 0.05f, NULL, NULL);
      yy += 26;
    }
  } else {
    sf_ui_add_label(&sf_ctx, "(no .bmp found)", (sf_ivec2_t){20, yy}, 0xFFAAAAAA);
    yy += 14;
  }
  return yy;
}

static void build_inspector(int y_start) {
  static char s_tagline[80];
  static char s_parentline[80];
  g_cam_pip_visible = false;
  int y = y_start;
  /* leave room for File panel (84 px tall + 10 px gap, pinned to bottom) */
  int panel_h = g_h - y_start - 94;
  sf_ui_add_panel(&sf_ctx, "Inspector", (sf_ivec2_t){10, y_start}, (sf_ivec2_t){220, y_start + panel_h});
  y += 26;

  sf_frame_t *sf = sel_frame();

  build_section_sep(y, "~ Create ~"); y += 16;
  y = build_spawn_buttons(y);
  y += 6;
  icon_btn(ICN_DELETE, "   Delete Selection", (sf_ivec2_t){20, y}, (sf_ivec2_t){210, y + 20}, cb_delete, NULL);
  y += 24;

  y += 6;
  build_section_sep(y, "~ Selection ~"); y += 16;

  if (!sf) {
    sf_ui_add_label(&sf_ctx, "(no selection)", (sf_ivec2_t){20, y}, 0xFFAAAAAA);
    return;
  }

  const char *tag = (g_sel_kind == SEL_ENTI) ? "entity" : (g_sel_kind == SEL_LIGHT) ? "light" : (g_sel_kind == SEL_CAM) ? "camera" : "emitter";
  snprintf(s_tagline, sizeof(s_tagline), "%s: %s", tag, sel_name());
  sf_ui_add_label(&sf_ctx, s_tagline, (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;

  /* sync rename buffer when selection changes */
  void *sel_ptr = (g_sel_kind == SEL_ENTI)  ? (void*)g_sel
                : (g_sel_kind == SEL_LIGHT) ? (void*)g_sel_light
                : (g_sel_kind == SEL_CAM)   ? (void*)g_sel_cam
                : (void*)g_sel_emitr;
  if (sel_ptr != g_rename_last) {
    const char *cur = sel_name();
    strncpy(g_rename_buf, cur ? cur : "", sizeof(g_rename_buf) - 1);
    g_rename_buf[sizeof(g_rename_buf) - 1] = '\0';
    g_rename_last = sel_ptr;
  }
  sf_ui_add_label(&sf_ctx, "name", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
  sf_ui_add_text_input(&sf_ctx, (sf_ivec2_t){20, y}, (sf_ivec2_t){140, y + 20}, g_rename_buf, sizeof(g_rename_buf), NULL, NULL);
  sf_ui_add_button(&sf_ctx, "Rename", (sf_ivec2_t){144, y}, (sf_ivec2_t){210, y + 20}, cb_apply_rename, NULL);
  y += 26;

  sf_ui_add_label(&sf_ctx, "pos x y z", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
  sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){82,  y + 20}, &sf->pos.x, 0.05f, cb_mark_dirty, NULL);
  sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){84,  y}, (sf_ivec2_t){146, y + 20}, &sf->pos.y, 0.05f, cb_mark_dirty, NULL);
  sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){148, y}, (sf_ivec2_t){210, y + 20}, &sf->pos.z, 0.05f, cb_mark_dirty, NULL);
  y += 26;
  sf_ui_add_label(&sf_ctx, "rot x y z", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
  sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){82,  y + 20}, &sf->rot.x, 0.01f, cb_mark_dirty, NULL);
  sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){84,  y}, (sf_ivec2_t){146, y + 20}, &sf->rot.y, 0.01f, cb_mark_dirty, NULL);
  sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){148, y}, (sf_ivec2_t){210, y + 20}, &sf->rot.z, 0.01f, cb_mark_dirty, NULL);
  y += 26;

  if (g_sel_kind == SEL_ENTI) {
    sf_ui_add_label(&sf_ctx, "scale x y z", (sf_ivec2_t){20, y}, SF_CLR_WHITE);
    sf_ui_add_checkbox(&sf_ctx, "lock", (sf_ivec2_t){140, y - 2}, (sf_ivec2_t){210, y + 14}, g_scale_lock, cb_scale_lock_tog, NULL);
    y += 14;
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){82,  y + 20}, &sf->scale.x, 0.01f, cb_scale_x, NULL);
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){84,  y}, (sf_ivec2_t){146, y + 20}, &sf->scale.y, 0.01f, cb_scale_y, NULL);
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){148, y}, (sf_ivec2_t){210, y + 20}, &sf->scale.z, 0.01f, cb_scale_z, NULL);
    y += 26;
    prim_meta_t *m = sel_meta();
    if (m && m->kind != PM_NONE) {
      switch (m->kind) {
        case PM_PLANE:
          sf_ui_add_label(&sf_ctx, "plane sx sz", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){112, y + 20}, &m->p[0], 0.05f, cb_regen_sel, NULL);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){116, y}, (sf_ivec2_t){210, y + 20}, &m->p[1], 0.05f, cb_regen_sel, NULL);
          y += 26;
          sf_ui_add_label(&sf_ctx, "res", (sf_ivec2_t){20, y}, SF_CLR_WHITE);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){60, y - 2}, (sf_ivec2_t){210, y + 18}, &m->p[2], 1.0f, cb_regen_sel, NULL);
          y += 26;
          break;
        case PM_BOX:
          sf_ui_add_label(&sf_ctx, "box sx sy sz", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){82,  y + 20}, &m->p[0], 0.05f, cb_regen_sel, NULL);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){84,  y}, (sf_ivec2_t){146, y + 20}, &m->p[1], 0.05f, cb_regen_sel, NULL);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){148, y}, (sf_ivec2_t){210, y + 20}, &m->p[2], 0.05f, cb_regen_sel, NULL);
          y += 26;
          break;
        case PM_SPHERE:
          sf_ui_add_label(&sf_ctx, "radius", (sf_ivec2_t){20, y}, SF_CLR_WHITE);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){80, y - 2}, (sf_ivec2_t){210, y + 18}, &m->p[0], 0.05f, cb_regen_sel, NULL);
          y += 26;
          sf_ui_add_label(&sf_ctx, "segs", (sf_ivec2_t){20, y}, SF_CLR_WHITE);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){80, y - 2}, (sf_ivec2_t){210, y + 18}, &m->p[1], 1.0f, cb_regen_sel, NULL);
          y += 26;
          break;
        case PM_CYL:
          sf_ui_add_label(&sf_ctx, "cyl r / h", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){112, y + 20}, &m->p[0], 0.05f, cb_regen_sel, NULL);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){116, y}, (sf_ivec2_t){210, y + 20}, &m->p[1], 0.05f, cb_regen_sel, NULL);
          y += 26;
          sf_ui_add_label(&sf_ctx, "segs", (sf_ivec2_t){20, y}, SF_CLR_WHITE);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){80, y - 2}, (sf_ivec2_t){210, y + 18}, &m->p[2], 1.0f, cb_regen_sel, NULL);
          y += 26;
          break;
        case PM_TERRAIN:
          sf_ui_add_label(&sf_ctx, "terrain sx sz", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){112, y + 20}, &m->p[0], 0.5f, cb_regen_sel, NULL);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){116, y}, (sf_ivec2_t){210, y + 20}, &m->p[1], 0.5f, cb_regen_sel, NULL);
          y += 26;
          sf_ui_add_label(&sf_ctx, "res", (sf_ivec2_t){20, y}, SF_CLR_WHITE);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){60, y - 2}, (sf_ivec2_t){210, y + 18}, &m->p[2], 1.0f, cb_regen_sel, NULL);
          y += 26;
          sf_ui_add_label(&sf_ctx, "amp / freq", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){112, y + 20}, &m->p[3], 0.05f,  cb_regen_sel, NULL);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){116, y}, (sf_ivec2_t){210, y + 20}, &m->p[4], 0.005f, cb_regen_sel, NULL);
          y += 26;
          sf_ui_add_label(&sf_ctx, "oct", (sf_ivec2_t){20, y}, SF_CLR_WHITE);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){60, y - 2}, (sf_ivec2_t){210, y + 18}, &m->p[5], 1.0f, cb_regen_sel, NULL);
          y += 26;
          sf_ui_add_label(&sf_ctx, "seed", (sf_ivec2_t){20, y}, SF_CLR_WHITE);
          sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){60, y - 2}, (sf_ivec2_t){210, y + 18}, &m->p[6], 1.0f, cb_regen_sel, NULL);
          y += 26;
          break;
        case PM_MODEL:
          if (g_model_count > 0) {
            if (m->model_idx < 0 || m->model_idx >= g_model_count) m->model_idx = 0;
            sf_ui_add_label(&sf_ctx, "model", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
            sf_ui_add_dropdown(&sf_ctx, (sf_ivec2_t){20, y}, (sf_ivec2_t){210, y + 20}, g_model_items, g_model_count, &m->model_idx, cb_regen_sel, NULL);
            y += 26;
          }
          break;
        default: break;
      }
    }
  } else if (g_sel_kind == SEL_LIGHT && g_sel_light) {
    g_light_type_sel = (g_sel_light->type == SF_LIGHT_DIR) ? 1 : 0;
    sf_ui_add_label(&sf_ctx, "type", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
    sf_ui_add_dropdown(&sf_ctx, (sf_ivec2_t){20, y}, (sf_ivec2_t){210, y + 20}, k_light_items, 2, &g_light_type_sel, cb_light_type, NULL);
    y += 26;
    sf_ui_add_label(&sf_ctx, "color r g b", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){82,  y + 20}, &g_sel_light->color.x, 0.01f, NULL, NULL);
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){84,  y}, (sf_ivec2_t){146, y + 20}, &g_sel_light->color.y, 0.01f, NULL, NULL);
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){148, y}, (sf_ivec2_t){210, y + 20}, &g_sel_light->color.z, 0.01f, NULL, NULL);
    y += 26;
    sf_ui_add_label(&sf_ctx, "intensity", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20, y}, (sf_ivec2_t){210, y + 20}, &g_sel_light->intensity, 0.05f, NULL, NULL);
    y += 26;
  } else if (g_sel_kind == SEL_EMITR && g_sel_emitr) {
    int ts = (g_sel_emitr->type == SF_EMITR_DIR) ? 0 : (g_sel_emitr->type == SF_EMITR_VOLUME) ? 2 : 1;
    static int _es;
    _es = ts;
    sf_ui_add_label(&sf_ctx, "type", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
    sf_ui_add_dropdown(&sf_ctx, (sf_ivec2_t){20, y}, (sf_ivec2_t){210, y + 20}, k_emitr_items, 3, &_es, NULL, NULL);
    g_sel_emitr->type = (_es == 0) ? SF_EMITR_DIR : (_es == 2) ? SF_EMITR_VOLUME : SF_EMITR_OMNI;
    y += 26;
    sf_ui_add_label(&sf_ctx, "rate / life", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){112, y + 20}, &g_sel_emitr->spawn_rate,    0.5f,  NULL, NULL);
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){116, y}, (sf_ivec2_t){210, y + 20}, &g_sel_emitr->particle_life, 0.05f, NULL, NULL);
    y += 26;
    sf_ui_add_label(&sf_ctx, "speed / spread", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){112, y + 20}, &g_sel_emitr->speed,  0.05f, NULL, NULL);
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){116, y}, (sf_ivec2_t){210, y + 20}, &g_sel_emitr->spread, 0.01f, NULL, NULL);
    y += 26;
    sf_ui_add_label(&sf_ctx, "sprite", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
    sf_ui_add_label(&sf_ctx, (g_sel_emitr->sprite && g_sel_emitr->sprite->name) ? g_sel_emitr->sprite->name : "(none)",
                 (sf_ivec2_t){20, y}, 0xFFAAAAAA); y += 14;
    if (g_tex_count > 0) {
      int total_pages = (g_tex_count + STUDIO_TEX_PER_PAGE - 1) / STUDIO_TEX_PER_PAGE;
      if (g_tex_page >= total_pages) g_tex_page = total_pages - 1;
      int base2 = g_tex_page * STUDIO_TEX_PER_PAGE;
      int page_n = g_tex_count - base2;
      if (page_n > STUDIO_TEX_PER_PAGE) page_n = STUDIO_TEX_PER_PAGE;
      for (int i = 0; i < page_n; i++) g_tex_page_items[i] = g_tex_items[base2 + i];
      if (g_tex_page_sel >= page_n) g_tex_page_sel = 0;
      sf_ui_add_dropdown(&sf_ctx, (sf_ivec2_t){20, y}, (sf_ivec2_t){210, y + 20}, g_tex_page_items, page_n, &g_tex_page_sel, NULL, NULL);
      y += 26;
      icon_btn(ICN_PREV, "", (sf_ivec2_t){20, y}, (sf_ivec2_t){50, y + 20}, cb_tex_page_prev, NULL);
      icon_btn(ICN_NEXT, "", (sf_ivec2_t){180, y}, (sf_ivec2_t){210, y + 20}, cb_tex_page_next, NULL);
      icon_btn(ICN_TEXTURE, "   Set Sprite", (sf_ivec2_t){54, y}, (sf_ivec2_t){176, y + 20}, cb_apply_emitr_sprite, NULL);
      y += 26;
    } else {
      sf_ui_add_label(&sf_ctx, "(no .bmp found)", (sf_ivec2_t){20, y}, 0xFFAAAAAA);
      y += 14;
    }
  } else if (g_sel_kind == SEL_CAM && g_sel_cam) {
    sf_ui_add_label(&sf_ctx, "fov / near / far", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20,  y}, (sf_ivec2_t){82,  y + 20}, &g_sel_cam->fov,        0.5f,  cb_cam_proj_dirty, NULL);
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){84,  y}, (sf_ivec2_t){146, y + 20}, &g_sel_cam->near_plane, 0.01f, cb_cam_proj_dirty, NULL);
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){148, y}, (sf_ivec2_t){210, y + 20}, &g_sel_cam->far_plane,  1.0f,  cb_cam_proj_dirty, NULL);
    y += 26;
    sf_ui_add_label(&sf_ctx, "preview", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
    int pip_w = 190;
    int pip_h = (g_sel_cam->h > 0 && g_sel_cam->w > 0) ? (pip_w * g_sel_cam->h) / g_sel_cam->w : 120;
    g_cam_pip_pos  = (sf_ivec2_t){20, y};
    g_cam_pip_size = (sf_ivec2_t){pip_w, pip_h};
    g_cam_pip_visible = true;
    y += pip_h + 6;
  }

  if (g_sel_kind == SEL_ENTI) {
    y += 6;
    build_section_sep(y, "~ Texture ~"); y += 16;
    y = build_texture_section(y);
  }

  y += 6;
  build_section_sep(y, "~ Parenting ~"); y += 16;
  const char *pn = (sf->parent && sf->parent->name) ? sf->parent->name : "(none)";
  snprintf(s_parentline, sizeof(s_parentline), "parent: %s", pn);
  sf_ui_add_label(&sf_ctx, s_parentline, (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
  sf_ui_add_dropdown(&sf_ctx, (sf_ivec2_t){20, y}, (sf_ivec2_t){210, y + 20}, g_parent_items, g_parent_count, &g_parent_sel, NULL, NULL);
  y += 26;
  icon_btn(ICN_PARENT, "   Parent",   (sf_ivec2_t){20,  y}, (sf_ivec2_t){112, y + 20}, cb_set_parent, NULL);
  icon_btn(ICN_UNGROUP, "   Unparent", (sf_ivec2_t){114, y}, (sf_ivec2_t){210, y + 20}, cb_unparent,   NULL);
}

static void build_sff_tab(void) {
  const int TOP = 30;

  /* Left: File panel pinned to bottom */
  int fy = g_h - 84;
  sf_ui_add_panel(&sf_ctx, "File", (sf_ivec2_t){10, fy}, (sf_ivec2_t){220, fy + 74});
  sf_ui_add_text_input(&sf_ctx, (sf_ivec2_t){20, fy + 22}, (sf_ivec2_t){210, fy + 42}, g_save_path, sizeof(g_save_path), NULL, NULL);
  icon_btn(ICN_SAVE, "   Save", (sf_ivec2_t){20,  fy + 46}, (sf_ivec2_t){112, fy + 66}, cb_save_sff, NULL);
  icon_btn(ICN_OPEN, "   Load", (sf_ivec2_t){116, fy + 46}, (sf_ivec2_t){210, fy + 66}, cb_load_sff, NULL);

  /* Inspector fills remaining left column above File panel */
  build_inspector(TOP);

  int rx0 = g_w - 230, rx1 = g_w - 10;
  build_debug_panel(rx0, rx1, TOP);
  build_outliner_panel(rx0, rx1, TOP + 104, g_h - 10);
}

static void build_sfui_tab(void);

static void rebuild_ui(void) {
  sf_ui_clear(&sf_ctx);
  g_spawn_btn_has_pos = false;
  for (int i = 0; i < 9; i++) g_spawn_btn_el[i] = NULL;
  g_icon_reg_n = 0;
  rebuild_parent_list();
  build_tab_bar();
  if (g_tab == TAB_SFF) build_sff_tab();
  else                  build_sfui_tab();
}

static const char* _design_type_str(int t) {
  switch (t) {
    case SF_UI_BUTTON: return "button"; case SF_UI_SLIDER: return "slider";
    case SF_UI_CHECKBOX: return "checkbox"; case SF_UI_LABEL: return "label";
    case SF_UI_TEXT_INPUT: return "text_input"; case SF_UI_DRAG_FLOAT: return "drag_float";
    case SF_UI_DROPDOWN: return "dropdown"; case SF_UI_PANEL: return "panel";
  }
  return "?";
}

static const char* _design_sel_text(sf_ui_lmn_t *el) {
  if (!el) return "";
  switch (el->type) {
    case SF_UI_BUTTON:   return el->button.text   ? el->button.text   : "";
    case SF_UI_LABEL:    return el->label.text    ? el->label.text    : "";
    case SF_UI_CHECKBOX: return el->checkbox.text ? el->checkbox.text : "";
    case SF_UI_PANEL:    return el->panel.title   ? el->panel.title   : "";
    default: return "";
  }
}

static void build_sfui_tab(void) {
  const int TOP = 30;
  int y = TOP;

  /* Left: palette */
  sf_ui_add_panel(&sf_ctx, "Palette", (sf_ivec2_t){10, TOP}, (sf_ivec2_t){220, TOP + 252});
  y = TOP + 22;
  const char *lbls[8] = { "Button", "Label", "Panel", "Checkbox", "Slider", "DragFloat", "TextInput", "Dropdown" };
  int kinds[8] = { SF_UI_BUTTON, SF_UI_LABEL, SF_UI_PANEL, SF_UI_CHECKBOX, SF_UI_SLIDER, SF_UI_DRAG_FLOAT, SF_UI_TEXT_INPUT, SF_UI_DROPDOWN };
  for (int i = 0; i < 8; i++) {
    sf_ui_add_button(&sf_ctx, lbls[i], (sf_ivec2_t){20, y}, (sf_ivec2_t){210, y + 20}, cb_design_add, (void*)(intptr_t)kinds[i]);
    y += 24;
  }
  y += 6;
  icon_btn(ICN_DELETE, "   Delete Selected", (sf_ivec2_t){20, y}, (sf_ivec2_t){210, y + 20}, cb_design_del, NULL);
  y += 28;

  /* Left: canvas */
  sf_ui_add_panel(&sf_ctx, "Canvas", (sf_ivec2_t){10, y}, (sf_ivec2_t){220, y + 70});
  y += 26;
  sf_ui_add_label(&sf_ctx, "size w h", (sf_ivec2_t){20, y}, SF_CLR_WHITE); y += 14;
  sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){20, y},  (sf_ivec2_t){112, y + 20}, &g_design_canvas_w, 4.0f, cb_canvas_resized, NULL);
  sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){116, y}, (sf_ivec2_t){210, y + 20}, &g_design_canvas_h, 4.0f, cb_canvas_resized, NULL);
  y += 28;

  /* Left: file */
  sf_ui_add_panel(&sf_ctx, "File", (sf_ivec2_t){10, y}, (sf_ivec2_t){220, y + 80});
  y += 26;
  sf_ui_add_text_input(&sf_ctx, (sf_ivec2_t){20, y}, (sf_ivec2_t){210, y + 20}, g_sfui_path, sizeof(g_sfui_path), NULL, NULL);
  y += 24;
  icon_btn(ICN_SAVE, "   Save", (sf_ivec2_t){20,  y}, (sf_ivec2_t){112, y + 20}, cb_design_save, NULL);
  icon_btn(ICN_OPEN, "   Load", (sf_ivec2_t){116, y}, (sf_ivec2_t){210, y + 20}, cb_design_load, NULL);

  /* Right: inspector */
  int rx0 = g_w - 230, rx1 = g_w - 10;
  int r = TOP;
  sf_ui_add_panel(&sf_ctx, "Properties", (sf_ivec2_t){rx0, r}, (sf_ivec2_t){rx1, r + 360});
  int ry = r + 22;
  if (!g_design_sel) {
    sf_ui_add_label(&sf_ctx, "(no selection)", (sf_ivec2_t){rx0 + 10, ry}, 0xFFAAAAAA);
  } else {
    static char s_type[40];
    snprintf(s_type, sizeof(s_type), "type: %s", _design_type_str(g_design_sel->type));
    sf_ui_add_label(&sf_ctx, s_type, (sf_ivec2_t){rx0 + 10, ry}, SF_CLR_WHITE); ry += 16;
    sf_ui_add_label(&sf_ctx, "name", (sf_ivec2_t){rx0 + 10, ry}, SF_CLR_WHITE); ry += 14;
    sf_ui_add_text_input(&sf_ctx, (sf_ivec2_t){rx0 + 10, ry}, (sf_ivec2_t){rx1 - 60, ry + 20}, g_design_name_buf, sizeof(g_design_name_buf), NULL, NULL);
    sf_ui_add_button(&sf_ctx, "Set", (sf_ivec2_t){rx1 - 56, ry}, (sf_ivec2_t){rx1 - 10, ry + 20}, cb_design_apply_name, NULL);
    ry += 26;

    sf_ivec2_t org = canvas_origin();
    float cz = g_design_zoom;
    if ((void*)g_design_sel != g_design_text_last) {
      snprintf(g_design_text_buf, sizeof(g_design_text_buf), "%s", _design_sel_text(g_design_sel));
      snprintf(g_design_name_buf, sizeof(g_design_name_buf), "%s", g_design_sel->name ? g_design_sel->name : "");
      g_design_v0x = (float)((g_design_sel->v0.x - org.x) / cz); g_design_v0y = (float)((g_design_sel->v0.y - org.y) / cz);
      g_design_v1x = (float)((g_design_sel->v1.x - org.x) / cz); g_design_v1y = (float)((g_design_sel->v1.y - org.y) / cz);
      if (g_design_sel->type == SF_UI_SLIDER) {
        g_design_slider_min = g_design_sel->slider.min_val;
        g_design_slider_max = g_design_sel->slider.max_val;
        g_design_slider_val = g_design_sel->slider.value;
      } else if (g_design_sel->type == SF_UI_DRAG_FLOAT) {
        g_design_df_step = g_design_sel->drag_float.step;
      } else if (g_design_sel->type == SF_UI_CHECKBOX) {
        g_design_checked = g_design_sel->checkbox.is_checked;
      } else if (g_design_sel->type == SF_UI_PANEL) {
        g_design_panel_collapsed = g_design_sel->panel.collapsed;
      } else if (g_design_sel->type == SF_UI_DROPDOWN) {
        g_design_dd_selected = g_design_sel->dropdown.selected ? *g_design_sel->dropdown.selected : 0;
      } else if (g_design_sel->type == SF_UI_TEXT_INPUT) {
        g_design_buflen = g_design_sel->text_input.buflen;
      }
      g_design_text_last = (void*)g_design_sel;
    }
    sf_ui_add_label(&sf_ctx, "v0 x y (canvas)", (sf_ivec2_t){rx0 + 10, ry}, SF_CLR_WHITE); ry += 14;
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){rx0 + 10, ry}, (sf_ivec2_t){rx0 + 105, ry + 20}, &g_design_v0x, 1.0f, cb_design_apply_bounds, NULL);
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){rx0 + 110, ry}, (sf_ivec2_t){rx1 - 10, ry + 20}, &g_design_v0y, 1.0f, cb_design_apply_bounds, NULL);
    ry += 26;
    sf_ui_add_label(&sf_ctx, "v1 x y (canvas)", (sf_ivec2_t){rx0 + 10, ry}, SF_CLR_WHITE); ry += 14;
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){rx0 + 10, ry}, (sf_ivec2_t){rx0 + 105, ry + 20}, &g_design_v1x, 1.0f, cb_design_apply_bounds, NULL);
    sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){rx0 + 110, ry}, (sf_ivec2_t){rx1 - 10, ry + 20}, &g_design_v1y, 1.0f, cb_design_apply_bounds, NULL);
    ry += 26;
    int t = g_design_sel->type;
    if (t == SF_UI_BUTTON || t == SF_UI_LABEL || t == SF_UI_CHECKBOX || t == SF_UI_PANEL) {
      sf_ui_add_label(&sf_ctx, "text", (sf_ivec2_t){rx0 + 10, ry}, SF_CLR_WHITE); ry += 14;
      sf_ui_add_text_input(&sf_ctx, (sf_ivec2_t){rx0 + 10, ry}, (sf_ivec2_t){rx1 - 60, ry + 20}, g_design_text_buf, sizeof(g_design_text_buf), NULL, NULL);
      sf_ui_add_button(&sf_ctx, "Set", (sf_ivec2_t){rx1 - 56, ry}, (sf_ivec2_t){rx1 - 10, ry + 20}, cb_design_apply_text, NULL);
      ry += 26;
    }
    if (t == SF_UI_SLIDER) {
      sf_ui_add_label(&sf_ctx, "min / max", (sf_ivec2_t){rx0 + 10, ry}, SF_CLR_WHITE); ry += 14;
      sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){rx0 + 10, ry},  (sf_ivec2_t){rx0 + 105, ry + 20}, &g_design_slider_min, 0.05f, cb_design_apply_slider, NULL);
      sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){rx0 + 110, ry}, (sf_ivec2_t){rx1 - 10, ry + 20},  &g_design_slider_max, 0.05f, cb_design_apply_slider, NULL);
      ry += 26;
      sf_ui_add_label(&sf_ctx, "value", (sf_ivec2_t){rx0 + 10, ry}, SF_CLR_WHITE); ry += 14;
      sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){rx0 + 10, ry}, (sf_ivec2_t){rx1 - 10, ry + 20}, &g_design_slider_val, 0.01f, cb_design_apply_slider, NULL);
      ry += 26;
    }
    if (t == SF_UI_DRAG_FLOAT) {
      sf_ui_add_label(&sf_ctx, "step", (sf_ivec2_t){rx0 + 10, ry}, SF_CLR_WHITE); ry += 14;
      sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){rx0 + 10, ry}, (sf_ivec2_t){rx1 - 10, ry + 20}, &g_design_df_step, 0.005f, cb_design_apply_df, NULL);
      ry += 26;
    }
    if (t == SF_UI_CHECKBOX) {
      sf_ui_add_checkbox(&sf_ctx, "initial checked", (sf_ivec2_t){rx0 + 10, ry}, (sf_ivec2_t){rx1 - 10, ry + 20}, g_design_checked, cb_design_apply_checked, NULL);
      ry += 26;
    }
    if (t == SF_UI_PANEL) {
      sf_ui_add_checkbox(&sf_ctx, "collapsed", (sf_ivec2_t){rx0 + 10, ry}, (sf_ivec2_t){rx1 - 10, ry + 20}, g_design_panel_collapsed, cb_design_apply_panel, NULL);
      ry += 26;
    }
    if (t == SF_UI_DROPDOWN) {
      static char s_dd[80];
      int n = g_design_sel->dropdown.n_items;
      snprintf(s_dd, sizeof(s_dd), "items: %d", n);
      sf_ui_add_label(&sf_ctx, s_dd, (sf_ivec2_t){rx0 + 10, ry}, SF_CLR_WHITE); ry += 14;
      for (int k = 0; k < n && k < 6; k++) {
        sf_ui_add_label(&sf_ctx, g_design_sel->dropdown.items[k] ? g_design_sel->dropdown.items[k] : "?", (sf_ivec2_t){rx0 + 16, ry}, 0xFFAAAAAA);
        ry += 14;
      }
      sf_ui_add_text_input(&sf_ctx, (sf_ivec2_t){rx0 + 10, ry}, (sf_ivec2_t){rx1 - 70, ry + 20}, g_design_dd_item_buf, sizeof(g_design_dd_item_buf), NULL, NULL);
      sf_ui_add_button(&sf_ctx, "+", (sf_ivec2_t){rx1 - 66, ry}, (sf_ivec2_t){rx1 - 38, ry + 20}, cb_design_dd_add, NULL);
      sf_ui_add_button(&sf_ctx, "-", (sf_ivec2_t){rx1 - 34, ry}, (sf_ivec2_t){rx1 - 10, ry + 20}, cb_design_dd_pop, NULL);
      ry += 26;
      sf_ui_add_label(&sf_ctx, "initial sel", (sf_ivec2_t){rx0 + 10, ry}, SF_CLR_WHITE); ry += 14;
      static float s_dd_sel;
      s_dd_sel = (float)g_design_dd_selected;
      sf_ui_add_drag_float(&sf_ctx, (sf_ivec2_t){rx0 + 10, ry}, (sf_ivec2_t){rx1 - 10, ry + 20}, &s_dd_sel, 1.0f, cb_design_apply_dd_sel, NULL);
      g_design_dd_selected = (int)s_dd_sel;
      ry += 26;
    }
  }
}

/* --- INPUT --- */

static sf_ui_lmn_t* _design_hit(int mx, int my) {
  if (!g_designer_ui) return NULL;
  for (int i = g_designer_ui->count - 1; i >= 0; i--) {
    sf_ui_lmn_t *el = &g_designer_ui->elements[i];
    if (!el->is_visible) continue;
    if (mx >= el->v0.x && mx <= el->v1.x && my >= el->v0.y && my <= el->v1.y) return el;
  }
  return NULL;
}

static bool _in_resize_handle(sf_ui_lmn_t *el, int mx, int my) {
  if (!el) return false;
  int hx = el->v1.x, hy = el->v1.y;
  return (mx >= hx - DESIGN_HANDLE && mx <= hx + 2 && my >= hy - DESIGN_HANDLE && my <= hy + 2);
}

static void design_on_mouse_down(int mx, int my) {
  if (sf_key_down(&sf_ctx, SF_KEY_LCTRL)) {
    g_design_panning = true;
    g_design_pan_last_mx = mx; g_design_pan_last_my = my;
    return;
  }
  if (g_design_sel && _in_resize_handle(g_design_sel, mx, my)) {
    g_design_resizing = true;
    g_ui_dirty = true;
    return;
  }
  sf_ui_lmn_t *hit = _design_hit(mx, my);
  g_design_sel = hit;
  if (hit) {
    g_design_dragging = true;
    g_design_drag_off = (sf_ivec2_t){mx - hit->v0.x, my - hit->v0.y};
  }
  g_ui_dirty = true;
}

static void design_on_wheel(int dy) {
  float factor = (dy > 0) ? 1.1f : (1.0f / 1.1f);
  _design_apply_zoom(g_design_zoom * factor);
  g_ui_dirty = true;
}

static void design_on_mouse_move(int mx, int my) {
  if (g_design_panning) {
    int dx = mx - g_design_pan_last_mx, dy = my - g_design_pan_last_my;
    g_design_pan_x += dx; g_design_pan_y += dy;
    _design_shift_elements(dx, dy);
    g_design_pan_last_mx = mx; g_design_pan_last_my = my;
    return;
  }
  if (g_design_resizing && g_design_sel) {
    int nx = mx, ny = my;
    if (nx < g_design_sel->v0.x + 8) nx = g_design_sel->v0.x + 8;
    if (ny < g_design_sel->v0.y + 8) ny = g_design_sel->v0.y + 8;
    g_design_sel->v1 = (sf_ivec2_t){nx, ny};
    sf_ivec2_t org = canvas_origin();
    float z = g_design_zoom;
    g_design_v1x = (float)((g_design_sel->v1.x - org.x) / z);
    g_design_v1y = (float)((g_design_sel->v1.y - org.y) / z);
    return;
  }
  if (!g_design_dragging || !g_design_sel) return;
  int w = g_design_sel->v1.x - g_design_sel->v0.x;
  int h = g_design_sel->v1.y - g_design_sel->v0.y;
  g_design_sel->v0 = (sf_ivec2_t){mx - g_design_drag_off.x, my - g_design_drag_off.y};
  g_design_sel->v1 = (sf_ivec2_t){g_design_sel->v0.x + w, g_design_sel->v0.y + h};
  sf_ivec2_t org = canvas_origin();
  float z = g_design_zoom;
  g_design_v0x = (float)((g_design_sel->v0.x - org.x) / z); g_design_v0y = (float)((g_design_sel->v0.y - org.y) / z);
  g_design_v1x = (float)((g_design_sel->v1.x - org.x) / z); g_design_v1y = (float)((g_design_sel->v1.y - org.y) / z);
}

static void design_on_mouse_up(void) { g_design_dragging = false; g_design_resizing = false; g_design_panning = false; }

static bool mouse_over_ui(int mx, int my) {
  if (!sf_ctx.ui) return false;
  for (int i = 0; i < sf_ctx.ui->count; i++) {
    sf_ui_lmn_t *el = &sf_ctx.ui->elements[i];
    if (!el->is_visible) continue;
    sf_ui_lmn_t *p = el->parent_panel;
    bool hidden = false;
    while (p) {
      if (!p->is_visible || p->panel.collapsed) { hidden = true; break; }
      p = p->parent_panel;
    }
    if (hidden) continue;
    if (el->type == SF_UI_LABEL) continue;
    if (mx >= el->v0.x && mx <= el->v1.x && my >= el->v0.y && my <= el->v1.y) return true;
    if (el->type == SF_UI_DROPDOWN && el->dropdown.is_open) {
      int h = el->v1.y - el->v0.y, w = el->v1.x - el->v0.x;
      if (mx >= el->v0.x && mx <= el->v0.x + w && my >= el->v1.y && my <= el->v1.y + h * el->dropdown.n_items) return true;
    }
  }
  return false;
}

static void update_camera(void) {
  sf_ctx_t *ctx = &sf_ctx;
  sf_cam_t *cam = &ctx->main_camera;
  float dt = ctx->delta_time;

  if (!ctx->ui || ctx->ui->focused == NULL) {
    float move = 8.0f * dt;
    if (sf_key_down(ctx, SF_KEY_LSHIFT)) move *= 3.0f;
    sf_fvec3_t fwd = sf_fvec3_sub(g_orbit_target, cam->frame->pos);
    fwd.y = 0.0f;
    fwd = sf_fvec3_norm(fwd);
    sf_fvec3_t up  = {0.0f, 1.0f, 0.0f};
    sf_fvec3_t right = sf_fvec3_norm(sf_fvec3_cross(fwd, up));
    if (sf_key_down(ctx, SF_KEY_W)) g_orbit_target = sf_fvec3_add(g_orbit_target, (sf_fvec3_t){fwd.x*move, 0.0f, fwd.z*move});
    if (sf_key_down(ctx, SF_KEY_S)) g_orbit_target = sf_fvec3_add(g_orbit_target, (sf_fvec3_t){-fwd.x*move, 0.0f, -fwd.z*move});
    if (sf_key_down(ctx, SF_KEY_A)) g_orbit_target = sf_fvec3_add(g_orbit_target, (sf_fvec3_t){-right.x*move, 0.0f, -right.z*move});
    if (sf_key_down(ctx, SF_KEY_D)) g_orbit_target = sf_fvec3_add(g_orbit_target, (sf_fvec3_t){ right.x*move, 0.0f,  right.z*move});
    if (sf_key_down(ctx, SF_KEY_Q)) g_orbit_target.y -= move;
    if (sf_key_down(ctx, SF_KEY_E)) g_orbit_target.y += move;
  }

  if (ctx->input.mouse_btns[SF_MOUSE_RIGHT]) {
    g_orbit_yaw   += ctx->input.mouse_dx * 0.005f;
    g_orbit_pitch += ctx->input.mouse_dy * 0.005f;
    if (g_orbit_pitch >  1.5f) g_orbit_pitch =  1.5f;
    if (g_orbit_pitch < -1.5f) g_orbit_pitch = -1.5f;
  }
  if (ctx->input.wheel_dy != 0) {
    g_orbit_dist *= (ctx->input.wheel_dy > 0) ? 0.9f : 1.1f;
    if (g_orbit_dist < 1.0f)   g_orbit_dist = 1.0f;
    if (g_orbit_dist > 200.0f) g_orbit_dist = 200.0f;
  }
  orbit_apply(cam);

  if (sf_key_pressed(ctx, SF_KEY_DEL) && g_sel) cb_delete(ctx, NULL);

  if (sf_key_pressed(ctx, SF_KEY_TAB) && (!ctx->ui || ctx->ui->focused == NULL)) {
    int dir = sf_key_down(ctx, SF_KEY_LSHIFT) ? -1 : 1;
    int total = sf_ctx.enti_count + sf_ctx.light_count + sf_ctx.cam_count + sf_ctx.emitr_count;
    if (total > 0) {
      int cur = -1, k = 0;
      for (int i = 0; i < sf_ctx.enti_count;  i++, k++) if (g_sel_kind == SEL_ENTI  && g_sel       == &sf_ctx.entities[i]) cur = k;
      for (int i = 0; i < sf_ctx.light_count; i++, k++) if (g_sel_kind == SEL_LIGHT && g_sel_light == &sf_ctx.lights[i])   cur = k;
      for (int i = 0; i < sf_ctx.cam_count;   i++, k++) if (g_sel_kind == SEL_CAM   && g_sel_cam   == &sf_ctx.cameras[i])  cur = k;
      for (int i = 0; i < sf_ctx.emitr_count; i++, k++) if (g_sel_kind == SEL_EMITR && g_sel_emitr == &sf_ctx.emitrs[i])   cur = k;
      int nxt = (cur < 0) ? (dir > 0 ? 0 : total - 1) : ((cur + dir + total) % total);
      sel_clear();
      if (nxt < sf_ctx.enti_count) {
        g_sel_kind = SEL_ENTI; g_sel = &sf_ctx.entities[nxt];
      } else if ((nxt -= sf_ctx.enti_count) < sf_ctx.light_count) {
        g_sel_kind = SEL_LIGHT; g_sel_light = &sf_ctx.lights[nxt];
      } else if ((nxt -= sf_ctx.light_count) < sf_ctx.cam_count) {
        g_sel_kind = SEL_CAM; g_sel_cam = &sf_ctx.cameras[nxt];
      } else {
        nxt -= sf_ctx.cam_count;
        g_sel_kind = SEL_EMITR; g_sel_emitr = &sf_ctx.emitrs[nxt];
      }
      g_ui_dirty = true;
    }
  }
}

static void try_pick(int mx, int my) {
  sf_ray_t ray = sf_ray_from_screen(&sf_ctx, &sf_ctx.main_camera, mx, my);
  float best_t = 1e30f;
  sel_kind_t best_kind = SEL_NONE;
  void *best_ptr = NULL;

  float t = 0.0f;
  sf_enti_t *he = sf_raycast_entities(&sf_ctx, ray, &t);
  if (he) { best_t = t; best_kind = SEL_ENTI; best_ptr = he; }

  const float light_r = 0.3f;
  for (int i = 0; i < sf_ctx.light_count; i++) {
    sf_light_t *l = &sf_ctx.lights[i];
    if (!l->frame) continue;
    sf_fvec3_t p = { l->frame->global_M.m[3][0], l->frame->global_M.m[3][1], l->frame->global_M.m[3][2] };
    sf_fvec3_t mn = { p.x - light_r, p.y - light_r, p.z - light_r };
    sf_fvec3_t mx3 = { p.x + light_r, p.y + light_r, p.z + light_r };
    float tt;
    if (sf_ray_aabb(ray, mn, mx3, &tt) && tt < best_t) { best_t = tt; best_kind = SEL_LIGHT; best_ptr = l; }
  }

  const float em_r = 0.4f;
  for (int i = 0; i < sf_ctx.emitr_count; i++) {
    sf_emitr_t *em = &sf_ctx.emitrs[i];
    if (!em->frame) continue;
    sf_fvec3_t p = { em->frame->global_M.m[3][0], em->frame->global_M.m[3][1], em->frame->global_M.m[3][2] };
    sf_fvec3_t mn = { p.x - em_r, p.y - em_r, p.z - em_r };
    sf_fvec3_t mx3 = { p.x + em_r, p.y + em_r, p.z + em_r };
    float tt;
    if (sf_ray_aabb(ray, mn, mx3, &tt) && tt < best_t) { best_t = tt; best_kind = SEL_EMITR; best_ptr = em; }
  }

  const float cam_r = 0.5f;
  for (int i = 0; i < sf_ctx.cam_count; i++) {
    sf_cam_t *c = &sf_ctx.cameras[i];
    if (!c->frame || c == &sf_ctx.main_camera) continue;
    sf_fvec3_t p = { c->frame->global_M.m[3][0], c->frame->global_M.m[3][1], c->frame->global_M.m[3][2] };
    sf_fvec3_t mn = { p.x - cam_r, p.y - cam_r, p.z - cam_r };
    sf_fvec3_t mx3 = { p.x + cam_r, p.y + cam_r, p.z + cam_r };
    float tt;
    if (sf_ray_aabb(ray, mn, mx3, &tt) && tt < best_t) { best_t = tt; best_kind = SEL_CAM; best_ptr = c; }
  }

  sel_clear();
  g_sel_kind = best_kind;
  if      (best_kind == SEL_ENTI)  g_sel       = (sf_enti_t*) best_ptr;
  else if (best_kind == SEL_LIGHT) g_sel_light = (sf_light_t*)best_ptr;
  else if (best_kind == SEL_CAM)   g_sel_cam   = (sf_cam_t*)  best_ptr;
  else if (best_kind == SEL_EMITR) g_sel_emitr = (sf_emitr_t*)best_ptr;
  g_ui_dirty = true;
}

/* --- MAIN --- */

int main(int argc, char *argv[]) {
  (void)argc; (void)argv;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window   *window   = SDL_CreateWindow("sf_studio", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, g_w, g_h, SDL_WINDOW_RESIZABLE);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_RenderSetLogicalSize(renderer, g_w, g_h);
  SDL_Texture  *texture  = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, g_w, g_h);
  SDL_StartTextInput();

  sf_init(&sf_ctx, g_w, g_h);
  scan_models();
  scan_textures();
  for (int i = 0; i < 9; i++) {
    char nm[32];
    snprintf(nm, sizeof(nm), "icon_%d", i);
    g_spawn_icon_tex[i] = sf_load_texture_bmp(&sf_ctx, k_spawn_icon_bmp[i], nm);
  }
  for (int i = 0; i < ICN_COUNT; i++) {
    char nm[32];
    snprintf(nm, sizeof(nm), "uicn_%d", i);
    g_icon_tex[i] = sf_load_texture_bmp(&sf_ctx, k_icon_bmp[i], nm);
  }
  orbit_apply(&sf_ctx.main_camera);

  sf_light_t *sun = sf_add_light(&sf_ctx, "sun", SF_LIGHT_POINT, (sf_fvec3_t){1.0f, 1.0f, 1.0f}, 4.0f);
  if (sun && sun->frame) { sun->frame->pos = (sf_fvec3_t){6.0f, 8.0f, 6.0f}; sun->frame->is_dirty = true; }

  {
    sf_ui_t *saved = sf_ctx.ui;
    g_designer_ui = sf_ui_create(&sf_ctx);
    sf_ctx.ui = saved;
  }

  SDL_Event event;
  while (sf_running(&sf_ctx)) {
    sf_input_cycle_state(&sf_ctx);

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) sf_stop(&sf_ctx);
      if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (!mouse_over_ui(event.button.x, event.button.y) && (!sf_ctx.ui || sf_ctx.ui->focused == NULL)) {
          if (g_tab == TAB_SFUI) design_on_mouse_down(event.button.x, event.button.y);
          else {
            if (!gizmo_begin_drag(event.button.x, event.button.y))
              try_pick(event.button.x, event.button.y);
          }
        }
      }
      if (event.type == SDL_MOUSEMOTION) {
        g_mouse_x = event.motion.x; g_mouse_y = event.motion.y;
        if (g_tab == TAB_SFUI) design_on_mouse_move(event.motion.x, event.motion.y);
        else if (g_gz_drag != GZ_AX_NONE) gizmo_drag_update(event.motion.x, event.motion.y);
      }
      if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (g_tab == TAB_SFUI) design_on_mouse_up();
        else gizmo_end_drag();
      }
      if (event.type == SDL_MOUSEWHEEL && g_tab == TAB_SFUI) {
        design_on_wheel(event.wheel.y);
      }
      sf_sdl_process_event(&sf_ctx, &event);
    }

    if (sf_key_pressed(&sf_ctx, SF_KEY_ESC) && (!sf_ctx.ui || sf_ctx.ui->focused == NULL)) sf_stop(&sf_ctx);

    sf_time_update(&sf_ctx);

    if (g_ui_dirty) { rebuild_ui(); g_ui_dirty = false; }

    sf_ui_update(&sf_ctx, sf_ctx.ui);
    if (g_tab == TAB_SFF) update_camera();

    if (g_tab == TAB_SFF) {
      sf_render_ctx(&sf_ctx);
      if (g_dbg_frames) sf_draw_debug_frames(&sf_ctx, &sf_ctx.main_camera, 1.0f);
      if (g_dbg_lights) sf_draw_debug_lights(&sf_ctx, &sf_ctx.main_camera, 0.3f);
      if (g_dbg_cams)   sf_draw_debug_cams  (&sf_ctx, &sf_ctx.main_camera, 2.0f);
      update_gizmo_geometry();
      if (g_gz_drag == GZ_AX_NONE) g_gz_hover = gizmo_hit(g_mouse_x, g_mouse_y);
      draw_gizmo();
    } else {
      sf_fill(&sf_ctx, &sf_ctx.main_camera, (sf_pkd_clr_t)0xFF1A1A20);
    }
    sf_draw_debug_perf(&sf_ctx, &sf_ctx.main_camera);

    if (g_tab == TAB_SFUI && g_designer_ui) {
      sf_ivec2_t org = canvas_origin();
      sf_ivec2_t c0 = org;
      sf_ivec2_t c1 = { org.x + (int)(g_design_canvas_w * g_design_zoom), org.y + (int)(g_design_canvas_h * g_design_zoom) };
      sf_rect(&sf_ctx, &sf_ctx.main_camera, (sf_pkd_clr_t)0xFF101018, c0, c1);
      sf_pkd_clr_t cedge = 0xFF4488FF;
      sf_line(&sf_ctx, &sf_ctx.main_camera, cedge, c0, (sf_ivec2_t){c1.x, c0.y});
      sf_line(&sf_ctx, &sf_ctx.main_camera, cedge, (sf_ivec2_t){c1.x, c0.y}, c1);
      sf_line(&sf_ctx, &sf_ctx.main_camera, cedge, c1, (sf_ivec2_t){c0.x, c1.y});
      sf_line(&sf_ctx, &sf_ctx.main_camera, cedge, (sf_ivec2_t){c0.x, c1.y}, c0);
      static char s_res[40];
      snprintf(s_res, sizeof(s_res), "%dx%d", (int)g_design_canvas_w, (int)g_design_canvas_h);
      sf_put_text(&sf_ctx, &sf_ctx.main_camera, s_res, (sf_ivec2_t){c0.x + 4, c0.y - 14}, cedge, 1);

      sf_ui_render(&sf_ctx, &sf_ctx.main_camera, g_designer_ui);
      if (g_design_sel) {
        sf_ivec2_t a = g_design_sel->v0, b = g_design_sel->v1;
        sf_pkd_clr_t hl = 0xFFFFFF00;
        sf_line(&sf_ctx, &sf_ctx.main_camera, hl, a, (sf_ivec2_t){b.x, a.y});
        sf_line(&sf_ctx, &sf_ctx.main_camera, hl, (sf_ivec2_t){b.x, a.y}, b);
        sf_line(&sf_ctx, &sf_ctx.main_camera, hl, b, (sf_ivec2_t){a.x, b.y});
        sf_line(&sf_ctx, &sf_ctx.main_camera, hl, (sf_ivec2_t){a.x, b.y}, a);
        sf_rect(&sf_ctx, &sf_ctx.main_camera, hl, (sf_ivec2_t){b.x - DESIGN_HANDLE, b.y - DESIGN_HANDLE}, b);
      }
    }
    sf_ui_render(&sf_ctx, &sf_ctx.main_camera, sf_ctx.ui);
    draw_spawn_icons();
    draw_ui_icons();
    draw_cam_pip_overlay();

    SDL_UpdateTexture(texture, NULL, sf_ctx.main_camera.buffer, sf_ctx.main_camera.w * sizeof(sf_pkd_clr_t));
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  sf_destroy(&sf_ctx);
  SDL_StopTextInput();
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
