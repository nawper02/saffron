/* sf_strike_ed -- top-down map editor for sf_strike, saves to .sff */
#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>

/* --- CONSTANTS --- */
#define SCREEN_W      (683*2)
#define SCREEN_H      (384*2)
#define PANEL_W       130
#define TOOLBAR_H     38
#define VIEW_X        PANEL_W
#define VIEW_Y        0
#define VIEW_W        (SCREEN_W - PANEL_W)
#define VIEW_H        (SCREEN_H - TOOLBAR_H)
#define GRID_DEFAULT  1.0f
#define WALL_HEIGHT   3.6f
#define WALL_THICK    0.6f
#define FLOOR_THICK   0.05f
#define CEIL_Y        3.6f
#define PROP_SIZE     1.0f
#define MAX_BRUSHES   512
#define MAX_LIGHTS    32
#define MAX_WAYPOINTS 16
#define MAX_UNDO      64

#define MAP_PATH SF_ASSET_PATH "/sf_sff/strike_map.sff"
#define BOX_OBJ_PATH SF_ASSET_PATH "/sf_sff/unit_box.obj"

/* --- TEXTURE TABLE --- */
typedef struct {
  const char *name;
  const char *file;
  sf_pkd_clr_t color;
} tex_entry_t;

static tex_entry_t g_textures[] = {
  {"brick",   "Brick_01-128x128.bmp",   0xFF8B5A3C},
  {"brick2",  "Brick_09-128x128.bmp",   0xFF6B4030},
  {"stone",   "Stone_05-128x128.bmp",   0xFF888888},
  {"stone2",  "Stone_09-128x128.bmp",   0xFFAAAAAA},
  {"wood",    "Wood_04-128x128.bmp",    0xFF7B5B3A},
  {"wood2",   "Wood_09-128x128.bmp",    0xFF9B7B5A},
  {"tile",    "Tile_01-128x128.bmp",    0xFFD0D0C8},
  {"metal",   "Metal_01-128x128.bmp",   0xFF505558},
  {"dirt",    "Dirt_02-128x128.bmp",    0xFF8B7355},
  {"plaster", "Plaster_05-128x128.bmp", 0xFFCBC0A8},
  {"metal2",  "Metal_06-128x128.bmp",   0xFF404448},
};
#define TEX_COUNT (int)(sizeof(g_textures)/sizeof(g_textures[0]))

/* --- DATA TYPES --- */
typedef enum { BRUSH_WALL, BRUSH_FLOOR, BRUSH_CEILING, BRUSH_PROP } brush_type_t;
typedef enum {
  TOOL_WALL, TOOL_FLOOR, TOOL_CEIL, TOOL_PROP,
  TOOL_LIGHT, TOOL_TSPAWN, TOOL_CTSPAWN, TOOL_WAYPOINT,
  TOOL_SELECT, TOOL_COUNT
} tool_t;

typedef struct {
  brush_type_t type;
  float cx, cz;
  float w, d;
  float y0, y1;
  int tex;
  float tex_scale;
} ed_brush_t;

typedef struct {
  float x, y, z;
  float r, g, b;
  float intensity;
} ed_light_t;

typedef struct {
  float x, y, z;
} ed_point_t;

/* --- EDITOR STATE --- */
static struct {
  sf_ctx_t ctx;
  ed_brush_t brushes[MAX_BRUSHES];
  int brush_count;
  ed_light_t lights[MAX_LIGHTS];
  int light_count;
  ed_point_t waypoints[MAX_WAYPOINTS];
  int wp_count;
  ed_point_t t_spawn, ct_spawn;
  bool has_t_spawn, has_ct_spawn;

  float cam_x, cam_z;
  float zoom;
  float grid_size;
  bool snap;

  tool_t tool;
  int cur_tex;
  int selected;

  bool dragging;
  float drag_x0, drag_z0;
  float drag_x1, drag_z1;

  bool panning;
  int pan_ox, pan_oy;
  float pan_cx, pan_cz;

  bool dirty;
  bool running;
} ed;

/* --- COORDINATE HELPERS --- */
int w2sx(float wx) { return VIEW_X + VIEW_W/2 + (int)((wx - ed.cam_x) * ed.zoom); }
int w2sy(float wz) { return VIEW_Y + VIEW_H/2 + (int)((wz - ed.cam_z) * ed.zoom); }
float s2wx(int sx) { return (float)(sx - VIEW_X - VIEW_W/2) / ed.zoom + ed.cam_x; }
float s2wz(int sy) { return (float)(sy - VIEW_Y - VIEW_H/2) / ed.zoom + ed.cam_z; }
float snap_v(float v) { return ed.snap ? roundf(v / ed.grid_size) * ed.grid_size : v; }
bool in_view(int sx, int sy) { return sx >= VIEW_X && sx < VIEW_X+VIEW_W && sy >= VIEW_Y && sy < VIEW_Y+VIEW_H; }
int text_w(const char *s, int sc) { return (int)strlen(s) * 8 * sc; }

