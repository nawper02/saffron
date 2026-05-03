/* sf_strike -- cs 1.6 style shooter built on saffron */
#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>

/* --- CONSTANTS --- */
#define SCREEN_W             (683*2)
#define SCREEN_H             (384*2)
#define MOVE_SPEED           6.0f
#define MOUSE_SENS           0.15f
#define GRAVITY              -20.0f
#define JUMP_VEL             7.5f
#define PLAYER_EYE_H         1.6f
#define PLAYER_HALF_W        0.3f
#define PLAYER_HALF_H        0.9f
#define MAX_WALLS            256
#define MAX_ENEMIES          8
#define MAX_KILL_FEED        5
#define AK_FIRE_RATE         0.1f
#define AK_DAMAGE            25
#define AK_MAG_SIZE          30
#define AK_RESERVE           90
#define AK_RELOAD_TIME       2.5f
#define AK_RANGE             80.0f
#define KNIFE_DAMAGE         55
#define KNIFE_RANGE          2.0f
#define KNIFE_RATE           0.5f
#define ENEMY_HP             80
#define ENEMY_SPEED          2.0f
#define ENEMY_DETECT_RANGE   20.0f
#define ENEMY_FIRE_RATE      0.8f
#define ENEMY_DAMAGE         8
#define PLAYER_MAX_HP        100
#define PLAYER_MAX_ARMOR     100
#define ROUND_FREEZE_TIME    2.0f
#define ROUND_WIN_TIME       3.0f
#define BUY_TIME             5.0f
#define START_MONEY          800
#define KILL_REWARD          300
#define ROUND_REWARD         1000
#define AK_COST              2700
#define ARMOR_COST           650
#define NUM_WAYPOINTS        8

/* --- GAME TYPES --- */
typedef enum {
  STATE_MENU,
  STATE_BUY,
  STATE_PLAYING,
  STATE_ROUND_WON,
  STATE_GAME_OVER
} game_state_t;

typedef enum {
  WPN_KNIFE,
  WPN_AK47,
  WPN_COUNT
} weapon_type_t;

typedef enum {
  AI_IDLE,
  AI_PATROL,
  AI_CHASE,
  AI_ATTACK,
  AI_DEAD
} ai_state_t;

typedef struct {
  weapon_type_t type;
  int           mag;
  int           reserve;
  float         fire_cd;
  float         reload_cd;
  bool          reloading;
} weapon_t;

typedef struct {
  sf_fvec3_t    pos;
  float         yaw, pitch;
  sf_phys_body_t body;
  int           hp, armor;
  weapon_t      weapons[WPN_COUNT];
  weapon_type_t cur_wpn;
  float         recoil;
  float         bob_timer;
  bool          walking;
  int           money;
  int           kills, deaths;
} player_t;

typedef struct {
  sf_puppet_inst_t *puppet;
  ai_state_t    state;
  sf_fvec3_t    pos;
  float         yaw;
  int           hp;
  float         fire_cd;
  float         state_timer;
  int           cur_waypoint;
  int           next_waypoint;
  sf_fvec3_t    last_known_player;
  sf_aabb_t     box;
  bool          active;
} enemy_t;

typedef struct {
  char          text[64];
  float         timer;
} kill_feed_entry_t;

typedef struct {
  sf_ctx_t      ctx;
  game_state_t  state;
  player_t      player;
  enemy_t       enemies[MAX_ENEMIES];
  int           enemy_count;
  sf_aabb_t     walls[MAX_WALLS];
  int           wall_count;
  sf_fvec3_t    waypoints[NUM_WAYPOINTS];
  int           wp_count;
  sf_fvec3_t    t_spawn, ct_spawn;
  int           round_num;
  float         state_timer;
  float         hit_marker_timer;
  float         damage_flash;
  kill_feed_entry_t kill_feed[MAX_KILL_FEED];
  sf_puppet_t  *human_puppet;
  sf_obj_t     *ak_barrel, *ak_body, *ak_mag, *ak_stock;
  sf_enti_t    *vm_barrel, *vm_body, *vm_mag, *vm_stock;
  sf_enti_t    *knife_blade, *knife_handle;
  float         wpn_sway_x, wpn_sway_y;
  bool          mouse_captured;
} game_t;

/* --- GLOBALS --- */
game_t g;
int g_mouse_accum_x = 0, g_mouse_accum_y = 0;

/* --- TEXT HELPERS --- */
int text_w(const char *s, int scale) { return (int)strlen(s) * 8 * scale; }
int center_x(const char *s, int scale, int screen_w) { return (screen_w - text_w(s, scale)) / 2; }

/* --- WALL HELPERS --- */
void add_wall(sf_fvec3_t pos, sf_fvec3_t half) {
  if (g.wall_count >= MAX_WALLS) return;
  g.walls[g.wall_count++] = sf_aabb_from_pos_size(pos, half);
}

void add_wall_enti(sf_ctx_t *ctx, const char *obj_name, const char *tex_name,
                   const char *enti_name, sf_fvec3_t pos, sf_fvec3_t scale,
                   float tex_scale) {
  sf_obj_t *obj = sf_get_obj_(ctx, obj_name, false);
  if (!obj) return;
  sf_enti_t *e = sf_add_enti(ctx, obj, enti_name);
  if (!e) return;
  sf_enti_set_pos(ctx, e, pos.x, pos.y, pos.z);
  sf_enti_set_scale(ctx, e, scale.x, scale.y, scale.z);
  if (tex_name) {
    sf_tex_t *t = sf_get_texture_(ctx, tex_name, false);
    if (t) { e->tex = t; e->tex_scale = (sf_fvec2_t){tex_scale, tex_scale}; }
  }
  sf_fvec3_t half = {scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f};
  add_wall(pos, half);
}

#define STRIKE_MAP_PATH SF_ASSET_PATH "/sf_sff/strike_map.sff"

/* --- LOAD MAP FROM .sff --- */
bool load_map_sff(sf_ctx_t *ctx) {
  FILE *test = fopen(STRIKE_MAP_PATH, "r");
  if (!test) return false;
  fclose(test);

  sf_load_sff(ctx, STRIKE_MAP_PATH, "strike_map");

  /* Build collision walls from all entities */
  g.wall_count = 0;
  for (int i = 0; i < ctx->enti_count; i++) {
    sf_enti_t *e = &ctx->entities[i];
    if (!e->frame) continue;
    sf_fvec3_t pos = e->frame->pos;
    sf_fvec3_t scl = e->frame->scale;
    sf_fvec3_t half = {scl.x * 0.5f, scl.y * 0.5f, scl.z * 0.5f};
    if (half.x > 0.01f && half.y > 0.01f && half.z > 0.01f)
      add_wall(pos, half);
  }

  /* Extract spawn points from named frames */
  g.t_spawn = (sf_fvec3_t){0, 0.1f, -10};
  g.ct_spawn = (sf_fvec3_t){0, 0.1f, 10};
  sf_frame_t *ts = _sf_sff_get_frame_(ctx, "t_spawn");
  if (ts) g.t_spawn = ts->pos;
  sf_frame_t *cts = _sf_sff_get_frame_(ctx, "ct_spawn");
  if (cts) g.ct_spawn = cts->pos;

  /* Extract waypoints from named frames */
  g.wp_count = 0;
  for (int i = 0; i < NUM_WAYPOINTS; i++) {
    char name[16];
    snprintf(name, 16, "wp_%d", i);
    sf_frame_t *wp = _sf_sff_get_frame_(ctx, name);
    if (wp) {
      g.waypoints[g.wp_count++] = wp->pos;
    } else break;
  }
  /* If no waypoints defined, create defaults */
  if (g.wp_count == 0) {
    g.waypoints[0] = g.t_spawn;
    g.waypoints[1] = g.ct_spawn;
    g.wp_count = 2;
  }

  /* Add boundary walls */
  add_wall((sf_fvec3_t){0, 2, 30}, (sf_fvec3_t){50, 4, 0.5f});
  add_wall((sf_fvec3_t){0, 2, -30}, (sf_fvec3_t){50, 4, 0.5f});
  add_wall((sf_fvec3_t){30, 2, 0}, (sf_fvec3_t){0.5f, 4, 50});
  add_wall((sf_fvec3_t){-30, 2, 0}, (sf_fvec3_t){0.5f, 4, 50});

  printf("Loaded map from %s: %d entities, %d walls, %d waypoints\n",
         STRIKE_MAP_PATH, ctx->enti_count, g.wall_count, g.wp_count);
  return true;
}

