/* sf_studio - interactive editor for saffron files */
#include <SDL_video.h>
#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>
#include <dirent.h>

typedef enum { SEL_NONE = 0, SEL_ENTI, SEL_LIGHT, SEL_CAM, SEL_EMITR } sel_kind_t;
typedef enum { TAB_SFF = 0, TAB_SFUI, TAB_SFGEN } tab_t;

static sf_ctx_t     sf_ctx;
static sel_kind_t   g_sel_kind      = SEL_NONE;
static sf_enti_t   *g_sel           = NULL;
static sf_light_t  *g_sel_light     = NULL;
static sf_cam_t    *g_sel_cam       = NULL;
static sf_emitr_t  *g_sel_emitr     = NULL;
static sf_frame_t  *g_sel_frame     = NULL; /* selected orphan frame (SEL_NONE) */
static int          g_new_prim      = 0;
static sf_orbit_cam_t g_orbit       = {{0.0f, 0.0f, 0.0f}, 0.8f, 0.5f, 12.0f};
static bool         g_ui_dirty      = true;
static bool         g_dbg_axes      = true;
static bool         g_dbg_frames    = true;
static bool         g_dbg_lights    = true;
static bool         g_dbg_cams      = true;
static int          g_w             = 683*2;
static int          g_h             = 384*2;
static char         g_save_path[SF_MAX_TEXT_INPUT_LEN] = "studio_out.sff";
static char         g_sfui_path[SF_MAX_TEXT_INPUT_LEN] = "studio_out.sfui";
static tab_t        g_tab = TAB_SFF;

/* ============================================================
   SFGEN TAB — globals
   ============================================================ */
typedef enum { SFGEN_TREE = 0, SFGEN_ROCK, SFGEN_BLDG } sfgen_type_t;
static sfgen_type_t  g_sfgen_type    = SFGEN_TREE;
static sf_ctx_t     g_sfgen_ctx;
static bool         g_sfgen_ready   = false;
static sf_orbit_cam_t g_sfgen_orbit = {{0.0f, 0.0f, 0.0f}, 0.5f, 0.35f, 12.0f};
static bool         g_sfgen_drag    = false;
static int          g_sfgen_lmx, g_sfgen_lmy;

/* Tree parameters */
static float ct_seed=42.f, ct_depth=4.f, ct_branch=3.f, ct_angle=32.f;
static float ct_len=.70f,  ct_taper=.65f, ct_grav=.15f, ct_wiggle=.12f;
static float ct_twist=137.5f, ct_tr=.22f, ct_tl=2.5f;
static float ct_ls=1.f, ct_ld=4.f, ct_lo=.85f;
static bool  ct_rand3d = true;

/* Rock parameters */
static float cr_seed=1.f, cr_subdiv=3.f, cr_rough=.30f, cr_freq=2.f;
static float cr_octaves=4.f, cr_persist=.50f, cr_flat=.20f;
static float cr_elongx=1.f, cr_elongz=1.f, cr_pointy=0.f, cr_bump=.5f;

/* Building parameters */
static float bk_seed=1.f,    bk_floors=4.f,    bk_floor_h=2.5f,   bk_width=4.f;
static float bk_depth=3.f,   bk_taper=0.1f,    bk_jitter=0.f,     bk_ledge=0.1f;
static float bk_win_cols=3.f, bk_win_size=0.5f, bk_win_inset=0.3f, bk_roof_type=1.f;
static float bk_roof_h=0.5f, bk_roof_scale=1.0f, bk_asym=0.f;

/* Row icons */
#define SFGEN_ICON_SZ 13
static sf_tex_t *g_sfgen_tree_icons[14];
static sf_tex_t *g_sfgen_rock_icons[11];
static sf_tex_t *g_sfgen_bldg_icons[14];

/* Texture / sprite lists */
#define SFGEN_MAX_TEX 64
#define SFGEN_MAX_SPR 32
static char        g_ct_tname[SFGEN_MAX_TEX][64];
static const char *g_ct_titem[SFGEN_MAX_TEX];
static int         g_ct_tnc = 0, g_ct_tsel = 0;
static char        g_cr_tname[SFGEN_MAX_TEX][64];
static const char *g_cr_titem[SFGEN_MAX_TEX];
static int         g_cr_tnc = 0, g_cr_tsel = 0;
static char        g_ct_sname[SFGEN_MAX_SPR][64];
static const char *g_ct_sitem[SFGEN_MAX_SPR];
static sf_sprite_2_t*g_ct_sprites[SFGEN_MAX_SPR];
static int         g_ct_snc = 0, g_ct_ssel = 0;
static sf_sprite_2_t*g_sfgen_leaf = NULL;
static sf_ui_lmn_t *g_sfgen_panel = NULL; /* SFGEN left panel — for icon visibility check */

/* Generated objects */
#define SFGEN_MAX_V     32000
#define SFGEN_MAX_UV    32000
#define SFGEN_MAX_F     32000
#define SFGEN_ISO_V     3000
#define SFGEN_ISO_F     6000
#define SFGEN_MID_SZ    65536
static sf_obj_t  *g_ct_obj  = NULL;
static sf_enti_t *g_ct_enti = NULL;
static sf_obj_t  *g_cr_obj  = NULL;
static sf_enti_t *g_cr_enti = NULL;
static sf_obj_t  *g_ck_obj       = NULL;
static sf_enti_t *g_ck_enti      = NULL;
static sf_obj_t  *g_ck_obj_win   = NULL;   /* window / door reveals */
static sf_enti_t *g_ck_enti_win  = NULL;
static sf_obj_t  *g_ck_obj_ledge  = NULL;  /* floor cornices */
static sf_enti_t *g_ck_enti_ledge = NULL;
static sf_obj_t  *g_ck_obj_roof  = NULL;
static sf_enti_t *g_ck_enti_roof = NULL;
static int        g_ck_tsel  = 0;   /* building wall texture index */
static int        g_ck_rtsel = 0;   /* building roof texture index */

/* SFF save paths */
static char g_sfgen_tree_sff[SF_MAX_TEXT_INPUT_LEN] = "tree_out.sff";
static char g_sfgen_rock_sff[SF_MAX_TEXT_INPUT_LEN] = "rock_out.sff";
static char g_sfgen_bldg_sff[SF_MAX_TEXT_INPUT_LEN] = "bldg_out.sff";

/* Icosphere state (rock) */
static sf_fvec3_t g_sfgen_iso_v[SFGEN_ISO_V];
static int        g_sfgen_iso_vi;
static int        g_sfgen_iso_f[SFGEN_ISO_F][3];
static int        g_sfgen_iso_fi;
static int        g_sfgen_mid_a[SFGEN_MID_SZ];
static int        g_sfgen_mid_b[SFGEN_MID_SZ];
static int        g_sfgen_mid_val[SFGEN_MID_SZ];

/* Tree RNG */
static uint32_t g_sfgen_rng;

/* Translate gizmo state */
static sf_gizmo_t g_gizmo = {0};

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
static char         g_model_files[STUDIO_MAX_MODELS][256];
static const char  *g_model_items[STUDIO_MAX_MODELS];
static int          g_model_count   = 0;

#define STUDIO_MAX_TEXS 128
static int          g_tex_sel       = 0;
static char         g_tex_files[STUDIO_MAX_TEXS][256];
static const char  *g_tex_items[STUDIO_MAX_TEXS];
static int          g_tex_count     = 0;

#define STUDIO_MAX_SFFS 64
static int          g_sff_sel       = 0;
static char         g_sff_files[STUDIO_MAX_SFFS][256];
static const char  *g_sff_items[STUDIO_MAX_SFFS];
static int          g_sff_count     = 0;

static const char *k_emitr_items[3] = { "dir", "omni", "vol" };

static const char *k_spawn_labels[9]    = { "Pln", "Box", "Sph", "Cyl", "Ter", "Lt", "Cam", "Em", "Mdl" };
static const char *k_spawn_icon_bmp[9]  = { "plane.bmp", "box.bmp", "sphere.bmp", "cylinder.bmp", "terrain.bmp", "light.bmp", "camera.bmp", "emitter.bmp", "model.bmp" };
static sf_tex_t   *g_spawn_icon_tex[9]  = { 0 };
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

static sf_ui_lmn_t *icon_btn(icon_id_t id, const char *label, sf_ivec2_t v0, sf_ivec2_t v1, sf_ui_cb cb, void *ud) {
  sf_ui_lmn_t *e = sf_ui_add_button(&sf_ctx, label, v0, v1, cb, ud);
  if (g_icon_tex[id]) {
    int bw = v1.x - v0.x, bh = v1.y - v0.y;
    int sz = bh - 2; if (sz > 18) sz = 18; if (sz < 10) sz = 10;
    int ix, iy;
    int txt_w = label ? (int)strlen(label) * 8 : 0;
    if (txt_w == 0) {
      ix = v0.x + (bw - sz) / 2;
    } else {
      int txt_x = v0.x + (bw - txt_w) / 2;
      ix = txt_x + 3;
    }
    iy = v0.y + (bh - sz) / 2;
    sf_ui_add_image(&sf_ctx, g_icon_tex[id], (sf_ivec2_t){ix, iy}, (sf_ivec2_t){ix+sz, iy+sz}, true);
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

/* ---- Asset browser (picker) ---- */
typedef enum { PICK_NONE=0, PICK_TEX, PICK_MODEL, PICK_SPRITE } picker_kind_t;
typedef enum { PICK_APPLY_ENTI_TEX=0, PICK_APPLY_EMITR_SPRITE, PICK_APPLY_MODEL,
               PICK_APPLY_SFGEN_BARK, PICK_APPLY_SFGEN_LEAF,
               PICK_APPLY_SFGEN_STONE, PICK_APPLY_SFGEN_BWALL, PICK_APPLY_SFGEN_BROOF,
               PICK_APPLY_SFGEN_BWIN, PICK_APPLY_SFGEN_BLEDGE } picker_apply_t;
static picker_kind_t  g_picker_open      = PICK_NONE;
static sf_ui_lmn_t   *g_picker_panel    = NULL;
static picker_apply_t g_picker_apply     = PICK_APPLY_ENTI_TEX;
static int            g_picker_scroll    = 0;
static int            g_picker_rows_vis  = 1;  /* updated each frame by build_browser_panel */
static sf_tex_t      *g_picker_thumbs[STUDIO_MAX_TEXS];
static int            g_picker_thumbs_n  = 0;
#define BROWSER_X0       228
#define BROWSER_X1       450
#define BROWSER_COLS     3
#define BROWSER_INNER_W  (BROWSER_X1 - BROWSER_X0 - 12)
#define BROWSER_CELL_W   ((BROWSER_INNER_W - (BROWSER_COLS-1)*2) / BROWSER_COLS)
#define BROWSER_THUMB_SZ 54
#define BROWSER_CELL_H   70

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
    case SEL_NONE:  return g_sel_frame;
    default:        return NULL;
  }
}

static const char* sel_name(void) {
  switch (g_sel_kind) {
    case SEL_ENTI:  return g_sel       && g_sel->name        ? g_sel->name        : "?";
    case SEL_LIGHT: return g_sel_light && g_sel_light->name  ? g_sel_light->name  : "?";
    case SEL_CAM:   return g_sel_cam   && g_sel_cam->name    ? g_sel_cam->name    : "?";
    case SEL_EMITR: return g_sel_emitr && g_sel_emitr->name  ? g_sel_emitr->name  : "?";
    case SEL_NONE:  return g_sel_frame && g_sel_frame->name  ? g_sel_frame->name  : "?";
    default:        return "?";
  }
}

static void sel_clear(void) {
  g_sel_kind = SEL_NONE;
  g_sel = NULL; g_sel_light = NULL; g_sel_cam = NULL; g_sel_emitr = NULL; g_sel_frame = NULL;
}

static void scan_models(void) {
  static char paths[STUDIO_MAX_MODELS][512];
  g_model_count = 0;
  const char *obj_dirs[] = { SF_ASSET_PATH "/sf_objs", SF_SRC_ASSET_PATH "/sf_objs" };
  g_model_count = sf_scan_assets(obj_dirs, 2, ".obj", g_model_files, paths, STUDIO_MAX_MODELS);
  const char *sff_dirs[] = { SF_ASSET_PATH "/sf_sff", SF_SRC_ASSET_PATH "/sf_sff" };
  int n = sf_scan_assets(sff_dirs, 2, ".sff", g_model_files + g_model_count, paths + g_model_count, STUDIO_MAX_MODELS - g_model_count);
  g_model_count += n;
  for (int i = 0; i < g_model_count; i++) g_model_items[i] = g_model_files[i];
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

/* Resolve path via realpath(); return false if path was already visited */
static bool _scan_dir_once(const char *path, char visited[][512], int *nv, int cap) {
  char *rp = realpath(path, NULL); /* dynamic alloc avoids PATH_MAX vs fixed-size mismatch */
  if (!rp) return false;
  for (int i = 0; i < *nv; i++) if (strcmp(visited[i], rp)==0) { free(rp); return false; }
  if (*nv < cap) snprintf(visited[(*nv)++], 512, "%s", rp);
  free(rp);
  return true;
}

static void scan_textures(void) {
  g_tex_count = 0;
  char visited[4][512]; int nv = 0;
  const char *dirs[] = { SF_ASSET_PATH "/sf_textures", SF_SRC_ASSET_PATH "/sf_textures", NULL };
  for (int i = 0; dirs[i]; i++)
    if (_scan_dir_once(dirs[i], visited, &nv, 4)) scan_textures_dir(dirs[i]);
}

static void scan_sffs(void) {
  static char paths[STUDIO_MAX_SFFS][512];
  const char *dirs[] = { SF_ASSET_PATH "/sf_sff", SF_SRC_ASSET_PATH "/sf_sff" };
  g_sff_count = sf_scan_assets(dirs, 2, ".sff", g_sff_files, paths, STUDIO_MAX_SFFS);
  for (int i = 0; i < g_sff_count; i++) g_sff_items[i] = g_sff_files[i];
}

/* _studio_copy_file replaced by sf_copy_file in saffron.h */

static void orbit_apply(sf_cam_t *cam) {
  sf_orbit_cam_apply(&sf_ctx, cam, &g_orbit);
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
      const char *dot = strrchr(fname, '.');
      if (dot && strcmp(dot, ".sff") == 0) {
        const char *sff_dirs[3] = { SF_ASSET_PATH "/sf_sff", SF_SRC_ASSET_PATH "/sf_sff", "./sf_assets/sf_sff" };
        for (int p = 0; p < 3; p++) {
          char r_path[512];
          snprintf(r_path, sizeof(r_path), "%s/%s", sff_dirs[p], fname);
          FILE *f = fopen(r_path, "r");
          if (!f) continue;
          fclose(f);
          sel_clear();
          sf_load_sff(&sf_ctx, r_path, "Loaded World");
          g_ui_dirty = true;
          return NULL;
        }
        return NULL;
      }
      char r_path[512];
      if (!_sf_resolve_asset(fname, r_path, sizeof(r_path))) return NULL;
      return sf_load_obj(&sf_ctx, r_path, name);
    }
    default: return NULL;
  }
}

static void cb_regen_sel(sf_ctx_t *ctx, void *ud) {
  (void)ud;
  prim_meta_t *m = sel_meta();
  if (!m || m->kind == PM_NONE) return;
  /* Save selection — build_obj_from_meta clears it when loading an SFF scene */
  sf_enti_t  *saved_sel  = g_sel;
  sel_kind_t  saved_kind = g_sel_kind;
  int before = sf_ctx.obj_count;
  char name[32];
  snprintf(name, sizeof(name), "mesh_%d", before);
  sf_obj_t *o = build_obj_from_meta(name, m);
  if (o) {
    g_sel->obj = *o;
    sf_ctx.obj_count = before;
  } else if (m->kind == PM_MODEL && saved_sel && saved_kind == SEL_ENTI) {
    /* An SFF scene was loaded — delete the placeholder entity that was selected */
    int idx = (int)(saved_sel - sf_ctx.entities);
    if (idx >= 0 && idx < sf_ctx.enti_count) {
      sf_remove_frame(&sf_ctx, saved_sel->frame);
      for (int i = idx; i < sf_ctx.enti_count - 1; i++) sf_ctx.entities[i] = sf_ctx.entities[i + 1];
      for (int i = idx; i < sf_ctx.enti_count - 1 && i < SF_MAX_ENTITIES - 1; i++) g_enti_meta[i] = g_enti_meta[i + 1];
      if (sf_ctx.enti_count - 1 < SF_MAX_ENTITIES) memset(&g_enti_meta[sf_ctx.enti_count - 1], 0, sizeof(prim_meta_t));
      sf_ctx.enti_count--;
      g_ui_dirty = true;
    }
  }
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
      if (l && l->frame) { l->frame->pos = g_orbit.target; l->frame->is_dirty = true; }
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
    case 8: {
      /* if selected file is .sff, load the whole scene instead of a mesh */
      if (g_model_sel >= 0 && g_model_sel < g_model_count) {
        const char *fname = g_model_files[g_model_sel];
        const char *dot = strrchr(fname, '.');
        if (dot && strcmp(dot, ".sff") == 0) {
          const char *sff_dirs[3] = { SF_ASSET_PATH "/sf_sff", SF_SRC_ASSET_PATH "/sf_sff", "./sf_assets/sf_sff" };
          for (int p = 0; p < 3; p++) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", sff_dirs[p], fname);
            FILE *f = fopen(path, "r");
            if (!f) continue;
            fclose(f);
            sel_clear();
            int enti_before = sf_ctx.enti_count;
            int bb_before   = sf_ctx.sprite_3d_count;
            sf_load_sff(&sf_ctx, path, "Loaded World");
            /* Fix billboard frame refs: entity dedup renames duplicates (e.g. tree_0 -> tree_0_1),
               so newly added billboards may still point to the original entity's frame instead of
               the freshly created one. Redirect them to the newly added entity's frame. */
            if (sf_ctx.enti_count > enti_before) {
              sf_frame_t *new_frame = sf_ctx.entities[enti_before].frame;
              for (int i = bb_before; i < sf_ctx.sprite_3d_count; i++) {
                sf_sprite_3_t *b = &sf_ctx.sprite_3ds[i];
                if (b->frame && b->frame != new_frame)
                  b->frame = new_frame;
              }
            }
            g_ui_dirty = true;
            return;
          }
          return; /* couldn't find the file */
        }
      }
      pk = PM_MODEL;
      break;
    }
    default: return;
  }
  prim_meta_t tmp; meta_set_defaults(&tmp, pk);
  sf_obj_t *o = build_obj_from_meta(objname, &tmp);
  if (!o) return;
  sf_enti_t *e = sf_add_enti(&sf_ctx, o, entiname);
  if (!e) return;
  if (e->frame) { e->frame->pos = g_orbit.target; e->frame->is_dirty = true; }
  int idx = (int)(e - sf_ctx.entities);
  if (idx >= 0 && idx < SF_MAX_ENTITIES) g_enti_meta[idx] = tmp;
  sel_clear();
  g_sel_kind = SEL_ENTI;
  g_sel = e;
  g_ui_dirty = true;
}