/* --- DRAW HELPERS --- */
void draw_rect_outline(sf_ctx_t *c, sf_cam_t *cam, sf_pkd_clr_t clr, int x0, int y0, int x1, int y1) {
  sf_line(c, cam, clr, (sf_ivec2_t){x0,y0}, (sf_ivec2_t){x1,y0});
  sf_line(c, cam, clr, (sf_ivec2_t){x1,y0}, (sf_ivec2_t){x1,y1});
  sf_line(c, cam, clr, (sf_ivec2_t){x1,y1}, (sf_ivec2_t){x0,y1});
  sf_line(c, cam, clr, (sf_ivec2_t){x0,y1}, (sf_ivec2_t){x0,y0});
}

sf_pkd_clr_t dim_color(sf_pkd_clr_t c, float f) {
  int r = (int)(((c>>16)&0xFF)*f), g = (int)(((c>>8)&0xFF)*f), b = (int)((c&0xFF)*f);
  if (r>255) r=255; if (g>255) g=255; if (b>255) b=255;
  return 0xFF000000 | (r<<16) | (g<<8) | b;
}

/* --- DRAW GRID --- */
void draw_grid(sf_ctx_t *c, sf_cam_t *cam) {
  float gs = ed.grid_size;
  if (ed.zoom * gs < 8) gs *= 4;
  if (ed.zoom * gs < 8) gs *= 4;

  float left = s2wx(VIEW_X), right = s2wx(VIEW_X + VIEW_W);
  float top = s2wz(VIEW_Y), bottom = s2wz(VIEW_Y + VIEW_H);

  float x0 = floorf(left / gs) * gs;
  float z0 = floorf(top / gs) * gs;

  sf_pkd_clr_t grid_clr = 0xFF2A2A2A;
  sf_pkd_clr_t axis_clr = 0xFF444444;

  for (float x = x0; x <= right; x += gs) {
    int sx = w2sx(x);
    if (sx < VIEW_X || sx >= VIEW_X + VIEW_W) continue;
    sf_pkd_clr_t clr = (fabsf(x) < 0.01f) ? axis_clr : grid_clr;
    sf_line(c, cam, clr, (sf_ivec2_t){sx, VIEW_Y}, (sf_ivec2_t){sx, VIEW_Y+VIEW_H});
  }
  for (float z = z0; z <= bottom; z += gs) {
    int sy = w2sy(z);
    if (sy < VIEW_Y || sy >= VIEW_Y + VIEW_H) continue;
    sf_pkd_clr_t clr = (fabsf(z) < 0.01f) ? axis_clr : grid_clr;
    sf_line(c, cam, clr, (sf_ivec2_t){VIEW_X, sy}, (sf_ivec2_t){VIEW_X+VIEW_W, sy});
  }
}

/* --- DRAW BRUSHES --- */
void draw_brushes(sf_ctx_t *c, sf_cam_t *cam) {
  for (int i = 0; i < ed.brush_count; i++) {
    ed_brush_t *b = &ed.brushes[i];
    int x0 = w2sx(b->cx - b->w/2), x1 = w2sx(b->cx + b->w/2);
    int y0 = w2sy(b->cz - b->d/2), y1 = w2sy(b->cz + b->d/2);

    float brightness = 0.7f;
    if (b->type == BRUSH_FLOOR) brightness = 0.5f;
    else if (b->type == BRUSH_CEILING) brightness = 0.4f;
    else if (b->type == BRUSH_PROP) brightness = 0.9f;

    sf_pkd_clr_t fill = dim_color(g_textures[b->tex].color, brightness);
    sf_rect(c, cam, fill, (sf_ivec2_t){x0,y0}, (sf_ivec2_t){x1,y1});

    sf_pkd_clr_t outline = (i == ed.selected) ? SF_CLR_YELLOW : 0xFF666666;
    draw_rect_outline(c, cam, outline, x0, y0, x1, y1);

    if (i == ed.selected) {
      draw_rect_outline(c, cam, SF_CLR_YELLOW, x0-1, y0-1, x1+1, y1+1);
    }
  }
}

/* --- DRAW LIGHTS --- */
void draw_lights(sf_ctx_t *c, sf_cam_t *cam) {
  for (int i = 0; i < ed.light_count; i++) {
    int sx = w2sx(ed.lights[i].x), sy = w2sy(ed.lights[i].z);
    sf_pkd_clr_t clr = SF_CLR_YELLOW;
    for (int dy = -4; dy <= 4; dy++)
      for (int dx = -4; dx <= 4; dx++)
        if (dx*dx + dy*dy <= 16)
          sf_pixel(c, cam, clr, (sf_ivec2_t){sx+dx, sy+dy});
    char buf[8]; snprintf(buf, 8, "L%d", i);
    sf_put_text(c, cam, buf, (sf_ivec2_t){sx+6, sy-4}, SF_CLR_YELLOW, 1);
  }
}