/* --- BUILD MAP (PROCEDURAL FALLBACK) --- */
void build_map(sf_ctx_t *ctx) {
  sf_obj_make_box(ctx, "unit_box", 1.0f, 1.0f, 1.0f);
  sf_obj_make_plane(ctx, "unit_plane", 1.0f, 1.0f, 1);

  sf_load_texture_bmp(ctx, "Brick_01-128x128.bmp", "brick");
  sf_load_texture_bmp(ctx, "Brick_09-128x128.bmp", "brick2");
  sf_load_texture_bmp(ctx, "Stone_05-128x128.bmp", "stone");
  sf_load_texture_bmp(ctx, "Stone_09-128x128.bmp", "stone2");
  sf_load_texture_bmp(ctx, "Wood_04-128x128.bmp",  "wood");
  sf_load_texture_bmp(ctx, "Wood_09-128x128.bmp",  "wood2");
  sf_load_texture_bmp(ctx, "Tile_01-128x128.bmp",  "tile");
  sf_load_texture_bmp(ctx, "Metal_01-128x128.bmp", "metal");
  sf_load_texture_bmp(ctx, "Dirt_02-128x128.bmp",  "dirt");
  sf_load_texture_bmp(ctx, "Plaster_05-128x128.bmp", "plaster");
  sf_load_texture_bmp(ctx, "Metal_06-128x128.bmp", "metal2");

  int wn = 0;
  char name[32];
  #define W(tex, px,py,pz, sx,sy,sz, ts) do { \
    snprintf(name, 32, "w_%d", wn++); \
    add_wall_enti(ctx, "unit_box", tex, name, \
      (sf_fvec3_t){px,py,pz}, (sf_fvec3_t){sx,sy,sz}, ts); \
  } while(0)
  #define C(n, tex, px,py,pz, sx,sy,sz, ts) do { \
    add_wall_enti(ctx, "unit_box", tex, n, \
      (sf_fvec3_t){px,py,pz}, (sf_fvec3_t){sx,sy,sz}, ts); \
  } while(0)

  /* --- FLOOR --- */
  sf_obj_t *floor_obj = sf_obj_make_plane(ctx, "floor_mesh", 50.0f, 50.0f, 4);
  sf_enti_t *floor_e = sf_add_enti(ctx, floor_obj, "floor");
  sf_enti_set_pos(ctx, floor_e, 0.0f, 0.0f, 0.0f);
  floor_e->tex = sf_get_texture_(ctx, "stone2", false);
  floor_e->tex_scale = (sf_fvec2_t){10.0f, 10.0f};
  add_wall((sf_fvec3_t){0, -0.5f, 0}, (sf_fvec3_t){25, 0.5f, 25});

  /* --- T SPAWN (south, z ~ -18) --- */
  /* Enclosed spawn area with brick walls */
  W("brick",  0, 1.8f, -20, 10, 3.6f, 0.6f, 3.0f);    /* back wall */
  W("brick",  -5, 1.8f, -16, 0.6f, 3.6f, 8, 3.0f);     /* left wall */
  W("brick",  5, 1.8f, -16, 0.6f, 3.6f, 8, 3.0f);       /* right wall */
  /* Ceiling over T spawn */
  W("plaster", 0, 3.6f, -16, 10, 0.3f, 8, 2.0f);
  /* T spawn floor tile inset */
  sf_enti_t *t_floor = sf_add_enti(ctx, sf_get_obj_(ctx, "unit_box", false), "t_floor");
  sf_enti_set_pos(ctx, t_floor, 0, 0.01f, -16);
  sf_enti_set_scale(ctx, t_floor, 9.0f, 0.02f, 7.0f);
  t_floor->tex = sf_get_texture_(ctx, "tile", false);
  t_floor->tex_scale = (sf_fvec2_t){4.0f, 4.0f};
  /* Crates for cover */
  C("cr_t0","wood",  -2.5f, 0.5f, -18, 1.0f, 1.0f, 1.0f, 2.0f);
  C("cr_t1","wood2", 3.0f, 0.5f, -17, 1.2f, 1.0f, 0.8f, 2.0f);
  C("cr_t2","wood",  -2.5f, 1.5f, -18, 0.8f, 0.8f, 0.8f, 2.0f);

  /* --- LONG A (left corridor, x ~ -4, z from -12 to 2) --- */
  W("stone",  -7, 1.8f, -5, 0.6f, 3.6f, 15, 3.0f);     /* outer left */
  W("brick2", -1, 1.8f, -7, 0.6f, 3.6f, 11, 3.0f);     /* inner right (divider) */
  /* Overhead beam across long A */
  W("wood",   -4, 3.4f, -4, 4, 0.4f, 0.6f, 2.0f);
  /* Long A cover */
  C("cr_la","wood",   -4.5f, 0.6f, -10, 1.5f, 1.2f, 0.6f, 2.0f);
  C("cr_lb","wood2",  -3.5f, 0.5f, -3, 1.0f, 1.0f, 1.0f, 2.0f);
  /* Long A floor -- darker dirt strip */
  sf_enti_t *la_fl = sf_add_enti(ctx, sf_get_obj_(ctx, "unit_box", false), "la_floor");
  sf_enti_set_pos(ctx, la_fl, -4, 0.01f, -5);
  sf_enti_set_scale(ctx, la_fl, 5.0f, 0.02f, 14.0f);
  la_fl->tex = sf_get_texture_(ctx, "dirt", false);
  la_fl->tex_scale = (sf_fvec2_t){3.0f, 6.0f};

  /* --- MID (right corridor, x ~ 4, z from -12 to 2) --- */
  W("stone2", 1, 1.8f, -5, 0.6f, 3.6f, 15, 3.0f);      /* inner left (divider) */
  W("stone",  7, 1.8f, -5, 0.6f, 3.6f, 15, 3.0f);       /* outer right */
  /* Mid doorway lintel (gap below for passage) */
  W("brick",  4, 3.0f, -5, 4, 1.2f, 0.5f, 2.0f);
  /* Mid cover -- barrels and crate */
  C("br_m0","metal",  4.0f, 0.5f, -8, 0.7f, 1.0f, 0.7f, 2.0f);
  C("br_m1","metal2", 3.5f, 0.5f, -2, 0.7f, 1.0f, 0.7f, 2.0f);
  C("cr_mid","wood",  5.0f, 0.6f, -5, 1.2f, 1.2f, 0.8f, 2.0f);

  /* --- BOMBSITE A (northwest, around x=-3, z=6) --- */
  W("brick",  -2, 1.8f, 10, 10, 3.6f, 0.6f, 3.0f);     /* back wall */
  W("brick2", -7, 1.8f, 6, 0.6f, 3.6f, 8, 3.0f);        /* left wall */
  /* A site elevated platform */
  C("a_plat","stone", -3, 0.35f, 6.5f, 5.0f, 0.7f, 3.5f, 3.0f);
  /* A site crates on platform */
  C("cr_a0","wood",  -5, 1.2f, 7.5f, 1.0f, 1.0f, 1.0f, 2.0f);
  C("cr_a1","wood2", -1.5f, 1.2f, 5.5f, 1.2f, 1.0f, 0.6f, 2.0f);
  /* Pillar on A site */
  C("pil_a","stone2", -5.5f, 1.5f, 4, 0.5f, 3.0f, 0.5f, 2.0f);

  /* --- CT SPAWN (northeast, x ~ 4, z ~ 8) --- */
  W("plaster", 4, 1.8f, 10, 8, 3.6f, 0.6f, 3.0f);      /* back wall */
  W("plaster", 7, 1.8f, 6, 0.6f, 3.6f, 8, 3.0f);        /* right wall */
  /* Ceiling over CT spawn */
  W("plaster", 4, 3.6f, 7, 6, 0.3f, 6, 2.0f);
  /* CT spawn floor tile */
  sf_enti_t *ct_fl = sf_add_enti(ctx, sf_get_obj_(ctx, "unit_box", false), "ct_floor");
  sf_enti_set_pos(ctx, ct_fl, 4, 0.01f, 7);
  sf_enti_set_scale(ctx, ct_fl, 6.0f, 0.02f, 6.0f);
  ct_fl->tex = sf_get_texture_(ctx, "tile", false);
  ct_fl->tex_scale = (sf_fvec2_t){3.0f, 3.0f};
  /* CT crates */
  C("cr_ct0","wood2", 2.5f, 0.5f, 8.5f, 1.0f, 1.0f, 1.0f, 2.0f);
  C("cr_ct1","wood",  5.5f, 0.5f, 7.5f, 0.8f, 1.0f, 1.2f, 2.0f);

  /* --- CONNECTING PASSAGE (between mid/long and sites, z ~ 2) --- */
  /* Partial wall with doorway gap */
  W("stone2", -4, 1.8f, 2, 2.5f, 3.6f, 0.5f, 3.0f);    /* left side of door */
  W("stone2",  4, 1.8f, 2, 2.5f, 3.6f, 0.5f, 3.0f);    /* right side of door */
  /* Lintel over doorway */
  W("stone",   0, 3.0f, 2, 2, 1.2f, 0.5f, 2.0f);

  /* --- CENTER ARENA (open area around z ~ 0 to 2) --- */
  /* Low wall for cover in center */
  C("cwall","brick", 0, 0.5f, 0, 3.0f, 1.0f, 0.5f, 2.0f);
  /* Pillars flanking center */
  C("pil0","stone2", -3, 1.5f, 0, 0.5f, 3.0f, 0.5f, 2.0f);
  C("pil1","stone2",  3, 1.5f, 0, 0.5f, 3.0f, 0.5f, 2.0f);

  /* --- BOUNDARY WALLS (invisible but collideable) --- */
  add_wall((sf_fvec3_t){0, 2, 11}, (sf_fvec3_t){25, 4, 0.5f});     /* north */
  add_wall((sf_fvec3_t){0, 2, -21}, (sf_fvec3_t){25, 4, 0.5f});    /* south */
  add_wall((sf_fvec3_t){8, 2, 0}, (sf_fvec3_t){0.5f, 4, 25});      /* east */
  add_wall((sf_fvec3_t){-8, 2, 0}, (sf_fvec3_t){0.5f, 4, 25});     /* west */

  #undef W
  #undef C

  /* --- LIGHTING --- */
  sf_light_t *sun = sf_add_light(ctx, "sun", SF_LIGHT_DIR,
    (sf_fvec3_t){1.0f, 0.95f, 0.8f}, 1.4f);
  if (sun) {
    sun->frame->rot = (sf_fvec3_t){SF_DEG2RAD(50.0f), SF_DEG2RAD(25.0f), 0.0f};
    sun->frame->is_dirty = true;
  }

  sf_light_t *l0 = sf_add_light(ctx, "l_tspawn", SF_LIGHT_POINT,
    (sf_fvec3_t){1.0f, 0.8f, 0.5f}, 5.0f);
  if (l0) { l0->frame->pos = (sf_fvec3_t){0, 3.2f, -16}; l0->frame->is_dirty = true; }

  sf_light_t *l1 = sf_add_light(ctx, "l_mid", SF_LIGHT_POINT,
    (sf_fvec3_t){0.7f, 0.7f, 1.0f}, 6.0f);
  if (l1) { l1->frame->pos = (sf_fvec3_t){4, 3.2f, -5}; l1->frame->is_dirty = true; }

  sf_light_t *l2 = sf_add_light(ctx, "l_long", SF_LIGHT_POINT,
    (sf_fvec3_t){1.0f, 0.9f, 0.6f}, 5.0f);
  if (l2) { l2->frame->pos = (sf_fvec3_t){-4, 3.2f, -5}; l2->frame->is_dirty = true; }

  sf_light_t *l3 = sf_add_light(ctx, "l_asite", SF_LIGHT_POINT,
    (sf_fvec3_t){1.0f, 0.85f, 0.6f}, 6.0f);
  if (l3) { l3->frame->pos = (sf_fvec3_t){-3, 3.2f, 6}; l3->frame->is_dirty = true; }

  sf_light_t *l4 = sf_add_light(ctx, "l_ct", SF_LIGHT_POINT,
    (sf_fvec3_t){0.8f, 0.9f, 1.0f}, 5.0f);
  if (l4) { l4->frame->pos = (sf_fvec3_t){4, 3.2f, 7}; l4->frame->is_dirty = true; }

  sf_light_t *l5 = sf_add_light(ctx, "l_center", SF_LIGHT_POINT,
    (sf_fvec3_t){1.0f, 1.0f, 0.9f}, 4.0f);
  if (l5) { l5->frame->pos = (sf_fvec3_t){0, 3.2f, 0}; l5->frame->is_dirty = true; }

  /* Waypoints for AI patrol */
  g.waypoints[0] = (sf_fvec3_t){-2, 0, -16};     /* T spawn */
  g.waypoints[1] = (sf_fvec3_t){ 3, 0, -16};     /* T spawn right */
  g.waypoints[2] = (sf_fvec3_t){-4, 0, -6};      /* long A mid */
  g.waypoints[3] = (sf_fvec3_t){ 4, 0, -6};      /* mid corridor */
  g.waypoints[4] = (sf_fvec3_t){ 0, 0,  0};      /* center arena */
  g.waypoints[5] = (sf_fvec3_t){-3, 0.7f, 6};    /* A site */
  g.waypoints[6] = (sf_fvec3_t){ 4, 0, 7};       /* CT spawn */
  g.waypoints[7] = (sf_fvec3_t){ 0, 0, -10};     /* between corridors */

  g.wp_count = 8;
  g.t_spawn  = (sf_fvec3_t){0, 0.1f, -17};
  g.ct_spawn = (sf_fvec3_t){4, 0.1f,   7};
}