static void spawn_emitter(void) {
  /* default sprite: try Stars.bmp, else fall back to first loaded texture */
  sf_sprite_2_t *spr = sf_get_sprite_(&sf_ctx, "spr_Stars", false);
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
    em->frame->pos = g_orbit.target;
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
    c->frame->pos = g_orbit.target;
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

/* Compact sprite_3ds, removing any whose frame is in the subtree rooted at df */
/* Collect frames in a subtree via sf_frame_walk */
typedef struct { sf_frame_t *frames[SF_MAX_FRAMES]; int count; } _subtree_set_t;
static bool _subtree_collect_cb(sf_frame_t *f, int depth, void *ud) {
  (void)depth;
  _subtree_set_t *s = (_subtree_set_t*)ud;
  if (s->count < SF_MAX_FRAMES) s->frames[s->count++] = f;
  return true;
}
static bool _frame_in_set(sf_frame_t *f, _subtree_set_t *s) {
  for (int i = 0; i < s->count; i++) if (s->frames[i] == f) return true;
  return false;
}
static void _purge_sprites_under(sf_frame_t *df) {
  _subtree_set_t set = {.count = 0};
  sf_frame_walk(&sf_ctx, df, _subtree_collect_cb, &set);
  int nc = 0;
  for (int bi = 0; bi < sf_ctx.sprite_3d_count; bi++)
    if (!sf_ctx.sprite_3ds[bi].frame || !_frame_in_set(sf_ctx.sprite_3ds[bi].frame, &set))
      sf_ctx.sprite_3ds[nc++] = sf_ctx.sprite_3ds[bi];
  sf_ctx.sprite_3d_count = nc;
}
/* Remove entities/lights/cameras/emitters whose frame is in the subtree of df */
static void _purge_entities_under(sf_frame_t *df) {
  _subtree_set_t set = {.count = 0};
  sf_frame_walk(&sf_ctx, df, _subtree_collect_cb, &set);
  for (int i = sf_ctx.enti_count - 1; i >= 0; i--) {
    if (sf_ctx.entities[i].frame && sf_ctx.entities[i].frame != df &&
        _frame_in_set(sf_ctx.entities[i].frame, &set)) {
      for (int j = i; j < sf_ctx.enti_count - 1; j++) sf_ctx.entities[j] = sf_ctx.entities[j + 1];
      if (sf_ctx.enti_count - 1 < SF_MAX_ENTITIES) memset(&g_enti_meta[sf_ctx.enti_count - 1], 0, sizeof(prim_meta_t));
      sf_ctx.enti_count--;
    }
  }
  for (int i = sf_ctx.light_count - 1; i >= 0; i--) {
    if (sf_ctx.lights[i].frame && _frame_in_set(sf_ctx.lights[i].frame, &set)) {
      for (int j = i; j < sf_ctx.light_count - 1; j++) sf_ctx.lights[j] = sf_ctx.lights[j + 1];
      sf_ctx.light_count--;
    }
  }
  for (int i = sf_ctx.cam_count - 1; i >= 0; i--) {
    if (sf_ctx.cameras[i].frame && _frame_in_set(sf_ctx.cameras[i].frame, &set)) {
      for (int j = i; j < sf_ctx.cam_count - 1; j++) sf_ctx.cameras[j] = sf_ctx.cameras[j + 1];
      sf_ctx.cam_count--;
    }
  }
  for (int i = sf_ctx.emitr_count - 1; i >= 0; i--) {
    if (sf_ctx.emitrs[i].frame && _frame_in_set(sf_ctx.emitrs[i].frame, &set)) {
      for (int j = i; j < sf_ctx.emitr_count - 1; j++) sf_ctx.emitrs[j] = sf_ctx.emitrs[j + 1];
      sf_ctx.emitr_count--;
    }
  }
}

static void cb_delete(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_sel_kind == SEL_ENTI && g_sel) {
    int idx = (int)(g_sel - sf_ctx.entities);
    if (idx < 0 || idx >= sf_ctx.enti_count) return;
    sf_frame_t *df = g_sel->frame;
    _purge_sprites_under(df);
    _purge_entities_under(df);
    /* After purge, g_sel may point past the end of the (shifted) array.
       Find the selected entity by its frame pointer, which is unambiguous. */
    idx = -1;
    for (int si = 0; si < sf_ctx.enti_count; si++) {
      if (sf_ctx.entities[si].frame == df) { idx = si; break; }
    }
    sf_remove_frame(&sf_ctx, df);
    if (idx < 0) { sel_clear(); g_ui_dirty = true; return; }
    for (int i = idx; i < sf_ctx.enti_count - 1; i++) sf_ctx.entities[i] = sf_ctx.entities[i + 1];
    for (int i = idx; i < sf_ctx.enti_count - 1 && i < SF_MAX_ENTITIES - 1; i++) g_enti_meta[i] = g_enti_meta[i + 1];
    if (sf_ctx.enti_count - 1 >= 0 && sf_ctx.enti_count - 1 < SF_MAX_ENTITIES) {
      memset(&g_enti_meta[sf_ctx.enti_count - 1], 0, sizeof(prim_meta_t));
    }
    sf_ctx.enti_count--;
  } else if (g_sel_kind == SEL_LIGHT && g_sel_light) {
    sf_remove_light(&sf_ctx, g_sel_light);
  } else if (g_sel_kind == SEL_CAM && g_sel_cam) {
    sf_remove_cam(&sf_ctx, g_sel_cam);
  } else if (g_sel_kind == SEL_EMITR && g_sel_emitr) {
    sf_remove_emitr(&sf_ctx, g_sel_emitr);
  } else if (g_sel_kind == SEL_NONE && g_sel_frame) {
    sf_remove_frame(&sf_ctx, g_sel_frame);
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

static void cb_install_sff(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  sf_save_sff(&sf_ctx, g_save_path);
  const char *sl = strrchr(g_save_path, '/');
  const char *fname = sl ? sl + 1 : g_save_path;
  char dst[512];
  snprintf(dst, sizeof(dst), SF_SRC_ASSET_PATH "/sf_sff/%s", fname);
  if (sf_copy_file(g_save_path, dst)) {
    SF_LOG(&sf_ctx, SF_LOG_INFO, "Installed %s\n", dst);
    scan_sffs();
    g_ui_dirty = true;
  } else {
    SF_LOG(&sf_ctx, SF_LOG_ERROR, "Install failed: %s\n", dst);
  }
}

static void cb_save_install_sff(sf_ctx_t *ctx, void *ud) {
  cb_save_sff(ctx, ud);
  cb_install_sff(ctx, ud);
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
  sf_sprite_2_t *spr = sf_get_sprite_(&sf_ctx, sname, false);
  if (!spr) spr = sf_load_sprite(&sf_ctx, sname, 1.0f, 0.3f, 1, tex->name);
  if (spr) g_sel_emitr->sprite = spr;
}

static void cb_clear_tex(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_sel_kind == SEL_ENTI && g_sel) g_sel->tex = NULL;
}

/* ============================================================
   Asset browser — callbacks and helpers
   ============================================================ */
static int picker_item_count(void) {
  if (g_picker_open == PICK_TEX || g_picker_open == PICK_SPRITE) return g_tex_count;
  if (g_picker_open == PICK_MODEL) return g_model_count;
  return 0;
}

static const char *picker_item_label(int i) {
  if (g_picker_open == PICK_MODEL)
    return (i >= 0 && i < g_model_count) ? g_model_items[i] : "";
  return (i >= 0 && i < g_tex_count)   ? g_tex_items[i]   : "";
}

/* Thumbnail cam + buffers for model preview rendering */
#define THUMB_CAM_SZ BROWSER_THUMB_SZ

/* Load a BMP from a full path into a malloc'd sf_tex_t (caller frees px + struct).
   Returns NULL on failure. */
static sf_tex_t *picker_load_bmp_private(const char *path) {
  FILE *file = fopen(path, "rb");
  if (!file) return NULL;
  uint8_t hdr[54];
  if (fread(hdr, 1, 54, file) != 54 || hdr[0]!='B' || hdr[1]!='M') { fclose(file); return NULL; }
  uint32_t off = hdr[10]|(hdr[11]<<8)|(hdr[12]<<16)|(hdr[13]<<24);
  int32_t w  = hdr[18]|(hdr[19]<<8)|(hdr[20]<<16)|(hdr[21]<<24);
  int32_t h  = hdr[22]|(hdr[23]<<8)|(hdr[24]<<16)|(hdr[25]<<24);
  int32_t ha = abs(h);
  if (w<=0||ha<=0) { fclose(file); return NULL; }
  sf_pkd_clr_t *px = (sf_pkd_clr_t*)malloc((size_t)w*(size_t)ha*sizeof(sf_pkd_clr_t));
  if (!px) { fclose(file); return NULL; }
  fseek(file, off, SEEK_SET);
  int pad = (4-(w*3)%4)%4;
  uint8_t bgr[3];
  for (int y=0;y<ha;y++) {
    int dy = (h>0)?(ha-1-y):y;
    for (int x=0;x<w;x++) {
      if (fread(bgr,1,3,file)!=3) bgr[0]=bgr[1]=bgr[2]=0;
      if (bgr[2]==255&&bgr[1]==0&&bgr[0]==255) { px[dy*w+x]=0; }
      else {
        uint8_t lr=(uint8_t)(powf(bgr[2]/255.f,2.2f)*255.f+.5f);
        uint8_t lg=(uint8_t)(powf(bgr[1]/255.f,2.2f)*255.f+.5f);
        uint8_t lb=(uint8_t)(powf(bgr[0]/255.f,2.2f)*255.f+.5f);
        px[dy*w+x]=(0xFFu<<24)|((uint32_t)lr<<16)|((uint32_t)lg<<8)|lb;
      }
    }
    fseek(file,pad,SEEK_CUR);
  }
  fclose(file);
  sf_tex_t *t=(sf_tex_t*)malloc(sizeof(sf_tex_t));
  if (!t){free(px);return NULL;}
  memset(t,0,sizeof(sf_tex_t));
  t->px=px; t->w=w; t->h=ha; t->w_mask=w-1; t->h_mask=ha-1;
  return t;
}

/* Render a loaded sf_obj_t into the thumb cam and store a malloc'd sf_tex_t at g_picker_thumbs[i].
   tex_path is the full path to a BMP to apply, or NULL for untextured. */
static void picker_render_obj_thumb(sf_obj_t *obj, const char *tex_path, int i) {
  /* Set up a temporary entity on the stack */
  sf_enti_t tmp_enti; memset(&tmp_enti, 0, sizeof(tmp_enti));
  sf_frame_t tmp_frame; memset(&tmp_frame, 0, sizeof(tmp_frame));
  tmp_frame.global_M = sf_make_idn_fmat4();
  tmp_enti.frame = &tmp_frame;
  tmp_enti.obj = *obj;
  tmp_enti.tex = tex_path ? picker_load_bmp_private(tex_path) : NULL;
  sf_tex_t *t = sf_render_thumb_enti(&sf_ctx, &tmp_enti, THUMB_CAM_SZ);
  if (tmp_enti.tex) { free((void*)tmp_enti.tex->px); free(tmp_enti.tex); }
  g_picker_thumbs[i] = t;
}

static void picker_render_model_thumb(int i) {
  const char *fname = g_model_files[i];
  const char *dot = strrchr(fname, '.');
  if (!dot) return;

  if (strcmp(dot, ".obj") == 0) {
    char r_path[512];
    if (!_sf_resolve_asset(fname, r_path, sizeof(r_path))) return;
    char nm[64];
    int stem = (int)(dot - fname); if (stem > 60) stem = 60;
    snprintf(nm, sizeof(nm), "%.*s", stem, fname);
    sf_obj_t *obj = sf_get_obj_(&sf_ctx, nm, false);
    if (!obj) obj = sf_load_obj(&sf_ctx, r_path, nm);
    if (!obj) return;
    /* Try stem-name texture match: tux.obj -> tux.bmp */
    char tex_bmp[80], tex_path[512];
    snprintf(tex_bmp, sizeof(tex_bmp), "%s.bmp", nm);
    const char *tex_full = _sf_resolve_asset(tex_bmp, tex_path, sizeof(tex_path)) ? tex_path : NULL;
    picker_render_obj_thumb(obj, tex_full, i);
  } else if (strcmp(dot, ".sff") == 0) {
    char r_path[512];
    if (!_sf_resolve_asset(fname, r_path, sizeof(r_path))) return;
    g_picker_thumbs[i] = sf_render_thumb_sff(&sf_ctx, r_path, THUMB_CAM_SZ);
  }
}

/* Load a BMP texture from disk into a privately malloc'd sf_tex_t (bypasses sf_ctx pool) */
static void picker_load_tex_thumb(int i) {
  const char *fname = g_tex_files[i];
  char path[512];
  if (!_sf_resolve_asset(fname, path, sizeof(path))) return;
  FILE *file = fopen(path, "rb");
  if (!file) return;
  uint8_t header[54];
  if (fread(header, 1, 54, file) != 54 || header[0] != 'B' || header[1] != 'M') {
    fclose(file); return;
  }
  uint32_t data_offset = header[10]|(header[11]<<8)|(header[12]<<16)|(header[13]<<24);
  int32_t w = header[18]|(header[19]<<8)|(header[20]<<16)|(header[21]<<24);
  int32_t h = header[22]|(header[23]<<8)|(header[24]<<16)|(header[25]<<24);
  int32_t h_abs = abs(h);
  if (w <= 0 || h_abs <= 0) { fclose(file); return; }
  sf_pkd_clr_t *px = (sf_pkd_clr_t*)malloc((size_t)w * (size_t)h_abs * sizeof(sf_pkd_clr_t));
  if (!px) { fclose(file); return; }
  fseek(file, data_offset, SEEK_SET);
  int padding = (4 - (w * 3) % 4) % 4;
  uint8_t bgr[3];
  for (int y = 0; y < h_abs; y++) {
    int dest_y = (h > 0) ? (h_abs - 1 - y) : y;
    for (int x = 0; x < w; x++) {
      if (fread(bgr, 1, 3, file) != 3) { bgr[0]=bgr[1]=bgr[2]=0; }
      if (bgr[2]==255 && bgr[1]==0 && bgr[0]==255) {
        px[dest_y*w+x] = 0x00000000;
      } else {
        uint8_t lr = (uint8_t)(powf(bgr[2]/255.0f,2.2f)*255.0f+0.5f);
        uint8_t lg = (uint8_t)(powf(bgr[1]/255.0f,2.2f)*255.0f+0.5f);
        uint8_t lb = (uint8_t)(powf(bgr[0]/255.0f,2.2f)*255.0f+0.5f);
        px[dest_y*w+x] = (0xFFu<<24)|((uint32_t)lr<<16)|((uint32_t)lg<<8)|lb;
      }
    }
    fseek(file, padding, SEEK_CUR);
  }
  fclose(file);
  sf_tex_t *t = (sf_tex_t*)malloc(sizeof(sf_tex_t));
  if (!t) { free(px); return; }
  memset(t, 0, sizeof(sf_tex_t));
  t->px = px; t->w = w; t->h = h_abs;
  t->w_mask = w - 1; t->h_mask = h_abs - 1;
  g_picker_thumbs[i] = t;
}

static void picker_load_thumbs(void) {
  /* Free all previously malloc'd thumbnails (model renders and private tex loads are all malloc'd) */
  for (int i = 0; i < g_picker_thumbs_n; i++) {
    if (g_picker_thumbs[i]) {
      free((void*)g_picker_thumbs[i]->px);
      free(g_picker_thumbs[i]);
    }
    g_picker_thumbs[i] = NULL;
  }

  g_picker_thumbs_n = picker_item_count();
  for (int i = 0; i < g_picker_thumbs_n; i++) {
    g_picker_thumbs[i] = NULL;
    if (g_picker_open == PICK_MODEL) {
      picker_render_model_thumb(i);
    } else {
      picker_load_tex_thumb(i);
    }
  }
}

static void cb_picker_close(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  g_picker_open = PICK_NONE; g_ui_dirty = true;
}
static void cb_picker_scroll_up(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_picker_scroll > 0) g_picker_scroll--;
  g_ui_dirty = true;
}
static void cb_picker_scroll_dn(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  g_picker_scroll++;
  g_ui_dirty = true;
}

static void cb_picker_pick(sf_ctx_t *ctx, void *ud) {
  (void)ctx;
  int idx = (int)(intptr_t)ud;
  if (g_picker_apply == PICK_APPLY_ENTI_TEX) {
    if (idx < 0 || idx >= g_tex_count) goto done;
    const char *fname = g_tex_files[idx];
    char texname[64]; const char *dot = strrchr(fname, '.'); int stem = dot?(int)(dot-fname):(int)strlen(fname); if(stem>60)stem=60;
    snprintf(texname, sizeof(texname), "%.*s", stem, fname);
    sf_tex_t *tex = NULL;
    for (int i = 0; i < sf_ctx.tex_count; i++)
      if (sf_ctx.textures[i].name && strcmp(sf_ctx.textures[i].name, texname)==0) { tex = &sf_ctx.textures[i]; break; }
    if (!tex) tex = sf_load_texture_bmp(&sf_ctx, fname, texname);
    if (tex && g_sel_kind == SEL_ENTI && g_sel) g_sel->tex = tex;
  } else if (g_picker_apply == PICK_APPLY_EMITR_SPRITE) {
    if (idx < 0 || idx >= g_tex_count || g_sel_kind != SEL_EMITR || !g_sel_emitr) goto done;
    const char *fname = g_tex_files[idx];
    char sname[64]; const char *dot = strrchr(fname, '.'); int stem = dot?(int)(dot-fname):(int)strlen(fname); if(stem>60)stem=60;
    snprintf(sname, sizeof(sname), "%.*s", stem, fname);
    sf_tex_t *tex = NULL;
    for (int i = 0; i < sf_ctx.tex_count; i++)
      if (sf_ctx.textures[i].name && strcmp(sf_ctx.textures[i].name, sname)==0) { tex = &sf_ctx.textures[i]; break; }
    if (!tex) tex = sf_load_texture_bmp(&sf_ctx, fname, sname);
    if (!tex) goto done;
    char sprname[80]; snprintf(sprname, sizeof(sprname), "spr_%s", sname);
    sf_sprite_2_t *spr = sf_get_sprite_(&sf_ctx, sprname, false);
    if (!spr) spr = sf_load_sprite(&sf_ctx, sprname, 1.0f, 0.3f, 1, tex->name);
    if (spr) g_sel_emitr->sprite = spr;
  } else if (g_picker_apply == PICK_APPLY_MODEL) {
    prim_meta_t *m = sel_meta();
    if (m) { m->model_idx = idx; cb_regen_sel(ctx, NULL); }
  } else if (g_picker_apply == PICK_APPLY_SFGEN_BARK) {
    if (idx < 0 || idx >= g_tex_count) goto done;
    const char *fname = g_tex_files[idx];
    char nm[64]; const char *dot2 = strrchr(fname, '.'); int stem2 = dot2?(int)(dot2-fname):(int)strlen(fname); if(stem2>60)stem2=60;
    snprintf(nm, sizeof(nm), "%.*s", stem2, fname);
    sf_tex_t *t = sf_get_texture_(&g_sfgen_ctx, nm, false);
    if (!t) t = sf_load_texture_bmp(&g_sfgen_ctx, fname, nm);
    if (t && g_ct_enti) { g_ct_enti->tex = t; g_ct_enti->tex_scale = (sf_fvec2_t){1.f,1.f}; }
  } else if (g_picker_apply == PICK_APPLY_SFGEN_LEAF) {
    if (idx < 0 || idx >= g_tex_count) goto done;
    const char *fname = g_tex_files[idx];
    char nm[64]; const char *dot3 = strrchr(fname, '.'); int stem3 = dot3?(int)(dot3-fname):(int)strlen(fname); if(stem3>60)stem3=60;
    snprintf(nm, sizeof(nm), "%.*s", stem3, fname);
    sf_tex_t *t = sf_get_texture_(&g_sfgen_ctx, nm, false);
    if (!t) t = sf_load_texture_bmp(&g_sfgen_ctx, fname, nm);
    if (t) {
      char sprname[80]; snprintf(sprname, sizeof(sprname), "spr_%s", nm);
      sf_sprite_2_t *spr = sf_get_sprite_(&g_sfgen_ctx, sprname, false);
      if (!spr) spr = sf_load_sprite(&g_sfgen_ctx, sprname, 1.0f, 0.7f, 1, nm);
      if (spr) g_sfgen_leaf = spr;
    }
  } else if (g_picker_apply == PICK_APPLY_SFGEN_STONE ||
             g_picker_apply == PICK_APPLY_SFGEN_BWALL ||
             g_picker_apply == PICK_APPLY_SFGEN_BROOF ||
             g_picker_apply == PICK_APPLY_SFGEN_BWIN  ||
             g_picker_apply == PICK_APPLY_SFGEN_BLEDGE) {
    if (idx < 0 || idx >= g_tex_count) goto done;
    const char *fname = g_tex_files[idx];
    char nm[64]; const char *dotc = strrchr(fname, '.'); int stc = dotc?(int)(dotc-fname):(int)strlen(fname); if(stc>60)stc=60;
    snprintf(nm, sizeof(nm), "%.*s", stc, fname);
    sf_tex_t *t = sf_get_texture_(&g_sfgen_ctx, nm, false);
    if (!t) t = sf_load_texture_bmp(&g_sfgen_ctx, fname, nm);
    if (t) {
      if (g_picker_apply == PICK_APPLY_SFGEN_STONE && g_cr_enti)
        { g_cr_enti->tex = t; g_cr_enti->tex_scale = (sf_fvec2_t){1.f,1.f}; }
      else if (g_picker_apply == PICK_APPLY_SFGEN_BWALL && g_ck_enti)
        { g_ck_enti->tex = t; g_ck_enti->tex_scale = (sf_fvec2_t){1.f,1.f}; }
      else if (g_picker_apply == PICK_APPLY_SFGEN_BROOF && g_ck_enti_roof)
        { g_ck_enti_roof->tex = t; g_ck_enti_roof->tex_scale = (sf_fvec2_t){1.f,1.f}; }
      else if (g_picker_apply == PICK_APPLY_SFGEN_BWIN && g_ck_enti_win)
        { g_ck_enti_win->tex = t; g_ck_enti_win->tex_scale = (sf_fvec2_t){1.f,1.f}; }
      else if (g_picker_apply == PICK_APPLY_SFGEN_BLEDGE && g_ck_enti_ledge)
        { g_ck_enti_ledge->tex = t; g_ck_enti_ledge->tex_scale = (sf_fvec2_t){1.f,1.f}; }
    }
  }
  done:
  g_picker_open = PICK_NONE; g_ui_dirty = true;
}