/* --- DRAW SPAWNS AND WAYPOINTS --- */
void draw_markers(sf_ctx_t *c, sf_cam_t *cam) {
  if (ed.has_t_spawn) {
    int sx = w2sx(ed.t_spawn.x), sy = w2sy(ed.t_spawn.z);
    sf_pkd_clr_t clr = SF_CLR_RED;
    for (int dy = -6; dy <= 6; dy++)
      for (int dx = -6; dx <= 6; dx++)
        if (abs(dx)+abs(dy) <= 6) sf_pixel(c, cam, clr, (sf_ivec2_t){sx+dx, sy+dy});
    sf_put_text(c, cam, "T", (sf_ivec2_t){sx-3, sy-3}, SF_CLR_WHITE, 1);
  }
  if (ed.has_ct_spawn) {
    int sx = w2sx(ed.ct_spawn.x), sy = w2sy(ed.ct_spawn.z);
    sf_pkd_clr_t clr = SF_CLR_BLUE;
    for (int dy = -6; dy <= 6; dy++)
      for (int dx = -6; dx <= 6; dx++)
        if (abs(dx)+abs(dy) <= 6) sf_pixel(c, cam, clr, (sf_ivec2_t){sx+dx, sy+dy});
    sf_put_text(c, cam, "CT", (sf_ivec2_t){sx-6, sy-3}, SF_CLR_WHITE, 1);
  }
  for (int i = 0; i < ed.wp_count; i++) {
    int sx = w2sx(ed.waypoints[i].x), sy = w2sy(ed.waypoints[i].z);
    sf_pkd_clr_t clr = SF_CLR_GREEN;
    for (int dy = -4; dy <= 4; dy++)
      for (int dx = -4; dx <= 4; dx++)
        if (dx*dx+dy*dy <= 16) sf_pixel(c, cam, clr, (sf_ivec2_t){sx+dx, sy+dy});
    char buf[8]; snprintf(buf, 8, "%d", i);
    sf_put_text(c, cam, buf, (sf_ivec2_t){sx+6, sy-3}, SF_CLR_GREEN, 1);
    if (i > 0) {
      int px = w2sx(ed.waypoints[i-1].x), py = w2sy(ed.waypoints[i-1].z);
      sf_line(c, cam, 0xFF225522, (sf_ivec2_t){px, py}, (sf_ivec2_t){sx, sy});
    }
  }
}

/* --- DRAW DRAG PREVIEW --- */
void draw_preview(sf_ctx_t *c, sf_cam_t *cam) {
  if (!ed.dragging) return;
  float x0 = fminf(ed.drag_x0, ed.drag_x1), x1 = fmaxf(ed.drag_x0, ed.drag_x1);
  float z0 = fminf(ed.drag_z0, ed.drag_z1), z1 = fmaxf(ed.drag_z0, ed.drag_z1);
  int sx0 = w2sx(x0), sy0 = w2sy(z0), sx1 = w2sx(x1), sy1 = w2sy(z1);

  sf_pkd_clr_t fill = dim_color(g_textures[ed.cur_tex].color, 0.3f);
  sf_rect(c, cam, fill, (sf_ivec2_t){sx0,sy0}, (sf_ivec2_t){sx1,sy1});
  draw_rect_outline(c, cam, SF_CLR_WHITE, sx0, sy0, sx1, sy1);

  char buf[32];
  float dw = x1 - x0, dd = z1 - z0;
  snprintf(buf, 32, "%.1fx%.1f", dw, dd);
  sf_put_text(c, cam, buf, (sf_ivec2_t){sx0+2, sy0+2}, SF_CLR_WHITE, 1);
}

/* --- DRAW LEFT PANEL (TEXTURES) --- */
void draw_panel(sf_ctx_t *c, sf_cam_t *cam) {
  sf_rect(c, cam, 0xFF1A1A1A, (sf_ivec2_t){0,0}, (sf_ivec2_t){PANEL_W, SCREEN_H});
  sf_put_text(c, cam, "Textures", (sf_ivec2_t){4, 4}, SF_CLR_WHITE, 1);

  int y = 20;
  for (int i = 0; i < TEX_COUNT; i++) {
    int h = 22;
    sf_pkd_clr_t bg = (i == ed.cur_tex) ? 0xFF555555 : 0xFF2A2A2A;
    sf_rect(c, cam, bg, (sf_ivec2_t){4, y}, (sf_ivec2_t){PANEL_W-4, y+h});
    sf_rect(c, cam, g_textures[i].color, (sf_ivec2_t){6, y+2}, (sf_ivec2_t){24, y+h-2});
    sf_put_text(c, cam, g_textures[i].name, (sf_ivec2_t){28, y+7}, SF_CLR_WHITE, 1);
    if (i == ed.cur_tex) draw_rect_outline(c, cam, SF_CLR_YELLOW, 4, y, PANEL_W-4, y+h);
    y += h + 2;
  }

  /* Brush count */
  y += 10;
  char buf[32];
  snprintf(buf, 32, "Brushes: %d", ed.brush_count);
  sf_put_text(c, cam, buf, (sf_ivec2_t){4, y}, SF_CLR_WHITE, 1);
  y += 12;
  snprintf(buf, 32, "Lights: %d", ed.light_count);
  sf_put_text(c, cam, buf, (sf_ivec2_t){4, y}, SF_CLR_WHITE, 1);
  y += 12;
  snprintf(buf, 32, "WPs: %d", ed.wp_count);
  sf_put_text(c, cam, buf, (sf_ivec2_t){4, y}, SF_CLR_WHITE, 1);
  y += 12;
  snprintf(buf, 32, "Grid: %.1f", ed.grid_size);
  sf_put_text(c, cam, buf, (sf_ivec2_t){4, y}, SF_CLR_WHITE, 1);
  y += 12;
  sf_put_text(c, cam, ed.snap ? "Snap: ON" : "Snap: OFF", (sf_ivec2_t){4, y}, ed.snap ? SF_CLR_GREEN : SF_CLR_RED, 1);
}