/* --- BUILD HUMANOID PUPPET --- */
void add_box_part(sf_puppet_t *p, float cx, float cy, float cz,
                  float hx, float hy, float hz, int bone_idx,
                  int *vi, int *fi) {
  /* Add a box (8 verts, 12 faces) to puppet, skinned to bone_idx */
  int base_v = *vi;
  float verts[8][3] = {
    {cx-hx, cy-hy, cz-hz}, {cx+hx, cy-hy, cz-hz},
    {cx+hx, cy+hy, cz-hz}, {cx-hx, cy+hy, cz-hz},
    {cx-hx, cy-hy, cz+hz}, {cx+hx, cy-hy, cz+hz},
    {cx+hx, cy+hy, cz+hz}, {cx-hx, cy+hy, cz+hz}
  };
  for (int i = 0; i < 8; i++) {
    p->v[*vi] = (sf_fvec3_t){verts[i][0], verts[i][1], verts[i][2]};
    p->skin[*vi].idx[0] = (uint8_t)bone_idx;
    p->skin[*vi].w[0] = 1.0f;
    (*vi)++;
  }
  int faces[12][3] = {
    {0,1,2},{0,2,3}, {4,6,5},{4,7,6},
    {0,4,5},{0,5,1}, {2,6,7},{2,7,3},
    {0,3,7},{0,7,4}, {1,5,6},{1,6,2}
  };
  for (int i = 0; i < 12; i++) {
    p->f[*fi].idx[0] = (sf_vtx_idx_t){base_v+faces[i][0], -1, -1};
    p->f[*fi].idx[1] = (sf_vtx_idx_t){base_v+faces[i][1], -1, -1};
    p->f[*fi].idx[2] = (sf_vtx_idx_t){base_v+faces[i][2], -1, -1};
    (*fi)++;
  }
}