static void picker_open_and_scan(picker_kind_t kind, picker_apply_t apply) {
  if (kind == PICK_MODEL) scan_models(); else scan_textures();
  g_picker_open  = kind;
  g_picker_apply = apply;
  g_picker_scroll = 0;
  picker_load_thumbs();
  g_ui_dirty = true;
}
static void cb_picker_refresh(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_picker_open == PICK_MODEL) scan_models(); else scan_textures();
  picker_load_thumbs();
  g_ui_dirty = true;
}
static void cb_browse_tex(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud; picker_open_and_scan(PICK_TEX, PICK_APPLY_ENTI_TEX);
}
static void cb_browse_model(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud; picker_open_and_scan(PICK_MODEL, PICK_APPLY_MODEL);
}
static void cb_browse_sprite(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud; picker_open_and_scan(PICK_SPRITE, PICK_APPLY_EMITR_SPRITE);
}
static void cb_browse_sfgen_bark(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud; picker_open_and_scan(PICK_TEX, PICK_APPLY_SFGEN_BARK);
}
static void cb_browse_sfgen_leaf(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud; picker_open_and_scan(PICK_TEX, PICK_APPLY_SFGEN_LEAF);
}
static void cb_browse_sfgen_stone(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud; picker_open_and_scan(PICK_TEX, PICK_APPLY_SFGEN_STONE);
}
static void cb_browse_sfgen_bwall(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud; picker_open_and_scan(PICK_TEX, PICK_APPLY_SFGEN_BWALL);
}
static void cb_browse_sfgen_broof(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud; picker_open_and_scan(PICK_TEX, PICK_APPLY_SFGEN_BROOF);
}
static void cb_browse_sfgen_bwin(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud; picker_open_and_scan(PICK_TEX, PICK_APPLY_SFGEN_BWIN);
}
static void cb_browse_sfgen_bledge(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud; picker_open_and_scan(PICK_TEX, PICK_APPLY_SFGEN_BLEDGE);
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

/* ============================================================
   SFGEN — tree math helpers
   ============================================================ */
static sf_fvec3_t sfgen_v3s(sf_fvec3_t v, float s) { return (sf_fvec3_t){v.x*s,v.y*s,v.z*s}; }
static float sfgen_v3len(sf_fvec3_t v) { return sqrtf(v.x*v.x+v.y*v.y+v.z*v.z); }
static sf_fvec3_t sfgen_v3rot(sf_fvec3_t v, sf_fvec3_t k, float theta) {
    float c=cosf(theta), s=sinf(theta), d=sf_fvec3_dot(k,v);
    sf_fvec3_t cr=sf_fvec3_cross(k,v);
    return (sf_fvec3_t){v.x*c+cr.x*s+k.x*d*(1.f-c),
                        v.y*c+cr.y*s+k.y*d*(1.f-c),
                        v.z*c+cr.z*s+k.z*d*(1.f-c)};
}

/* ============================================================
   SFGEN — tree RNG
   ============================================================ */
static void sfgen_rseed(uint32_t s) { g_sfgen_rng = s ? s : 1; }
static float sfgen_rnd(void) {
    g_sfgen_rng ^= g_sfgen_rng<<13; g_sfgen_rng ^= g_sfgen_rng>>17; g_sfgen_rng ^= g_sfgen_rng<<5;
    return (float)(g_sfgen_rng & 0xFFFF) / 65535.f;
}
static float sfgen_rf2(void) { return sfgen_rnd()*2.f - 1.f; }

/* ============================================================
   SFGEN — add cylinder segment (tree mesh)
   ============================================================ */
#define SFGEN_SEGS 6
static void sfgen_add_seg(sf_obj_t *o,
    sf_fvec3_t p0, sf_fvec3_t p1, float r0, float r1, float vt0, float vt1)
{
    if (o->v_cnt + (SFGEN_SEGS+1)*2 > o->v_cap) return;
    if (o->f_cnt + SFGEN_SEGS*2      > o->f_cap) return;
    sf_fvec3_t ax = sf_fvec3_norm(sf_fvec3_sub(p1,p0));
    sf_fvec3_t tmp = {0,1,0};
    if (fabsf(ax.y) > 0.98f) tmp = (sf_fvec3_t){1,0,0};
    sf_fvec3_t ri = sf_fvec3_norm(sf_fvec3_cross(tmp,ax));
    sf_fvec3_t fw = sf_fvec3_cross(ax,ri);
    int bv=o->v_cnt, bt=o->vt_cnt;
    for (int i=0; i<=SFGEN_SEGS; i++) {
        float th=(float)i/SFGEN_SEGS*2.f*SF_PI, c=cosf(th), s=sinf(th), u=(float)i/SFGEN_SEGS;
        sf_fvec3_t d={ri.x*c+fw.x*s, ri.y*c+fw.y*s, ri.z*c+fw.z*s};
        sf_obj_add_vert(o, (sf_fvec3_t){p0.x+d.x*r0,p0.y+d.y*r0,p0.z+d.z*r0});
        sf_obj_add_vert(o, (sf_fvec3_t){p1.x+d.x*r1,p1.y+d.y*r1,p1.z+d.z*r1});
        sf_obj_add_uv(o, (sf_fvec2_t){u, vt0});
        sf_obj_add_uv(o, (sf_fvec2_t){u, vt1});
    }
    for (int i=0; i<SFGEN_SEGS; i++) {
        int v00=bv+i*2,v10=bv+i*2+1,v01=bv+(i+1)*2,v11=bv+(i+1)*2+1;
        int t00=bt+i*2,t10=bt+i*2+1,t01=bt+(i+1)*2,t11=bt+(i+1)*2+1;
        sf_obj_add_face_uv(o,v00,v01,v11,t00,t01,t11);
        sf_obj_add_face_uv(o,v00,v11,v10,t00,t11,t10);
    }
}

/* ============================================================
   SFGEN — tree grow
   ============================================================ */
static void sfgen_grow(sf_obj_t *obj, sf_fvec3_t pos, sf_fvec3_t dir,
    float rad, float len, float vt, int depth, int maxd)
{
    if (depth < 0) return;
    if (obj->v_cnt + (SFGEN_SEGS+1)*2 > obj->v_cap) return;
    float er = rad * ct_taper;
    sf_fvec3_t end = sf_fvec3_add(pos, sfgen_v3s(dir, len));
    float wa = ct_wiggle * len;
    end.x += sfgen_rf2()*wa; end.y += sfgen_rf2()*wa*0.25f; end.z += sfgen_rf2()*wa;
    float avgr = (rad+er)*0.5f;
    float vt1  = vt + len / (2.f*SF_PI*avgr + 1e-4f);
    sfgen_add_seg(obj, pos, end, rad, er, vt, vt1);
    if (depth == 0) {
        int n = (int)(ct_ld + .5f);
        for (int i=0; i<n && g_sfgen_ctx.sprite_3d_count < SF_MAX_SPRITE_3DS; i++) {
            float sp = len * 0.9f;
            sf_fvec3_t lp = {end.x+sfgen_rf2()*sp, end.y+sfgen_rnd()*sp*0.8f, end.z+sfgen_rf2()*sp};
            float ls = fminf(ct_ls*(0.7f+sfgen_rnd()*0.3f), 2.0f);
            float lo = fminf(ct_lo, 1.0f);
            float la = sfgen_rnd()*2.f*SF_PI;
            char bname[32]; snprintf(bname, sizeof(bname), "lf_%d", g_sfgen_ctx.sprite_3d_count);
            sf_sprite_3_t *bl = sf_add_sprite_3d(&g_sfgen_ctx, g_sfgen_leaf, bname, lp, ls, lo, la);
            if (bl) {
                bl->frame = g_ct_enti ? g_ct_enti->frame : NULL;
                if (ct_rand3d) {
                    bl->normal.x=sfgen_rf2(); bl->normal.y=sfgen_rf2(); bl->normal.z=sfgen_rf2();
                    float nl=sqrtf(bl->normal.x*bl->normal.x+bl->normal.y*bl->normal.y+bl->normal.z*bl->normal.z);
                    if (nl>0.001f){bl->normal.x/=nl;bl->normal.y/=nl;bl->normal.z/=nl;}
                }
            }
        }
        return;
    }
    sf_fvec3_t up={0,1,0};
    float d_up=sf_fvec3_dot(dir,up);
    sf_fvec3_t perp=sf_fvec3_sub(up, sfgen_v3s(dir,d_up));
    float pl=sfgen_v3len(perp);
    if (pl<0.01f) perp=(sf_fvec3_t){1,0,0}; else perp=sfgen_v3s(perp,1.f/pl);
    int   nb      = (int)(ct_branch + .5f);
    float base_tw = sfgen_rnd()*2.f*SF_PI;
    float ang     = SF_DEG2RAD(ct_angle) + sfgen_rf2()*SF_DEG2RAD(7.f);
    float gf = ct_grav*(float)(maxd-depth+1)/(float)(maxd+1);
    if (gf> 1.f) gf= 1.f; if (gf<-1.f) gf=-1.f;
    sf_fvec3_t gdir = {0, gf<0?1.f:-1.f, 0};
    float gabs = fabsf(gf);
    for (int i=0; i<nb; i++) {
        sf_fvec3_t cd = sfgen_v3rot(dir, perp, ang);
        cd = sfgen_v3rot(cd, dir, base_tw + SF_DEG2RAD(ct_twist)*(float)i);
        if (gabs>0.001f)
            cd = sf_fvec3_norm(sf_fvec3_add(sfgen_v3s(cd,1.f-gabs),sfgen_v3s(gdir,gabs)));
        sf_fvec3_t right = sf_fvec3_cross(cd,up);
        float rl = sfgen_v3len(right);
        if (rl > 0.01f) {
            right = sfgen_v3s(right, 1.f/rl);
            sf_fvec3_t fw2 = sf_fvec3_cross(cd,right);
            float wf = ct_wiggle*0.3f;
            cd = sf_fvec3_norm((sf_fvec3_t){
                cd.x+(sfgen_rf2()*right.x+sfgen_rf2()*fw2.x)*wf,
                cd.y+(sfgen_rf2()*right.y+sfgen_rf2()*fw2.y)*wf,
                cd.z+(sfgen_rf2()*right.z+sfgen_rf2()*fw2.z)*wf});
        }
        float cl = len*ct_len*(0.88f+sfgen_rnd()*0.24f);
        sfgen_grow(obj, end, cd, er, cl, vt1, depth-1, maxd);
    }
}

/* ============================================================
   SFGEN — generate tree
   ============================================================ */
static void sfgen_generate_tree(void) {
    if (!g_ct_obj || !g_ct_enti) return;
    g_ct_obj->v_cnt=0; g_ct_obj->vt_cnt=0; g_ct_obj->f_cnt=0; g_ct_obj->src_path=NULL;
    sf_clear_sprite_3ds(&g_sfgen_ctx);
    sfgen_rseed((uint32_t)(ct_seed*17239.f)+1);
    int maxd = (int)(ct_depth+.5f);
    float lean = ct_wiggle*0.2f;
    sf_fvec3_t tdir = sf_fvec3_norm((sf_fvec3_t){sfgen_rf2()*lean, 1.f, sfgen_rf2()*lean});
    sfgen_grow(g_ct_obj, (sf_fvec3_t){0,0,0}, tdir, ct_tr, ct_tl, 0.f, maxd, maxd);
    sf_obj_recompute_bs(g_ct_obj);
    g_ct_enti->obj.v_cnt     = g_ct_obj->v_cnt;
    g_ct_enti->obj.vt_cnt    = g_ct_obj->vt_cnt;
    g_ct_enti->obj.f_cnt     = g_ct_obj->f_cnt;
    g_ct_enti->obj.bs_center = g_ct_obj->bs_center;
    g_ct_enti->obj.bs_radius = g_ct_obj->bs_radius;
}

/* ============================================================
   SFGEN — rock noise
   ============================================================ */

/* ============================================================
   SFGEN — icosphere for rock
   ============================================================ */
static void sfgen_mid_clear(void) { memset(g_sfgen_mid_a,-1,sizeof(g_sfgen_mid_a)); }
static int sfgen_midpt(int a, int b) {
    if (a>b){int t=a;a=b;b=t;}
    uint32_t slot=((uint32_t)a*92837111u^(uint32_t)b*689287499u)&(SFGEN_MID_SZ-1);
    while (g_sfgen_mid_a[slot]!=-1) {
        if (g_sfgen_mid_a[slot]==a && g_sfgen_mid_b[slot]==b) return g_sfgen_mid_val[slot];
        slot=(slot+1)&(SFGEN_MID_SZ-1);
    }
    if (g_sfgen_iso_vi>=SFGEN_ISO_V) return 0;
    sf_fvec3_t m={(g_sfgen_iso_v[a].x+g_sfgen_iso_v[b].x)*0.5f,
                  (g_sfgen_iso_v[a].y+g_sfgen_iso_v[b].y)*0.5f,
                  (g_sfgen_iso_v[a].z+g_sfgen_iso_v[b].z)*0.5f};
    float l=sqrtf(m.x*m.x+m.y*m.y+m.z*m.z);
    if (l>0.f){m.x/=l;m.y/=l;m.z/=l;}
    int idx=g_sfgen_iso_vi;
    g_sfgen_iso_v[g_sfgen_iso_vi++]=m;
    g_sfgen_mid_a[slot]=a; g_sfgen_mid_b[slot]=b; g_sfgen_mid_val[slot]=idx;
    return idx;
}
static void sfgen_build_iso(int subdiv) {
    g_sfgen_iso_vi=0; g_sfgen_iso_fi=0; sfgen_mid_clear();
    float t=(1.f+sqrtf(5.f))*0.5f, s=1.f/sqrtf(1.f+t*t), ts=t*s;
    sf_fvec3_t iv[]={
        {-s,ts,0},{s,ts,0},{-s,-ts,0},{s,-ts,0},
        {0,-s,ts},{0,s,ts},{0,-s,-ts},{0,s,-ts},
        {ts,0,-s},{ts,0,s},{-ts,0,-s},{-ts,0,s}
    };
    for (int i=0;i<12;i++) g_sfgen_iso_v[g_sfgen_iso_vi++]=iv[i];
    static const int IF[20][3]={
        {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
        {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
        {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
        {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}
    };
    for (int i=0;i<20;i++){
        g_sfgen_iso_f[g_sfgen_iso_fi][0]=IF[i][0];
        g_sfgen_iso_f[g_sfgen_iso_fi][1]=IF[i][1];
        g_sfgen_iso_f[g_sfgen_iso_fi][2]=IF[i][2];
        g_sfgen_iso_fi++;
    }
    static int nf_buf[SFGEN_ISO_F][3];
    for (int lv=0;lv<subdiv;lv++){
        int nf=g_sfgen_iso_fi, nfi=0;
        for (int i=0;i<nf;i++){
            int a=g_sfgen_iso_f[i][0],b=g_sfgen_iso_f[i][1],c=g_sfgen_iso_f[i][2];
            int ab=sfgen_midpt(a,b),bc=sfgen_midpt(b,c),ca=sfgen_midpt(c,a);
            nf_buf[nfi][0]=a;  nf_buf[nfi][1]=ab; nf_buf[nfi][2]=ca; nfi++;
            nf_buf[nfi][0]=b;  nf_buf[nfi][1]=bc; nf_buf[nfi][2]=ab; nfi++;
            nf_buf[nfi][0]=c;  nf_buf[nfi][1]=ca; nf_buf[nfi][2]=bc; nfi++;
            nf_buf[nfi][0]=ab; nf_buf[nfi][1]=bc; nf_buf[nfi][2]=ca; nfi++;
        }
        g_sfgen_iso_fi=nfi;
        memcpy(g_sfgen_iso_f, nf_buf, nfi*sizeof(g_sfgen_iso_f[0]));
        sfgen_mid_clear();
    }
}

/* ============================================================
   SFGEN — generate rock
   ============================================================ */
static void sfgen_generate_rock(void) {
    if (!g_cr_obj || !g_cr_enti) return;
    g_cr_obj->v_cnt=0; g_cr_obj->vt_cnt=0; g_cr_obj->f_cnt=0; g_cr_obj->src_path=NULL;
    int subdiv=(int)(cr_subdiv+0.5f);
    if (subdiv<1) subdiv=1; if (subdiv>4) subdiv=4;
    sfgen_build_iso(subdiv);
    uint32_t seed=(uint32_t)(cr_seed*12345.f)+1u;
    int oct=(int)(cr_octaves+0.5f);
    if (oct<1) oct=1; if (oct>8) oct=8;
    static sf_fvec3_t dv[SFGEN_ISO_V];
    for (int i=0;i<g_sfgen_iso_vi;i++){
        sf_fvec3_t v=g_sfgen_iso_v[i];
        float n  = sf_noise_fbm_3d(v.x*cr_freq,          v.y*cr_freq,          v.z*cr_freq,          oct, cr_persist, seed);
        float nb = sf_noise_fbm_3d(v.x*cr_freq*4.f+7.3f, v.y*cr_freq*4.f+3.1f, v.z*cr_freq*4.f+5.7f, 2,   0.5f,       seed+99u);
        float disp=(n*2.f-1.f)*cr_rough + (nb*2.f-1.f)*cr_rough*cr_bump*0.25f;
        float r=1.f+disp;
        v.x*=r; v.y*=r; v.z*=r;
        v.x*=cr_elongx; v.z*=cr_elongz;
        v.y*=(1.f-cr_flat*0.8f);
        if (v.y>0.f) v.y*=(1.f+cr_pointy);
        dv[i]=v;
    }
    float tile=1.5f;
    for (int i=0;i<g_sfgen_iso_fi;i++){
        int ai=g_sfgen_iso_f[i][0],bi=g_sfgen_iso_f[i][1],ci=g_sfgen_iso_f[i][2];
        sf_fvec3_t na=g_sfgen_iso_v[ai],nb2=g_sfgen_iso_v[bi],nc=g_sfgen_iso_v[ci];
        sf_fvec2_t uva={(na.x+1.f)*tile*0.5f,(na.z+1.f)*tile*0.5f};
        sf_fvec2_t uvb={(nb2.x+1.f)*tile*0.5f,(nb2.z+1.f)*tile*0.5f};
        sf_fvec2_t uvc={(nc.x+1.f)*tile*0.5f,(nc.z+1.f)*tile*0.5f};
        int bv=g_cr_obj->v_cnt, bt=g_cr_obj->vt_cnt;
        sf_obj_add_vert(g_cr_obj,dv[ai]); sf_obj_add_vert(g_cr_obj,dv[bi]); sf_obj_add_vert(g_cr_obj,dv[ci]);
        sf_obj_add_uv(g_cr_obj,uva); sf_obj_add_uv(g_cr_obj,uvb); sf_obj_add_uv(g_cr_obj,uvc);
        sf_obj_add_face_uv(g_cr_obj,bv,bv+1,bv+2,bt,bt+1,bt+2);
    }
    sf_obj_recompute_bs(g_cr_obj);
    g_cr_enti->obj.v_cnt=g_cr_obj->v_cnt; g_cr_enti->obj.vt_cnt=g_cr_obj->vt_cnt;
    g_cr_enti->obj.f_cnt=g_cr_obj->f_cnt;
    g_cr_enti->obj.bs_center=g_cr_obj->bs_center; g_cr_enti->obj.bs_radius=g_cr_obj->bs_radius;
}

/* ============================================================
   SFGEN — building generator helpers
   ============================================================ */
/* Emit quad with UVs; vertices a,b,c,d must be in CCW order when viewed from outside.
   b-a is the U-axis, d-a is the V-axis for UV mapping. */
static void bk_emit_quad(sf_obj_t *o,
                         sf_fvec3_t a, sf_fvec3_t b, sf_fvec3_t c, sf_fvec3_t d,
                         float tile)
{
    sf_fvec3_t eu = {b.x-a.x, b.y-a.y, b.z-a.z};
    sf_fvec3_t ev = {d.x-a.x, d.y-a.y, d.z-a.z};
    float ul = sqrtf(eu.x*eu.x+eu.y*eu.y+eu.z*eu.z);
    float vl = sqrtf(ev.x*ev.x+ev.y*ev.y+ev.z*ev.z);
    sf_fvec2_t uva={0.f,0.f}, uvb={ul*tile,0.f}, uvc={ul*tile,vl*tile}, uvd={0.f,vl*tile};
    int bv=o->v_cnt, bt=o->vt_cnt;
    sf_obj_add_vert(o,a); sf_obj_add_vert(o,b); sf_obj_add_vert(o,c); sf_obj_add_vert(o,d);
    sf_obj_add_uv(o,uva); sf_obj_add_uv(o,uvb); sf_obj_add_uv(o,uvc); sf_obj_add_uv(o,uvd);
    sf_obj_add_face_uv(o,bv,bv+1,bv+2,bt,bt+1,bt+2);
    sf_obj_add_face_uv(o,bv,bv+2,bv+3,bt,bt+2,bt+3);
}

/* Same as bk_emit_quad but with reversed winding (for when vertex order is CW from outside). */
static void bk_emit_quad_rev(sf_obj_t *o,
                              sf_fvec3_t a, sf_fvec3_t b, sf_fvec3_t c, sf_fvec3_t d,
                              float tile)
{
    sf_fvec3_t eu = {b.x-a.x, b.y-a.y, b.z-a.z};
    sf_fvec3_t ev = {d.x-a.x, d.y-a.y, d.z-a.z};
    float ul = sqrtf(eu.x*eu.x+eu.y*eu.y+eu.z*eu.z);
    float vl = sqrtf(ev.x*ev.x+ev.y*ev.y+ev.z*ev.z);
    sf_fvec2_t uva={0.f,0.f}, uvb={ul*tile,0.f}, uvc={ul*tile,vl*tile}, uvd={0.f,vl*tile};
    int bv=o->v_cnt, bt=o->vt_cnt;
    sf_obj_add_vert(o,a); sf_obj_add_vert(o,b); sf_obj_add_vert(o,c); sf_obj_add_vert(o,d);
    sf_obj_add_uv(o,uva); sf_obj_add_uv(o,uvb); sf_obj_add_uv(o,uvc); sf_obj_add_uv(o,uvd);
    sf_obj_add_face_uv(o,bv,bv+2,bv+1,bt,bt+2,bt+1);
    sf_obj_add_face_uv(o,bv,bv+3,bv+2,bt,bt+3,bt+2);
}

/* Emit a quad with automatic winding correction.
   Pass 'out' as the direction the face should be visible from (outward normal).
   Vertices may be in any order consistent with a planar quad. */
static void bk_quad(sf_obj_t *o,
                    sf_fvec3_t a, sf_fvec3_t b, sf_fvec3_t c, sf_fvec3_t d,
                    sf_fvec3_t out, float tile)
{
    sf_fvec3_t ab = {b.x-a.x, b.y-a.y, b.z-a.z};
    sf_fvec3_t ac = {c.x-a.x, c.y-a.y, c.z-a.z};
    float nx = ab.y*ac.z - ab.z*ac.y;
    float ny = ab.z*ac.x - ab.x*ac.z;
    float nz = ab.x*ac.y - ab.y*ac.x;
    if (nx*out.x + ny*out.y + nz*out.z < 0.f)
        bk_emit_quad_rev(o, a, b, c, d, tile);
    else
        bk_emit_quad(o, a, b, c, d, tile);
}

static void bk_emit_tri(sf_obj_t *o,
                        sf_fvec3_t a, sf_fvec3_t b, sf_fvec3_t c,
                        float tile)
{
    sf_fvec3_t e1={b.x-a.x,b.y-a.y,b.z-a.z};
    sf_fvec3_t e2={c.x-a.x,c.y-a.y,c.z-a.z};
    float l1=sqrtf(e1.x*e1.x+e1.y*e1.y+e1.z*e1.z);
    float l2=sqrtf(e2.x*e2.x+e2.y*e2.y+e2.z*e2.z);
    sf_fvec2_t uva={0.f,0.f}, uvb={l1*tile,0.f}, uvc={0.f,l2*tile};
    int bv=o->v_cnt, bt=o->vt_cnt;
    sf_obj_add_vert(o,a); sf_obj_add_vert(o,b); sf_obj_add_vert(o,c);
    sf_obj_add_uv(o,uva); sf_obj_add_uv(o,uvb); sf_obj_add_uv(o,uvc);
    sf_obj_add_face_uv(o,bv,bv+1,bv+2,bt,bt+1,bt+2);
}

/* Emit a triangle with automatic winding correction. */
static void bk_tri(sf_obj_t *o,
                   sf_fvec3_t a, sf_fvec3_t b, sf_fvec3_t c,
                   sf_fvec3_t out, float tile)
{
    sf_fvec3_t ab = {b.x-a.x, b.y-a.y, b.z-a.z};
    sf_fvec3_t ac = {c.x-a.x, c.y-a.y, c.z-a.z};
    float nx = ab.y*ac.z - ab.z*ac.y;
    float ny = ab.z*ac.x - ab.x*ac.z;
    float nz = ab.x*ac.y - ab.y*ac.x;
    if (nx*out.x + ny*out.y + nz*out.z < 0.f)
        bk_emit_tri(o, a, c, b, tile);
    else
        bk_emit_tri(o, a, b, c, tile);
}

/* Roof helper — called after all floors have been stacked.
   All faces use bk_quad/bk_tri with explicit outward normals to guarantee correct winding. */
static void bk_emit_roof(sf_obj_t *o, float cx, float cz, float y_base, float w, float d, float tile) {
    int rtype = (int)(bk_roof_type + 0.5f);
    if (rtype < 0) rtype = 0; if (rtype > 5) rtype = 5;
    float rs = bk_roof_scale; if (rs < 0.1f) rs = 0.1f;
    float hw = w * 0.5f * rs, hd = d * 0.5f * rs;
    float rh = bk_roof_h * (w > d ? w : d) * 0.5f;
    if (rh < 0.1f) rh = 0.1f;

    sf_fvec3_t fl = {cx-hw, y_base, cz+hd};
    sf_fvec3_t fr = {cx+hw, y_base, cz+hd};
    sf_fvec3_t br = {cx+hw, y_base, cz-hd};
    sf_fvec3_t bl = {cx-hw, y_base, cz-hd};

    static const sf_fvec3_t UP    = {0.f, 1.f, 0.f};
    static const sf_fvec3_t DOWN  = {0.f,-1.f, 0.f};
    static const sf_fvec3_t FWD   = {0.f, 0.f, 1.f};
    static const sf_fvec3_t BACK  = {0.f, 0.f,-1.f};
    static const sf_fvec3_t RIGHT = {1.f, 0.f, 0.f};
    static const sf_fvec3_t LEFT  = {-1.f, 0.f, 0.f};

    switch (rtype) {
    case 0: { /* Flat + parapet */
        bk_quad(o, fl, fr, br, bl, DOWN, tile);  /* underside */
        bk_quad(o, fl, fr, br, bl, UP, tile);
        float ph = rh * 0.4f, po = 0.15f;
        sf_fvec3_t ofl={cx-hw-po,y_base,cz+hd+po}, ofr={cx+hw+po,y_base,cz+hd+po};
        sf_fvec3_t obr={cx+hw+po,y_base,cz-hd-po}, obl={cx-hw-po,y_base,cz-hd-po};
        sf_fvec3_t ufl={ofl.x,y_base+ph,ofl.z}, ufr={ofr.x,y_base+ph,ofr.z};
        sf_fvec3_t ubr={obr.x,y_base+ph,obr.z}, ubl={obl.x,y_base+ph,obl.z};
        bk_quad(o, ofl, ofr, ufr, ufl, FWD,   tile);
        bk_quad(o, ofr, obr, ubr, ufr, RIGHT,  tile);
        bk_quad(o, obr, obl, ubl, ubr, BACK,   tile);
        bk_quad(o, obl, ofl, ufl, ubl, LEFT,   tile);
        /* parapet inner top ring (faces up) */
        bk_quad(o, fl,  fr,  ufr, ufl, UP, tile);
        bk_quad(o, fr,  br,  ubr, ufr, UP, tile);
        bk_quad(o, br,  bl,  ubl, ubr, UP, tile);
        bk_quad(o, bl,  fl,  ufl, ubl, UP, tile);
        break;
    }
    case 1: { /* Pitched (ridge along X) */
        bk_quad(o, fl, fr, br, bl, DOWN, tile);  /* underside */
        sf_fvec3_t rl={cx-hw,y_base+rh,cz}, rr={cx+hw,y_base+rh,cz};
        sf_fvec3_t front_out = {0.f, hd, rh};   /* outward+upward for front slope */
        sf_fvec3_t back_out  = {0.f, hd,-rh};
        bk_quad(o, fl, fr, rr, rl, front_out, tile);
        bk_quad(o, br, bl, rl, rr, back_out,  tile);
        bk_tri(o, fl, rl, bl, LEFT,  tile);
        bk_tri(o, fr, rr, br, RIGHT, tile);
        break;
    }
    case 2: { /* Hip roof */
        bk_quad(o, fl, fr, br, bl, DOWN, tile);  /* underside */
        sf_fvec3_t rl={cx-hw*0.5f,y_base+rh,cz}, rr={cx+hw*0.5f,y_base+rh,cz};
        sf_fvec3_t front_out = {0.f, hd, rh};
        sf_fvec3_t back_out  = {0.f, hd,-rh};
        bk_quad(o, fl, fr, rr, rl, front_out, tile);
        bk_quad(o, br, bl, rl, rr, back_out,  tile);
        bk_tri(o, fl, rl, bl, LEFT,  tile);
        bk_tri(o, fr, rr, br, RIGHT, tile);
        break;
    }
    case 3: { /* Dome approximation (stacked rings) */
        int rings = 6, segs = 8;
        sf_fvec3_t prev_ring[8], cur_ring_v[8];
        sf_fvec3_t center_base = {cx, y_base, cz};
        for (int s = 0; s < segs; s++) {
            float a_s = (float)s / segs * 3.14159f * 2.f;
            prev_ring[s] = (sf_fvec3_t){cx + hw*cosf(a_s), y_base, cz + hd*sinf(a_s)};
        }
        /* Bottom cap — fan of triangles facing down */
        for (int s = 0; s < segs; s++) {
            int ns = (s+1) % segs;
            bk_tri(o, center_base, prev_ring[s], prev_ring[ns], DOWN, tile);
        }
        for (int r = 1; r <= rings; r++) {
            float phi = (float)r / rings * 3.14159f * 0.5f;
            float cr = cosf(phi), sr = sinf(phi);
            for (int s = 0; s < segs; s++) {
                float a_s = (float)s / segs * 3.14159f * 2.f;
                cur_ring_v[s] = (sf_fvec3_t){cx + hw*cr*cosf(a_s), y_base + rh*sr, cz + hd*cr*sinf(a_s)};
            }
            for (int s = 0; s < segs; s++) {
                int ns = (s+1) % segs;
                /* outward = midpoint direction from dome center */
                if (r < rings) {
                    sf_fvec3_t mid = {
                        (cur_ring_v[s].x+cur_ring_v[ns].x+prev_ring[ns].x+prev_ring[s].x)*0.25f - cx,
                        0.f,
                        (cur_ring_v[s].z+cur_ring_v[ns].z+prev_ring[ns].z+prev_ring[s].z)*0.25f - cz
                    };
                    bk_quad(o, cur_ring_v[s], cur_ring_v[ns], prev_ring[ns], prev_ring[s], mid, tile);
                } else {
                    sf_fvec3_t mid = {
                        (prev_ring[ns].x+prev_ring[s].x+cur_ring_v[s].x)*0.333f - cx,
                        0.f,
                        (prev_ring[ns].z+prev_ring[s].z+cur_ring_v[s].z)*0.333f - cz
                    };
                    bk_tri(o, prev_ring[ns], prev_ring[s], cur_ring_v[s], mid, tile);
                }
            }
            for (int s = 0; s < segs; s++) prev_ring[s] = cur_ring_v[s];
        }
        break;
    }
    case 4: { /* Stepped / ziggurat */
        bk_quad(o, fl, fr, br, bl, DOWN, tile);  /* underside */
        int steps = 3;
        float sw = w, sd = d, sy = y_base;
        for (int s = 0; s < steps; s++) {
            float nw = sw * 0.65f, nd = sd * 0.65f, nh = rh / steps;
            float nhw = nw*0.5f, nhd = nd*0.5f;
            sf_fvec3_t tfl={cx-nhw,sy+nh,cz+nhd}, tfr={cx+nhw,sy+nh,cz+nhd};
            sf_fvec3_t tbr={cx+nhw,sy+nh,cz-nhd}, tbl={cx-nhw,sy+nh,cz-nhd};
            sf_fvec3_t bfl2={cx-sw*0.5f,sy,cz+sd*0.5f}, bfr2={cx+sw*0.5f,sy,cz+sd*0.5f};
            sf_fvec3_t bbr2={cx+sw*0.5f,sy,cz-sd*0.5f}, bbl2={cx-sw*0.5f,sy,cz-sd*0.5f};
            /* 4 outward step walls */
            bk_quad(o, bfl2, bfr2, tfr, tfl, FWD,   tile);
            bk_quad(o, bfr2, bbr2, tbr, tfr, RIGHT,  tile);
            bk_quad(o, bbr2, bbl2, tbl, tbr, BACK,   tile);
            bk_quad(o, bbl2, bfl2, tfl, tbl, LEFT,   tile);
            /* step top annular ring (upward-facing) */
            bk_quad(o, tfl, tfr, bfr2, bfl2, UP, tile);
            bk_quad(o, tfr, tbr, bbr2, bfr2, UP, tile);
            bk_quad(o, tbr, tbl, bbl2, bbr2, UP, tile);
            bk_quad(o, tbl, tfl, bfl2, bbl2, UP, tile);
            sw=nw; sd=nd; sy+=nh;
        }
        /* Flat top */
        sf_fvec3_t tfl2={cx-sw*0.5f,sy,cz+sd*0.5f}, tfr2={cx+sw*0.5f,sy,cz+sd*0.5f};
        sf_fvec3_t tbr2={cx+sw*0.5f,sy,cz-sd*0.5f}, tbl2={cx-sw*0.5f,sy,cz-sd*0.5f};
        bk_quad(o, tfl2, tfr2, tbr2, tbl2, UP, tile);
        break;
    }
    case 5: { /* Spire */
        int segs = 6;
        sf_fvec3_t base_ring[6];
        float r_s = (hw + hd) * 0.35f;
        sf_fvec3_t center_base = {cx, y_base, cz};
        for (int s = 0; s < segs; s++) {
            float a_s = (float)s / segs * 3.14159f * 2.f;
            base_ring[s] = (sf_fvec3_t){cx + r_s*cosf(a_s), y_base, cz + r_s*sinf(a_s)};
        }
        sf_fvec3_t apex = {cx, y_base + rh, cz};
        for (int s = 0; s < segs; s++) {
            int ns = (s+1) % segs;
            sf_fvec3_t out = {
                (base_ring[s].x+base_ring[ns].x)*0.5f - cx,
                0.f,
                (base_ring[s].z+base_ring[ns].z)*0.5f - cz
            };
            bk_tri(o, base_ring[s], apex, base_ring[ns], out, tile);
        }
        /* Bottom cap */
        for (int s = 0; s < segs; s++) {
            int ns = (s+1) % segs;
            bk_tri(o, center_base, base_ring[s], base_ring[ns], DOWN, tile);
        }
        break;
    }
    }
}

/* Sync an entity's obj metadata from its backing sf_obj_t after geometry rebuild. */
static void bk_sync_enti(sf_enti_t *e, sf_obj_t *o) {
    if (!e || !o) return;
    sf_obj_recompute_bs(o);
    e->obj.v_cnt   = o->v_cnt;
    e->obj.vt_cnt  = o->vt_cnt;
    e->obj.f_cnt   = o->f_cnt;
    e->obj.bs_center = o->bs_center;
    e->obj.bs_radius = o->bs_radius;
}

static void sfgen_generate_building(void) {
    if (!g_ck_obj || !g_ck_enti) return;

    /* Clear all building mesh objects */
    sf_obj_t *objs[] = { g_ck_obj, g_ck_obj_win, g_ck_obj_ledge, g_ck_obj_roof };
    for (int i = 0; i < 4; i++) {
        if (objs[i]) { objs[i]->v_cnt=0; objs[i]->vt_cnt=0; objs[i]->f_cnt=0; objs[i]->src_path=NULL; }
    }

    uint32_t seed = (uint32_t)(bk_seed * 13337.f) + 1u;
    #define BK_RAND() (seed = seed * 1664525u + 1013904223u, (float)(seed >> 8) / 16777216.f)

    int floors = (int)(bk_floors + 0.5f);
    if (floors < 1) floors = 1; if (floors > 10) floors = 10;
    int n_cols = (int)(bk_win_cols + 0.5f);
    if (n_cols < 1) n_cols = 1; if (n_cols > 8) n_cols = 8;
    float eff_inset = bk_win_inset; if (eff_inset < 0.05f) eff_inset = 0.05f;
    float tile = 1.5f;
    float cur_w = bk_width, cur_d = bk_depth;
    float cur_y = 0.f, cx = 0.f, cz = 0.f;
    int door_col = n_cols / 2;

    /* Objects to route each face type into */
    sf_obj_t *obj_wall  = g_ck_obj;
    sf_obj_t *obj_win   = g_ck_obj_win   ? g_ck_obj_win   : g_ck_obj;
    sf_obj_t *obj_ledge = g_ck_obj_ledge ? g_ck_obj_ledge : g_ck_obj;

    static const sf_fvec3_t UP = {0.f, 1.f, 0.f};
    static const sf_fvec3_t DN = {0.f,-1.f, 0.f};

    for (int fl = 0; fl < floors; fl++) {
        float hw = cur_w * 0.5f, hd = cur_d * 0.5f;
        float fh = bk_floor_h;

        sf_fvec3_t corners[4] = {
            {cx-hw, cur_y, cz+hd},
            {cx+hw, cur_y, cz+hd},
            {cx+hw, cur_y, cz-hd},
            {cx-hw, cur_y, cz-hd},
        };

        for (int ww = 0; ww < 4; ww++) {
            sf_fvec3_t wbl = corners[ww];
            sf_fvec3_t wbr = corners[(ww+1) % 4];
            sf_fvec3_t right_v = {wbr.x-wbl.x, 0.f, wbr.z-wbl.z};
            float wall_w = sqrtf(right_v.x*right_v.x + right_v.z*right_v.z);
            right_v.x /= wall_w; right_v.z /= wall_w;
            /* outward wall normal: cross(right_v, up) */
            sf_fvec3_t nrm = {-right_v.z, 0.f, right_v.x};
            /* inward offset for window/door depth */
            sf_fvec3_t inv = {-nrm.x*eff_inset, 0.f, -nrm.z*eff_inset};
            /* left-reveal outward direction = -right_v */
            sf_fvec3_t lrev_out = { right_v.x, 0.f,  right_v.z};
            sf_fvec3_t rrev_out = {-right_v.x, 0.f, -right_v.z};

            #define BK_WP(u,v)  ((sf_fvec3_t){wbl.x+right_v.x*(u),(wbl.y+(v)),wbl.z+right_v.z*(u)})
            #define BK_WPI(u,v) ((sf_fvec3_t){wbl.x+right_v.x*(u)+inv.x,(wbl.y+(v)),wbl.z+right_v.z*(u)+inv.z})

            int is_front = (ww == 0);
            float margin_side = (1.f - bk_win_size) * 0.5f;
            if (margin_side < 0.05f) margin_side = 0.05f;
            if (margin_side > 0.45f) margin_side = 0.45f;

            for (int col = 0; col < n_cols; col++) {
                float u0 = (float)col / n_cols * wall_w;
                float u3 = (float)(col+1) / n_cols * wall_w;
                float col_ww = u3 - u0;
                bool is_door = (is_front && fl == 0 && col == door_col);
                float u1, u2, v1, v2;
                if (is_door) {
                    u1 = u0 + col_ww * 0.15f;
                    u2 = u3 - col_ww * 0.15f;
                    v1 = 0.f;
                    v2 = fh * 0.90f;
                } else {
                    u1 = u0 + col_ww * margin_side;
                    u2 = u3 - col_ww * margin_side;
                    v1 = fh * 0.20f;
                    v2 = fh * 0.80f;
                }
                float v0 = 0.f, v3 = fh;

                /* 4 solid wall strips around opening (wall mesh) */
                bk_quad(obj_wall, BK_WP(u0,v0), BK_WP(u3,v0), BK_WP(u3,v1), BK_WP(u0,v1), nrm, tile);
                bk_quad(obj_wall, BK_WP(u0,v2), BK_WP(u3,v2), BK_WP(u3,v3), BK_WP(u0,v3), nrm, tile);
                bk_quad(obj_wall, BK_WP(u0,v1), BK_WP(u1,v1), BK_WP(u1,v2), BK_WP(u0,v2), nrm, tile);
                bk_quad(obj_wall, BK_WP(u2,v1), BK_WP(u3,v1), BK_WP(u3,v2), BK_WP(u2,v2), nrm, tile);

                /* Opening reveals (window mesh): sill, lintel, left, right, back */
                if (!is_door) {
                    /* Sill: bottom horizontal face, visible from above */
                    bk_quad(obj_win, BK_WPI(u1,v1), BK_WPI(u2,v1), BK_WP(u2,v1), BK_WP(u1,v1), UP, tile);
                    /* Lintel: top horizontal face, visible from below */
                    bk_quad(obj_win, BK_WP(u1,v2),  BK_WP(u2,v2),  BK_WPI(u2,v2), BK_WPI(u1,v2), DN, tile);
                } else {
                    /* Door: lintel only (no sill at ground level) */
                    bk_quad(obj_win, BK_WP(u1,v2), BK_WP(u2,v2), BK_WPI(u2,v2), BK_WPI(u1,v2), DN, tile);
                }
                /* Left reveal faces -right_v */
                bk_quad(obj_win, BK_WP(u1,v1), BK_WP(u1,v2), BK_WPI(u1,v2), BK_WPI(u1,v1), lrev_out, tile);
                /* Right reveal faces +right_v */
                bk_quad(obj_win, BK_WP(u2,v2), BK_WP(u2,v1), BK_WPI(u2,v1), BK_WPI(u2,v2), rrev_out, tile);
                /* Back face (same outward as wall, visible from outside looking in) */
                bk_quad(obj_win, BK_WPI(u1,v1), BK_WPI(u2,v1), BK_WPI(u2,v2), BK_WPI(u1,v2), nrm, tile);
            }
            #undef BK_WP
            #undef BK_WPI
        }

        /* Floor bottom cap (facing down) and top cap (facing up) */
        bk_quad(obj_wall, corners[0], corners[1], corners[2], corners[3], DN, tile);
        { sf_fvec3_t tc0={cx-hw,cur_y+fh,cz+hd}, tc1={cx+hw,cur_y+fh,cz+hd};
          sf_fvec3_t tc2={cx+hw,cur_y+fh,cz-hd}, tc3={cx-hw,cur_y+fh,cz-hd};
          bk_quad(obj_wall, tc0, tc1, tc2, tc3, UP, tile); }

        /* Floor ledge/cornice (ledge mesh) */
        if (bk_ledge > 0.01f) {
            float lh = bk_ledge * 0.6f;
            float lo = bk_ledge * 0.3f;
            float y_top = cur_y + fh;
            float hw2 = hw + lo, hd2 = hd + lo;
            sf_fvec3_t lc[4] = {
                {cx-hw2,y_top-lh,cz+hd2},{cx+hw2,y_top-lh,cz+hd2},
                {cx+hw2,y_top-lh,cz-hd2},{cx-hw2,y_top-lh,cz-hd2}
            };
            sf_fvec3_t lt[4] = {
                {cx-hw2,y_top,cz+hd2},{cx+hw2,y_top,cz+hd2},
                {cx+hw2,y_top,cz-hd2},{cx-hw2,y_top,cz-hd2}
            };
            sf_fvec3_t li[4] = {
                {cx-hw,y_top,cz+hd},{cx+hw,y_top,cz+hd},
                {cx+hw,y_top,cz-hd},{cx-hw,y_top,cz-hd}
            };
            static const sf_fvec3_t side_nrm[4] = {
                {0.f,0.f,1.f},{1.f,0.f,0.f},{0.f,0.f,-1.f},{-1.f,0.f,0.f}
            };
            for (int w = 0; w < 4; w++) {
                int n2 = (w+1)%4;
                /* Outer vertical face */
                bk_quad(obj_ledge, lc[w], lc[n2], lt[n2], lt[w], side_nrm[w], tile);
                /* Top angled/horizontal face (faces up) */
                bk_quad(obj_ledge, lt[w], lt[n2], li[n2], li[w], UP, tile);
            }
        }

        cur_y += fh;
        cur_w -= bk_taper * 2.f; if (cur_w < 0.5f) cur_w = 0.5f;
        cur_d -= bk_taper * 2.f; if (cur_d < 0.5f) cur_d = 0.5f;
        cx += (BK_RAND() * 2.f - 1.f) * bk_jitter;
        cz += (BK_RAND() * 2.f - 1.f) * bk_jitter;
    }

    /* --- ASYMMETRIC PROTRUSIONS --- */
    if (bk_asym > 0.01f) {
        int n_prot = (int)(bk_asym * 3.f + 0.5f);
        if (n_prot > 3) n_prot = 3;
        float base_hw = bk_width * 0.5f, base_hd = bk_depth * 0.5f;
        int floors_main = (int)(bk_floors + 0.5f);
        if (floors_main < 1) floors_main = 1;
        for (int pi = 0; pi < n_prot; pi++) {
            int wall_idx = (int)(BK_RAND() * 4.f) % 4;
            float p_t     = BK_RAND() * 0.5f + 0.25f;
            float p_hr    = BK_RAND() * 0.25f + 0.15f;
            float p_depth = (BK_RAND() * 0.45f + 0.2f) * fminf(bk_width, bk_depth) * 0.5f;
            int p_fl = 1 + (int)(BK_RAND() * (float)(floors_main - 1) + 0.5f);
            if (p_fl > floors_main) p_fl = floors_main;

            float wall_len;
            sf_fvec3_t wrv, wnrm, wbase;
            switch (wall_idx % 4) {
                case 0: wrv=(sf_fvec3_t){1,0,0}; wnrm=(sf_fvec3_t){0,0,1};
                        wbase=(sf_fvec3_t){-base_hw,0,base_hd}; wall_len=bk_width; break;
                case 1: wrv=(sf_fvec3_t){0,0,-1}; wnrm=(sf_fvec3_t){1,0,0};
                        wbase=(sf_fvec3_t){base_hw,0,base_hd}; wall_len=bk_depth; break;
                case 2: wrv=(sf_fvec3_t){-1,0,0}; wnrm=(sf_fvec3_t){0,0,-1};
                        wbase=(sf_fvec3_t){base_hw,0,-base_hd}; wall_len=bk_width; break;
                default: wrv=(sf_fvec3_t){0,0,1}; wnrm=(sf_fvec3_t){-1,0,0};
                        wbase=(sf_fvec3_t){-base_hw,0,-base_hd}; wall_len=bk_depth; break;
            }
            float p_hw_abs = p_hr * wall_len;
            float p_coff   = p_t * wall_len;
            if (p_coff - p_hw_abs < 0.1f) p_coff = p_hw_abs + 0.1f;
            if (p_coff + p_hw_abs > wall_len - 0.1f) p_coff = wall_len - p_hw_abs - 0.1f;
            float oc = p_coff - p_hw_abs, oc2 = p_coff + p_hw_abs;

            /* Protrusion footprint corners; side 2 (c2→c3) is the back — skip */
            float px_c[4] = {
                wbase.x+wrv.x*oc +wnrm.x*p_depth, wbase.x+wrv.x*oc2+wnrm.x*p_depth,
                wbase.x+wrv.x*oc2,                 wbase.x+wrv.x*oc
            };
            float pz_c[4] = {
                wbase.z+wrv.z*oc +wnrm.z*p_depth, wbase.z+wrv.z*oc2+wnrm.z*p_depth,
                wbase.z+wrv.z*oc2,                 wbase.z+wrv.z*oc
            };

            for (int pfl = 0; pfl < p_fl; pfl++) {
                float py = (float)pfl * bk_floor_h;
                float pfh = bk_floor_h;
                for (int side = 0; side < 4; side++) {
                    if (side == 2) continue;  /* skip inner back face */
                    int nside = (side+1)%4;
                    sf_fvec3_t pwbl = {px_c[side],  py, pz_c[side]};
                    sf_fvec3_t pwbr = {px_c[nside], py, pz_c[nside]};
                    sf_fvec3_t prv  = {pwbr.x-pwbl.x, 0.f, pwbr.z-pwbl.z};
                    float wlen = sqrtf(prv.x*prv.x + prv.z*prv.z);
                    if (wlen < 0.01f) continue;
                    prv.x /= wlen; prv.z /= wlen;
                    sf_fvec3_t pnrm = {-prv.z, 0.f, prv.x};
                    sf_fvec3_t pinv = {-pnrm.x*eff_inset, 0.f, -pnrm.z*eff_inset};
                    sf_fvec3_t lro  = { prv.x, 0.f,  prv.z};
                    sf_fvec3_t rro  = {-prv.x, 0.f, -prv.z};

                    float margin_p = (1.f - bk_win_size) * 0.5f;
                    if (margin_p < 0.05f) margin_p = 0.05f;
                    if (margin_p > 0.45f) margin_p = 0.45f;
                    int p_ncols = (wlen > pfh * 0.6f) ? 1 : 0;
                    if (p_ncols < 1) p_ncols = 1;

                    #define PWP(u,v)  ((sf_fvec3_t){pwbl.x+prv.x*(u), pwbl.y+(v), pwbl.z+prv.z*(u)})
                    #define PWPI(u,v) ((sf_fvec3_t){pwbl.x+prv.x*(u)+pinv.x, pwbl.y+(v), pwbl.z+prv.z*(u)+pinv.z})
                    for (int col = 0; col < p_ncols; col++) {
                        float u0 = (float)col / p_ncols * wlen;
                        float u3 = (float)(col+1) / p_ncols * wlen;
                        float cww = u3 - u0;
                        float u1 = u0 + cww*margin_p, u2 = u3 - cww*margin_p;
                        float v1 = pfh*0.20f, v2 = pfh*0.80f, v3 = pfh;
                        bk_quad(obj_wall, PWP(u0,0), PWP(u3,0), PWP(u3,v1), PWP(u0,v1), pnrm, tile);
                        bk_quad(obj_wall, PWP(u0,v2), PWP(u3,v2), PWP(u3,v3), PWP(u0,v3), pnrm, tile);
                        bk_quad(obj_wall, PWP(u0,v1), PWP(u1,v1), PWP(u1,v2), PWP(u0,v2), pnrm, tile);
                        bk_quad(obj_wall, PWP(u2,v1), PWP(u3,v1), PWP(u3,v2), PWP(u2,v2), pnrm, tile);
                        bk_quad(obj_win,  PWP(u1,v1),  PWP(u2,v1),  PWPI(u2,v1), PWPI(u1,v1), UP,   tile);
                        bk_quad(obj_win,  PWP(u1,v2),  PWP(u2,v2),  PWPI(u2,v2), PWPI(u1,v2), DN,   tile);
                        bk_quad(obj_win,  PWP(u1,v1),  PWP(u1,v2),  PWPI(u1,v2), PWPI(u1,v1), lro,  tile);
                        bk_quad(obj_win,  PWP(u2,v2),  PWP(u2,v1),  PWPI(u2,v1), PWPI(u2,v2), rro,  tile);
                        bk_quad(obj_win,  PWPI(u1,v1), PWPI(u2,v1), PWPI(u2,v2), PWPI(u1,v2), pnrm, tile);
                    }
                    #undef PWP
                    #undef PWPI
                }
                /* Ledge between protrusion floors */
                if (bk_ledge > 0.01f && pfl < p_fl - 1) {
                    float lh = bk_ledge * 0.6f, lo = bk_ledge * 0.3f;
                    float ytop = py + bk_floor_h;
                    for (int side = 0; side < 4; side++) {
                        if (side == 2) continue;
                        int nside = (side+1)%4;
                        sf_fvec3_t a0 = {px_c[side],  ytop-lh, pz_c[side]};
                        sf_fvec3_t a1 = {px_c[nside], ytop-lh, pz_c[nside]};
                        sf_fvec3_t b0 = {a0.x, ytop, a0.z}, b1 = {a1.x, ytop, a1.z};
                        sf_fvec3_t rr2 = {a1.x-a0.x, 0.f, a1.z-a0.z};
                        float rl2 = sqrtf(rr2.x*rr2.x+rr2.z*rr2.z);
                        if (rl2 < 0.001f) continue;
                        rr2.x /= rl2; rr2.z /= rl2;
                        sf_fvec3_t pnrm2 = {-rr2.z, 0.f, rr2.x};
                        sf_fvec3_t ao0={a0.x+pnrm2.x*lo, ytop-lh, a0.z+pnrm2.z*lo};
                        sf_fvec3_t ao1={a1.x+pnrm2.x*lo, ytop-lh, a1.z+pnrm2.z*lo};
                        sf_fvec3_t bo0={ao0.x, ytop, ao0.z}, bo1={ao1.x, ytop, ao1.z};
                        bk_quad(obj_ledge, ao0, ao1, bo1, bo0, pnrm2, tile);
                        bk_quad(obj_ledge, bo0, bo1, b1,  b0,  UP,    tile);
                    }
                }
            }
            /* Protrusion roof */
            if (g_ck_obj_roof) {
                float prx = wbase.x + wrv.x*p_coff + wnrm.x*p_depth*0.5f;
                float prz = wbase.z + wrv.z*p_coff + wnrm.z*p_depth*0.5f;
                float pry = (float)p_fl * bk_floor_h;
                bk_emit_roof(g_ck_obj_roof, prx, prz, pry, p_hw_abs*2.f, p_depth, tile);
            }
        }
    }

    #undef BK_RAND

    /* Emit main roof */
    if (g_ck_obj_roof)
        bk_emit_roof(g_ck_obj_roof, cx, cz, cur_y, cur_w, cur_d, tile);

    /* Sync all entity metadata */
    bk_sync_enti(g_ck_enti,      g_ck_obj);
    bk_sync_enti(g_ck_enti_win,  g_ck_obj_win);
    bk_sync_enti(g_ck_enti_ledge, g_ck_obj_ledge);
    bk_sync_enti(g_ck_enti_roof, g_ck_obj_roof);
}

/* ============================================================
   SFGEN — texture scanners
   ============================================================ */
static void sfgen_scan_bark(void) {
    const char *dirs[]={SF_ASSET_PATH"/sf_textures/128x128/Wood",
                        SF_ASSET_PATH"/sf_textures/128x128/Misc",NULL};
    g_ct_tnc=0;
    for (int d=0;dirs[d]&&g_ct_tnc<SFGEN_MAX_TEX;d++){
        DIR *dir=opendir(dirs[d]); if(!dir) continue;
        struct dirent *e;
        while ((e=readdir(dir))&&g_ct_tnc<SFGEN_MAX_TEX){
            if (e->d_name[0]=='.') continue;
            const char *dot=strrchr(e->d_name,'.');
            if (!dot||strcmp(dot,".bmp")!=0) continue;
            snprintf(g_ct_tname[g_ct_tnc],64,"%s",e->d_name);
            g_ct_titem[g_ct_tnc]=g_ct_tname[g_ct_tnc]; g_ct_tnc++;
        }
        closedir(dir);
    }
}
static sf_tex_t *sfgen_ensure_bark(int idx) {
    if (idx<0||idx>=g_ct_tnc) return NULL;
    char nm[64]; snprintf(nm,sizeof(nm),"%s",g_ct_tname[idx]);
    char *dot=strrchr(nm,'.'); if(dot)*dot='\0';
    sf_tex_t *t=sf_get_texture_(&g_sfgen_ctx,nm,false);
    if (!t) t=sf_load_texture_bmp(&g_sfgen_ctx,g_ct_tname[idx],nm);
    return t;
}
static void sfgen_apply_bark(void) {
    if (!g_ct_enti) return;
    sf_tex_t *t=sfgen_ensure_bark(g_ct_tsel);
    if (t){g_ct_enti->tex=t; g_ct_enti->tex_scale=(sf_fvec2_t){1.f,1.f};}
}

static void sfgen_scan_stone(void) {
    const char *dirs[]={SF_ASSET_PATH"/sf_textures/128x128/Stone",
                        SF_ASSET_PATH"/sf_textures/128x128/Misc",NULL};
    g_cr_tnc=0;
    for (int d=0;dirs[d]&&g_cr_tnc<SFGEN_MAX_TEX;d++){
        DIR *dir=opendir(dirs[d]); if(!dir) continue;
        struct dirent *e;
        while ((e=readdir(dir))&&g_cr_tnc<SFGEN_MAX_TEX){
            if (e->d_name[0]=='.') continue;
            const char *dot=strrchr(e->d_name,'.');
            if (!dot||strcmp(dot,".bmp")!=0) continue;
            snprintf(g_cr_tname[g_cr_tnc],64,"%s",e->d_name);
            g_cr_titem[g_cr_tnc]=g_cr_tname[g_cr_tnc]; g_cr_tnc++;
        }
        closedir(dir);
    }
}
static sf_tex_t *sfgen_ensure_stone(int idx) {
    if (idx<0||idx>=g_cr_tnc) return NULL;
    char nm[64]; snprintf(nm,sizeof(nm),"%s",g_cr_tname[idx]);
    char *dot=strrchr(nm,'.'); if(dot)*dot='\0';
    sf_tex_t *t=sf_get_texture_(&g_sfgen_ctx,nm,false);
    if (!t) t=sf_load_texture_bmp(&g_sfgen_ctx,g_cr_tname[idx],nm);
    return t;
}
static void sfgen_apply_stone(void) {
    if (!g_cr_enti) return;
    sf_tex_t *t=sfgen_ensure_stone(g_cr_tsel);
    if (t){g_cr_enti->tex=t; g_cr_enti->tex_scale=(sf_fvec2_t){1.f,1.f};}
}
static void sfgen_apply_bldg_tex(void) {
    if (g_ck_enti && g_cr_tnc > 0) {
        sf_tex_t *t = sfgen_ensure_stone(g_ck_tsel);
        if (t) { g_ck_enti->tex = t; g_ck_enti->tex_scale = (sf_fvec2_t){1.f,1.f}; }
    }
    if (g_ck_enti_roof && g_cr_tnc > 0) {
        sf_tex_t *t = sfgen_ensure_stone(g_ck_rtsel);
        if (t) { g_ck_enti_roof->tex = t; g_ck_enti_roof->tex_scale = (sf_fvec2_t){1.f,1.f}; }
    }
}

static void sfgen_load_sprites(void) {
    const char *dir_path=SF_ASSET_PATH"/sf_sprites";
    DIR *dir=opendir(dir_path); if(!dir) return;
    struct dirent *e;
    while ((e=readdir(dir))&&g_ct_snc<SFGEN_MAX_SPR){
        if (e->d_name[0]=='.') continue;
        const char *dot=strrchr(e->d_name,'.');
        if (!dot||strcmp(dot,".bmp")!=0) continue;
        int nlen=(int)(dot-e->d_name);
        snprintf(g_ct_sname[g_ct_snc],64,"%.*s",nlen,e->d_name);
        g_ct_sitem[g_ct_snc]=g_ct_sname[g_ct_snc];
        sf_tex_t *t=sf_load_texture_bmp(&g_sfgen_ctx,e->d_name,g_ct_sname[g_ct_snc]);
        char sprname[80]; snprintf(sprname,sizeof(sprname),"spr_%s",g_ct_sname[g_ct_snc]);
        g_ct_sprites[g_ct_snc]=t?sf_load_sprite(&g_sfgen_ctx,sprname,1.f,0.7f,1,g_ct_sname[g_ct_snc]):NULL;
        g_ct_snc++;
    }
    closedir(dir);
    if (g_ct_snc>0&&g_ct_sprites[0]) g_sfgen_leaf=g_ct_sprites[0];
}

/* ============================================================
   SFGEN — copy file helper
   ============================================================ */
/* ============================================================
   SFGEN — save / install
   ============================================================ */
static void sfgen_save_tree(void) {
    if (!g_ct_obj) return;
    const char *sl=strrchr(g_sfgen_tree_sff,'/');
    char base[256]; snprintf(base,sizeof(base),"%s",sl?sl+1:g_sfgen_tree_sff);
    char *bdot=strrchr(base,'.'); if(bdot)*bdot='\0';
    char gen_id[64];
    sf_gen_asset_id(gen_id, sizeof(gen_id));
    sf_gen_save_obj(&g_sfgen_ctx, g_ct_obj, gen_id);
    FILE *f=fopen(g_sfgen_tree_sff,"w");
    if (!f) return;
    fprintf(f,"# Saffron Tree Model\n\nmesh %s \"%s.obj\"\n\n",g_ct_obj->name,g_ct_obj->name);
    if (g_ct_enti&&g_ct_enti->tex&&g_ct_enti->tex->name)
        fprintf(f,"texture %s \"%s.bmp\"\n",g_ct_enti->tex->name,g_ct_enti->tex->name);
    if (g_sfgen_leaf&&g_sfgen_leaf->frame_count>0&&g_sfgen_leaf->frames[0]&&g_sfgen_leaf->frames[0]->name)
        fprintf(f,"texture %s \"%s.bmp\"\n",g_sfgen_leaf->frames[0]->name,g_sfgen_leaf->frames[0]->name);
    fprintf(f,"\n");
    if (g_sfgen_leaf&&g_sfgen_leaf->name){
        const char *ftex=(g_sfgen_leaf->frame_count>0&&g_sfgen_leaf->frames[0])?g_sfgen_leaf->frames[0]->name:"";
        fprintf(f,"sprite %s {\n    duration = %.2f\n    scale    = %.3f\n    frames   = [%s]\n}\n\n",
                g_sfgen_leaf->name,g_sfgen_leaf->frame_duration,g_sfgen_leaf->base_scale,ftex);
    }
    if (g_ct_enti&&g_ct_enti->name&&g_ct_enti->frame){
        sf_fvec3_t p=g_ct_enti->frame->pos,r=g_ct_enti->frame->rot,s=g_ct_enti->frame->scale;
        fprintf(f,"entity %s {\n    mesh=%s\n    pos=(%.3f,%.3f,%.3f)\n    rot=(%.3f,%.3f,%.3f)\n    scale=(%.3f,%.3f,%.3f)\n",
                g_ct_enti->name,g_ct_obj->name,p.x,p.y,p.z,r.x,r.y,r.z,s.x,s.y,s.z);
        if (g_ct_enti->tex&&g_ct_enti->tex->name)
            fprintf(f,"    texture=%s\n",g_ct_enti->tex->name);
        fprintf(f,"}\n\n");
    }
    for (int i=0;i<g_sfgen_ctx.sprite_3d_count;i++){
        sf_sprite_3_t *b=&g_sfgen_ctx.sprite_3ds[i];
        if (!b->sprite||!b->sprite->name) continue;
        fprintf(f,"billboard %s {\n    sprite=%s\n    pos=(%.4f,%.4f,%.4f)\n    scale=%.4f\n    opacity=%.4f\n    angle=%.4f\n    normal=(%.4f,%.4f,%.4f)\n",
                b->name[0]?b->name:"bill",b->sprite->name,
                b->pos.x,b->pos.y,b->pos.z,b->scale,b->opacity,b->angle,
                b->normal.x,b->normal.y,b->normal.z);
        if (g_ct_enti&&g_ct_enti->name) fprintf(f,"    frame=%s\n",g_ct_enti->name);
        fprintf(f,"}\n\n");
    }
    fclose(f);
}

static void sfgen_save_rock(void) {
    if (!g_cr_obj) return;
    const char *sl=strrchr(g_sfgen_rock_sff,'/');
    char base[256]; snprintf(base,sizeof(base),"%s",sl?sl+1:g_sfgen_rock_sff);
    char *bdot=strrchr(base,'.'); if(bdot)*bdot='\0';
    char gen_id[64];
    sf_gen_asset_id(gen_id, sizeof(gen_id));
    sf_gen_save_obj(&g_sfgen_ctx, g_cr_obj, gen_id);
    FILE *f=fopen(g_sfgen_rock_sff,"w");
    if (!f) return;
    fprintf(f,"# Saffron Rock Model\n\nmesh %s \"%s.obj\"\n\n",g_cr_obj->name,g_cr_obj->name);
    if (g_cr_enti&&g_cr_enti->tex&&g_cr_enti->tex->name)
        fprintf(f,"texture %s \"%s.bmp\"\n\n",g_cr_enti->tex->name,g_cr_enti->tex->name);
    if (g_cr_enti&&g_cr_enti->name){
        sf_fvec3_t p=g_cr_enti->frame->pos,r=g_cr_enti->frame->rot,s=g_cr_enti->frame->scale;
        fprintf(f,"entity %s {\n    mesh=%s\n    pos=(%.3f,%.3f,%.3f)\n    rot=(%.3f,%.3f,%.3f)\n    scale=(%.3f,%.3f,%.3f)\n",
                g_cr_enti->name,g_cr_obj->name,p.x,p.y,p.z,r.x,r.y,r.z,s.x,s.y,s.z);
        if (g_cr_enti->tex&&g_cr_enti->tex->name)
            fprintf(f,"    texture=%s\n",g_cr_enti->tex->name);
        fprintf(f,"}\n");
    }
    fclose(f);
}

static void sfgen_install_tree(void) {
    sfgen_save_tree();
    const char *sl=strrchr(g_sfgen_tree_sff,'/');
    const char *fname = sl ? sl + 1 : g_sfgen_tree_sff;
    char sff_dst[512];
    snprintf(sff_dst, sizeof(sff_dst), SF_SRC_ASSET_PATH "/sf_sff/%s", fname);
    sf_copy_file(g_sfgen_tree_sff, sff_dst);
}

static void sfgen_install_rock(void) {
    sfgen_save_rock();
    const char *sl=strrchr(g_sfgen_rock_sff,'/');
    const char *fname = sl ? sl + 1 : g_sfgen_rock_sff;
    char sff_dst[512];
    snprintf(sff_dst, sizeof(sff_dst), SF_SRC_ASSET_PATH "/sf_sff/%s", fname);
    sf_copy_file(g_sfgen_rock_sff, sff_dst);
}

/* Helper: write a texture declaration if this entity has a texture not yet written */
static void bk_save_tex(FILE *f, sf_enti_t *e, const char *written[], int *n_written) {
    if (!e || !e->tex || !e->tex->name) return;
    for (int i = 0; i < *n_written; i++) if (strcmp(written[i], e->tex->name)==0) return;
    fprintf(f, "texture %s \"%s.bmp\"\n\n", e->tex->name, e->tex->name);
    written[(*n_written)++] = e->tex->name;
}
/* Helper: write an entity block parented to the main building entity */
static void bk_save_enti(FILE *f, sf_enti_t *e, sf_obj_t *o, const char *mesh_suffix,
                          const char *base, const char *parent_name) {
    if (!e || !e->name || !o || o->f_cnt == 0) return;
    char mesh_name[256]; snprintf(mesh_name, sizeof(mesh_name), "%s%s", base, mesh_suffix);
    sf_fvec3_t p=e->frame->pos, r=e->frame->rot, s=e->frame->scale;
    fprintf(f,"entity %s {\n    mesh=%s\n    pos=(%.3f,%.3f,%.3f)\n    rot=(%.3f,%.3f,%.3f)\n    scale=(%.3f,%.3f,%.3f)\n",
            e->name, mesh_name, p.x,p.y,p.z, r.x,r.y,r.z, s.x,s.y,s.z);
    if (e->tex && e->tex->name)
        fprintf(f,"    texture=%s\n", e->tex->name);
    if (parent_name)
        fprintf(f,"    frame=%s\n", parent_name);
    fprintf(f,"}\n");
}

static void sfgen_save_building(void) {
    if (!g_ck_obj) return;
    const char *sl=strrchr(g_sfgen_bldg_sff,'/');
    char base[256]; snprintf(base,sizeof(base),"%s",sl?sl+1:g_sfgen_bldg_sff);
    char *bdot=strrchr(base,'.'); if(bdot)*bdot='\0';
    char gen_id[64];
    sf_gen_asset_id(gen_id, sizeof(gen_id));

    /* Save each non-empty obj file to sf_generated */
    struct { sf_obj_t *o; const char *suf; } parts[] = {
        {g_ck_obj,       ""},
        {g_ck_obj_win,   "_win"},
        {g_ck_obj_ledge, "_ledge"},
        {g_ck_obj_roof,  "_roof"},
    };
    for (int i = 0; i < 4; i++) {
        if (!parts[i].o || parts[i].o->f_cnt == 0) continue;
        sf_gen_save_obj(&g_sfgen_ctx, parts[i].o, gen_id);
    }

    FILE *f=fopen(g_sfgen_bldg_sff,"w");
    if (!f) return;
    fprintf(f,"# Saffron Building Model\n\n");
    for (int i = 0; i < 4; i++) {
        if (!parts[i].o || parts[i].o->f_cnt == 0) continue;
        fprintf(f,"mesh %s \"%s.obj\"\n", parts[i].o->name, parts[i].o->name);
    }
    fprintf(f,"\n");

    /* Texture declarations (deduplicated) */
    const char *written[8]; int n_written = 0;
    bk_save_tex(f, g_ck_enti,      written, &n_written);
    bk_save_tex(f, g_ck_enti_win,  written, &n_written);
    bk_save_tex(f, g_ck_enti_ledge,written, &n_written);
    bk_save_tex(f, g_ck_enti_roof, written, &n_written);

    /* Entity declarations */
    const char *parent = g_ck_enti ? g_ck_enti->name : NULL;
    if (g_ck_enti && g_ck_enti->name) {
        /* Main wall entity (no parent) */
        sf_fvec3_t p=g_ck_enti->frame->pos, r=g_ck_enti->frame->rot, s=g_ck_enti->frame->scale;
        fprintf(f,"entity %s {\n    mesh=%s\n    pos=(%.3f,%.3f,%.3f)\n    rot=(%.3f,%.3f,%.3f)\n    scale=(%.3f,%.3f,%.3f)\n",
                g_ck_enti->name, base, p.x,p.y,p.z, r.x,r.y,r.z, s.x,s.y,s.z);
        if (g_ck_enti->tex && g_ck_enti->tex->name)
            fprintf(f,"    texture=%s\n", g_ck_enti->tex->name);
        fprintf(f,"}\n");
    }
    bk_save_enti(f, g_ck_enti_win,   g_ck_obj_win,   "_win",   base, parent);
    bk_save_enti(f, g_ck_enti_ledge, g_ck_obj_ledge, "_ledge", base, parent);
    bk_save_enti(f, g_ck_enti_roof,  g_ck_obj_roof,  "_roof",  base, parent);
    fclose(f);
}
static void sfgen_install_building(void) {
    sfgen_save_building();
    const char *sl=strrchr(g_sfgen_bldg_sff,'/');
    const char *fname = sl ? sl + 1 : g_sfgen_bldg_sff;
    char sff_dst[512];
    snprintf(sff_dst, sizeof(sff_dst), SF_SRC_ASSET_PATH "/sf_sff/%s", fname);
    sf_copy_file(g_sfgen_bldg_sff, sff_dst);
}

/* ============================================================
   SFGEN — update craft camera
   ============================================================ */
static void sfgen_update_camera(void) {
    float bldg_h = (g_sfgen_type==SFGEN_BLDG) ? bk_floors * bk_floor_h * 0.5f : 0.f;
    g_sfgen_orbit.target = (sf_fvec3_t){0.f, g_sfgen_type==SFGEN_TREE ? 3.5f : bldg_h, 0.f};
    sf_orbit_cam_apply(&g_sfgen_ctx, &g_sfgen_ctx.main_camera, &g_sfgen_orbit);
    /* Shift projection center to open area */
    float cx = (float)(g_w - 220) * 0.5f + 220.f;
    g_sfgen_ctx.main_camera.P = sf_make_psp_fmat4(60.f, (float)g_w/(float)g_h, 0.1f, 200.f);
    g_sfgen_ctx.main_camera.P.m[2][0] = 1.f - 2.f*cx/(float)g_w;
    g_sfgen_ctx.main_camera.is_proj_dirty = false;
}

/* ============================================================
   SFGEN — UI callbacks
   ============================================================ */
static void cb_sfgen_type_tree(sf_ctx_t *c, void *u) {
    (void)c;(void)u; g_sfgen_type=SFGEN_TREE; g_ui_dirty=true;
    g_sfgen_orbit.dist=12.f; g_sfgen_orbit.pitch=0.35f;
}
static void cb_sfgen_type_rock(sf_ctx_t *c, void *u) {
    (void)c;(void)u; g_sfgen_type=SFGEN_ROCK; g_ui_dirty=true;
    g_sfgen_orbit.dist=5.f; g_sfgen_orbit.pitch=0.35f;
}
static void cb_sfgen_type_bldg(sf_ctx_t *c, void *u) {
    (void)c;(void)u; g_sfgen_type=SFGEN_BLDG; g_ui_dirty=true;
    g_sfgen_orbit.dist=18.f; g_sfgen_orbit.pitch=0.30f;
}
static void cb_sfgen_save(sf_ctx_t *c, void *u) {
    (void)c;(void)u;
    if (g_sfgen_type==SFGEN_TREE) sfgen_save_tree();
    else if (g_sfgen_type==SFGEN_ROCK) sfgen_save_rock();
    else sfgen_save_building();
}
static void cb_sfgen_install(sf_ctx_t *c, void *u) {
    (void)c;(void)u;
    if (g_sfgen_type==SFGEN_TREE) sfgen_install_tree();
    else if (g_sfgen_type==SFGEN_ROCK) sfgen_install_rock();
    else sfgen_install_building();
}
static void cb_sfgen_save_install(sf_ctx_t *c, void *u) { cb_sfgen_save(c,u); cb_sfgen_install(c,u); }
static void cb_sfgen_rand3d(sf_ctx_t *c, void *u) { (void)c;(void)u; ct_rand3d=!ct_rand3d; }
static void cb_tab_sfgen(sf_ctx_t *c, void *u) { (void)c;(void)u; g_tab=TAB_SFGEN; g_picker_open=PICK_NONE; g_ui_dirty=true; }

/* Tree presets */
typedef struct { const char *name; float depth,branch,angle,len,taper,grav,wiggle,twist,tr,tl,ls,ld,lo; } sfgen_tree_preset_t;
static const sfgen_tree_preset_t k_tree_presets[] = {
    {"Oak",   4,3,35.f,.70f,.65f,.15f,.12f,137.5f,.22f,2.5f,1.f,4,.85f},
    {"Pine",  5,2,18.f,.76f,.72f,-.05f,.06f,137.5f,.16f,3.f,0.7f,2,.80f},
    {"Shrub", 3,4,48.f,.62f,.60f,.22f,.20f,137.5f,.30f,1.5f,1.1f,5,.90f},
};
#define N_TREE_PRESETS 3
static void sfgen_apply_tree_preset(const sfgen_tree_preset_t *p){
    ct_depth=p->depth; ct_branch=p->branch; ct_angle=p->angle;
    ct_len=p->len; ct_taper=p->taper; ct_grav=p->grav;
    ct_wiggle=p->wiggle; ct_twist=p->twist; ct_tr=p->tr;
    ct_tl=p->tl; ct_ls=p->ls; ct_ld=p->ld; ct_lo=p->lo;
}
static void cb_ctpre0(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_tree_preset(&k_tree_presets[0]);}
static void cb_ctpre1(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_tree_preset(&k_tree_presets[1]);}
static void cb_ctpre2(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_tree_preset(&k_tree_presets[2]);}

/* Rock presets */
typedef struct { const char *name; float subdiv,rough,freq,octaves,persist,flat,elongx,elongz,pointy,bump; } sfgen_rock_preset_t;
static const sfgen_rock_preset_t k_rock_presets[] = {
    {"Cobble",3,.20f,2.f,4,.50f,.30f,1.f,1.f,0.f,.30f},
    {"Boulder",3,.40f,1.5f,5,.55f,.10f,1.2f,.9f,0.f,.70f},
    {"Slab",3,.15f,2.5f,3,.45f,.65f,1.4f,1.2f,-.3f,.20f},
    {"Spire",3,.25f,1.8f,4,.50f,0.f,.6f,.6f,1.2f,.40f},
    {"Pebble",3,.08f,3.f,2,.40f,.10f,1.1f,.9f,0.f,.10f},
};
#define N_ROCK_PRESETS 5
static void sfgen_apply_rock_preset(const sfgen_rock_preset_t *p){
    cr_subdiv=p->subdiv; cr_rough=p->rough; cr_freq=p->freq;
    cr_octaves=p->octaves; cr_persist=p->persist; cr_flat=p->flat;
    cr_elongx=p->elongx; cr_elongz=p->elongz; cr_pointy=p->pointy; cr_bump=p->bump;
}
static void cb_crpre0(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_rock_preset(&k_rock_presets[0]);}
static void cb_crpre1(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_rock_preset(&k_rock_presets[1]);}
static void cb_crpre2(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_rock_preset(&k_rock_presets[2]);}
static void cb_crpre3(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_rock_preset(&k_rock_presets[3]);}
static void cb_crpre4(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_rock_preset(&k_rock_presets[4]);}

/* Building presets */
typedef struct { const char *name; float floors,floor_h,width,depth,taper,jitter,ledge,win_cols,win_size,win_inset,roof_type,roof_h,roof_scale; } sfgen_bldg_preset_t;
static const sfgen_bldg_preset_t k_bldg_presets[] = {
    {"Tower",  8, 2.5f, 3.f, 3.f, 0.05f,0.f, 0.15f,2.f,0.45f,0.35f,5.f,0.8f,1.1f},
    {"Keep",   4, 3.0f, 6.f, 5.f, 0.0f, 0.f, 0.20f,4.f,0.40f,0.30f,0.f,0.3f,1.0f},
    {"Pagoda", 5, 2.0f, 4.f, 4.f, 0.25f,0.f, 0.30f,2.f,0.50f,0.25f,4.f,0.4f,1.3f},
    {"Deco",   6, 2.8f, 5.f, 4.f, 0.12f,0.1f,0.12f,3.f,0.45f,0.35f,0.f,0.2f,1.0f},
    {"Chapel", 3, 3.5f, 4.f, 7.f, 0.0f, 0.f, 0.10f,3.f,0.50f,0.30f,1.f,0.6f,1.2f},
};
#define N_BLDG_PRESETS 5
static void sfgen_apply_bldg_preset(const sfgen_bldg_preset_t *p) {
    bk_floors=p->floors; bk_floor_h=p->floor_h; bk_width=p->width; bk_depth=p->depth;
    bk_taper=p->taper; bk_jitter=p->jitter; bk_ledge=p->ledge;
    bk_win_cols=p->win_cols; bk_win_size=p->win_size; bk_win_inset=p->win_inset;
    bk_roof_type=p->roof_type; bk_roof_h=p->roof_h; bk_roof_scale=p->roof_scale;
}
static void cb_bkpre0(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_bldg_preset(&k_bldg_presets[0]);}
static void cb_bkpre1(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_bldg_preset(&k_bldg_presets[1]);}
static void cb_bkpre2(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_bldg_preset(&k_bldg_presets[2]);}
static void cb_bkpre3(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_bldg_preset(&k_bldg_presets[3]);}
static void cb_bkpre4(sf_ctx_t*c,void*u){(void)c;(void)u;sfgen_apply_bldg_preset(&k_bldg_presets[4]);}

static float rndrf(float lo, float hi) { return lo + (float)(rand()&0x7fff)/32767.f*(hi-lo); }
static void cb_sfgen_rand_tree(sf_ctx_t *c, void *u) {
    (void)c;(void)u;
    ct_seed   = (float)(rand()%1000+1);
    ct_depth  = (float)(2 + rand()%4);
    ct_branch = (float)(2 + rand()%4);
    ct_angle  = rndrf(15.f,55.f);
    ct_len    = rndrf(0.50f,0.90f);
    ct_taper  = rndrf(0.50f,0.85f);
    ct_grav   = rndrf(-0.10f,0.30f);
    ct_wiggle = rndrf(0.00f,0.25f);
    ct_twist  = rndrf(110.f,160.f);
    ct_tr     = rndrf(0.10f,0.40f);
    ct_tl     = rndrf(1.5f,4.5f);
    ct_ls     = rndrf(0.6f,1.5f);
    ct_ld     = rndrf(2.f,7.f);
    ct_lo     = rndrf(0.6f,1.0f);
}
static void cb_sfgen_rand_rock(sf_ctx_t *c, void *u) {
    (void)c;(void)u;
    cr_seed    = (float)(rand()%1000+1);
    cr_subdiv  = (float)(2 + rand()%3);
    cr_rough   = rndrf(0.05f,0.55f);
    cr_freq    = rndrf(1.0f,4.0f);
    cr_octaves = (float)(2 + rand()%4);
    cr_persist = rndrf(0.30f,0.70f);
    cr_flat    = rndrf(-0.20f,0.60f);
    cr_elongx  = rndrf(0.6f,1.8f);
    cr_elongz  = rndrf(0.6f,1.8f);
    cr_pointy  = rndrf(-0.50f,1.50f);
    cr_bump    = rndrf(0.0f,0.80f);
}
static void cb_sfgen_rand_bldg(sf_ctx_t *c, void *u) {
    (void)c;(void)u;
    bk_seed       = (float)(rand()%1000+1);
    bk_floors     = (float)(2 + rand()%8);
    bk_floor_h    = rndrf(2.0f,4.0f);
    bk_width      = rndrf(2.5f,7.0f);
    bk_depth      = rndrf(2.5f,7.0f);
    bk_taper      = rndrf(0.0f,0.30f);
    bk_jitter     = rndrf(0.0f,0.15f);
    bk_ledge      = rndrf(0.0f,0.25f);
    bk_win_cols   = (float)(1 + rand()%5);
    bk_win_size   = rndrf(0.3f,0.7f);
    bk_win_inset  = rndrf(0.1f,0.5f);
    bk_roof_type  = (float)(rand()%5);
    bk_roof_h     = rndrf(0.2f,1.5f);
    bk_roof_scale = rndrf(0.8f,1.5f);
}

/* ============================================================
   SFGEN — init context (called once from main)
   ============================================================ */
static void sfgen_init(void) {
    sf_init(&g_sfgen_ctx, g_w, g_h);
    sf_camera_set_psp(&g_sfgen_ctx, &g_sfgen_ctx.main_camera, 60.f, 0.1f, 200.f);

    sf_light_t *key = sf_add_light(&g_sfgen_ctx, "key", SF_LIGHT_DIR,
        (sf_fvec3_t){1.0f, 0.95f, 0.88f}, 1.3f);
    if (key) sf_frame_look_at(key->frame, (sf_fvec3_t){1.f,-1.f,-0.5f});

    sf_light_t *fill = sf_add_light(&g_sfgen_ctx, "fill", SF_LIGHT_DIR,
        (sf_fvec3_t){0.45f, 0.55f, 0.65f}, 0.5f);
    if (fill) sf_frame_look_at(fill->frame, (sf_fvec3_t){-1.f,-0.3f,1.f});

    sfgen_scan_bark();
    sfgen_scan_stone();
    sfgen_load_sprites();

    /* Load row icons */
    static const char *k_tree_icn[14] = {
        "seed.bmp","depth.bmp","branches.bmp","angle.bmp",
        "length.bmp","taper.bmp","gravity.bmp","wiggle.bmp",
        "twist.bmp","trunk_r.bmp","trunk_l.bmp",
        "leaf_scl.bmp","leaf_cnt.bmp","leaf_opacity.bmp"
    };
    static const char *k_rock_icn[11] = {
        "rseed.bmp","subdiv.bmp","roughness.bmp","frequency.bmp",
        "octaves.bmp","persist.bmp","flatness.bmp",
        "elong_x.bmp","elong_z.bmp","pointy.bmp","bump.bmp"
    };
    for (int i = 0; i < 14; i++) {
        char nm[32]; snprintf(nm, sizeof(nm), "sfgen_ticn_%d", i);
        g_sfgen_tree_icons[i] = sf_load_texture_bmp(&g_sfgen_ctx, k_tree_icn[i], nm);
    }
    for (int i = 0; i < 11; i++) {
        char nm[32]; snprintf(nm, sizeof(nm), "sfgen_ricn_%d", i);
        g_sfgen_rock_icons[i] = sf_load_texture_bmp(&g_sfgen_ctx, k_rock_icn[i], nm);
    }
    static const char *k_bldg_icn[14] = {
        "bld_seed.bmp","bld_floors.bmp","bld_floor_h.bmp","bld_width.bmp",
        "bld_depth.bmp","bld_taper.bmp","bld_jitter.bmp","bld_ledge.bmp",
        "bld_win_cols.bmp","bld_win_size.bmp","bld_win_inset.bmp",
        "bld_roof_type.bmp","bld_roof_h.bmp","bld_roof_scale.bmp"
    };
    for (int i = 0; i < 14; i++) {
        char nm[32]; snprintf(nm, sizeof(nm), "sfgen_bicn_%d", i);
        g_sfgen_bldg_icons[i] = sf_load_texture_bmp(&g_sfgen_ctx, k_bldg_icn[i], nm);
    }

    /* Tree objects */
    g_ct_obj  = sf_obj_create_empty(&g_sfgen_ctx, "ctree", SFGEN_MAX_V, SFGEN_MAX_UV, SFGEN_MAX_F);
    g_ct_enti = sf_add_enti(&g_sfgen_ctx, g_ct_obj, "ctree_enti");
    if (g_ct_enti) sf_enti_set_pos(&g_sfgen_ctx, g_ct_enti, 0.f, 0.f, 0.f);

    /* Rock objects */
    g_cr_obj  = sf_obj_create_empty(&g_sfgen_ctx, "crock", SFGEN_MAX_V, SFGEN_MAX_UV, SFGEN_MAX_F);
    g_cr_enti = sf_add_enti(&g_sfgen_ctx, g_cr_obj, "crock_enti");
    if (g_cr_enti) sf_enti_set_pos(&g_sfgen_ctx, g_cr_enti, 0.f, 0.f, 0.f);

    /* Building objects */
    g_ck_obj  = sf_obj_create_empty(&g_sfgen_ctx, "cbldg",       SFGEN_MAX_V,   SFGEN_MAX_UV,   SFGEN_MAX_F);
    g_ck_enti = sf_add_enti(&g_sfgen_ctx, g_ck_obj, "cbldg_enti");
    if (g_ck_enti) sf_enti_set_pos(&g_sfgen_ctx, g_ck_enti, 0.f, 0.f, 0.f);

    g_ck_obj_win   = sf_obj_create_empty(&g_sfgen_ctx, "cbldg_win",   SFGEN_MAX_V/2, SFGEN_MAX_UV/2, SFGEN_MAX_F/2);
    g_ck_enti_win  = sf_add_enti(&g_sfgen_ctx, g_ck_obj_win, "cbldg_win_enti");
    if (g_ck_enti_win) {
        sf_enti_set_pos(&g_sfgen_ctx, g_ck_enti_win, 0.f, 0.f, 0.f);
        if (g_ck_enti && g_ck_enti->frame && g_ck_enti_win->frame)
            sf_frame_set_parent(g_ck_enti_win->frame, g_ck_enti->frame);
    }
    g_ck_obj_ledge  = sf_obj_create_empty(&g_sfgen_ctx, "cbldg_ledge", SFGEN_MAX_V/4, SFGEN_MAX_UV/4, SFGEN_MAX_F/4);
    g_ck_enti_ledge = sf_add_enti(&g_sfgen_ctx, g_ck_obj_ledge, "cbldg_ledge_enti");
    if (g_ck_enti_ledge) {
        sf_enti_set_pos(&g_sfgen_ctx, g_ck_enti_ledge, 0.f, 0.f, 0.f);
        if (g_ck_enti && g_ck_enti->frame && g_ck_enti_ledge->frame)
            sf_frame_set_parent(g_ck_enti_ledge->frame, g_ck_enti->frame);
    }
    g_ck_obj_roof  = sf_obj_create_empty(&g_sfgen_ctx, "cbldg_roof",  SFGEN_MAX_V/4, SFGEN_MAX_UV/4, SFGEN_MAX_F/4);
    g_ck_enti_roof = sf_add_enti(&g_sfgen_ctx, g_ck_obj_roof, "cbldg_roof_enti");
    if (g_ck_enti_roof) {
        sf_enti_set_pos(&g_sfgen_ctx, g_ck_enti_roof, 0.f, 0.f, 0.f);
        if (g_ck_enti && g_ck_enti->frame && g_ck_enti_roof->frame)
            sf_frame_set_parent(g_ck_enti_roof->frame, g_ck_enti->frame);
    }

    /* Initial generation */
    sfgen_apply_tree_preset(&k_tree_presets[0]);
    sfgen_generate_tree();
    sfgen_apply_bark();
    sfgen_apply_rock_preset(&k_rock_presets[0]);
    sfgen_generate_rock();
    sfgen_apply_stone();
    sfgen_apply_bldg_preset(&k_bldg_presets[0]);
    sfgen_generate_building();
    sfgen_apply_bldg_tex();

    sfgen_update_camera();
    g_sfgen_ready = true;
}

static void build_browser_panel(void); /* forward decl */

/* ============================================================
   SFGEN — build tab UI
   ============================================================ */
/* panel collapse state helpers */
/* Layout-aware collapsible panel helper */
static sf_ui_lmn_t* lay_panel_s(const char *title, int x, int y, int width) {
  sf_ui_lmn_t *p = sf_ui_lay_begin_panel(&sf_ctx, title, x, y, width, SF_UI_PANEL_FIXED);
  return p;
}

static void build_sfgen_tab(void) {
    const int PNL_X0=10, PNL_W=208, TOP=30;

/* Layout-based SFGEN_DF: icon + label + drag_float in 3 columns */
#define SFGEN_DF(lbl, tgt, step, icons, idx) do { \
    sf_ui_lay_col(&sf_ctx, 3, (float[]){0.08f, 0.37f, 0.55f}); \
    sf_tex_t **_ic = (sf_tex_t**)(icons); \
    if (_ic && _ic[idx]) sf_ui_lay_image(&sf_ctx, _ic[idx], true); \
    else sf_ui_lay_label(&sf_ctx, "", 0); \
    sf_ui_lay_label(&sf_ctx, lbl, 0); \
    sf_ui_lay_drag_float(&sf_ctx, &(tgt), step, NULL, NULL); \
} while(0)

    char *sff_path = (g_sfgen_type==SFGEN_TREE) ? g_sfgen_tree_sff :
                     (g_sfgen_type==SFGEN_ROCK) ? g_sfgen_rock_sff : g_sfgen_bldg_sff;

    /* ----- Type selector panel ----- */
    g_sfgen_panel = lay_panel_s("SFGEN", PNL_X0, TOP, PNL_W);
    { sf_pkd_clr_t sel_clr=0xFF88AAFF, nrm_clr=0xFFAAAAAA;
      sf_ui_lay_col(&sf_ctx, 3, NULL);
      sf_ui_lmn_t *bt=sf_ui_lay_button(&sf_ctx,"Tree",cb_sfgen_type_tree,NULL);
      sf_ui_lmn_t *br=sf_ui_lay_button(&sf_ctx,"Rock",cb_sfgen_type_rock,NULL);
      sf_ui_lmn_t *bb=sf_ui_lay_button(&sf_ctx,"Bldg",cb_sfgen_type_bldg,NULL);
      if (bt) bt->style.color_text=(g_sfgen_type==SFGEN_TREE)?sel_clr:nrm_clr;
      if (br) br->style.color_text=(g_sfgen_type==SFGEN_ROCK)?sel_clr:nrm_clr;
      if (bb) bb->style.color_text=(g_sfgen_type==SFGEN_BLDG)?sel_clr:nrm_clr; }
    int y = sf_ui_lay_end_panel(&sf_ctx);

    if (g_sfgen_type == SFGEN_TREE) {
        /* Presets panel */
        lay_panel_s("Presets", PNL_X0, y, PNL_W);
        sf_ui_lay_col(&sf_ctx, 2, (float[]){0.7f, 0.3f});
        sf_ui_lay_label(&sf_ctx, "Presets", 0);
        sf_ui_lay_button(&sf_ctx, "Rnd", cb_sfgen_rand_tree, NULL);
        { static void(*cbs[3])(sf_ctx_t*,void*)={cb_ctpre0,cb_ctpre1,cb_ctpre2};
          sf_ui_lay_col(&sf_ctx, N_TREE_PRESETS, NULL);
          for (int i=0;i<N_TREE_PRESETS;i++)
              sf_ui_lay_button(&sf_ctx, k_tree_presets[i].name, cbs[i], NULL); }
        y = sf_ui_lay_end_panel(&sf_ctx);

        /* Parameters panel */
        lay_panel_s("Parameters", PNL_X0, y, PNL_W);
        sf_ui_lay_row(&sf_ctx, 14);
        SFGEN_DF("Seed",    ct_seed,   1.f,   g_sfgen_tree_icons, 0);
        SFGEN_DF("Depth",   ct_depth,  0.5f,  g_sfgen_tree_icons, 1);
        SFGEN_DF("Branches",ct_branch, 1.f,   g_sfgen_tree_icons, 2);
        SFGEN_DF("Angle",   ct_angle,  1.f,   g_sfgen_tree_icons, 3);
        SFGEN_DF("Length",  ct_len,    0.01f,  g_sfgen_tree_icons, 4);
        SFGEN_DF("Taper",   ct_taper,  0.01f,  g_sfgen_tree_icons, 5);
        SFGEN_DF("Gravity", ct_grav,   0.01f,  g_sfgen_tree_icons, 6);
        SFGEN_DF("Wiggle",  ct_wiggle, 0.01f,  g_sfgen_tree_icons, 7);
        SFGEN_DF("Twist",   ct_twist,  1.f,   g_sfgen_tree_icons, 8);
        SFGEN_DF("Trunk R", ct_tr,     0.01f,  g_sfgen_tree_icons, 9);
        SFGEN_DF("Trunk L", ct_tl,     0.1f,  g_sfgen_tree_icons, 10);
        SFGEN_DF("Leaf Scl",ct_ls,     0.05f, g_sfgen_tree_icons, 11);
        SFGEN_DF("Leaf Cnt",ct_ld,     1.f,   g_sfgen_tree_icons, 12);
        SFGEN_DF("Leaf Opa",ct_lo,     0.01f,  g_sfgen_tree_icons, 13);
        y = sf_ui_lay_end_panel(&sf_ctx);

        /* Textures panel */
        lay_panel_s("Textures", PNL_X0, y, PNL_W);
        { const char *bn=(g_ct_enti&&g_ct_enti->tex&&g_ct_enti->tex->name)?g_ct_enti->tex->name:"(none)";
          sf_ui_lay_label(&sf_ctx, "Bark Tex", 0);
          sf_ui_lay_label(&sf_ctx, bn, 0xFFAAAAAA);
          sf_ui_lay_button(&sf_ctx, "Browse", cb_browse_sfgen_bark, NULL);
          const char *sn=(g_sfgen_leaf&&g_sfgen_leaf->name)?g_sfgen_leaf->name:"(none)";
          sf_ui_lay_label(&sf_ctx, "Leaf Spr", 0);
          sf_ui_lay_label(&sf_ctx, sn, 0xFFAAAAAA);
          sf_ui_lay_button(&sf_ctx, "Browse", cb_browse_sfgen_leaf, NULL); }
        y = sf_ui_lay_end_panel(&sf_ctx);

    } else if (g_sfgen_type == SFGEN_ROCK) {
        /* Presets panel */
        lay_panel_s("Presets", PNL_X0, y, PNL_W);
        sf_ui_lay_col(&sf_ctx, 2, (float[]){0.7f, 0.3f});
        sf_ui_lay_label(&sf_ctx, "Presets", 0);
        sf_ui_lay_button(&sf_ctx, "Rnd", cb_sfgen_rand_rock, NULL);
        { static void(*cbs[5])(sf_ctx_t*,void*)={cb_crpre0,cb_crpre1,cb_crpre2,cb_crpre3,cb_crpre4};
          sf_ui_lay_col(&sf_ctx, 3, NULL);
          for (int i=0;i<3;i++)
              sf_ui_lay_button(&sf_ctx, k_rock_presets[i].name, cbs[i], NULL);
          sf_ui_lay_col(&sf_ctx, 3, NULL);
          for (int i=3;i<N_ROCK_PRESETS;i++)
              sf_ui_lay_button(&sf_ctx, k_rock_presets[i].name, cbs[i], NULL);
          for (int i=N_ROCK_PRESETS;i<6;i++)
              sf_ui_lay_label(&sf_ctx, "", 0); }
        y = sf_ui_lay_end_panel(&sf_ctx);

        /* Parameters panel */
        lay_panel_s("Parameters", PNL_X0, y, PNL_W);
        sf_ui_lay_row(&sf_ctx, 14);
        SFGEN_DF("Seed",     cr_seed,    1.f,   g_sfgen_rock_icons, 0);
        SFGEN_DF("Subdiv",   cr_subdiv,  1.f,   g_sfgen_rock_icons, 1);
        SFGEN_DF("Roughness",cr_rough,   0.01f, g_sfgen_rock_icons, 2);
        SFGEN_DF("Frequency",cr_freq,    0.05f, g_sfgen_rock_icons, 3);
        SFGEN_DF("Octaves",  cr_octaves, 1.f,   g_sfgen_rock_icons, 4);
        SFGEN_DF("Persist",  cr_persist, 0.01f, g_sfgen_rock_icons, 5);
        SFGEN_DF("Flatness", cr_flat,    0.01f, g_sfgen_rock_icons, 6);
        SFGEN_DF("Elong X",  cr_elongx,  0.02f, g_sfgen_rock_icons, 7);
        SFGEN_DF("Elong Z",  cr_elongz,  0.02f, g_sfgen_rock_icons, 8);
        SFGEN_DF("Pointy",   cr_pointy,  0.05f, g_sfgen_rock_icons, 9);
        SFGEN_DF("Bump",     cr_bump,    0.02f, g_sfgen_rock_icons, 10);
        y = sf_ui_lay_end_panel(&sf_ctx);

        /* Textures panel */
        lay_panel_s("Textures", PNL_X0, y, PNL_W);
        { const char *stn=(g_cr_enti&&g_cr_enti->tex&&g_cr_enti->tex->name)?g_cr_enti->tex->name:"(none)";
          sf_ui_lay_label(&sf_ctx, "Stone Tex", 0);
          sf_ui_lay_label(&sf_ctx, stn, 0xFFAAAAAA);
          sf_ui_lay_button(&sf_ctx, "Browse", cb_browse_sfgen_stone, NULL); }
        y = sf_ui_lay_end_panel(&sf_ctx);

    } else { /* SFGEN_BLDG */
        /* Presets panel */
        lay_panel_s("Presets", PNL_X0, y, PNL_W);
        sf_ui_lay_col(&sf_ctx, 2, (float[]){0.7f, 0.3f});
        sf_ui_lay_label(&sf_ctx, "Presets", 0);
        sf_ui_lay_button(&sf_ctx, "Rnd", cb_sfgen_rand_bldg, NULL);
        { static void(*cbs[5])(sf_ctx_t*,void*)={cb_bkpre0,cb_bkpre1,cb_bkpre2,cb_bkpre3,cb_bkpre4};
          sf_ui_lay_col(&sf_ctx, 3, NULL);
          for (int i=0;i<3;i++)
              sf_ui_lay_button(&sf_ctx, k_bldg_presets[i].name, cbs[i], NULL);
          sf_ui_lay_col(&sf_ctx, 3, NULL);
          for (int i=3;i<N_BLDG_PRESETS;i++)
              sf_ui_lay_button(&sf_ctx, k_bldg_presets[i].name, cbs[i], NULL);
          for (int i=N_BLDG_PRESETS;i<6;i++)
              sf_ui_lay_label(&sf_ctx, "", 0); }
        y = sf_ui_lay_end_panel(&sf_ctx);

        /* Parameters panel */
        lay_panel_s("Parameters", PNL_X0, y, PNL_W);
        sf_ui_lay_row(&sf_ctx, 14);
        SFGEN_DF("Seed",     bk_seed,       1.f,   g_sfgen_bldg_icons, 0);
        SFGEN_DF("Floors",   bk_floors,     1.f,   g_sfgen_bldg_icons, 1);
        SFGEN_DF("Floor H",  bk_floor_h,    0.1f,  g_sfgen_bldg_icons, 2);
        SFGEN_DF("Width",    bk_width,      0.1f,  g_sfgen_bldg_icons, 3);
        SFGEN_DF("Depth",    bk_depth,      0.1f,  g_sfgen_bldg_icons, 4);
        SFGEN_DF("Taper",    bk_taper,      0.02f, g_sfgen_bldg_icons, 5);
        SFGEN_DF("Jitter",   bk_jitter,     0.02f, g_sfgen_bldg_icons, 6);
        SFGEN_DF("Ledge",    bk_ledge,      0.02f, g_sfgen_bldg_icons, 7);
        SFGEN_DF("Win Cols", bk_win_cols,   1.f,   g_sfgen_bldg_icons, 8);
        SFGEN_DF("Win Size", bk_win_size,   0.02f, g_sfgen_bldg_icons, 9);
        SFGEN_DF("Win Inset",bk_win_inset,  0.02f, g_sfgen_bldg_icons, 10);
        SFGEN_DF("Roof Type",bk_roof_type,  1.f,   g_sfgen_bldg_icons, 11);
        SFGEN_DF("Roof H",   bk_roof_h,     0.05f, g_sfgen_bldg_icons, 12);
        SFGEN_DF("Roof Scl", bk_roof_scale, 0.05f, g_sfgen_bldg_icons, 13);
        SFGEN_DF("Asym",     bk_asym,       0.05f, NULL, 0);
        y = sf_ui_lay_end_panel(&sf_ctx);

        /* Textures panel */
        lay_panel_s("Textures", PNL_X0, y, PNL_W);
        { const char *wn2=(g_ck_enti&&g_ck_enti->tex&&g_ck_enti->tex->name)?g_ck_enti->tex->name:"(none)";
          sf_ui_lay_label(&sf_ctx, "Wall Tex", 0);
          sf_ui_lay_label(&sf_ctx, wn2, 0xFFAAAAAA);
          sf_ui_lay_button(&sf_ctx, "Browse", cb_browse_sfgen_bwall, NULL);
          const char *rn2=(g_ck_enti_roof&&g_ck_enti_roof->tex&&g_ck_enti_roof->tex->name)?g_ck_enti_roof->tex->name:"(none)";
          sf_ui_lay_label(&sf_ctx, "Roof Tex", 0);
          sf_ui_lay_label(&sf_ctx, rn2, 0xFFAAAAAA);
          sf_ui_lay_button(&sf_ctx, "Browse", cb_browse_sfgen_broof, NULL);
          const char *wn3=(g_ck_enti_win&&g_ck_enti_win->tex&&g_ck_enti_win->tex->name)?g_ck_enti_win->tex->name:"(none)";
          sf_ui_lay_label(&sf_ctx, "Win Tex", 0);
          sf_ui_lay_label(&sf_ctx, wn3, 0xFFAAAAAA);
          sf_ui_lay_button(&sf_ctx, "Browse", cb_browse_sfgen_bwin, NULL);
          const char *ln2=(g_ck_enti_ledge&&g_ck_enti_ledge->tex&&g_ck_enti_ledge->tex->name)?g_ck_enti_ledge->tex->name:"(none)";
          sf_ui_lay_label(&sf_ctx, "Ledge Tex", 0);
          sf_ui_lay_label(&sf_ctx, ln2, 0xFFAAAAAA);
          sf_ui_lay_button(&sf_ctx, "Browse", cb_browse_sfgen_bledge, NULL); }
        y = sf_ui_lay_end_panel(&sf_ctx);
    }

    /* File panel */
    lay_panel_s("File", PNL_X0, y, PNL_W);
    sf_ui_lay_row(&sf_ctx, 18);
    sf_ui_lay_text_input(&sf_ctx, sff_path, SF_MAX_TEXT_INPUT_LEN, NULL, NULL);
    sf_ui_lay_row(&sf_ctx, 18);
    sf_ui_lay_button(&sf_ctx, "Save", cb_sfgen_save_install, NULL);
    sf_ui_lay_end_panel(&sf_ctx);

#undef SFGEN_DF
    build_browser_panel();
}

/* --- UI REBUILD --- */

static void cb_tab_sff(sf_ctx_t *c, void *ud)  { (void)c; (void)ud; g_tab = TAB_SFF;  g_ui_dirty = true; }
static void cb_tab_sfui(sf_ctx_t *c, void *ud) { (void)c; (void)ud; g_tab = TAB_SFUI; g_picker_open = PICK_NONE; g_ui_dirty = true; }

static void build_tab_bar(void) {
  int bw = 56, bh = 16, bx = 80, by = 0;
  const char *sff_lbl   = (g_tab == TAB_SFF)    ? "[SFF]"    : "SFF";
  const char *sfui_lbl  = (g_tab == TAB_SFUI)   ? "[SFUI]"   : "SFUI";
  const char *sfgen_lbl  = (g_tab == TAB_SFGEN) ? "[SFGEN]" : "SFGEN";
  sf_ui_lmn_t *b0 = sf_ui_add_button(&sf_ctx, sff_lbl,  (sf_ivec2_t){bx,             by}, (sf_ivec2_t){bx + bw,      by + bh}, cb_tab_sff,    NULL);
  sf_ui_lmn_t *b1 = sf_ui_add_button(&sf_ctx, sfui_lbl, (sf_ivec2_t){bx + bw + 4,   by}, (sf_ivec2_t){bx + 2*bw+4,  by + bh}, cb_tab_sfui,   NULL);
  sf_ui_lmn_t *b2 = sf_ui_add_button(&sf_ctx, sfgen_lbl, (sf_ivec2_t){bx + 2*bw + 8, by}, (sf_ivec2_t){bx + 3*bw+8,  by + bh}, cb_tab_sfgen, NULL);
  sf_pkd_clr_t transp = (sf_pkd_clr_t)0xFF111111;
  sf_pkd_clr_t blue   = (sf_pkd_clr_t)0xFF88AAFF;
  if (b0) { b0->style.color_base = transp; b0->style.color_hover = (sf_pkd_clr_t)0xFF222233; b0->style.color_active = (sf_pkd_clr_t)0xFF222233; if (g_tab == TAB_SFF)    b0->style.color_text = blue; }
  if (b1) { b1->style.color_base = transp; b1->style.color_hover = (sf_pkd_clr_t)0xFF222233; b1->style.color_active = (sf_pkd_clr_t)0xFF222233; if (g_tab == TAB_SFUI)   b1->style.color_text = blue; }
  if (b2) { b2->style.color_base = transp; b2->style.color_hover = (sf_pkd_clr_t)0xFF222233; b2->style.color_active = (sf_pkd_clr_t)0xFF222233; if (g_tab == TAB_SFGEN) b2->style.color_text = blue; }
}

/* build_section_sep removed — subsections are now proper butted panels */

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

static bool _outl_walk_cb(sf_frame_t *f, int depth, void *ud) {
  (void)ud;
  if (!f || g_outl_count >= OUTL_MAX) return false;
  if (f == sf_ctx.main_camera.frame) return true; /* skip internal frame but continue */
  void *p = NULL;
  sel_kind_t k = _outl_kind_of_frame(f, &p);
  /* include orphan frames (SEL_NONE) so the user can select and delete them */
  g_outl_items[g_outl_count++] = (outl_item_t){ k, p, f, depth };
  return true;
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
    /* Walk children of root (skip root itself which is the scene root) */
    for (sf_frame_t *c = root->first_child; c; c = c->next_sibling)
      sf_frame_walk(&sf_ctx, c, _outl_walk_cb, NULL);
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
    case SEL_ENTI:  g_sel        = (sf_enti_t*) it->ptr; break;
    case SEL_LIGHT: g_sel_light  = (sf_light_t*)it->ptr; break;
    case SEL_CAM:   g_sel_cam    = (sf_cam_t*)  it->ptr; break;
    case SEL_EMITR: g_sel_emitr  = (sf_emitr_t*)it->ptr; break;
    case SEL_NONE:  g_sel_frame  = it->frame;             break;
    default: break;
  }
  g_ui_dirty = true;
}

static void cb_outl_scroll_up  (sf_ctx_t *c, void *ud) { (void)c; (void)ud; g_outl_scroll -= 5; if (g_outl_scroll < 0) g_outl_scroll = 0; g_ui_dirty = true; }
static void cb_outl_scroll_dn  (sf_ctx_t *c, void *ud) { (void)c; (void)ud; g_outl_scroll += 5; g_ui_dirty = true; }


/* ============================================================
   Asset browser — UI panel (called from build_sff_tab)
   ============================================================ */
/* persistent string storage for per-cell labels built each UI rebuild */
static char s_cell_name[STUDIO_MAX_TEXS][8];
static char s_cell_badge[STUDIO_MAX_TEXS][8];

static void build_browser_panel(void) {
  if (g_picker_open == PICK_NONE) return;
  int bx0 = BROWSER_X0, bx1 = BROWSER_X1;
  int by0 = 30, by1 = g_h - 10;
  const char *title = (g_picker_open == PICK_MODEL) ? "Browse Models" :
                      (g_picker_open == PICK_SPRITE) ? "Browse Sprites" : "Browse Textures";
  g_picker_panel = sf_ui_add_panel(&sf_ctx, title, (sf_ivec2_t){bx0, by0}, (sf_ivec2_t){bx1, by1});
  if (g_picker_panel) g_picker_panel->panel.flags = SF_UI_PANEL_CLOSABLE;
  sf_ui_add_button(&sf_ctx, "R", (sf_ivec2_t){bx1-36, by0+1}, (sf_ivec2_t){bx1-18, by0+15},
                   cb_picker_refresh, NULL);
  int content_y0 = by0 + 24;
  int content_y1 = by1 - 26;
  int rows_vis = (content_y1 - content_y0) / BROWSER_CELL_H;
  if (rows_vis < 1) rows_vis = 1;
  g_picker_rows_vis = rows_vis;
  int n = picker_item_count();
  int total_rows = (n + BROWSER_COLS - 1) / BROWSER_COLS;
  int total_pages = (total_rows + rows_vis - 1) / rows_vis;
  if (total_pages < 1) total_pages = 1;
  int max_scroll = total_pages - 1;
  if (g_picker_scroll > max_scroll) g_picker_scroll = max_scroll;
  int first = g_picker_scroll * rows_vis * BROWSER_COLS;
  for (int row = 0; row < rows_vis; row++) {
    for (int col = 0; col < BROWSER_COLS; col++) {
      int idx = first + row * BROWSER_COLS + col;
      if (idx >= n) break;
      int cx = bx0 + 6 + col * (BROWSER_CELL_W + 2);
      int cy = content_y0 + row * BROWSER_CELL_H;
      int tx = cx + 4, ty = cy + 4;
      const char *lb = picker_item_label(idx);
      sf_tex_t *thumb = (idx < g_picker_thumbs_n) ? g_picker_thumbs[idx] : NULL;

      /* Cell click button — background color = hash-tint when no thumb */
      sf_pkd_clr_t base_clr = (sf_pkd_clr_t)0xFF2A2A2A;
      if (!thumb) {
        uint32_t hh = 0x811c9dc5u;
        for (const char *p = lb; *p; p++) hh = (hh ^ (unsigned char)*p) * 0x01000193u;
        uint32_t r = 0x20 + (hh & 0x3F), g2 = 0x20 + (hh>>8 & 0x3F), b2 = 0x20 + (hh>>16 & 0x3F);
        base_clr = (sf_pkd_clr_t)(0xFF000000u | (r<<16) | (g2<<8) | b2);
      }
      sf_ui_lmn_t *b = sf_ui_add_button(&sf_ctx, "",
        (sf_ivec2_t){cx, cy},
        (sf_ivec2_t){cx + BROWSER_CELL_W, cy + BROWSER_CELL_H - 2},
        cb_picker_pick, (void*)(intptr_t)idx);
      if (b) {
        b->style.color_base   = base_clr;
        b->style.color_hover  = (sf_pkd_clr_t)0xFF3A4A5A;
        b->style.color_active = (sf_pkd_clr_t)0xFF5A7ACA;
        b->style.draw_outline = true;
      }

      /* Thumbnail image (renders on top of button, hides when panel collapses) */
      if (thumb && thumb->px)
        sf_ui_add_image(&sf_ctx, thumb,
                        (sf_ivec2_t){tx, ty},
                        (sf_ivec2_t){tx + BROWSER_THUMB_SZ, ty + BROWSER_THUMB_SZ}, false);

      /* Filename label (truncated, no extension) */
      int si = idx < STUDIO_MAX_TEXS ? idx : STUDIO_MAX_TEXS - 1;
      { int k = 0;
        for (const char *p = lb; *p && *p != '.' && k < 5; p++) s_cell_name[si][k++] = *p;
        s_cell_name[si][k] = '\0'; }
      sf_ui_add_label(&sf_ctx, s_cell_name[si],
                      (sf_ivec2_t){tx, ty + BROWSER_THUMB_SZ + 2}, 0xFFCCCCCC);

      /* File-type badge label (top-left of thumbnail) */
      { const char *ext = strrchr(lb, '.');
        const char *badge = ext ? ext+1 : "?";
        int k = 0;
        for (const char *p = badge; *p && k < 7; p++) s_cell_badge[si][k++] = *p;
        s_cell_badge[si][k] = '\0'; }
      sf_ui_add_label(&sf_ctx, s_cell_badge[si],
                      (sf_ivec2_t){tx+1, ty+1}, 0xFF5A7ACA);
    }
  }
  static char s_bpageinfo[32];
  snprintf(s_bpageinfo, sizeof(s_bpageinfo), "%d/%d (%d)", g_picker_scroll + 1, total_pages, n);
  int sy = by1 - 24;
  sf_ui_add_button(&sf_ctx, "<", (sf_ivec2_t){bx0+6,  sy}, (sf_ivec2_t){bx0+30, sy+18}, cb_picker_scroll_up, NULL);
  sf_ui_add_label (&sf_ctx, s_bpageinfo, (sf_ivec2_t){bx0+34, sy+4}, 0xFFAAAAAA);
  sf_ui_add_button(&sf_ctx, ">", (sf_ivec2_t){bx1-30, sy}, (sf_ivec2_t){bx1-6,  sy+18}, cb_picker_scroll_dn, NULL);
}

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
    const char *kind_glyph = (it->kind == SEL_ENTI)  ? "[E]" : (it->kind == SEL_LIGHT) ? "[L]"
                           : (it->kind == SEL_CAM)   ? "[C]" : (it->kind == SEL_EMITR) ? "[M]" : "[F]";
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
      (it->kind == SEL_ENTI  && it->ptr   == g_sel)       ||
      (it->kind == SEL_LIGHT && it->ptr   == g_sel_light) ||
      (it->kind == SEL_CAM   && it->ptr   == g_sel_cam)   ||
      (it->kind == SEL_EMITR && it->ptr   == g_sel_emitr) ||
      (it->kind == SEL_NONE  && it->frame == g_sel_frame));
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
  sf_ui_lay_begin_panel(&sf_ctx, "Overlay", rx0, r, rx1 - rx0, 0);
  sf_ui_lay_row(&sf_ctx, 18);
  sf_ui_lay_checkbox(&sf_ctx, "frames",  g_dbg_frames, cb_dbg_frames, NULL);
  sf_ui_lay_checkbox(&sf_ctx, "lights",  g_dbg_lights, cb_dbg_lights, NULL);
  sf_ui_lay_checkbox(&sf_ctx, "cameras", g_dbg_cams,   cb_dbg_cams,   NULL);
  sf_ui_lay_end_panel(&sf_ctx);
}