/* --- DRAW TOOLBAR --- */
void draw_toolbar(sf_ctx_t *c, sf_cam_t *cam) {
  int y = SCREEN_H - TOOLBAR_H;
  sf_rect(c, cam, 0xFF1A1A1A, (sf_ivec2_t){PANEL_W, y}, (sf_ivec2_t){SCREEN_W, SCREEN_H});

  const char *names[] = {"[W]all","[F]loor","[C]eil","[B]ox","[L]ight","[T]sp","CT[Y]","Way[P]","[E]dit"};
  int x = PANEL_W + 8;
  for (int i = 0; i < TOOL_COUNT; i++) {
    sf_pkd_clr_t clr = (i == ed.tool) ? SF_CLR_YELLOW : 0xFF888888;
    sf_put_text(c, cam, names[i], (sf_ivec2_t){x, y+8}, clr, 1);
    x += text_w(names[i], 1) + 12;
  }

  /* Status */
  int mx, my;
  SDL_GetMouseState(&mx, &my);
  float wx = s2wx(mx), wz = s2wz(my);
  char buf[64];
  snprintf(buf, 64, "X:%.1f Z:%.1f  Zoom:%.0f%%", wx, wz, ed.zoom * 100.0f / 30.0f);
  sf_put_text(c, cam, buf, (sf_ivec2_t){SCREEN_W - 250, y+8}, SF_CLR_WHITE, 1);

  /* Save hint */
  sf_put_text(c, cam, "Ctrl+S save", (sf_ivec2_t){SCREEN_W - 250, y+22}, 0xFF666666, 1);
}

/* --- ADD BRUSH --- */
void add_brush(brush_type_t type, float x0, float z0, float x1, float z1) {
  if (ed.brush_count >= MAX_BRUSHES) return;
  float cx = (x0+x1)/2, cz = (z0+z1)/2;
  float w = fabsf(x1-x0), d = fabsf(z1-z0);
  if (w < 0.1f) w = 0.1f;
  if (d < 0.1f) d = 0.1f;

  ed_brush_t *b = &ed.brushes[ed.brush_count++];
  b->type = type;
  b->cx = cx; b->cz = cz;
  b->w = w; b->d = d;
  b->tex = ed.cur_tex;
  b->tex_scale = 3.0f;

  switch (type) {
    case BRUSH_WALL:    b->y0 = 0; b->y1 = WALL_HEIGHT; break;
    case BRUSH_FLOOR:   b->y0 = 0; b->y1 = FLOOR_THICK; break;
    case BRUSH_CEILING: b->y0 = CEIL_Y - FLOOR_THICK; b->y1 = CEIL_Y; break;
    case BRUSH_PROP:    b->y0 = 0; b->y1 = PROP_SIZE; break;
  }
  ed.dirty = true;
}

/* --- FIND BRUSH UNDER CURSOR --- */
int find_brush_at(float wx, float wz) {
  for (int i = ed.brush_count - 1; i >= 0; i--) {
    ed_brush_t *b = &ed.brushes[i];
    if (wx >= b->cx - b->w/2 && wx <= b->cx + b->w/2 &&
        wz >= b->cz - b->d/2 && wz <= b->cz + b->d/2) return i;
  }
  return -1;
}

/* --- DELETE SELECTED --- */
void delete_selected(void) {
  if (ed.selected < 0 || ed.selected >= ed.brush_count) return;
  for (int i = ed.selected; i < ed.brush_count - 1; i++)
    ed.brushes[i] = ed.brushes[i+1];
  ed.brush_count--;
  ed.selected = -1;
  ed.dirty = true;
}

/* --- FIND LIGHT AT --- */
int find_light_at(float wx, float wz) {
  for (int i = ed.light_count - 1; i >= 0; i--) {
    float dx = wx - ed.lights[i].x, dz = wz - ed.lights[i].z;
    if (dx*dx + dz*dz < 1.0f) return i;
  }
  return -1;
}

/* --- SAVE .obj FOR UNIT BOX --- */
void save_unit_box_obj(void) {
  FILE *f = fopen(BOX_OBJ_PATH, "r");
  if (f) { fclose(f); return; }
  sf_obj_t *box = sf_obj_make_box(&ed.ctx, "__ed_box", 1, 1, 1);
  if (box) sf_obj_save_obj(&ed.ctx, box, BOX_OBJ_PATH);
}