sf_puppet_t* build_humanoid(sf_ctx_t *ctx) {
  /* 10 bones: hips, spine, head, L_upper_arm, L_forearm, R_upper_arm, R_forearm, L_thigh, L_shin, R_thigh, R_shin */
  /* Actually 11 bones */
  int bone_cnt = 11;
  int max_v = 11 * 8;
  int max_f = 11 * 12;
  sf_puppet_t *p = sf_puppet_create(ctx, "humanoid", max_v, 0, max_f, bone_cnt);
  if (!p) return NULL;

  /* Define bones (parent index, -1 for root) */
  /* 0:hips 1:spine 2:head 3:L_uarm 4:L_farm 5:R_uarm 6:R_farm 7:L_thigh 8:L_shin 9:R_thigh 10:R_shin */
  const char *bnames[] = {"hips","spine","head","L_uarm","L_farm","R_uarm","R_farm","L_thigh","L_shin","R_thigh","R_shin"};
  int parents[] = {-1, 0, 1, 1, 3, 1, 5, 0, 7, 0, 9};
  float bone_y[] = {0.9f, 1.2f, 1.55f, 1.45f, 1.15f, 1.45f, 1.15f, 0.75f, 0.4f, 0.75f, 0.4f};

  for (int b = 0; b < bone_cnt; b++) {
    snprintf(p->bones[b].name, 32, "%s", bnames[b]);
    p->bones[b].parent = parents[b];
    p->bones[b].inv_bind = sf_make_tsl_fmat4(0, -bone_y[b], 0);
  }

  int vi = 0, fi = 0;
  /* Build body parts as boxes skinned to their bone */
  /* hips (belt area) */
  add_box_part(p, 0, 0.9f, 0, 0.15f, 0.08f, 0.09f, 0, &vi, &fi);
  /* spine (torso/chest - wider, taller) */
  add_box_part(p, 0, 1.2f, 0, 0.18f, 0.22f, 0.10f, 1, &vi, &fi);
  /* head (slightly taller than wide) */
  add_box_part(p, 0, 1.62f, 0, 0.08f, 0.10f, 0.08f, 2, &vi, &fi);
  /* L upper arm */
  add_box_part(p, -0.25f, 1.32f, 0, 0.05f, 0.14f, 0.05f, 3, &vi, &fi);
  /* L forearm */
  add_box_part(p, -0.25f, 1.04f, 0, 0.04f, 0.14f, 0.04f, 4, &vi, &fi);
  /* R upper arm */
  add_box_part(p, 0.25f, 1.32f, 0, 0.05f, 0.14f, 0.05f, 5, &vi, &fi);
  /* R forearm */
  add_box_part(p, 0.25f, 1.04f, 0, 0.04f, 0.14f, 0.04f, 6, &vi, &fi);
  /* L thigh */
  add_box_part(p, -0.08f, 0.55f, 0, 0.06f, 0.18f, 0.06f, 7, &vi, &fi);
  /* L shin */
  add_box_part(p, -0.08f, 0.22f, 0, 0.05f, 0.18f, 0.05f, 8, &vi, &fi);
  /* R thigh */
  add_box_part(p, 0.08f, 0.55f, 0, 0.06f, 0.18f, 0.06f, 9, &vi, &fi);
  /* R shin */
  add_box_part(p, 0.08f, 0.22f, 0, 0.05f, 0.18f, 0.05f, 10, &vi, &fi);

  p->v_cnt = vi;
  p->f_cnt = fi;

  /* Compute bounding sphere (can't cast to sf_obj_t, layout differs) */
  {
    sf_fvec3_t c = {0,0,0};
    for (int i = 0; i < vi; i++) { c.x += p->v[i].x; c.y += p->v[i].y; c.z += p->v[i].z; }
    float inv = 1.0f / (float)vi;
    c.x *= inv; c.y *= inv; c.z *= inv;
    float r2 = 0;
    for (int i = 0; i < vi; i++) {
      float dx = p->v[i].x-c.x, dy = p->v[i].y-c.y, dz = p->v[i].z-c.z;
      float d2 = dx*dx+dy*dy+dz*dz;
      if (d2 > r2) r2 = d2;
    }
    p->bs_center = c;
    p->bs_radius = sqrtf(r2);
  }

  /* Create animation clips */
  /* We'll create clips programmatically: idle and walk */
  p->clips = (sf_anim_clip_t*)sf_arena_alloc(ctx, &ctx->arena, 3 * sizeof(sf_anim_clip_t));
  p->clip_cnt = 3;

  /* CLIP 0: IDLE - slight breathing sway on spine */
  {
    sf_anim_clip_t *clip = &p->clips[0];
    memset(clip, 0, sizeof(*clip));
    snprintf(clip->name, 32, "idle");
    clip->duration = 2.0f;
    clip->chan_cnt = 1;
    clip->channels = (sf_anim_chan_t*)sf_arena_alloc(ctx, &ctx->arena, 1 * sizeof(sf_anim_chan_t));
    memset(clip->channels, 0, sizeof(sf_anim_chan_t));
    clip->channels[0].bone = 1;
    clip->channels[0].pos_cnt = 3;
    clip->channels[0].pos = (sf_key_pos_t*)sf_arena_alloc(ctx, &ctx->arena, 3 * sizeof(sf_key_pos_t));
    clip->channels[0].pos[0] = (sf_key_pos_t){0.0f, {0, 1.2f, 0}};
    clip->channels[0].pos[1] = (sf_key_pos_t){1.0f, {0, 1.22f, 0}};
    clip->channels[0].pos[2] = (sf_key_pos_t){2.0f, {0, 1.2f, 0}};
  }

  /* CLIP 1: WALK - legs swing, arms swing opposite */
  {
    sf_anim_clip_t *clip = &p->clips[1];
    memset(clip, 0, sizeof(*clip));
    snprintf(clip->name, 32, "walk");
    clip->duration = 0.8f;
    clip->chan_cnt = 4;
    clip->channels = (sf_anim_chan_t*)sf_arena_alloc(ctx, &ctx->arena, 4 * sizeof(sf_anim_chan_t));
    memset(clip->channels, 0, 4 * sizeof(sf_anim_chan_t));

    /* L thigh swing */
    clip->channels[0].bone = 7;
    clip->channels[0].rot_cnt = 3;
    clip->channels[0].rot = (sf_key_rot_t*)sf_arena_alloc(ctx, &ctx->arena, 3 * sizeof(sf_key_rot_t));
    clip->channels[0].rot[0] = (sf_key_rot_t){0.0f, sf_quat_from_euler((sf_fvec3_t){0.4f, 0, 0})};
    clip->channels[0].rot[1] = (sf_key_rot_t){0.4f, sf_quat_from_euler((sf_fvec3_t){-0.4f, 0, 0})};
    clip->channels[0].rot[2] = (sf_key_rot_t){0.8f, sf_quat_from_euler((sf_fvec3_t){0.4f, 0, 0})};

    /* R thigh swing (opposite) */
    clip->channels[1].bone = 9;
    clip->channels[1].rot_cnt = 3;
    clip->channels[1].rot = (sf_key_rot_t*)sf_arena_alloc(ctx, &ctx->arena, 3 * sizeof(sf_key_rot_t));
    clip->channels[1].rot[0] = (sf_key_rot_t){0.0f, sf_quat_from_euler((sf_fvec3_t){-0.4f, 0, 0})};
    clip->channels[1].rot[1] = (sf_key_rot_t){0.4f, sf_quat_from_euler((sf_fvec3_t){0.4f, 0, 0})};
    clip->channels[1].rot[2] = (sf_key_rot_t){0.8f, sf_quat_from_euler((sf_fvec3_t){-0.4f, 0, 0})};

    /* L arm swing (opposite to L leg) */
    clip->channels[2].bone = 3;
    clip->channels[2].rot_cnt = 3;
    clip->channels[2].rot = (sf_key_rot_t*)sf_arena_alloc(ctx, &ctx->arena, 3 * sizeof(sf_key_rot_t));
    clip->channels[2].rot[0] = (sf_key_rot_t){0.0f, sf_quat_from_euler((sf_fvec3_t){-0.3f, 0, 0})};
    clip->channels[2].rot[1] = (sf_key_rot_t){0.4f, sf_quat_from_euler((sf_fvec3_t){0.3f, 0, 0})};
    clip->channels[2].rot[2] = (sf_key_rot_t){0.8f, sf_quat_from_euler((sf_fvec3_t){-0.3f, 0, 0})};

    /* R arm swing */
    clip->channels[3].bone = 5;
    clip->channels[3].rot_cnt = 3;
    clip->channels[3].rot = (sf_key_rot_t*)sf_arena_alloc(ctx, &ctx->arena, 3 * sizeof(sf_key_rot_t));
    clip->channels[3].rot[0] = (sf_key_rot_t){0.0f, sf_quat_from_euler((sf_fvec3_t){0.3f, 0, 0})};
    clip->channels[3].rot[1] = (sf_key_rot_t){0.4f, sf_quat_from_euler((sf_fvec3_t){-0.3f, 0, 0})};
    clip->channels[3].rot[2] = (sf_key_rot_t){0.8f, sf_quat_from_euler((sf_fvec3_t){0.3f, 0, 0})};
  }

  /* CLIP 2: DEATH - fall backwards */
  {
    sf_anim_clip_t *clip = &p->clips[2];
    memset(clip, 0, sizeof(*clip));
    snprintf(clip->name, 32, "death");
    clip->duration = 0.6f;
    clip->chan_cnt = 1;
    clip->channels = (sf_anim_chan_t*)sf_arena_alloc(ctx, &ctx->arena, 1 * sizeof(sf_anim_chan_t));
    memset(clip->channels, 0, sizeof(sf_anim_chan_t));
    clip->channels[0].bone = 0;
    clip->channels[0].rot_cnt = 2;
    clip->channels[0].rot = (sf_key_rot_t*)sf_arena_alloc(ctx, &ctx->arena, 2 * sizeof(sf_key_rot_t));
    clip->channels[0].rot[0] = (sf_key_rot_t){0.0f, {0,0,0,1}};
    clip->channels[0].rot[1] = (sf_key_rot_t){0.6f, sf_quat_from_euler((sf_fvec3_t){-1.5f, 0, 0.2f})};
  }

  return p;
}

/* --- BUILD WEAPON VIEWMODELS --- */
void build_weapons(sf_ctx_t *ctx) {
  /* AK-47 viewmodel from boxes */
  g.ak_barrel = sf_obj_make_box(ctx, "ak_barrel", 0.03f, 0.03f, 0.6f);
  g.ak_body   = sf_obj_make_box(ctx, "ak_body",   0.06f, 0.08f, 0.35f);
  g.ak_mag    = sf_obj_make_box(ctx, "ak_mag",    0.04f, 0.12f, 0.06f);
  g.ak_stock  = sf_obj_make_box(ctx, "ak_stock",  0.04f, 0.06f, 0.2f);

  g.vm_barrel = sf_add_enti(ctx, g.ak_barrel, "vm_barrel");
  g.vm_body   = sf_add_enti(ctx, g.ak_body,   "vm_body");
  g.vm_mag    = sf_add_enti(ctx, g.ak_mag,    "vm_mag");
  g.vm_stock  = sf_add_enti(ctx, g.ak_stock,  "vm_stock");

  /* Color the weapon parts */
  sf_tex_t *wood_tex = sf_get_texture_(ctx, "wood2", false);
  sf_tex_t *metal_tex = sf_get_texture_(ctx, "metal", false);
  if (g.vm_barrel) g.vm_barrel->tex = metal_tex;
  if (g.vm_body)   g.vm_body->tex   = metal_tex;
  if (g.vm_mag)    g.vm_mag->tex    = metal_tex;
  if (g.vm_stock)  g.vm_stock->tex  = wood_tex;

  /* Knife */
  sf_obj_t *blade = sf_obj_make_box(ctx, "knife_blade", 0.02f, 0.15f, 0.02f);
  sf_obj_t *handle = sf_obj_make_box(ctx, "knife_handle", 0.03f, 0.08f, 0.03f);
  g.knife_blade  = sf_add_enti(ctx, blade,  "vm_knife_blade");
  g.knife_handle = sf_add_enti(ctx, handle, "vm_knife_handle");
  if (g.knife_blade)  g.knife_blade->tex  = metal_tex;
  if (g.knife_handle) g.knife_handle->tex = wood_tex;
}

/* --- PLAYER INIT --- */
void player_init(player_t *p, sf_fvec3_t spawn) {
  memset(p, 0, sizeof(player_t));
  p->pos = spawn;
  p->yaw = 0; p->pitch = 0;
  p->hp = PLAYER_MAX_HP;
  p->armor = 0;
  p->money = START_MONEY;
  p->cur_wpn = WPN_KNIFE;
  p->weapons[WPN_KNIFE] = (weapon_t){WPN_KNIFE, 0, 0, 0, 0, false};
  p->weapons[WPN_AK47]  = (weapon_t){WPN_AK47, 0, 0, 0, 0, false};
  p->body.gravity = GRAVITY;
  p->body.friction = 8.0f;
  p->body.vel = (sf_fvec3_t){0,0,0};
  p->body.box = sf_aabb_from_pos_size(p->pos, (sf_fvec3_t){PLAYER_HALF_W, PLAYER_HALF_H, PLAYER_HALF_W});
}

/* --- SPAWN ENEMIES --- */
void spawn_enemies(int count) {
  if (count > MAX_ENEMIES) count = MAX_ENEMIES;
  g.enemy_count = count;
  for (int i = 0; i < count; i++) {
    enemy_t *e = &g.enemies[i];
    memset(e, 0, sizeof(enemy_t));
    e->active = true;
    e->hp = ENEMY_HP;
    e->state = AI_PATROL;
    e->cur_waypoint = i % g.wp_count;
    e->next_waypoint = (e->cur_waypoint + 1) % g.wp_count;
    e->pos = g.waypoints[e->cur_waypoint];
    e->pos.y = 0;
    e->yaw = 0;
    e->box = sf_aabb_from_pos_size(e->pos, (sf_fvec3_t){0.3f, 0.9f, 0.3f});

    char pname[32];
    snprintf(pname, 32, "enemy_%d", i);
    e->puppet = sf_puppet_add_inst(&g.ctx, g.human_puppet, pname);
    if (e->puppet) {
      e->puppet->frame->pos = e->pos;
      e->puppet->frame->is_dirty = true;
      e->puppet->tex = sf_get_texture_(&g.ctx, "brick2", false);
      e->puppet->tex_scale = (sf_fvec2_t){2.0f, 2.0f};
      sf_puppet_play(e->puppet, 0, true);
    }
  }
}