static const char *k_light_items[2] = { "point", "dir" };
static int g_light_type_sel = 0;

static void cb_light_type(sf_ctx_t *ctx, void *ud) {
  (void)ctx; (void)ud;
  if (g_sel_kind == SEL_LIGHT && g_sel_light) {
    g_sel_light->type = (g_light_type_sel == 0) ? SF_LIGHT_POINT : SF_LIGHT_DIR;
  }
}

/* Create: icon-button grid via layout. Called within a layout panel. */
static void build_spawn_buttons(void) {
  sf_ui_lay_row(&sf_ctx, 34);
  sf_ui_lay_col(&sf_ctx, 5, NULL);
  for (int i = 0; i < 5; i++) {
    const char *label = g_spawn_icon_tex[i] ? "" : k_spawn_labels[i];
    g_spawn_btn_el[i] = sf_ui_lay_button(&sf_ctx, label, cb_spawn_kind, (void*)(intptr_t)i);
    if (g_spawn_icon_tex[i] && g_spawn_btn_el[i])
      sf_ui_add_image(&sf_ctx, g_spawn_icon_tex[i],
                      (sf_ivec2_t){g_spawn_btn_el[i]->v0.x+1, g_spawn_btn_el[i]->v0.y+1},
                      (sf_ivec2_t){g_spawn_btn_el[i]->v1.x-1, g_spawn_btn_el[i]->v1.y-1}, true);
  }
  sf_ui_lay_row(&sf_ctx, 34);
  sf_ui_lay_col(&sf_ctx, 5, NULL);
  for (int i = 5; i < 9; i++) {
    const char *label = g_spawn_icon_tex[i] ? "" : k_spawn_labels[i];
    g_spawn_btn_el[i] = sf_ui_lay_button(&sf_ctx, label, cb_spawn_kind, (void*)(intptr_t)i);
    if (g_spawn_icon_tex[i] && g_spawn_btn_el[i])
      sf_ui_add_image(&sf_ctx, g_spawn_icon_tex[i],
                      (sf_ivec2_t){g_spawn_btn_el[i]->v0.x+1, g_spawn_btn_el[i]->v0.y+1},
                      (sf_ivec2_t){g_spawn_btn_el[i]->v1.x-1, g_spawn_btn_el[i]->v1.y-1}, true);
  }
  /* fill remaining cols in the second row */
  sf_ui_lay_label(&sf_ctx, "", 0);
}