/* --- SAVE MAP TO .sff --- */
void save_map(void) {
  save_unit_box_obj();

  FILE *f = fopen(MAP_PATH, "w");
  if (!f) { fprintf(stderr, "Cannot write %s\n", MAP_PATH); return; }

  fprintf(f, "# SF Strike Map - Generated by SF Strike Editor\n\n");

  /* Mesh */
  fprintf(f, "mesh unit_box \"unit_box.obj\"\n\n");

  /* Collect used textures */
  bool tex_used[TEX_COUNT] = {0};
  for (int i = 0; i < ed.brush_count; i++) tex_used[ed.brushes[i].tex] = true;

  for (int i = 0; i < TEX_COUNT; i++) {
    if (tex_used[i])
      fprintf(f, "texture %s \"%s\"\n", g_textures[i].name, g_textures[i].file);
  }
  fprintf(f, "\n");

  /* Sunlight */
  fprintf(f, "light sun {\n");
  fprintf(f, "    type      = dir\n");
  fprintf(f, "    color     = (1.000, 0.950, 0.800)\n");
  fprintf(f, "    intensity = 1.400\n");
  fprintf(f, "}\n\n");

  /* Entities */
  for (int i = 0; i < ed.brush_count; i++) {
    ed_brush_t *b = &ed.brushes[i];
    const char *prefix = "brush";
    if (b->type == BRUSH_WALL) prefix = "wall";
    else if (b->type == BRUSH_FLOOR) prefix = "floor";
    else if (b->type == BRUSH_CEILING) prefix = "ceil";
    else if (b->type == BRUSH_PROP) prefix = "prop";

    float py = (b->y0 + b->y1) / 2;
    float sy = b->y1 - b->y0;

    fprintf(f, "entity %s_%d {\n", prefix, i);
    fprintf(f, "    mesh      = unit_box\n");
    fprintf(f, "    pos       = (%.3f, %.3f, %.3f)\n", b->cx, py, b->cz);
    fprintf(f, "    scale     = (%.3f, %.3f, %.3f)\n", b->w, sy, b->d);
    fprintf(f, "    texture   = %s\n", g_textures[b->tex].name);
    fprintf(f, "    tex_scale = (%.1f, %.1f)\n", b->tex_scale, b->tex_scale);
    fprintf(f, "}\n\n");
  }

  /* Lights */
  for (int i = 0; i < ed.light_count; i++) {
    ed_light_t *l = &ed.lights[i];
    fprintf(f, "light light_%d {\n", i);
    fprintf(f, "    type      = point\n");
    fprintf(f, "    pos       = (%.3f, %.3f, %.3f)\n", l->x, l->y, l->z);
    fprintf(f, "    color     = (%.3f, %.3f, %.3f)\n", l->r, l->g, l->b);
    fprintf(f, "    intensity = %.1f\n", l->intensity);
    fprintf(f, "}\n\n");
  }

  /* Spawn frames */
  if (ed.has_t_spawn) {
    fprintf(f, "frame t_spawn {\n");
    fprintf(f, "    pos = (%.3f, %.3f, %.3f)\n", ed.t_spawn.x, ed.t_spawn.y, ed.t_spawn.z);
    fprintf(f, "}\n\n");
  }
  if (ed.has_ct_spawn) {
    fprintf(f, "frame ct_spawn {\n");
    fprintf(f, "    pos = (%.3f, %.3f, %.3f)\n", ed.ct_spawn.x, ed.ct_spawn.y, ed.ct_spawn.z);
    fprintf(f, "}\n\n");
  }

  /* Waypoint frames */
  for (int i = 0; i < ed.wp_count; i++) {
    fprintf(f, "frame wp_%d {\n", i);
    fprintf(f, "    pos = (%.3f, %.3f, %.3f)\n", ed.waypoints[i].x, ed.waypoints[i].y, ed.waypoints[i].z);
    fprintf(f, "}\n\n");
  }

  fclose(f);
  ed.dirty = false;
  printf("Saved map to %s (%d brushes, %d lights, %d waypoints)\n",
         MAP_PATH, ed.brush_count, ed.light_count, ed.wp_count);
}