/* --- KILL FEED --- */
void add_kill_feed(const char *text) {
  for (int i = MAX_KILL_FEED - 1; i > 0; i--)
    g.kill_feed[i] = g.kill_feed[i-1];
  snprintf(g.kill_feed[0].text, 64, "%s", text);
  g.kill_feed[0].timer = 4.0f;
}

/* --- SHOOTING --- */
void player_shoot(sf_ctx_t *ctx) {
  player_t *p = &g.player;
  weapon_t *w = &p->weapons[p->cur_wpn];

  if (w->fire_cd > 0 || w->reloading) return;

  if (p->cur_wpn == WPN_AK47) {
    if (w->mag <= 0) return;
    w->mag--;
    w->fire_cd = AK_FIRE_RATE;
    p->recoil += 2.0f;

    /* Raycast from camera center */
    sf_fmat4_t cm = ctx->main_camera.frame->global_M;
    sf_fvec3_t eye = {cm.m[3][0], cm.m[3][1], cm.m[3][2]};
    sf_fvec3_t fwd = {-cm.m[2][0], -cm.m[2][1], -cm.m[2][2]};
    sf_ray_t ray = {eye, fwd};

    float best_t = AK_RANGE;
    int best_enemy = -1;
    for (int i = 0; i < g.enemy_count; i++) {
      if (!g.enemies[i].active || g.enemies[i].state == AI_DEAD) continue;
      float t;
      if (sf_ray_aabb(ray, g.enemies[i].box.min, g.enemies[i].box.max, &t)) {
        if (t < best_t) { best_t = t; best_enemy = i; }
      }
    }
    if (best_enemy >= 0) {
      enemy_t *e = &g.enemies[best_enemy];
      e->hp -= AK_DAMAGE;
      g.hit_marker_timer = 0.15f;
      if (e->hp <= 0) {
        e->state = AI_DEAD;
        e->state_timer = 0;
        if (e->puppet) sf_puppet_play(e->puppet, 2, false);
        p->kills++;
        p->money += KILL_REWARD;
        char msg[64];
        snprintf(msg, 64, "Killed enemy_%d", best_enemy);
        add_kill_feed(msg);
      }
    }
  } else if (p->cur_wpn == WPN_KNIFE) {
    w->fire_cd = KNIFE_RATE;

    sf_fmat4_t cm = ctx->main_camera.frame->global_M;
    sf_fvec3_t eye = {cm.m[3][0], cm.m[3][1], cm.m[3][2]};
    sf_fvec3_t fwd = {-cm.m[2][0], -cm.m[2][1], -cm.m[2][2]};
    sf_ray_t ray = {eye, fwd};

    for (int i = 0; i < g.enemy_count; i++) {
      if (!g.enemies[i].active || g.enemies[i].state == AI_DEAD) continue;
      float t;
      if (sf_ray_aabb(ray, g.enemies[i].box.min, g.enemies[i].box.max, &t) && t < KNIFE_RANGE) {
        g.enemies[i].hp -= KNIFE_DAMAGE;
        g.hit_marker_timer = 0.15f;
        if (g.enemies[i].hp <= 0) {
          g.enemies[i].state = AI_DEAD;
          g.enemies[i].state_timer = 0;
          if (g.enemies[i].puppet) sf_puppet_play(g.enemies[i].puppet, 2, false);
          p->kills++;
          p->money += KILL_REWARD;
          char msg[64];
          snprintf(msg, 64, "Knifed enemy_%d", i);
          add_kill_feed(msg);
        }
        break;
      }
    }
  }
}

/* --- AI UPDATE --- */
void update_enemies(sf_ctx_t *ctx, float dt) {
  player_t *p = &g.player;
  for (int i = 0; i < g.enemy_count; i++) {
    enemy_t *e = &g.enemies[i];
    if (!e->active) continue;

    e->fire_cd -= dt;
    if (e->fire_cd < 0) e->fire_cd = 0;
    e->state_timer += dt;

    if (e->state == AI_DEAD) {
      if (e->puppet) sf_puppet_update(ctx, e->puppet, dt);
      continue;
    }

    /* Check line of sight to player */
    sf_fvec3_t to_player = sf_fvec3_sub(p->pos, e->pos);
    float dist_sq = to_player.x*to_player.x + to_player.y*to_player.y + to_player.z*to_player.z;
    float dist = sqrtf(dist_sq);
    bool can_see = (dist < ENEMY_DETECT_RANGE);

    if (can_see) {
      /* Raycast to check for wall obstruction */
      sf_fvec3_t dir = sf_fvec3_norm(to_player);
      sf_ray_t ray = {e->pos, dir};
      ray.o.y += 1.5f;
      for (int w = 0; w < g.wall_count; w++) {
        float t;
        if (sf_ray_aabb(ray, g.walls[w].min, g.walls[w].max, &t) && t < dist) {
          can_see = false;
          break;
        }
      }
    }

    if (can_see && e->state != AI_DEAD) {
      e->last_known_player = p->pos;
      if (dist < 15.0f) {
        e->state = AI_ATTACK;
      } else {
        e->state = AI_CHASE;
      }
    } else if (e->state == AI_ATTACK || e->state == AI_CHASE) {
      if (e->state_timer > 3.0f) {
        e->state = AI_PATROL;
        e->state_timer = 0;
      }
    }

    sf_fvec3_t move_dir = {0,0,0};
    float speed = ENEMY_SPEED;

    switch (e->state) {
      case AI_PATROL: {
        sf_fvec3_t target = g.waypoints[e->next_waypoint];
        sf_fvec3_t diff = sf_fvec3_sub(target, e->pos);
        diff.y = 0;
        float d = sqrtf(diff.x*diff.x + diff.z*diff.z);
        if (d < 1.0f) {
          e->cur_waypoint = e->next_waypoint;
          e->next_waypoint = (e->next_waypoint + 1) % g.wp_count;
        }
        if (d > 0.1f) move_dir = sf_fvec3_norm(diff);
        if (e->puppet && !e->puppet->is_playing) sf_puppet_play(e->puppet, 1, true);
        break;
      }
      case AI_CHASE: {
        sf_fvec3_t diff = sf_fvec3_sub(e->last_known_player, e->pos);
        diff.y = 0;
        float d = sqrtf(diff.x*diff.x + diff.z*diff.z);
        if (d > 0.5f) move_dir = sf_fvec3_norm(diff);
        speed = ENEMY_SPEED * 1.3f;
        if (e->puppet && e->puppet->cur_clip != 1) sf_puppet_play(e->puppet, 1, true);
        break;
      }
      case AI_ATTACK: {
        /* Face player */
        sf_fvec3_t diff = sf_fvec3_sub(p->pos, e->pos);
        e->yaw = atan2f(-diff.x, -diff.z);

        /* Strafe slightly */
        float strafe = sinf(ctx->elapsed_time * 2.0f + (float)i * 1.5f) * 0.3f;
        sf_fvec3_t right = {cosf(e->yaw), 0, -sinf(e->yaw)};
        move_dir = (sf_fvec3_t){right.x * strafe, 0, right.z * strafe};
        speed = ENEMY_SPEED * 0.5f;

        /* Shoot */
        if (e->fire_cd <= 0 && can_see) {
          e->fire_cd = ENEMY_FIRE_RATE;
          /* Hit check with spread */
          float spread = 0.05f;
          (void)spread;
          float hit_chance = 0.6f - (dist / ENEMY_DETECT_RANGE) * 0.4f;
          if (((float)rand() / (float)RAND_MAX) < hit_chance) {
            int dmg = ENEMY_DAMAGE;
            if (p->armor > 0) {
              int absorbed = dmg / 2;
              if (absorbed > p->armor) absorbed = p->armor;
              p->armor -= absorbed;
              dmg -= absorbed;
            }
            p->hp -= dmg;
            g.damage_flash = 0.3f;
            if (p->hp <= 0) {
              p->hp = 0;
              p->deaths++;
              g.state = STATE_GAME_OVER;
              g.state_timer = 0;
            }
          }
        }
        if (e->puppet && e->puppet->cur_clip != 0) sf_puppet_play(e->puppet, 0, true);
        break;
      }
      default: break;
    }

    /* Apply movement */
    if (move_dir.x != 0 || move_dir.z != 0) {
      e->pos.x += move_dir.x * speed * dt;
      e->pos.z += move_dir.z * speed * dt;
      e->yaw = atan2f(-move_dir.x, -move_dir.z);
    }

    /* Simple collision against walls */
    e->box = sf_aabb_from_pos_size(e->pos, (sf_fvec3_t){0.3f, 0.9f, 0.3f});
    for (int w = 0; w < g.wall_count; w++) {
      if (sf_aabb_overlap(e->box, g.walls[w])) {
        sf_fvec3_t mtv = sf_aabb_resolve(e->box, g.walls[w]);
        e->pos = sf_fvec3_add(e->pos, mtv);
        e->box = sf_aabb_from_pos_size(e->pos, (sf_fvec3_t){0.3f, 0.9f, 0.3f});
      }
    }

    /* Update puppet */
    if (e->puppet) {
      e->puppet->frame->pos = e->pos;
      e->puppet->frame->rot = (sf_fvec3_t){0, e->yaw, 0};
      e->puppet->frame->is_dirty = true;
      sf_puppet_update(ctx, e->puppet, dt);
    }
  }
}

/* --- CHECK ROUND STATE --- */
void check_round_state(void) {
  if (g.state != STATE_PLAYING) return;
  bool all_dead = true;
  for (int i = 0; i < g.enemy_count; i++) {
    if (g.enemies[i].active && g.enemies[i].state != AI_DEAD) {
      all_dead = false;
      break;
    }
  }
  if (all_dead) {
    g.state = STATE_ROUND_WON;
    g.state_timer = 0;
    g.player.money += ROUND_REWARD;
  }
}