/* scale-blit an arbitrary sf_pkd_clr_t buffer (src w*h) into main cam at dest rect */
static void draw_cam_pip_overlay(void) {
  if (!g_cam_pip_visible || g_sel_kind != SEL_CAM || !g_sel_cam || !g_sel_cam->buffer) return;
  sf_draw_cam_pip_scaled(&sf_ctx, &sf_ctx.main_camera, g_sel_cam, g_cam_pip_pos,
                         g_cam_pip_size.x, g_cam_pip_size.y);
}

/* --- translate gizmo --- */

static void update_gizmo_geometry(void) {
  sf_gizmo_update(&sf_ctx, &sf_ctx.main_camera, &g_gizmo, sel_frame());
  g_gizmo.hover_axis = sf_gizmo_hit(&g_gizmo, g_mouse_x, g_mouse_y);
}

static void draw_gizmo(void) {
  sf_gizmo_render(&sf_ctx, &sf_ctx.main_camera, &g_gizmo);
}

static bool gizmo_begin_drag(int mx, int my) {
  sf_gz_axis_t a = sf_gizmo_hit(&g_gizmo, mx, my);
  if (a == SF_GZ_AX_NONE) return false;
  sf_gizmo_begin_drag(&g_gizmo, a, mx, my);
  return true;
}