/* --- LOAD MAP FROM .sff --- */
void load_map(void) {
  FILE *test = fopen(MAP_PATH, "r");
  if (!test) return;
  fclose(test);

  ed.brush_count = 0;
  ed.light_count = 0;
  ed.wp_count = 0;
  ed.has_t_spawn = false;
  ed.has_ct_spawn = false;
  ed.selected = -1;

  /* Parse the .sff file manually to extract editor data */
  FILE *f = fopen(MAP_PATH, "r");
  if (!f) return;

  char line[512];
  while (fgets(line, sizeof(line), f)) {
    char *s = line;
    while (*s == ' ' || *s == '\t') s++;

    /* Entity block */
    if (strncmp(s, "entity ", 7) == 0) {
      char ename[64] = {0};
      sscanf(s, "entity %63s", ename);

      ed_brush_t b = {0};
      b.tex_scale = 3.0f;
      float px=0,py=0,pz=0,sx=1,sy=1,sz=1;
      char texname[64] = {0};
      float ts_x=3, ts_y=3;

      while (fgets(line, sizeof(line), f)) {
        char *l = line;
        while (*l == ' ' || *l == '\t') l++;
        if (l[0] == '}') break;

        if (strncmp(l, "pos", 3) == 0) sscanf(strchr(l, '('), "(%f, %f, %f)", &px, &py, &pz);
        else if (strncmp(l, "scale", 5) == 0) sscanf(strchr(l, '('), "(%f, %f, %f)", &sx, &sy, &sz);
        else if (strncmp(l, "tex_scale", 9) == 0) sscanf(strchr(l, '('), "(%f, %f)", &ts_x, &ts_y);
        else if (strncmp(l, "texture", 7) == 0) {
          char *eq = strchr(l, '=');
          if (eq) { eq++; while (*eq == ' ') eq++; sscanf(eq, "%63s", texname); }
        }
      }

      b.cx = px; b.cz = pz;
      b.w = sx; b.d = sz;
      b.y0 = py - sy/2; b.y1 = py + sy/2;
      b.tex_scale = ts_x;

      /* Determine type from name prefix */
      if (strncmp(ename, "wall", 4) == 0) b.type = BRUSH_WALL;
      else if (strncmp(ename, "floor", 5) == 0) b.type = BRUSH_FLOOR;
      else if (strncmp(ename, "ceil", 4) == 0) b.type = BRUSH_CEILING;
      else if (strncmp(ename, "prop", 4) == 0) b.type = BRUSH_PROP;
      else b.type = BRUSH_WALL;

      /* Find texture index */
      b.tex = 0;
      for (int i = 0; i < TEX_COUNT; i++) {
        if (strcmp(g_textures[i].name, texname) == 0) { b.tex = i; break; }
      }

      if (ed.brush_count < MAX_BRUSHES) ed.brushes[ed.brush_count++] = b;
    }

    /* Light block */
    else if (strncmp(s, "light ", 6) == 0) {
      char lname[64] = {0};
      sscanf(s, "light %63s", lname);
      if (strcmp(lname, "sun") == 0) {
        /* Skip sun, it's auto-generated */
        while (fgets(line, sizeof(line), f)) {
          char *l = line; while (*l == ' ' || *l == '\t') l++;
          if (l[0] == '}') break;
        }
        continue;
      }

      ed_light_t l = {0, 3.2f, 0, 1.0f, 0.9f, 0.7f, 5.0f};
      while (fgets(line, sizeof(line), f)) {
        char *p = line; while (*p == ' ' || *p == '\t') p++;
        if (p[0] == '}') break;
        if (strncmp(p, "pos", 3) == 0) sscanf(strchr(p, '('), "(%f, %f, %f)", &l.x, &l.y, &l.z);
        else if (strncmp(p, "color", 5) == 0) sscanf(strchr(p, '('), "(%f, %f, %f)", &l.r, &l.g, &l.b);
        else if (strncmp(p, "intensity", 9) == 0) { char *eq = strchr(p, '='); if (eq) l.intensity = (float)atof(eq+1); }
      }
      if (ed.light_count < MAX_LIGHTS) ed.lights[ed.light_count++] = l;
    }

    /* Frame block */
    else if (strncmp(s, "frame ", 6) == 0) {
      char fname[64] = {0};
      sscanf(s, "frame %63s", fname);
      float fx=0, fy=0, fz=0;
      while (fgets(line, sizeof(line), f)) {
        char *p = line; while (*p == ' ' || *p == '\t') p++;
        if (p[0] == '}') break;
        if (strncmp(p, "pos", 3) == 0) sscanf(strchr(p, '('), "(%f, %f, %f)", &fx, &fy, &fz);
      }
      if (strcmp(fname, "t_spawn") == 0) { ed.t_spawn = (ed_point_t){fx,fy,fz}; ed.has_t_spawn = true; }
      else if (strcmp(fname, "ct_spawn") == 0) { ed.ct_spawn = (ed_point_t){fx,fy,fz}; ed.has_ct_spawn = true; }
      else if (strncmp(fname, "wp_", 3) == 0 && ed.wp_count < MAX_WAYPOINTS)
        ed.waypoints[ed.wp_count++] = (ed_point_t){fx, fy, fz};
    }
  }
  fclose(f);
  printf("Loaded map: %d brushes, %d lights, %d waypoints\n",
         ed.brush_count, ed.light_count, ed.wp_count);
}