/* --- START ROUND --- */
void start_round(void) {
  g.round_num++;
  g.state = STATE_BUY;
  g.state_timer = 0;
  player_init(&g.player, g.ct_spawn);

  /* Keep money, kills, deaths, and purchased weapons */
  int enemies = 2 + g.round_num;
  if (enemies > MAX_ENEMIES) enemies = MAX_ENEMIES;

  /* Remove old puppet instances */
  g.ctx.puppet_inst_count = 0;

  spawn_enemies(enemies);
}

/* --- DRAW SKY GRADIENT --- */
void draw_sky(sf_ctx_t *ctx, sf_cam_t *cam) {
  int h = cam->h;
  int w = cam->w;
  for (int y = 0; y < h; y++) {
    float t = (float)y / (float)h;
    uint8_t r = (uint8_t)(100 + (uint8_t)(t * 80));
    uint8_t gr = (uint8_t)(140 + (uint8_t)(t * 60));
    uint8_t b = (uint8_t)(200 + (uint8_t)(t * 40));
    if (r > 255) r = 255; if (gr > 255) gr = 255; if (b > 255) b = 255;
    sf_pkd_clr_t c = _sf_pack_color((sf_unpkd_clr_t){r, gr, b, 255});
    sf_rect(ctx, cam, c, (sf_ivec2_t){0, y}, (sf_ivec2_t){w, y+1});
  }
}

/* --- DRAW VIEWMODEL --- */
void draw_viewmodel(sf_ctx_t *ctx, sf_cam_t *cam) {
  player_t *p = &g.player;

  /* Calculate sway */
  float target_sx = ctx->input.mouse_dx * -0.002f;
  float target_sy = ctx->input.mouse_dy * -0.002f;
  g.wpn_sway_x += (target_sx - g.wpn_sway_x) * 8.0f * ctx->delta_time;
  g.wpn_sway_y += (target_sy - g.wpn_sway_y) * 8.0f * ctx->delta_time;

  /* Head bob offset */
  float bob_y = 0;
  if (p->walking && p->body.grounded) {
    bob_y = sinf(p->bob_timer * 10.0f) * 0.015f;
  }

  /* Clear depth so viewmodel draws on top */
  sf_clear_depth(ctx, cam);

  float sway_x = g.wpn_sway_x;
  float sway_y = g.wpn_sway_y + bob_y;

  /* Recoil kick */
  float recoil_y = p->recoil * 0.01f;

  if (p->cur_wpn == WPN_AK47) {
    /* Position AK parts relative to camera */
    sf_fmat4_t cm = cam->frame->global_M;
    sf_fvec3_t eye = {cm.m[3][0], cm.m[3][1], cm.m[3][2]};
    sf_fvec3_t fwd = {-cm.m[2][0], -cm.m[2][1], -cm.m[2][2]};
    sf_fvec3_t rgt = { cm.m[0][0],  cm.m[0][1],  cm.m[0][2]};
    sf_fvec3_t up  = { cm.m[1][0],  cm.m[1][1],  cm.m[1][2]};

    /* Base position: right side, slightly down and forward */
    sf_fvec3_t base;
    base.x = eye.x + fwd.x*0.4f + rgt.x*(0.15f+sway_x) + up.x*(-0.12f+sway_y-recoil_y);
    base.y = eye.y + fwd.y*0.4f + rgt.y*(0.15f+sway_x) + up.y*(-0.12f+sway_y-recoil_y);
    base.z = eye.z + fwd.z*0.4f + rgt.z*(0.15f+sway_x) + up.z*(-0.12f+sway_y-recoil_y);

    if (g.vm_barrel) { sf_enti_set_pos(ctx, g.vm_barrel, base.x + fwd.x*0.2f, base.y + fwd.y*0.2f, base.z + fwd.z*0.2f); sf_enti_set_rot(ctx, g.vm_barrel, p->pitch, -p->yaw, 0); }
    if (g.vm_body)   { sf_enti_set_pos(ctx, g.vm_body,   base.x, base.y, base.z); sf_enti_set_rot(ctx, g.vm_body, p->pitch, -p->yaw, 0); }
    if (g.vm_mag)    { sf_enti_set_pos(ctx, g.vm_mag,    base.x + up.x*(-0.05f), base.y + up.y*(-0.05f), base.z + up.z*(-0.05f)); sf_enti_set_rot(ctx, g.vm_mag, p->pitch, -p->yaw, 0); }
    if (g.vm_stock)  { sf_enti_set_pos(ctx, g.vm_stock,  base.x - fwd.x*0.2f, base.y - fwd.y*0.2f, base.z - fwd.z*0.2f); sf_enti_set_rot(ctx, g.vm_stock, p->pitch, -p->yaw, 0); }

    /* Make visible */
    if (g.vm_barrel) g.vm_barrel->frame->scale = (sf_fvec3_t){1,1,1};
    if (g.vm_body)   g.vm_body->frame->scale   = (sf_fvec3_t){1,1,1};
    if (g.vm_mag)    g.vm_mag->frame->scale     = (sf_fvec3_t){1,1,1};
    if (g.vm_stock)  g.vm_stock->frame->scale   = (sf_fvec3_t){1,1,1};
    if (g.knife_blade)  g.knife_blade->frame->scale  = (sf_fvec3_t){0,0,0};
    if (g.knife_handle) g.knife_handle->frame->scale = (sf_fvec3_t){0,0,0};
  } else {
    /* Knife */
    sf_fmat4_t cm = cam->frame->global_M;
    sf_fvec3_t eye = {cm.m[3][0], cm.m[3][1], cm.m[3][2]};
    sf_fvec3_t fwd = {-cm.m[2][0], -cm.m[2][1], -cm.m[2][2]};
    sf_fvec3_t rgt = { cm.m[0][0],  cm.m[0][1],  cm.m[0][2]};
    sf_fvec3_t up  = { cm.m[1][0],  cm.m[1][1],  cm.m[1][2]};

    sf_fvec3_t base;
    base.x = eye.x + fwd.x*0.35f + rgt.x*(0.18f+sway_x) + up.x*(-0.1f+sway_y);
    base.y = eye.y + fwd.y*0.35f + rgt.y*(0.18f+sway_x) + up.y*(-0.1f+sway_y);
    base.z = eye.z + fwd.z*0.35f + rgt.z*(0.18f+sway_x) + up.z*(-0.1f+sway_y);

    if (g.knife_blade) { sf_enti_set_pos(ctx, g.knife_blade, base.x + up.x*0.08f, base.y + up.y*0.08f, base.z + up.z*0.08f); sf_enti_set_rot(ctx, g.knife_blade, p->pitch, -p->yaw, 0); g.knife_blade->frame->scale = (sf_fvec3_t){1,1,1}; }
    if (g.knife_handle) { sf_enti_set_pos(ctx, g.knife_handle, base.x, base.y, base.z); sf_enti_set_rot(ctx, g.knife_handle, p->pitch, -p->yaw, 0); g.knife_handle->frame->scale = (sf_fvec3_t){1,1,1}; }

    if (g.vm_barrel) g.vm_barrel->frame->scale = (sf_fvec3_t){0,0,0};
    if (g.vm_body)   g.vm_body->frame->scale   = (sf_fvec3_t){0,0,0};
    if (g.vm_mag)    g.vm_mag->frame->scale     = (sf_fvec3_t){0,0,0};
    if (g.vm_stock)  g.vm_stock->frame->scale   = (sf_fvec3_t){0,0,0};
  }

  /* Re-render viewmodel entities on top */
  if (p->cur_wpn == WPN_AK47) {
    sf_update_frames(&g.ctx);
    if (g.vm_barrel) sf_render_enti(ctx, cam, g.vm_barrel);
    if (g.vm_body)   sf_render_enti(ctx, cam, g.vm_body);
    if (g.vm_mag)    sf_render_enti(ctx, cam, g.vm_mag);
    if (g.vm_stock)  sf_render_enti(ctx, cam, g.vm_stock);
  } else {
    sf_update_frames(&g.ctx);
    if (g.knife_blade)  sf_render_enti(ctx, cam, g.knife_blade);
    if (g.knife_handle) sf_render_enti(ctx, cam, g.knife_handle);
  }
}