static void gizmo_drag_update(int mx, int my) {
  if (g_gizmo.drag_axis == SF_GZ_AX_NONE) return;
  sf_gizmo_drag(&g_gizmo, mx, my);
  g_ui_dirty = true;
}

static void gizmo_end_drag(void) {
  sf_gizmo_end_drag(&g_gizmo);
}

/* draw icons over the spawn buttons (called after sf_render_ui) */

/* adds texture picker + scale for the selected entity. Called within a layout panel. */
static void build_texture_section(void) {
  const char *cur_name = (g_sel_kind == SEL_ENTI && g_sel && g_sel->tex && g_sel->tex->name)
                         ? g_sel->tex->name : "(none)";
  sf_ui_lay_label(&sf_ctx, cur_name, 0xFFAAAAAA);
  sf_ui_lay_row(&sf_ctx, 20);
  sf_ui_lay_col(&sf_ctx, 2, NULL);
  sf_ui_lay_button(&sf_ctx, "Browse", cb_browse_tex, NULL);
  sf_ui_lay_button(&sf_ctx, "Clear", cb_clear_tex, NULL);
  if (g_sel_kind == SEL_ENTI && g_sel) {
    sf_ui_lay_label(&sf_ctx, "scale u v", SF_CLR_WHITE);
    sf_ui_lay_col(&sf_ctx, 2, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel->tex_scale.x, 0.05f, NULL, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel->tex_scale.y, 0.05f, NULL, NULL);
  }
}