/* --- HANDLE TOOL CLICK --- */
void handle_click(float wx, float wz) {
  wx = snap_v(wx); wz = snap_v(wz);

  switch (ed.tool) {
    case TOOL_WALL: case TOOL_FLOOR: case TOOL_CEIL:
      ed.dragging = true;
      ed.drag_x0 = wx; ed.drag_z0 = wz;
      ed.drag_x1 = wx; ed.drag_z1 = wz;
      break;

    case TOOL_PROP:
      add_brush(BRUSH_PROP, wx - PROP_SIZE/2, wz - PROP_SIZE/2,
                wx + PROP_SIZE/2, wz + PROP_SIZE/2);
      break;

    case TOOL_LIGHT:
      if (ed.light_count < MAX_LIGHTS) {
        ed.lights[ed.light_count++] = (ed_light_t){wx, 3.2f, wz, 1.0f, 0.9f, 0.7f, 5.0f};
        ed.dirty = true;
      }
      break;

    case TOOL_TSPAWN:
      ed.t_spawn = (ed_point_t){wx, 0.1f, wz};
      ed.has_t_spawn = true;
      ed.dirty = true;
      break;

    case TOOL_CTSPAWN:
      ed.ct_spawn = (ed_point_t){wx, 0.1f, wz};
      ed.has_ct_spawn = true;
      ed.dirty = true;
      break;

    case TOOL_WAYPOINT:
      if (ed.wp_count < MAX_WAYPOINTS) {
        ed.waypoints[ed.wp_count++] = (ed_point_t){wx, 0, wz};
        ed.dirty = true;
      }
      break;

    case TOOL_SELECT: {
      int idx = find_brush_at(wx, wz);
      if (idx >= 0) {
        ed.selected = idx;
        ed.dragging = true;
        ed.drag_x0 = wx; ed.drag_z0 = wz;
      } else {
        ed.selected = -1;
      }
      break;
    }
    default: break;
  }
}

void handle_drag(float wx, float wz) {
  wx = snap_v(wx); wz = snap_v(wz);

  if (ed.tool == TOOL_SELECT && ed.selected >= 0 && ed.dragging) {
    float dx = wx - ed.drag_x0, dz = wz - ed.drag_z0;
    ed.brushes[ed.selected].cx += dx;
    ed.brushes[ed.selected].cz += dz;
    ed.drag_x0 = wx; ed.drag_z0 = wz;
    ed.dirty = true;
  } else if (ed.dragging) {
    ed.drag_x1 = wx;
    ed.drag_z1 = wz;
  }
}

void handle_release(void) {
  if (!ed.dragging) return;
  ed.dragging = false;

  if (ed.tool == TOOL_SELECT) return;

  brush_type_t type = BRUSH_WALL;
  if (ed.tool == TOOL_FLOOR) type = BRUSH_FLOOR;
  else if (ed.tool == TOOL_CEIL) type = BRUSH_CEILING;

  float x0 = fminf(ed.drag_x0, ed.drag_x1), x1 = fmaxf(ed.drag_x0, ed.drag_x1);
  float z0 = fminf(ed.drag_z0, ed.drag_z1), z1 = fmaxf(ed.drag_z0, ed.drag_z1);
  if (fabsf(x1 - x0) < 0.05f && fabsf(z1 - z0) < 0.05f) return;

  add_brush(type, x0, z0, x1, z1);
}

/* --- HANDLE TEXTURE PALETTE CLICK --- */
void handle_panel_click(int mx, int my) {
  int y = 20;
  for (int i = 0; i < TEX_COUNT; i++) {
    int h = 22;
    if (my >= y && my < y + h) {
      ed.cur_tex = i;
      /* Apply to selected brush */
      if (ed.selected >= 0) {
        ed.brushes[ed.selected].tex = i;
        ed.dirty = true;
      }
      return;
    }
    y += h + 2;
  }
}

/* --- RIGHT CLICK --- */
void handle_right_click(float wx, float wz) {
  if (ed.dragging) { ed.dragging = false; return; }

  /* Try to delete light at cursor */
  int li = find_light_at(wx, wz);
  if (li >= 0) {
    for (int i = li; i < ed.light_count-1; i++) ed.lights[i] = ed.lights[i+1];
    ed.light_count--;
    ed.dirty = true;
    return;
  }

  /* Delete brush at cursor */
  int bi = find_brush_at(wx, wz);
  if (bi >= 0) {
    ed.selected = bi;
    delete_selected();
  }
}