/* --- DRAW HUD --- */
void draw_hud(sf_ctx_t *ctx, sf_cam_t *cam) {
  player_t *p = &g.player;
  int w = cam->w, h = cam->h;
  char buf[64];

  /* Crosshair */
  int cx = w / 2, cy = h / 2;
  int gap = 4;
  if (!p->body.grounded) gap = 8;
  if (p->recoil > 0.5f) gap += (int)(p->recoil);
  sf_pkd_clr_t cross_c = SF_CLR_WHITE;
  sf_line(ctx, cam, cross_c, (sf_ivec2_t){cx - 12, cy}, (sf_ivec2_t){cx - gap, cy});
  sf_line(ctx, cam, cross_c, (sf_ivec2_t){cx + gap, cy}, (sf_ivec2_t){cx + 12, cy});
  sf_line(ctx, cam, cross_c, (sf_ivec2_t){cx, cy - 12}, (sf_ivec2_t){cx, cy - gap});
  sf_line(ctx, cam, cross_c, (sf_ivec2_t){cx, cy + gap}, (sf_ivec2_t){cx, cy + 12});

  /* Hit marker */
  if (g.hit_marker_timer > 0) {
    int hm = 6;
    sf_line(ctx, cam, SF_CLR_WHITE, (sf_ivec2_t){cx-hm, cy-hm}, (sf_ivec2_t){cx-3, cy-3});
    sf_line(ctx, cam, SF_CLR_WHITE, (sf_ivec2_t){cx+hm, cy-hm}, (sf_ivec2_t){cx+3, cy-3});
    sf_line(ctx, cam, SF_CLR_WHITE, (sf_ivec2_t){cx-hm, cy+hm}, (sf_ivec2_t){cx-3, cy+3});
    sf_line(ctx, cam, SF_CLR_WHITE, (sf_ivec2_t){cx+hm, cy+hm}, (sf_ivec2_t){cx+3, cy+3});
  }

  /* Health */
  sf_pkd_clr_t hp_color = (p->hp > 50) ? SF_CLR_GREEN : SF_CLR_RED;
  sf_rect(ctx, cam, 0xFF333333, (sf_ivec2_t){10, h-40}, (sf_ivec2_t){210, h-20});
  int hp_w = (int)(200.0f * ((float)p->hp / PLAYER_MAX_HP));
  if (hp_w > 0) sf_rect(ctx, cam, hp_color, (sf_ivec2_t){10, h-40}, (sf_ivec2_t){10+hp_w, h-20});
  snprintf(buf, 64, "HP: %d", p->hp);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){15, h-38}, SF_CLR_WHITE, 2);

  /* Armor */
  if (p->armor > 0) {
    sf_rect(ctx, cam, 0xFF333333, (sf_ivec2_t){10, h-55}, (sf_ivec2_t){210, h-43});
    int ar_w = (int)(200.0f * ((float)p->armor / PLAYER_MAX_ARMOR));
    sf_rect(ctx, cam, SF_CLR_BLUE, (sf_ivec2_t){10, h-55}, (sf_ivec2_t){10+ar_w, h-43});
  }

  /* Ammo */
  if (p->cur_wpn == WPN_AK47) {
    weapon_t *wpn = &p->weapons[WPN_AK47];
    snprintf(buf, 64, "%d / %d", wpn->mag, wpn->reserve);
    sf_put_text(ctx, cam, buf, (sf_ivec2_t){w-180, h-38}, SF_CLR_WHITE, 2);
    if (wpn->reloading) {
      sf_put_text(ctx, cam, "RELOADING", (sf_ivec2_t){center_x("RELOADING",1,w), h/2+30}, SF_CLR_WHITE, 1);
    }
  } else {
    sf_put_text(ctx, cam, "KNIFE", (sf_ivec2_t){w-100, h-38}, SF_CLR_WHITE, 2);
  }

  /* Money */
  snprintf(buf, 64, "$%d", p->money);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){w-120, 10}, SF_CLR_GREEN, 2);

  /* Round */
  snprintf(buf, 64, "Round %d", g.round_num);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){center_x(buf,2,w), 10}, SF_CLR_WHITE, 2);

  /* Kill feed */
  for (int i = 0; i < MAX_KILL_FEED; i++) {
    if (g.kill_feed[i].timer > 0) {
      sf_put_text(ctx, cam, g.kill_feed[i].text, (sf_ivec2_t){w-250, 40 + i*18}, SF_CLR_WHITE, 1);
    }
  }

  /* Damage flash */
  if (g.damage_flash > 0) {
    uint8_t alpha = (uint8_t)(g.damage_flash * 200);
    sf_pkd_clr_t flash = _sf_pack_color((sf_unpkd_clr_t){255, 0, 0, alpha});
    sf_rect(ctx, cam, flash, (sf_ivec2_t){0,0}, (sf_ivec2_t){w, h});
  }

  /* Kills/Deaths */
  snprintf(buf, 64, "K:%d D:%d", p->kills, p->deaths);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){10, 10}, SF_CLR_WHITE, 1);
}

/* --- DRAW MENU --- */
void draw_menu(sf_ctx_t *ctx, sf_cam_t *cam) {
  int w = cam->w, h = cam->h;
  sf_rect(ctx, cam, 0xCC000000, (sf_ivec2_t){0,0}, (sf_ivec2_t){w,h});
  const char *t1 = "SF STRIKE";
  const char *t2 = "Press ENTER to start";
  const char *t3 = "WASD move  Mouse aim  LMB shoot";
  const char *t4 = "R reload  1/2 switch weapon  SPACE jump";
  sf_put_text(ctx, cam, t1, (sf_ivec2_t){center_x(t1,4,w), h/2-40}, SF_CLR_WHITE, 4);
  sf_put_text(ctx, cam, t2, (sf_ivec2_t){center_x(t2,1,w), h/2+20}, SF_CLR_WHITE, 1);
  sf_put_text(ctx, cam, t3, (sf_ivec2_t){center_x(t3,1,w), h/2+50}, SF_CLR_WHITE, 1);
  sf_put_text(ctx, cam, t4, (sf_ivec2_t){center_x(t4,1,w), h/2+65}, SF_CLR_WHITE, 1);
}

/* --- DRAW BUY MENU --- */
void draw_buy_menu(sf_ctx_t *ctx, sf_cam_t *cam) {
  int w = cam->w, h = cam->h;
  char buf[64];
  sf_rect(ctx, cam, 0xBB000000, (sf_ivec2_t){w/2-150, h/2-80}, (sf_ivec2_t){w/2+150, h/2+80});
  const char *title = "BUY PHASE";
  sf_put_text(ctx, cam, title, (sf_ivec2_t){center_x(title,2,w), h/2-70}, SF_CLR_WHITE, 2);
  snprintf(buf, 64, "Money: $%d", g.player.money);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){center_x(buf,1,w), h/2-40}, SF_CLR_GREEN, 1);
  snprintf(buf, 64, "Press B: AK-47 ($%d)", AK_COST);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){center_x(buf,1,w), h/2-10}, SF_CLR_WHITE, 1);
  snprintf(buf, 64, "Press V: Armor ($%d)", ARMOR_COST);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){center_x(buf,1,w), h/2+10}, SF_CLR_WHITE, 1);
  snprintf(buf, 64, "Auto-start in %.0fs", BUY_TIME - g.state_timer);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){center_x(buf,1,w), h/2+50}, SF_CLR_WHITE, 1);
}

/* --- DRAW GAME OVER --- */
void draw_game_over(sf_ctx_t *ctx, sf_cam_t *cam) {
  int w = cam->w, h = cam->h;
  char buf[64];
  sf_rect(ctx, cam, 0xCC000000, (sf_ivec2_t){0,0}, (sf_ivec2_t){w,h});
  const char *t1 = "YOU DIED";
  sf_put_text(ctx, cam, t1, (sf_ivec2_t){center_x(t1,4,w), h/2-30}, SF_CLR_RED, 4);
  snprintf(buf, 64, "Survived %d rounds  Kills: %d", g.round_num, g.player.kills);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){center_x(buf,1,w), h/2+20}, SF_CLR_WHITE, 1);
  const char *t3 = "Press R to restart";
  sf_put_text(ctx, cam, t3, (sf_ivec2_t){center_x(t3,1,w), h/2+50}, SF_CLR_WHITE, 1);
}

/* --- DRAW ROUND WON --- */
void draw_round_won(sf_ctx_t *ctx, sf_cam_t *cam) {
  int w = cam->w, h = cam->h;
  char buf[64];
  snprintf(buf, 64, "ROUND %d CLEAR!", g.round_num);
  sf_put_text(ctx, cam, buf, (sf_ivec2_t){center_x(buf,2,w), 50}, SF_CLR_GREEN, 2);
}