static int build_inspector(int y_start) {
  static char s_parentline[80];
  g_cam_pip_visible = false;
  const int X0 = 10, PNL_W = 210;

  /* ---- Create panel ---- */
  int y = y_start;
  lay_panel_s("Create", X0, y, PNL_W);
  build_spawn_buttons();
  sf_ui_lay_spacing(&sf_ctx, 6);
  sf_ui_lay_row(&sf_ctx, 20);
  sf_ui_lay_button(&sf_ctx, "Delete Selection", cb_delete, NULL);
  y = sf_ui_lay_end_panel(&sf_ctx);

  /* ---- Selection panel ---- */
  lay_panel_s("Selection", X0, y, PNL_W);

  sf_frame_t *sf = sel_frame();
  if (!sf) {
    sf_ui_lay_label(&sf_ctx, "(no selection)", 0xFFAAAAAA);
    y = sf_ui_lay_end_panel(&sf_ctx);
    return y;
  }

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
  sf_ui_lay_label(&sf_ctx, "name", SF_CLR_WHITE);
  sf_ui_lay_row(&sf_ctx, 20);
  sf_ui_lay_col(&sf_ctx, 2, (float[]){0.65f, 0.35f});
  sf_ui_lay_text_input(&sf_ctx, g_rename_buf, sizeof(g_rename_buf), NULL, NULL);
  sf_ui_lay_button(&sf_ctx, "Rename", cb_apply_rename, NULL);

  sf_ui_lay_label(&sf_ctx, "pos x y z", SF_CLR_WHITE);
  sf_ui_lay_col(&sf_ctx, 3, NULL);
  sf_ui_lay_drag_float(&sf_ctx, &sf->pos.x, 0.05f, cb_mark_dirty, NULL);
  sf_ui_lay_drag_float(&sf_ctx, &sf->pos.y, 0.05f, cb_mark_dirty, NULL);
  sf_ui_lay_drag_float(&sf_ctx, &sf->pos.z, 0.05f, cb_mark_dirty, NULL);

  sf_ui_lay_label(&sf_ctx, "rot x y z", SF_CLR_WHITE);
  sf_ui_lay_col(&sf_ctx, 3, NULL);
  sf_ui_lay_drag_float(&sf_ctx, &sf->rot.x, 0.01f, cb_mark_dirty, NULL);
  sf_ui_lay_drag_float(&sf_ctx, &sf->rot.y, 0.01f, cb_mark_dirty, NULL);
  sf_ui_lay_drag_float(&sf_ctx, &sf->rot.z, 0.01f, cb_mark_dirty, NULL);

  if (g_sel_kind == SEL_ENTI) {
    sf_ui_lay_col(&sf_ctx, 2, (float[]){0.6f, 0.4f});
    sf_ui_lay_label(&sf_ctx, "scale x y z", SF_CLR_WHITE);
    sf_ui_lay_checkbox(&sf_ctx, "lock", g_scale_lock, cb_scale_lock_tog, NULL);
    sf_ui_lay_col(&sf_ctx, 3, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &sf->scale.x, 0.01f, cb_scale_x, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &sf->scale.y, 0.01f, cb_scale_y, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &sf->scale.z, 0.01f, cb_scale_z, NULL);
    prim_meta_t *m = sel_meta();
    if (m && m->kind != PM_NONE) {
      switch (m->kind) {
        case PM_PLANE:
          sf_ui_lay_label(&sf_ctx, "plane sx sz", SF_CLR_WHITE);
          sf_ui_lay_col(&sf_ctx, 2, NULL);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[0], 0.05f, cb_regen_sel, NULL);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[1], 0.05f, cb_regen_sel, NULL);
          sf_ui_lay_col(&sf_ctx, 2, (float[]){0.2f, 0.8f});
          sf_ui_lay_label(&sf_ctx, "res", SF_CLR_WHITE);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[2], 1.0f, cb_regen_sel, NULL);
          break;
        case PM_BOX:
          sf_ui_lay_label(&sf_ctx, "box sx sy sz", SF_CLR_WHITE);
          sf_ui_lay_col(&sf_ctx, 3, NULL);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[0], 0.05f, cb_regen_sel, NULL);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[1], 0.05f, cb_regen_sel, NULL);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[2], 0.05f, cb_regen_sel, NULL);
          break;
        case PM_SPHERE:
          sf_ui_lay_col(&sf_ctx, 2, (float[]){0.3f, 0.7f});
          sf_ui_lay_label(&sf_ctx, "radius", SF_CLR_WHITE);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[0], 0.05f, cb_regen_sel, NULL);
          sf_ui_lay_col(&sf_ctx, 2, (float[]){0.3f, 0.7f});
          sf_ui_lay_label(&sf_ctx, "segs", SF_CLR_WHITE);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[1], 1.0f, cb_regen_sel, NULL);
          break;
        case PM_CYL:
          sf_ui_lay_label(&sf_ctx, "cyl r / h", SF_CLR_WHITE);
          sf_ui_lay_col(&sf_ctx, 2, NULL);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[0], 0.05f, cb_regen_sel, NULL);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[1], 0.05f, cb_regen_sel, NULL);
          sf_ui_lay_col(&sf_ctx, 2, (float[]){0.3f, 0.7f});
          sf_ui_lay_label(&sf_ctx, "segs", SF_CLR_WHITE);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[2], 1.0f, cb_regen_sel, NULL);
          break;
        case PM_TERRAIN:
          sf_ui_lay_label(&sf_ctx, "terrain sx sz", SF_CLR_WHITE);
          sf_ui_lay_col(&sf_ctx, 2, NULL);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[0], 0.5f, cb_regen_sel, NULL);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[1], 0.5f, cb_regen_sel, NULL);
          sf_ui_lay_col(&sf_ctx, 2, (float[]){0.2f, 0.8f});
          sf_ui_lay_label(&sf_ctx, "res", SF_CLR_WHITE);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[2], 1.0f, cb_regen_sel, NULL);
          sf_ui_lay_label(&sf_ctx, "amp / freq", SF_CLR_WHITE);
          sf_ui_lay_col(&sf_ctx, 2, NULL);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[3], 0.05f, cb_regen_sel, NULL);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[4], 0.005f, cb_regen_sel, NULL);
          sf_ui_lay_col(&sf_ctx, 2, (float[]){0.2f, 0.8f});
          sf_ui_lay_label(&sf_ctx, "oct", SF_CLR_WHITE);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[5], 1.0f, cb_regen_sel, NULL);
          sf_ui_lay_col(&sf_ctx, 2, (float[]){0.2f, 0.8f});
          sf_ui_lay_label(&sf_ctx, "seed", SF_CLR_WHITE);
          sf_ui_lay_drag_float(&sf_ctx, &m->p[6], 1.0f, cb_regen_sel, NULL);
          break;
        case PM_MODEL: {
          sf_ui_lay_label(&sf_ctx, "model", SF_CLR_WHITE);
          const char *mname = (m->model_idx >= 0 && m->model_idx < g_model_count)
                              ? g_model_items[m->model_idx] : "(none)";
          sf_ui_lay_label(&sf_ctx, mname, 0xFFAAAAAA);
          sf_ui_lay_row(&sf_ctx, 20);
          sf_ui_lay_button(&sf_ctx, "Browse", cb_browse_model, NULL);
          break;
        }
        default: break;
      }
    }
  } else if (g_sel_kind == SEL_LIGHT && g_sel_light) {
    g_light_type_sel = (g_sel_light->type == SF_LIGHT_DIR) ? 1 : 0;
    sf_ui_lay_label(&sf_ctx, "type", SF_CLR_WHITE);
    sf_ui_lay_row(&sf_ctx, 20);
    sf_ui_lay_dropdown(&sf_ctx, k_light_items, 2, &g_light_type_sel, cb_light_type, NULL);
    sf_ui_lay_label(&sf_ctx, "color r g b", SF_CLR_WHITE);
    sf_ui_lay_col(&sf_ctx, 3, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel_light->color.x, 0.01f, NULL, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel_light->color.y, 0.01f, NULL, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel_light->color.z, 0.01f, NULL, NULL);
    sf_ui_lay_label(&sf_ctx, "intensity", SF_CLR_WHITE);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel_light->intensity, 0.05f, NULL, NULL);
  } else if (g_sel_kind == SEL_EMITR && g_sel_emitr) {
    int ts = (g_sel_emitr->type == SF_EMITR_DIR) ? 0 : (g_sel_emitr->type == SF_EMITR_VOLUME) ? 2 : 1;
    static int _es;
    _es = ts;
    sf_ui_lay_label(&sf_ctx, "type", SF_CLR_WHITE);
    sf_ui_lay_row(&sf_ctx, 20);
    sf_ui_lay_dropdown(&sf_ctx, k_emitr_items, 3, &_es, NULL, NULL);
    g_sel_emitr->type = (_es == 0) ? SF_EMITR_DIR : (_es == 2) ? SF_EMITR_VOLUME : SF_EMITR_OMNI;
    sf_ui_lay_label(&sf_ctx, "rate / life", SF_CLR_WHITE);
    sf_ui_lay_col(&sf_ctx, 2, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel_emitr->spawn_rate, 0.5f, NULL, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel_emitr->particle_life, 0.05f, NULL, NULL);
    sf_ui_lay_label(&sf_ctx, "speed / spread", SF_CLR_WHITE);
    sf_ui_lay_col(&sf_ctx, 2, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel_emitr->speed, 0.05f, NULL, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel_emitr->spread, 0.01f, NULL, NULL);
    sf_ui_lay_label(&sf_ctx, "sprite", SF_CLR_WHITE);
    sf_ui_lay_label(&sf_ctx, (g_sel_emitr->sprite && g_sel_emitr->sprite->name) ? g_sel_emitr->sprite->name : "(none)", 0xFFAAAAAA);
    sf_ui_lay_row(&sf_ctx, 20);
    sf_ui_lay_button(&sf_ctx, "Browse", cb_browse_sprite, NULL);
  } else if (g_sel_kind == SEL_CAM && g_sel_cam) {
    sf_ui_lay_label(&sf_ctx, "fov / near / far", SF_CLR_WHITE);
    sf_ui_lay_col(&sf_ctx, 3, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel_cam->fov, 0.5f, cb_cam_proj_dirty, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel_cam->near_plane, 0.01f, cb_cam_proj_dirty, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_sel_cam->far_plane, 1.0f, cb_cam_proj_dirty, NULL);
    sf_ui_lay_label(&sf_ctx, "preview", SF_CLR_WHITE);
    /* Camera PIP: reserve space and record position for blit overlay */
    int pip_w = PNL_W - 8;
    int pip_h = (g_sel_cam->h > 0 && g_sel_cam->w > 0) ? (pip_w * g_sel_cam->h) / g_sel_cam->w : 120;
    sf_ui_lay_row(&sf_ctx, pip_h);
    /* Use a dummy label to consume the row and get its position */
    sf_ui_lmn_t *pip_el = sf_ui_lay_label(&sf_ctx, "", 0);
    if (pip_el) {
      g_cam_pip_pos  = pip_el->v0;
      g_cam_pip_size = (sf_ivec2_t){pip_w, pip_h};
      g_cam_pip_visible = true;
    }
  }
  y = sf_ui_lay_end_panel(&sf_ctx);

  /* ---- Texture panel (entity only) ---- */
  if (g_sel_kind == SEL_ENTI) {
    lay_panel_s("Texture", X0, y, PNL_W);
    build_texture_section();
    y = sf_ui_lay_end_panel(&sf_ctx);
  }

  /* ---- Parenting panel ---- */
  lay_panel_s("Parenting", X0, y, PNL_W);
  const char *pn = (sf->parent && sf->parent->name) ? sf->parent->name : "(none)";
  snprintf(s_parentline, sizeof(s_parentline), "parent: %s", pn);
  sf_ui_lay_label(&sf_ctx, s_parentline, SF_CLR_WHITE);
  sf_ui_lay_row(&sf_ctx, 20);
  sf_ui_lay_dropdown(&sf_ctx, g_parent_items, g_parent_count, &g_parent_sel, NULL, NULL);
  sf_ui_lay_row(&sf_ctx, 20);
  sf_ui_lay_col(&sf_ctx, 2, NULL);
  sf_ui_lay_button(&sf_ctx, "Parent", cb_set_parent, NULL);
  sf_ui_lay_button(&sf_ctx, "Unparent", cb_unparent, NULL);
  y = sf_ui_lay_end_panel(&sf_ctx);
  return y;
}