/* --- MAIN --- */
int main(int argc, char *argv[]) {
  (void)argc; (void)argv;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("SF Strike Editor",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    SCREEN_W, SCREEN_H, SDL_WINDOW_FULLSCREEN_DESKTOP);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);
  SDL_Texture *texture = SDL_CreateTexture(renderer,
    SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);

  memset(&ed, 0, sizeof(ed));
  sf_init(&ed.ctx, SCREEN_W, SCREEN_H);
  ed.zoom = 30.0f;
  ed.grid_size = 1.0f;
  ed.snap = true;
  ed.tool = TOOL_WALL;
  ed.selected = -1;
  ed.running = true;

  /* Try to load existing map */
  load_map();

  SDL_Event event;
  bool ctrl_held = false;

  while (ed.running) {
    sf_input_cycle_state(&ed.ctx);

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) ed.running = false;

      if (event.type == SDL_KEYDOWN) {
        ctrl_held = (event.key.keysym.mod & KMOD_CTRL) != 0;
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE: ed.running = false; break;
          case SDLK_w: if (!ctrl_held) ed.tool = TOOL_WALL; break;
          case SDLK_f: ed.tool = TOOL_FLOOR; break;
          case SDLK_c: ed.tool = TOOL_CEIL; break;
          case SDLK_b: ed.tool = TOOL_PROP; break;
          case SDLK_l: ed.tool = TOOL_LIGHT; break;
          case SDLK_t: ed.tool = TOOL_TSPAWN; break;
          case SDLK_y: ed.tool = TOOL_CTSPAWN; break;
          case SDLK_p: ed.tool = TOOL_WAYPOINT; break;
          case SDLK_e: ed.tool = TOOL_SELECT; break;
          case SDLK_g: ed.snap = !ed.snap; break;
          case SDLK_s: if (ctrl_held) save_map(); break;
          case SDLK_DELETE: case SDLK_x: delete_selected(); break;
          case SDLK_TAB: ed.cur_tex = (ed.cur_tex + 1) % TEX_COUNT; break;
          case SDLK_EQUALS: case SDLK_PLUS: ed.zoom *= 1.2f; break;
          case SDLK_MINUS: ed.zoom /= 1.2f; if (ed.zoom < 5) ed.zoom = 5; break;
          case SDLK_LEFTBRACKET: ed.grid_size /= 2; if (ed.grid_size < 0.25f) ed.grid_size = 0.25f; break;
          case SDLK_RIGHTBRACKET: ed.grid_size *= 2; if (ed.grid_size > 8) ed.grid_size = 8; break;
          default: break;
        }
      }
      if (event.type == SDL_KEYUP) {
        ctrl_held = (event.key.keysym.mod & KMOD_CTRL) != 0;
      }

      if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mx = event.button.x, my = event.button.y;

        if (event.button.button == SDL_BUTTON_LEFT) {
          if (mx < PANEL_W) {
            handle_panel_click(mx, my);
          } else if (in_view(mx, my)) {
            handle_click(s2wx(mx), s2wz(my));
          }
        }
        else if (event.button.button == SDL_BUTTON_RIGHT && in_view(mx, my)) {
          handle_right_click(s2wx(mx), s2wz(my));
        }
        else if (event.button.button == SDL_BUTTON_MIDDLE) {
          ed.panning = true;
          ed.pan_ox = mx; ed.pan_oy = my;
          ed.pan_cx = ed.cam_x; ed.pan_cz = ed.cam_z;
        }
      }

      if (event.type == SDL_MOUSEBUTTONUP) {
        if (event.button.button == SDL_BUTTON_LEFT) handle_release();
        if (event.button.button == SDL_BUTTON_MIDDLE) ed.panning = false;
      }

      if (event.type == SDL_MOUSEMOTION) {
        int mx = event.motion.x, my = event.motion.y;
        if (ed.panning) {
          ed.cam_x = ed.pan_cx - (mx - ed.pan_ox) / ed.zoom;
          ed.cam_z = ed.pan_cz - (my - ed.pan_oy) / ed.zoom;
        }
        if (ed.dragging && in_view(mx, my)) {
          handle_drag(s2wx(mx), s2wz(my));
        }
      }

      if (event.type == SDL_MOUSEWHEEL) {
        float factor = (event.wheel.y > 0) ? 1.15f : (1.0f / 1.15f);
        ed.zoom *= factor;
        if (ed.zoom < 5) ed.zoom = 5;
        if (ed.zoom > 500) ed.zoom = 500;
      }
    }

    /* Arrow keys for panning */
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    float pan_speed = 5.0f / ed.zoom;
    if (keys[SDL_SCANCODE_LEFT])  ed.cam_x -= pan_speed;
    if (keys[SDL_SCANCODE_RIGHT]) ed.cam_x += pan_speed;
    if (keys[SDL_SCANCODE_UP])    ed.cam_z -= pan_speed;
    if (keys[SDL_SCANCODE_DOWN])  ed.cam_z += pan_speed;

    /* Render */
    sf_cam_t *cam = &ed.ctx.main_camera;
    sf_rect(&ed.ctx, cam, 0xFF1E1E1E, (sf_ivec2_t){0,0}, (sf_ivec2_t){SCREEN_W, SCREEN_H});

    draw_grid(&ed.ctx, cam);
    draw_brushes(&ed.ctx, cam);
    draw_lights(&ed.ctx, cam);
    draw_markers(&ed.ctx, cam);
    draw_preview(&ed.ctx, cam);
    draw_panel(&ed.ctx, cam);
    draw_toolbar(&ed.ctx, cam);

    /* Cursor crosshair */
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    if (in_view(mx, my)) {
      sf_line(&ed.ctx, cam, 0xFF444444, (sf_ivec2_t){mx-8,my}, (sf_ivec2_t){mx+8,my});
      sf_line(&ed.ctx, cam, 0xFF444444, (sf_ivec2_t){mx,my-8}, (sf_ivec2_t){mx,my+8});
    }

    SDL_UpdateTexture(texture, NULL, cam->buffer, cam->w * sizeof(sf_pkd_clr_t));
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_Delay(16);
  }

  sf_destroy(&ed.ctx);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