/* --- MAIN LOOP CALLBACK --- */
void on_render_start(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
  (void)ev; (void)userdata;
  float dt = ctx->delta_time;
  if (dt > 0.05f) dt = 0.05f;
  player_t *p = &g.player;
  sf_cam_t *cam = &ctx->main_camera;

  /* Update timers */
  g.state_timer += dt;
  g.hit_marker_timer -= dt;
  if (g.hit_marker_timer < 0) g.hit_marker_timer = 0;
  g.damage_flash -= dt * 2.0f;
  if (g.damage_flash < 0) g.damage_flash = 0;
  for (int i = 0; i < MAX_KILL_FEED; i++) {
    g.kill_feed[i].timer -= dt;
    if (g.kill_feed[i].timer < 0) g.kill_feed[i].timer = 0;
  }

  /* Weapon cooldowns */
  for (int i = 0; i < WPN_COUNT; i++) {
    p->weapons[i].fire_cd -= dt;
    if (p->weapons[i].fire_cd < 0) p->weapons[i].fire_cd = 0;
    if (p->weapons[i].reloading) {
      p->weapons[i].reload_cd -= dt;
      if (p->weapons[i].reload_cd <= 0) {
        p->weapons[i].reloading = false;
        int need = AK_MAG_SIZE - p->weapons[i].mag;
        int avail = p->weapons[i].reserve < need ? p->weapons[i].reserve : need;
        p->weapons[i].mag += avail;
        p->weapons[i].reserve -= avail;
      }
    }
  }

  /* Recoil recovery */
  p->recoil *= (1.0f - 10.0f * dt);
  if (p->recoil < 0.01f) p->recoil = 0;

  /* Hide viewmodel entities so sf_render_ctx doesn't draw them in world */
  if (g.vm_barrel)    g.vm_barrel->frame->scale    = (sf_fvec3_t){0,0,0};
  if (g.vm_body)      g.vm_body->frame->scale      = (sf_fvec3_t){0,0,0};
  if (g.vm_mag)       g.vm_mag->frame->scale       = (sf_fvec3_t){0,0,0};
  if (g.vm_stock)     g.vm_stock->frame->scale     = (sf_fvec3_t){0,0,0};
  if (g.knife_blade)  g.knife_blade->frame->scale  = (sf_fvec3_t){0,0,0};
  if (g.knife_handle) g.knife_handle->frame->scale = (sf_fvec3_t){0,0,0};

  if (g.state == STATE_MENU) {
    /* Position camera high up looking down for menu background */
    cam->frame->pos = (sf_fvec3_t){0, 8.0f, -20.0f};
    cam->frame->rot = (sf_fvec3_t){SF_DEG2RAD(-15.0f), 0, 0};
    cam->frame->is_dirty = true;
    return;
  }
  if (g.state == STATE_GAME_OVER) return;

  if (g.state == STATE_BUY) {
    if (sf_key_pressed(ctx, SF_KEY_B) && p->money >= AK_COST) {
      p->money -= AK_COST;
      p->weapons[WPN_AK47].mag = AK_MAG_SIZE;
      p->weapons[WPN_AK47].reserve = AK_RESERVE;
      p->cur_wpn = WPN_AK47;
    }
    if (sf_key_pressed(ctx, SF_KEY_V) && p->money >= ARMOR_COST && p->armor < PLAYER_MAX_ARMOR) {
      p->money -= ARMOR_COST;
      p->armor = PLAYER_MAX_ARMOR;
    }
    if (g.state_timer >= BUY_TIME) {
      g.state = STATE_PLAYING;
      g.state_timer = 0;
    }
    return;
  }

  if (g.state == STATE_ROUND_WON) {
    if (g.state_timer >= ROUND_WIN_TIME) {
      start_round();
    }
    return;
  }

  /* --- PLAYING STATE --- */

  /* Mouse look */
  if (g.mouse_captured) {
    float mdx = (float)ctx->input.mouse_dx * MOUSE_SENS;
    float mdy = (float)ctx->input.mouse_dy * MOUSE_SENS;
    p->yaw += SF_DEG2RAD(mdx);
    p->pitch -= SF_DEG2RAD(mdy);
    if (p->pitch > SF_DEG2RAD(89.0f)) p->pitch = SF_DEG2RAD(89.0f);
    if (p->pitch < SF_DEG2RAD(-89.0f)) p->pitch = SF_DEG2RAD(-89.0f);
  }

  /* Movement */
  float fwd_input = 0, right_input = 0;
  if (sf_key_down(ctx, SF_KEY_W)) fwd_input += 1.0f;
  if (sf_key_down(ctx, SF_KEY_S)) fwd_input -= 1.0f;
  if (sf_key_down(ctx, SF_KEY_A)) right_input -= 1.0f;
  if (sf_key_down(ctx, SF_KEY_D)) right_input += 1.0f;

  float cy = cosf(p->yaw), sy = sinf(p->yaw);
  float move_x = (-sy * fwd_input + cy * right_input);
  float move_z = (-cy * fwd_input - sy * right_input);
  float move_len = sqrtf(move_x*move_x + move_z*move_z);
  if (move_len > 0.01f) {
    move_x /= move_len;
    move_z /= move_len;
  }

  p->body.vel.x = move_x * MOVE_SPEED;
  p->body.vel.z = move_z * MOVE_SPEED;
  p->walking = (move_len > 0.01f);
  if (p->walking) p->bob_timer += dt;

  /* Jump */
  if (sf_key_pressed(ctx, SF_KEY_SPACE) && p->body.grounded) {
    p->body.vel.y = JUMP_VEL;
    p->body.grounded = false;
  }

  /* Physics */
  sf_phys_step(&p->body, dt);
  sf_phys_collide_aabbs(&p->body, g.walls, g.wall_count);

  /* Update player pos from AABB center */
  p->pos.x = (p->body.box.min.x + p->body.box.max.x) * 0.5f;
  p->pos.y = p->body.box.min.y;
  p->pos.z = (p->body.box.min.z + p->body.box.max.z) * 0.5f;

  /* Set camera */
  float eye_y = p->pos.y + PLAYER_EYE_H;
  if (p->walking && p->body.grounded) {
    eye_y += sinf(p->bob_timer * 10.0f) * 0.04f;
  }
  cam->frame->pos = (sf_fvec3_t){p->pos.x, eye_y, p->pos.z};
  cam->frame->rot = (sf_fvec3_t){p->pitch, -p->yaw, 0};
  cam->frame->is_dirty = true;

  /* Weapon switch */
  if (sf_key_pressed(ctx, SF_KEY_1)) p->cur_wpn = WPN_KNIFE;
  if (sf_key_pressed(ctx, SF_KEY_2) && p->weapons[WPN_AK47].mag + p->weapons[WPN_AK47].reserve > 0) p->cur_wpn = WPN_AK47;

  /* Reload */
  if (sf_key_pressed(ctx, SF_KEY_R) && p->cur_wpn == WPN_AK47) {
    weapon_t *w = &p->weapons[WPN_AK47];
    if (!w->reloading && w->mag < AK_MAG_SIZE && w->reserve > 0) {
      w->reloading = true;
      w->reload_cd = AK_RELOAD_TIME;
    }
  }

  /* Shooting (mouse left) */
  if (ctx->input.mouse_btns[SF_MOUSE_LEFT]) {
    player_shoot(ctx);
  }

  /* Update enemies */
  update_enemies(ctx, dt);

  /* Check round state */
  check_round_state();
}

/* --- RENDER CALLBACK --- */
void on_render_end(sf_ctx_t *ctx, const sf_event_t *ev, void *userdata) {
  (void)ev; (void)userdata;
  sf_cam_t *cam = &ctx->main_camera;

  if (g.state == STATE_MENU) {
    draw_sky(ctx, cam);
    draw_menu(ctx, cam);
    return;
  }

  /* Draw viewmodel and HUD after scene */
  draw_viewmodel(ctx, cam);
  draw_hud(ctx, cam);

  if (g.state == STATE_BUY) {
    draw_buy_menu(ctx, cam);
  } else if (g.state == STATE_ROUND_WON) {
    draw_round_won(ctx, cam);
  } else if (g.state == STATE_GAME_OVER) {
    draw_game_over(ctx, cam);
  }
}

/* --- MAIN --- */
int main(int argc, char *argv[]) {
  (void)argc; (void)argv;
  srand((unsigned)time(NULL));

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("SF Strike",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    SCREEN_W, SCREEN_H, SDL_WINDOW_FULLSCREEN_DESKTOP);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);
  SDL_Texture *texture = SDL_CreateTexture(renderer,
    SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);

  memset(&g, 0, sizeof(g));
  sf_init(&g.ctx, SCREEN_W, SCREEN_H);
  g.ctx.main_camera.far_plane = 100.0f;

  sf_event_reg(&g.ctx, SF_EVT_RENDER_START, on_render_start, NULL);
  sf_event_reg(&g.ctx, SF_EVT_RENDER_END, on_render_end, NULL);

  /* Build world - try .sff first, fall back to procedural */
  if (!load_map_sff(&g.ctx)) {
    build_map(&g.ctx);
  }
  build_weapons(&g.ctx);
  g.human_puppet = build_humanoid(&g.ctx);

  /* Initial state */
  g.state = STATE_MENU;
  g.round_num = 0;
  player_init(&g.player, g.ct_spawn);

  /* Hide viewmodel parts initially */
  if (g.vm_barrel) g.vm_barrel->frame->scale = (sf_fvec3_t){0,0,0};
  if (g.vm_body)   g.vm_body->frame->scale   = (sf_fvec3_t){0,0,0};
  if (g.vm_mag)    g.vm_mag->frame->scale     = (sf_fvec3_t){0,0,0};
  if (g.vm_stock)  g.vm_stock->frame->scale   = (sf_fvec3_t){0,0,0};
  if (g.knife_blade)  g.knife_blade->frame->scale  = (sf_fvec3_t){0,0,0};
  if (g.knife_handle) g.knife_handle->frame->scale = (sf_fvec3_t){0,0,0};

  g.mouse_captured = false;

  SDL_Event event;
  while (sf_running(&g.ctx)) {
    sf_input_cycle_state(&g.ctx);
    g_mouse_accum_x = 0;
    g_mouse_accum_y = 0;

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) sf_stop(&g.ctx);
      if (event.type == SDL_MOUSEMOTION) {
        g_mouse_accum_x += event.motion.xrel;
        g_mouse_accum_y += event.motion.yrel;
      }
      sf_sdl_process_event(&g.ctx, &event);
    }
    /* Override mouse delta with accumulated relative motion */
    g.ctx.input.mouse_dx = g_mouse_accum_x;
    g.ctx.input.mouse_dy = g_mouse_accum_y;

    if (sf_key_pressed(&g.ctx, SF_KEY_ESC)) {
      if (g.mouse_captured) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        g.mouse_captured = false;
      } else {
        sf_stop(&g.ctx);
      }
    }

    if (g.state == STATE_MENU) {
      if (sf_key_pressed(&g.ctx, SF_KEY_RETURN)) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        g.mouse_captured = true;
        start_round();
      }
    }

    if (g.state == STATE_GAME_OVER) {
      if (sf_key_pressed(&g.ctx, SF_KEY_R)) {
        g.round_num = 0;
        g.player.kills = 0;
        g.player.deaths = 0;
        g.player.money = START_MONEY;
        g.ctx.puppet_inst_count = 0;
        start_round();
      }
    }

    /* Capture mouse on click when not captured */
    if (!g.mouse_captured && g.ctx.input.mouse_btns[SF_MOUSE_LEFT] && g.state != STATE_MENU) {
      SDL_SetRelativeMouseMode(SDL_TRUE);
      g.mouse_captured = true;
    }

    sf_time_update(&g.ctx);

    /* Draw sky before scene render */
    draw_sky(&g.ctx, &g.ctx.main_camera);

    sf_render_ctx(&g.ctx);

    SDL_UpdateTexture(texture, NULL, g.ctx.main_camera.buffer,
      g.ctx.main_camera.w * sizeof(sf_pkd_clr_t));
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  sf_destroy(&g.ctx);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