static void build_sff_tab(void) {
  const int TOP = 30, PNL_W = 210;

  /* Inspector fills left column; returns bottom y of last panel */
  int fy = build_inspector(TOP);

  /* File panel butts against inspector */
  lay_panel_s("File", 10, fy, PNL_W);
  sf_ui_lay_row(&sf_ctx, 18);
  sf_ui_lay_row(&sf_ctx, 18);
  sf_ui_lay_text_input(&sf_ctx, g_save_path, sizeof(g_save_path), NULL, NULL);
  sf_ui_lay_row(&sf_ctx, 18);
  sf_ui_lay_col(&sf_ctx, 2, NULL);
  sf_ui_lay_button(&sf_ctx, "Save", cb_save_install_sff, NULL);
  sf_ui_lay_button(&sf_ctx, "Load", cb_load_sff, NULL);
  sf_ui_lay_end_panel(&sf_ctx);

  int rx0 = g_w - 230, rx1 = g_w - 10;
  build_debug_panel(rx0, rx1, TOP);
  build_outliner_panel(rx0, rx1, TOP + 104, g_h - 10);
  build_browser_panel();
}

static void build_sfui_tab(void);

static void rebuild_ui(void) {
  sf_ui_clear(&sf_ctx);
  for (int i = 0; i < 9; i++) g_spawn_btn_el[i] = NULL;
  rebuild_parent_list();
  build_tab_bar();
  if      (g_tab == TAB_SFF)    build_sff_tab();
  else if (g_tab == TAB_SFUI)   build_sfui_tab();
  else                          build_sfgen_tab();
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
  const int TOP = 30, PNL_W = 210;
  int y = TOP;

  /* Left: palette */
  sf_ui_lay_begin_panel(&sf_ctx, "Palette", 10, TOP, PNL_W, 0);
  const char *lbls[8] = { "Button", "Label", "Panel", "Checkbox", "Slider", "DragFloat", "TextInput", "Dropdown" };
  int kinds[8] = { SF_UI_BUTTON, SF_UI_LABEL, SF_UI_PANEL, SF_UI_CHECKBOX, SF_UI_SLIDER, SF_UI_DRAG_FLOAT, SF_UI_TEXT_INPUT, SF_UI_DROPDOWN };
  sf_ui_lay_row(&sf_ctx, 20);
  for (int i = 0; i < 8; i++)
    sf_ui_lay_button(&sf_ctx, lbls[i], cb_design_add, (void*)(intptr_t)kinds[i]);
  sf_ui_lay_spacing(&sf_ctx, 6);
  sf_ui_lay_row(&sf_ctx, 20);
  sf_ui_lay_button(&sf_ctx, "Delete Selected", cb_design_del, NULL);
  y = sf_ui_lay_end_panel(&sf_ctx);

  /* Left: canvas */
  sf_ui_lay_begin_panel(&sf_ctx, "Canvas", 10, y, PNL_W, 0);
  sf_ui_lay_label(&sf_ctx, "size w h", SF_CLR_WHITE);
  sf_ui_lay_row(&sf_ctx, 20);
  sf_ui_lay_col(&sf_ctx, 2, NULL);
  sf_ui_lay_drag_float(&sf_ctx, &g_design_canvas_w, 4.0f, cb_canvas_resized, NULL);
  sf_ui_lay_drag_float(&sf_ctx, &g_design_canvas_h, 4.0f, cb_canvas_resized, NULL);
  y = sf_ui_lay_end_panel(&sf_ctx);

  /* Left: file */
  sf_ui_lay_begin_panel(&sf_ctx, "File", 10, y, PNL_W, 0);
  sf_ui_lay_row(&sf_ctx, 20);
  sf_ui_lay_text_input(&sf_ctx, g_sfui_path, sizeof(g_sfui_path), NULL, NULL);
  sf_ui_lay_row(&sf_ctx, 20);
  sf_ui_lay_col(&sf_ctx, 2, NULL);
  sf_ui_lay_button(&sf_ctx, "Save", cb_design_save, NULL);
  sf_ui_lay_button(&sf_ctx, "Load", cb_design_load, NULL);
  sf_ui_lay_end_panel(&sf_ctx);

  /* Right: inspector */
  int rx0 = g_w - 230, rpw = 220;
  sf_ui_lay_begin_panel(&sf_ctx, "Properties", rx0, TOP, rpw, 0);
  if (!g_design_sel) {
    sf_ui_lay_label(&sf_ctx, "(no selection)", 0xFFAAAAAA);
  } else {
    static char s_type[40];
    snprintf(s_type, sizeof(s_type), "type: %s", _design_type_str(g_design_sel->type));
    sf_ui_lay_label(&sf_ctx, s_type, SF_CLR_WHITE);
    sf_ui_lay_label(&sf_ctx, "name", SF_CLR_WHITE);
    sf_ui_lay_row(&sf_ctx, 20);
    sf_ui_lay_col(&sf_ctx, 2, (float[]){0.7f, 0.3f});
    sf_ui_lay_text_input(&sf_ctx, g_design_name_buf, sizeof(g_design_name_buf), NULL, NULL);
    sf_ui_lay_button(&sf_ctx, "Set", cb_design_apply_name, NULL);

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
    sf_ui_lay_label(&sf_ctx, "v0 x y (canvas)", SF_CLR_WHITE);
    sf_ui_lay_col(&sf_ctx, 2, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_design_v0x, 1.0f, cb_design_apply_bounds, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_design_v0y, 1.0f, cb_design_apply_bounds, NULL);
    sf_ui_lay_label(&sf_ctx, "v1 x y (canvas)", SF_CLR_WHITE);
    sf_ui_lay_col(&sf_ctx, 2, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_design_v1x, 1.0f, cb_design_apply_bounds, NULL);
    sf_ui_lay_drag_float(&sf_ctx, &g_design_v1y, 1.0f, cb_design_apply_bounds, NULL);
    int t = g_design_sel->type;
    if (t == SF_UI_BUTTON || t == SF_UI_LABEL || t == SF_UI_CHECKBOX || t == SF_UI_PANEL) {
      sf_ui_lay_label(&sf_ctx, "text", SF_CLR_WHITE);
      sf_ui_lay_row(&sf_ctx, 20);
      sf_ui_lay_col(&sf_ctx, 2, (float[]){0.7f, 0.3f});
      sf_ui_lay_text_input(&sf_ctx, g_design_text_buf, sizeof(g_design_text_buf), NULL, NULL);
      sf_ui_lay_button(&sf_ctx, "Set", cb_design_apply_text, NULL);
    }
    if (t == SF_UI_SLIDER) {
      sf_ui_lay_label(&sf_ctx, "min / max", SF_CLR_WHITE);
      sf_ui_lay_col(&sf_ctx, 2, NULL);
      sf_ui_lay_drag_float(&sf_ctx, &g_design_slider_min, 0.05f, cb_design_apply_slider, NULL);
      sf_ui_lay_drag_float(&sf_ctx, &g_design_slider_max, 0.05f, cb_design_apply_slider, NULL);
      sf_ui_lay_label(&sf_ctx, "value", SF_CLR_WHITE);
      sf_ui_lay_drag_float(&sf_ctx, &g_design_slider_val, 0.01f, cb_design_apply_slider, NULL);
    }
    if (t == SF_UI_DRAG_FLOAT) {
      sf_ui_lay_label(&sf_ctx, "step", SF_CLR_WHITE);
      sf_ui_lay_drag_float(&sf_ctx, &g_design_df_step, 0.005f, cb_design_apply_df, NULL);
    }
    if (t == SF_UI_CHECKBOX) {
      sf_ui_lay_row(&sf_ctx, 20);
      sf_ui_lay_checkbox(&sf_ctx, "initial checked", g_design_checked, cb_design_apply_checked, NULL);
    }
    if (t == SF_UI_PANEL) {
      sf_ui_lay_row(&sf_ctx, 20);
      sf_ui_lay_checkbox(&sf_ctx, "collapsed", g_design_panel_collapsed, cb_design_apply_panel, NULL);
    }
    if (t == SF_UI_DROPDOWN) {
      static char s_dd[80];
      int n = g_design_sel->dropdown.n_items;
      snprintf(s_dd, sizeof(s_dd), "items: %d", n);
      sf_ui_lay_label(&sf_ctx, s_dd, SF_CLR_WHITE);
      for (int k = 0; k < n && k < 6; k++)
        sf_ui_lay_label(&sf_ctx, g_design_sel->dropdown.items[k] ? g_design_sel->dropdown.items[k] : "?", 0xFFAAAAAA);
      sf_ui_lay_row(&sf_ctx, 20);
      sf_ui_lay_col(&sf_ctx, 3, (float[]){0.6f, 0.2f, 0.2f});
      sf_ui_lay_text_input(&sf_ctx, g_design_dd_item_buf, sizeof(g_design_dd_item_buf), NULL, NULL);
      sf_ui_lay_button(&sf_ctx, "+", cb_design_dd_add, NULL);
      sf_ui_lay_button(&sf_ctx, "-", cb_design_dd_pop, NULL);
      sf_ui_lay_label(&sf_ctx, "initial sel", SF_CLR_WHITE);
      static float s_dd_sel;
      s_dd_sel = (float)g_design_dd_selected;
      sf_ui_lay_drag_float(&sf_ctx, &s_dd_sel, 1.0f, cb_design_apply_dd_sel, NULL);
      g_design_dd_selected = (int)s_dd_sel;
    }
  }
  sf_ui_lay_end_panel(&sf_ctx);
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
      int n = el->dropdown.n_items;
      int mv = (el->dropdown.max_visible > 0 && n > el->dropdown.max_visible)
               ? el->dropdown.max_visible : n;
      if (mx >= el->v0.x && mx <= el->v0.x + w && my >= el->v1.y && my <= el->v1.y + h * mv) return true;
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
    sf_fvec3_t fwd = sf_fvec3_sub(g_orbit.target, cam->frame->pos);
    fwd.y = 0.0f;
    fwd = sf_fvec3_norm(fwd);
    sf_fvec3_t right = sf_fvec3_norm(sf_fvec3_cross(fwd, (sf_fvec3_t){0,1,0}));
    if (sf_key_down(ctx, SF_KEY_W)) g_orbit.target = sf_fvec3_add(g_orbit.target, (sf_fvec3_t){fwd.x*move, 0.0f, fwd.z*move});
    if (sf_key_down(ctx, SF_KEY_S)) g_orbit.target = sf_fvec3_add(g_orbit.target, (sf_fvec3_t){-fwd.x*move, 0.0f, -fwd.z*move});
    if (sf_key_down(ctx, SF_KEY_A)) g_orbit.target = sf_fvec3_add(g_orbit.target, (sf_fvec3_t){-right.x*move, 0.0f, -right.z*move});
    if (sf_key_down(ctx, SF_KEY_D)) g_orbit.target = sf_fvec3_add(g_orbit.target, (sf_fvec3_t){ right.x*move, 0.0f,  right.z*move});
    if (sf_key_down(ctx, SF_KEY_Q)) g_orbit.target.y -= move;
    if (sf_key_down(ctx, SF_KEY_E)) g_orbit.target.y += move;
  }

  if (ctx->input.mouse_btns[SF_MOUSE_RIGHT])
    sf_orbit_cam_rotate(&g_orbit, ctx->input.mouse_dx * 0.005f, ctx->input.mouse_dy * 0.005f);
  if (ctx->input.wheel_dy != 0 && !mouse_over_ui(ctx->input.mouse_x, ctx->input.mouse_y)) {
    float factor = (ctx->input.wheel_dy > 0) ? -g_orbit.dist * 0.1f : g_orbit.dist * 0.1f;
    sf_orbit_cam_zoom(&g_orbit, factor);
    if (g_orbit.dist > 200.0f) g_orbit.dist = 200.0f;
  }
  orbit_apply(cam);

  if (sf_key_pressed(ctx, SF_KEY_DEL)) cb_delete(ctx, NULL);

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
  sf_hit_t hits[SF_MAX_HITS];
  int n = sf_raycast_all(&sf_ctx, ray, hits, SF_MAX_HITS);

  sel_clear();
  if (n > 0) {
    sf_hit_t *h = &hits[0];
    switch (h->kind) {
      case SF_HIT_ENTI:  g_sel_kind = SEL_ENTI;  g_sel       = (sf_enti_t*)  h->ptr; break;
      case SF_HIT_LIGHT: g_sel_kind = SEL_LIGHT; g_sel_light = (sf_light_t*) h->ptr; break;
      case SF_HIT_CAM:   g_sel_kind = SEL_CAM;   g_sel_cam   = (sf_cam_t*)   h->ptr; break;
      case SF_HIT_EMITR: g_sel_kind = SEL_EMITR; g_sel_emitr = (sf_emitr_t*) h->ptr; break;
      default: break;
    }
  }
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
  sfgen_init();
  scan_models();
  scan_textures();
  scan_sffs();
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
          else if (g_tab == TAB_SFGEN && event.button.x > 220) {
            g_sfgen_drag = true; g_sfgen_lmx = event.button.x; g_sfgen_lmy = event.button.y;
          } else {
            if (!gizmo_begin_drag(event.button.x, event.button.y))
              try_pick(event.button.x, event.button.y);
          }
        }
      }
      if (event.type == SDL_MOUSEMOTION) {
        g_mouse_x = event.motion.x; g_mouse_y = event.motion.y;
        if (g_tab == TAB_SFUI) design_on_mouse_move(event.motion.x, event.motion.y);
        else if (g_tab == TAB_SFGEN && g_sfgen_drag) {
          float dx = (float)(event.motion.x - g_sfgen_lmx) * 0.007f;
          float dy = (float)(event.motion.y - g_sfgen_lmy) * 0.007f;
          g_sfgen_orbit.yaw -= dx; g_sfgen_orbit.pitch += dy;
          if (g_sfgen_orbit.pitch > 1.4f) g_sfgen_orbit.pitch = 1.4f;
          if (g_sfgen_orbit.pitch < -0.1f) g_sfgen_orbit.pitch = -0.1f;
          g_sfgen_lmx = event.motion.x; g_sfgen_lmy = event.motion.y;
        }
        else if (g_gizmo.drag_axis != SF_GZ_AX_NONE) gizmo_drag_update(event.motion.x, event.motion.y);
      }
      if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (g_tab == TAB_SFUI) design_on_mouse_up();
        else if (g_tab == TAB_SFGEN) g_sfgen_drag = false;
        else gizmo_end_drag();
      }
      if (event.type == SDL_MOUSEWHEEL) {
        if (g_tab == TAB_SFUI) design_on_wheel(event.wheel.y);
        else if (g_tab == TAB_SFGEN) {
          g_sfgen_orbit.dist -= event.wheel.y * 0.5f;
          if (g_sfgen_orbit.dist < 1.f) g_sfgen_orbit.dist = 1.f;
          if (g_sfgen_orbit.dist > 60.f) g_sfgen_orbit.dist = 60.f;
        }
      }
      sf_sdl_process_event(&sf_ctx, &event);
    }

    if (sf_key_pressed(&sf_ctx, SF_KEY_ESC) && (!sf_ctx.ui || sf_ctx.ui->focused == NULL)) sf_stop(&sf_ctx);

    sf_time_update(&sf_ctx);

    if (g_ui_dirty) { rebuild_ui(); g_ui_dirty = false; }

    sf_ui_update(&sf_ctx, sf_ctx.ui);
    /* detect closable panel close */
    if (g_picker_panel && !g_picker_panel->is_visible && g_picker_open != PICK_NONE) {
      g_picker_open = PICK_NONE; g_picker_panel = NULL; g_ui_dirty = true;
    }
    if (g_tab == TAB_SFF) update_camera();

    if (g_tab == TAB_SFGEN && g_sfgen_ready) {
      sfgen_update_camera();
      /* Regenerate on param change */
      static float s_last_ct[14];
      static float s_last_cr[11];
      static float s_last_bk[15];
      static int s_last_ct_tsel=-1, s_last_cr_tsel=-1, s_last_ct_ssel=-1, s_last_bk_tsel=-1, s_last_bk_rtsel=-1;
      bool tree_dirty = (ct_seed!=s_last_ct[0]||ct_depth!=s_last_ct[1]||ct_branch!=s_last_ct[2]||ct_angle!=s_last_ct[3]||
                         ct_len!=s_last_ct[4]||ct_taper!=s_last_ct[5]||ct_grav!=s_last_ct[6]||ct_wiggle!=s_last_ct[7]||
                         ct_twist!=s_last_ct[8]||ct_tr!=s_last_ct[9]||ct_tl!=s_last_ct[10]||
                         ct_ls!=s_last_ct[11]||ct_ld!=s_last_ct[12]||ct_lo!=s_last_ct[13]);
      bool rock_dirty = (cr_seed!=s_last_cr[0]||cr_subdiv!=s_last_cr[1]||cr_rough!=s_last_cr[2]||cr_freq!=s_last_cr[3]||
                         cr_octaves!=s_last_cr[4]||cr_persist!=s_last_cr[5]||cr_flat!=s_last_cr[6]||
                         cr_elongx!=s_last_cr[7]||cr_elongz!=s_last_cr[8]||cr_pointy!=s_last_cr[9]||cr_bump!=s_last_cr[10]);
      bool bldg_dirty = (bk_seed!=s_last_bk[0]||bk_floors!=s_last_bk[1]||bk_floor_h!=s_last_bk[2]||
                         bk_width!=s_last_bk[3]||bk_depth!=s_last_bk[4]||bk_taper!=s_last_bk[5]||
                         bk_jitter!=s_last_bk[6]||bk_ledge!=s_last_bk[7]||bk_win_cols!=s_last_bk[8]||
                         bk_win_size!=s_last_bk[9]||bk_win_inset!=s_last_bk[10]||
                         bk_roof_type!=s_last_bk[11]||bk_roof_h!=s_last_bk[12]||
                         bk_roof_scale!=s_last_bk[13]||bk_asym!=s_last_bk[14]);
      if (tree_dirty) {
        sfgen_generate_tree();
        s_last_ct[0]=ct_seed; s_last_ct[1]=ct_depth; s_last_ct[2]=ct_branch; s_last_ct[3]=ct_angle;
        s_last_ct[4]=ct_len;  s_last_ct[5]=ct_taper; s_last_ct[6]=ct_grav;   s_last_ct[7]=ct_wiggle;
        s_last_ct[8]=ct_twist;s_last_ct[9]=ct_tr;    s_last_ct[10]=ct_tl;
        s_last_ct[11]=ct_ls;  s_last_ct[12]=ct_ld;   s_last_ct[13]=ct_lo;
      }
      if (rock_dirty) {
        sfgen_generate_rock();
        s_last_cr[0]=cr_seed;    s_last_cr[1]=cr_subdiv;  s_last_cr[2]=cr_rough;
        s_last_cr[3]=cr_freq;    s_last_cr[4]=cr_octaves; s_last_cr[5]=cr_persist;
        s_last_cr[6]=cr_flat;    s_last_cr[7]=cr_elongx;  s_last_cr[8]=cr_elongz;
        s_last_cr[9]=cr_pointy;  s_last_cr[10]=cr_bump;
      }
      if (bldg_dirty) {
        sfgen_generate_building();
        s_last_bk[0]=bk_seed;    s_last_bk[1]=bk_floors;   s_last_bk[2]=bk_floor_h;
        s_last_bk[3]=bk_width;   s_last_bk[4]=bk_depth;    s_last_bk[5]=bk_taper;
        s_last_bk[6]=bk_jitter;  s_last_bk[7]=bk_ledge;    s_last_bk[8]=bk_win_cols;
        s_last_bk[9]=bk_win_size;s_last_bk[10]=bk_win_inset;s_last_bk[11]=bk_roof_type;
        s_last_bk[12]=bk_roof_h; s_last_bk[13]=bk_roof_scale; s_last_bk[14]=bk_asym;
      }
      if (g_ct_tsel != s_last_ct_tsel) { sfgen_apply_bark(); s_last_ct_tsel = g_ct_tsel; }
      if (g_ct_ssel != s_last_ct_ssel) {
        if (g_ct_ssel>=0&&g_ct_ssel<g_ct_snc) g_sfgen_leaf=g_ct_sprites[g_ct_ssel];
        s_last_ct_ssel = g_ct_ssel;
      }
      if (g_cr_tsel != s_last_cr_tsel) { sfgen_apply_stone(); s_last_cr_tsel = g_cr_tsel; }
      if (g_ck_tsel != s_last_bk_tsel || g_ck_rtsel != s_last_bk_rtsel) {
        sfgen_apply_bldg_tex();
        s_last_bk_tsel = g_ck_tsel;
        s_last_bk_rtsel = g_ck_rtsel;
      }
    }

    if (g_tab == TAB_SFF) {
      sf_render_ctx(&sf_ctx);
      if (g_dbg_frames) sf_draw_debug_frames(&sf_ctx, &sf_ctx.main_camera, 1.0f);
      if (g_dbg_lights) sf_draw_debug_lights(&sf_ctx, &sf_ctx.main_camera, 0.3f);
      if (g_dbg_cams)   sf_draw_debug_cams  (&sf_ctx, &sf_ctx.main_camera, 2.0f);
      update_gizmo_geometry();
      /* hover is now computed inside update_gizmo_geometry */
      draw_gizmo();
    } else if (g_tab == TAB_SFGEN && g_sfgen_ready) {
      /* Hide inactive types during render */
      if (g_ct_enti) g_ct_enti->obj.f_cnt = (g_sfgen_type==SFGEN_TREE) ? g_ct_obj->f_cnt : 0;
      if (g_cr_enti) g_cr_enti->obj.f_cnt = (g_sfgen_type==SFGEN_ROCK) ? g_cr_obj->f_cnt : 0;
      if (g_ck_enti)       g_ck_enti->obj.f_cnt       = (g_sfgen_type==SFGEN_BLDG) ? g_ck_obj->f_cnt       : 0;
      if (g_ck_enti_win)   g_ck_enti_win->obj.f_cnt   = (g_sfgen_type==SFGEN_BLDG) ? g_ck_obj_win->f_cnt   : 0;
      if (g_ck_enti_ledge) g_ck_enti_ledge->obj.f_cnt = (g_sfgen_type==SFGEN_BLDG) ? g_ck_obj_ledge->f_cnt : 0;
      if (g_ck_enti_roof)  g_ck_enti_roof->obj.f_cnt  = (g_sfgen_type==SFGEN_BLDG) ? g_ck_obj_roof->f_cnt  : 0;
      int saved_spr3d = g_sfgen_ctx.sprite_3d_count;
      if (g_sfgen_type != SFGEN_TREE) g_sfgen_ctx.sprite_3d_count = 0;
      sf_render_ctx(&g_sfgen_ctx);
      g_sfgen_ctx.sprite_3d_count = saved_spr3d;
      if (g_ct_enti)       g_ct_enti->obj.f_cnt       = g_ct_obj->f_cnt;
      if (g_cr_enti)       g_cr_enti->obj.f_cnt       = g_cr_obj->f_cnt;
      if (g_ck_enti)       g_ck_enti->obj.f_cnt       = g_ck_obj->f_cnt;
      if (g_ck_enti_win)   g_ck_enti_win->obj.f_cnt   = g_ck_obj_win->f_cnt;
      if (g_ck_enti_ledge) g_ck_enti_ledge->obj.f_cnt = g_ck_obj_ledge->f_cnt;
      memcpy(sf_ctx.main_camera.buffer, g_sfgen_ctx.main_camera.buffer,
             (size_t)g_w * g_h * sizeof(sf_pkd_clr_t));
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
    sf_ui_render_popups(&sf_ctx, &sf_ctx.main_camera, sf_ctx.ui);
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
