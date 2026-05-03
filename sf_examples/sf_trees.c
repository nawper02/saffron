/* sf_trees — Procedural Tree Generator for Saffron */
#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
   CONSTANTS
   ============================================================ */
#define W        800            /* window width                       */
#define H        600            /* window height                      */
#define SEGS     6              /* cylinder polygon sides             */
#define MAX_V    32000          /* mesh vertex capacity               */
#define MAX_UV   32000
#define MAX_F    32000
#define MAX_LEA  4096           /* leaf billboard positions           */
#define MAX_TEX  64             /* scannable bark textures            */
#define MAX_SPR  32             /* scannable leaf sprites             */

/* UI panel */
#define PX1    192              /* panel right edge (x)               */
#define PY0    18               /* panel top: sits below stats bar    */
#define LX     5                /* label x                            */
#define IX     78               /* drag-float input x                 */
#define IX1    188              /* input right edge                   */
#define RH     15               /* row height (px)                    */
#define RY(n)  (PY0 + 22 + (n)*RH)   /* absolute top-y of row n     */

/* ============================================================
   GLOBALS
   ============================================================ */
static sf_ctx_t      g_ctx;
static sf_obj_t     *g_obj  = NULL;   /* pooled procedural mesh       */
static sf_enti_t    *g_enti = NULL;   /* scene entity                 */
static sf_sprite_t  *g_leaf = NULL;   /* active leaf sprite           */

typedef struct { sf_fvec3_t p; float s; } leaf_t;
static leaf_t  g_leaves[MAX_LEA];
static int     g_nleaf = 0;

/* Camera orbit */
static float   g_yaw   =  0.5f;
static float   g_pitch =  0.35f;
static float   g_dist  = 15.0f;
static bool    g_drag  = false;       /* left-mouse drag active       */
static int     g_lmx, g_lmy;

/* Tree parameters (drag-float targets) */
static float p_seed   = 42.0f;
static float p_depth  =  4.0f;
static float p_branch =  3.0f;
static float p_angle  = 32.0f;    /* branch spread angle (degrees)  */
static float p_len    =  0.70f;   /* child length / parent length   */
static float p_taper  =  0.65f;   /* child start-r / parent start-r */
static float p_grav   =  0.15f;   /* gravity bend (pos = droop)     */
static float p_wiggle =  0.12f;   /* random noise magnitude         */
static float p_twist  =137.5f;    /* sibling twist (golden angle)   */
static float p_tr     =  0.22f;   /* trunk base radius              */
static float p_tl     =  2.5f;    /* trunk segment length           */
static float p_ls     =  1.0f;    /* leaf sprite scale              */
static float p_ld     =  4.0f;    /* leaves per terminal branch     */

/* Bark textures */
static char       g_tname[MAX_TEX][64];
static const char*g_titem[MAX_TEX];
static int        g_tnc = 0;
static int        g_tsel = 0;

/* Leaf sprites */
static char        g_sname[MAX_SPR][64]; /* display name (no ext)    */
static const char *g_sitem[MAX_SPR];
static sf_sprite_t*g_sprites[MAX_SPR];
static int         g_snc = 0;
static int         g_ssel = 0;

/* SFF save path */
static char g_sff_path[SF_MAX_TEXT_INPUT_LEN] = "tree_out.sff";

/* Preset data */
typedef struct {
    const char *name;
    float depth,branch,angle,len,taper,grav,wiggle,twist,tr,tl,ls,ld;
} preset_t;
static const preset_t k_presets[] = {
/*          depth branch  ang   len  taper  grav wiggle  twist    tr    tl   ls  ld */
{"Oak",      4,    3,   35.f, .70f, .65f, .15f, .12f, 137.5f, .22f, 2.5f, 1.0f, 4},
{"Pine",     5,    2,   18.f, .76f, .72f,-.05f, .06f, 137.5f, .16f, 3.0f, 0.7f, 2},
{"Weeping",  4,    3,   42.f, .68f, .62f,  .4f, .15f, 137.5f, .20f, 2.5f, 1.2f, 4},
{"Birch",    5,    2,   28.f, .72f, .68f, .08f, .18f,  90.0f, .13f, 3.5f, 0.8f, 3},
{"Shrub",    3,    4,   48.f, .62f, .60f, .22f, .20f, 137.5f, .30f, 1.5f, 1.1f, 5},
};
#define N_PRESETS 5

/* ============================================================
   MATH HELPERS
   ============================================================ */
static sf_fvec3_t v3s(sf_fvec3_t v, float s) {
    return (sf_fvec3_t){v.x*s, v.y*s, v.z*s};
}
static float v3len(sf_fvec3_t v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}
/* Rodrigues rotation: rotate v around unit axis k by angle theta */
static sf_fvec3_t v3rot(sf_fvec3_t v, sf_fvec3_t k, float theta) {
    float c = cosf(theta), s = sinf(theta), d = sf_fvec3_dot(k, v);
    sf_fvec3_t cr = sf_fvec3_cross(k, v);
    return (sf_fvec3_t){
        v.x*c + cr.x*s + k.x*d*(1.f-c),
        v.y*c + cr.y*s + k.y*d*(1.f-c),
        v.z*c + cr.z*s + k.z*d*(1.f-c)
    };
}

/* ============================================================
   RNG  (xorshift32)
   ============================================================ */
static uint32_t g_rng;
static void rseed(uint32_t s) { g_rng = s ? s : 1; }
static float rf(void) {
    g_rng ^= g_rng << 13; g_rng ^= g_rng >> 17; g_rng ^= g_rng << 5;
    return (float)(g_rng & 0xFFFF) / 65535.f;
}
static float rf2(void) { return rf()*2.f - 1.f; }  /* [-1, 1] */

/* ============================================================
   MESH — ADD CYLINDER SEGMENT (outward CCW winding)
   Generates a tapered cylinder from p0 to p1 with radii r0,r1.
   UV: u wraps around circumference, v from vt0 to vt1.
   ============================================================ */
static void add_seg(sf_obj_t *o,
    sf_fvec3_t p0, sf_fvec3_t p1, float r0, float r1,
    float vt0, float vt1)
{
    if (o->v_cnt + (SEGS+1)*2 > o->v_cap) return;
    if (o->f_cnt + SEGS*2      > o->f_cap) return;

    sf_fvec3_t ax = sf_fvec3_norm(sf_fvec3_sub(p1, p0));
    sf_fvec3_t tmp = {0,1,0};
    if (fabsf(ax.y) > 0.98f) tmp = (sf_fvec3_t){1,0,0};
    sf_fvec3_t ri = sf_fvec3_norm(sf_fvec3_cross(tmp, ax));
    sf_fvec3_t fw = sf_fvec3_cross(ax, ri);

    int bv = o->v_cnt, bt = o->vt_cnt;
    for (int i = 0; i <= SEGS; i++) {
        float th = (float)i / SEGS * 2.f * SF_PI;
        float c = cosf(th), s = sinf(th), u = (float)i / SEGS;
        sf_fvec3_t d = {ri.x*c+fw.x*s, ri.y*c+fw.y*s, ri.z*c+fw.z*s};
        sf_obj_add_vert(o, (sf_fvec3_t){p0.x+d.x*r0, p0.y+d.y*r0, p0.z+d.z*r0});
        sf_obj_add_vert(o, (sf_fvec3_t){p1.x+d.x*r1, p1.y+d.y*r1, p1.z+d.z*r1});
        sf_obj_add_uv(o, (sf_fvec2_t){u, vt0});
        sf_obj_add_uv(o, (sf_fvec2_t){u, vt1});
    }
    for (int i = 0; i < SEGS; i++) {
        int v00=bv+i*2, v10=bv+i*2+1, v01=bv+(i+1)*2, v11=bv+(i+1)*2+1;
        int t00=bt+i*2, t10=bt+i*2+1, t01=bt+(i+1)*2, t11=bt+(i+1)*2+1;
        /* outward-facing CCW triangles */
        sf_obj_add_face_uv(o, v00, v01, v11, t00, t01, t11);
        sf_obj_add_face_uv(o, v00, v11, v10, t00, t11, t10);
    }
}

/* ============================================================
   TREE GROWTH — RECURSIVE L-SYSTEM BRANCHING
   ============================================================ */
static void grow(sf_obj_t *obj,
    sf_fvec3_t pos, sf_fvec3_t dir,
    float rad, float len, float vt,
    int depth, int maxd)
{
    if (depth < 0) return;
    if (obj->v_cnt + (SEGS+1)*2 > obj->v_cap) return;

    float er = rad * p_taper;

    sf_fvec3_t end = sf_fvec3_add(pos, v3s(dir, len));
    float wa = p_wiggle * len;
    end.x += rf2() * wa;
    end.y += rf2() * wa * 0.25f;
    end.z += rf2() * wa;

    float avgr = (rad + er) * 0.5f;
    float vt1  = vt + len / (2.f * SF_PI * avgr + 1e-4f);

    add_seg(obj, pos, end, rad, er, vt, vt1);

    /* terminal: scatter leaf billboard positions around tip */
    if (depth == 0) {
        int n = (int)(p_ld + .5f);
        for (int i = 0; i < n && g_nleaf < MAX_LEA; i++) {
            float sp = len * 0.9f;
            g_leaves[g_nleaf].p = (sf_fvec3_t){
                end.x + rf2()*sp,
                end.y + rf() *sp * 0.8f,
                end.z + rf2()*sp
            };
            /* clamp to [0,1]: scale_mult*255 is alpha, >1 wraps to 0 */
            g_leaves[g_nleaf].s = fminf(p_ls * (0.7f + rf()*0.3f), 1.0f);
            g_nleaf++;
        }
        return;
    }

    sf_fvec3_t up = {0, 1, 0};
    float d_up = sf_fvec3_dot(dir, up);
    sf_fvec3_t perp = sf_fvec3_sub(up, v3s(dir, d_up));
    float pl = v3len(perp);
    if (pl < 0.01f) perp = (sf_fvec3_t){1, 0, 0};
    else            perp = v3s(perp, 1.f / pl);

    int   nb      = (int)(p_branch + .5f);
    float base_tw = rf() * 2.f * SF_PI;
    float ang     = SF_DEG2RAD(p_angle) + rf2() * SF_DEG2RAD(7.f);

    float gf = p_grav * (float)(maxd - depth + 1) / (float)(maxd + 1);
    gf = gf >  1.f ?  1.f : (gf < -1.f ? -1.f : gf);
    sf_fvec3_t gdir = {0, gf < 0 ? 1.f : -1.f, 0};
    float gabs = fabsf(gf);

    for (int i = 0; i < nb; i++) {
        sf_fvec3_t cd = v3rot(dir, perp, ang);
        cd = v3rot(cd, dir, base_tw + SF_DEG2RAD(p_twist) * (float)i);

        if (gabs > 0.001f)
            cd = sf_fvec3_norm(
                sf_fvec3_add(v3s(cd, 1.f - gabs), v3s(gdir, gabs)));

        sf_fvec3_t right = sf_fvec3_cross(cd, up);
        float rl = v3len(right);
        if (rl > 0.01f) {
            right = v3s(right, 1.f / rl);
            sf_fvec3_t fw2 = sf_fvec3_cross(cd, right);
            float wf = p_wiggle * 0.3f;
            cd = sf_fvec3_norm((sf_fvec3_t){
                cd.x + (rf2()*right.x + rf2()*fw2.x)*wf,
                cd.y + (rf2()*right.y + rf2()*fw2.y)*wf,
                cd.z + (rf2()*right.z + rf2()*fw2.z)*wf
            });
        }

        float cl = len * p_len * (0.88f + rf() * 0.24f);
        grow(obj, end, cd, er, cl, vt1, depth - 1, maxd);
    }
}

/* ============================================================
   GENERATE TREE
   ============================================================ */
static void generate_tree(void) {
    if (!g_obj || !g_enti) return;

    g_obj->v_cnt  = 0;
    g_obj->vt_cnt = 0;
    g_obj->f_cnt  = 0;
    g_obj->src_path = NULL;
    g_nleaf = 0;

    rseed((uint32_t)(p_seed * 17239.f) + 1);
    int maxd = (int)(p_depth + .5f);

    float lean = p_wiggle * 0.2f;
    sf_fvec3_t tdir = sf_fvec3_norm(
        (sf_fvec3_t){rf2()*lean, 1.f, rf2()*lean});

    grow(g_obj, (sf_fvec3_t){0,0,0}, tdir,
         p_tr, p_tl, 0.f, maxd, maxd);

    sf_obj_recompute_bs(g_obj);

    g_enti->obj.v_cnt     = g_obj->v_cnt;
    g_enti->obj.vt_cnt    = g_obj->vt_cnt;
    g_enti->obj.f_cnt     = g_obj->f_cnt;
    g_enti->obj.bs_center = g_obj->bs_center;
    g_enti->obj.bs_radius = g_obj->bs_radius;
}

/* ============================================================
   TEXTURE / SPRITE HELPERS
   ============================================================ */
static void scan_bark_textures(void) {
    const char *dirs[] = {
        SF_ASSET_PATH "/sf_textures/128x128/Wood",
        SF_ASSET_PATH "/sf_textures/128x128/Stone",
        SF_ASSET_PATH "/sf_textures/128x128/Misc",
        NULL
    };
    g_tnc = 0;
    for (int d = 0; dirs[d] && g_tnc < MAX_TEX; d++) {
        DIR *dir = opendir(dirs[d]);
        if (!dir) continue;
        struct dirent *e;
        while ((e = readdir(dir)) && g_tnc < MAX_TEX) {
            if (e->d_name[0] == '.') continue;
            const char *dot = strrchr(e->d_name, '.');
            if (!dot || strcmp(dot, ".bmp") != 0) continue;
            bool dup = false;
            for (int i = 0; i < g_tnc; i++)
                if (strcmp(g_tname[i], e->d_name) == 0) { dup=true; break; }
            if (dup) continue;
            snprintf(g_tname[g_tnc], sizeof(g_tname[0]), "%s", e->d_name);
            g_titem[g_tnc] = g_tname[g_tnc];
            g_tnc++;
        }
        closedir(dir);
    }
}

static sf_tex_t* ensure_bark_tex(int idx) {
    if (idx < 0 || idx >= g_tnc) return NULL;
    char tname[64];
    snprintf(tname, sizeof(tname), "%s", g_tname[idx]);
    char *dot = strrchr(tname, '.');
    if (dot) *dot = '\0';
    sf_tex_t *t = sf_get_texture_(&g_ctx, tname, false);
    if (!t) t = sf_load_texture_bmp(&g_ctx, g_tname[idx], tname);
    return t;
}

static void apply_bark_tex(void) {
    if (!g_enti) return;
    sf_tex_t *t = ensure_bark_tex(g_tsel);
    if (t) {
        g_enti->tex = t;
        g_enti->tex_scale = (sf_fvec2_t){1.f, 1.f};
    }
}

/* Scan sf_sprites dir, load textures and create sprites for each .bmp found. */
static void load_leaf_sprites(void) {
    const char *dir_path = SF_ASSET_PATH "/sf_sprites";
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    struct dirent *e;
    while ((e = readdir(dir)) && g_snc < MAX_SPR) {
        if (e->d_name[0] == '.') continue;
        const char *dot = strrchr(e->d_name, '.');
        if (!dot || strcmp(dot, ".bmp") != 0) continue;
        int nlen = (int)(dot - e->d_name);
        snprintf(g_sname[g_snc], sizeof(g_sname[0]), "%.*s", nlen, e->d_name);
        g_sitem[g_snc] = g_sname[g_snc];
        sf_tex_t *t = sf_load_texture_bmp(&g_ctx, e->d_name, g_sname[g_snc]);
        char sprname[80];
        snprintf(sprname, sizeof(sprname), "spr_%s", g_sname[g_snc]);
        g_sprites[g_snc] = t ? sf_load_sprite(&g_ctx, sprname, 1.f, 0.7f, 1, g_sname[g_snc]) : NULL;
        g_snc++;
    }
    closedir(dir);
    if (g_snc > 0 && g_sprites[0]) g_leaf = g_sprites[0];
}

/* Nearest-neighbour blit of texture t into the camera pixel buffer at (px,py), size pw x ph. */
static void blit_preview(sf_tex_t *t, int px, int py, int pw, int ph) {
    if (!t || !t->px || t->w <= 0 || t->h <= 0) return;
    sf_pkd_clr_t *buf = g_ctx.main_camera.buffer;
    int bw = g_ctx.main_camera.w;
    int bh = g_ctx.main_camera.h;
    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            int bx = px + x, by = py + y;
            if (bx < 0 || bx >= bw || by < 0 || by >= bh) continue;
            int tx = x * t->w / pw;
            int ty = y * t->h / ph;
            sf_pkd_clr_t c = t->px[ty * t->w + tx];
            if ((c & 0x00FFFFFF) == 0x00FF00FF) continue;  /* skip magenta */
            buf[by * bw + bx] = c;
        }
    }
}

/* ============================================================
   CAMERA
   ============================================================ */
static void update_camera(void) {
    float x = sinf(g_yaw) * cosf(g_pitch) * g_dist;
    float y = sinf(g_pitch) * g_dist + 3.5f;
    float z = cosf(g_yaw) * cosf(g_pitch) * g_dist;
    sf_camera_set_pos(&g_ctx, &g_ctx.main_camera, x, y, z);
    sf_camera_look_at(&g_ctx, &g_ctx.main_camera, (sf_fvec3_t){0, 3.5f, 0});
}

/* ============================================================
   SAVE TREE — model-only .sff (mesh + bark tex + leaf sprite + entity)
   ============================================================ */
static void save_tree(void) {
    if (!g_obj) return;

    /* Derive mesh basename from .sff path */
    const char *sl  = strrchr(g_sff_path, '/');
    char base[256];
    snprintf(base, sizeof(base), "%s", sl ? sl+1 : g_sff_path);
    char *bdot = strrchr(base, '.');
    if (bdot) *bdot = '\0';

    /* Output dir */
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", g_sff_path);
    char *sl2 = strrchr(dir, '/');
    if (sl2) *sl2 = '\0'; else { dir[0]='.'; dir[1]='\0'; }

    /* Save mesh .obj */
    char obj_path[512];
    snprintf(obj_path, sizeof(obj_path), "%s/%s.obj", dir, base);
    sf_obj_save_obj(&g_ctx, g_obj, obj_path);

    /* Write model-only .sff */
    FILE *f = fopen(g_sff_path, "w");
    if (!f) { SF_LOG(&g_ctx, SF_LOG_ERROR, "Cannot write %s\n", g_sff_path); return; }

    fprintf(f, "# Saffron Tree Model\n\n");
    fprintf(f, "mesh %s \"%s.obj\"\n\n", g_obj->name, base);

    if (g_enti && g_enti->tex && g_enti->tex->name)
        fprintf(f, "texture %s \"%s.bmp\"\n", g_enti->tex->name, g_enti->tex->name);

    if (g_leaf && g_leaf->frame_count > 0 && g_leaf->frames[0] && g_leaf->frames[0]->name)
        fprintf(f, "texture %s \"%s.bmp\"\n", g_leaf->frames[0]->name, g_leaf->frames[0]->name);

    fprintf(f, "\n");

    if (g_leaf && g_leaf->name) {
        const char *ftex = (g_leaf->frame_count > 0 && g_leaf->frames[0])
                           ? g_leaf->frames[0]->name : "";
        fprintf(f, "sprite %s {\n", g_leaf->name);
        fprintf(f, "    duration = %.2f\n", g_leaf->frame_duration);
        fprintf(f, "    scale    = %.3f\n", g_leaf->base_scale);
        fprintf(f, "    frames   = [%s]\n", ftex);
        fprintf(f, "}\n\n");
    }

    if (g_enti && g_enti->name && g_enti->frame) {
        sf_fvec3_t p = g_enti->frame->pos;
        sf_fvec3_t r = g_enti->frame->rot;
        sf_fvec3_t s = g_enti->frame->scale;
        fprintf(f, "entity %s {\n", g_enti->name);
        fprintf(f, "    mesh    = %s\n", g_obj->name);
        fprintf(f, "    pos     = (%.3f, %.3f, %.3f)\n", p.x, p.y, p.z);
        fprintf(f, "    rot     = (%.3f, %.3f, %.3f)\n", r.x, r.y, r.z);
        fprintf(f, "    scale   = (%.3f, %.3f, %.3f)\n", s.x, s.y, s.z);
        if (g_enti->tex && g_enti->tex->name)
            fprintf(f, "    texture = %s\n", g_enti->tex->name);
        fprintf(f, "}\n");
    }

    fclose(f);
    SF_LOG(&g_ctx, SF_LOG_INFO, "Saved tree: %s\n", g_sff_path);
}

/* ============================================================
   UI CALLBACKS
   ============================================================ */
static void cb_save(sf_ctx_t *ctx, void *ud) { (void)ctx; (void)ud; save_tree(); }

static void apply_preset(const preset_t *p) {
    p_depth  = p->depth;   p_branch = p->branch;
    p_angle  = p->angle;   p_len    = p->len;
    p_taper  = p->taper;   p_grav   = p->grav;
    p_wiggle = p->wiggle;  p_twist  = p->twist;
    p_tr     = p->tr;      p_tl     = p->tl;
    p_ls     = p->ls;      p_ld     = p->ld;
}

static void cb_preset0(sf_ctx_t *c, void *u){(void)c;(void)u; apply_preset(&k_presets[0]);}
static void cb_preset1(sf_ctx_t *c, void *u){(void)c;(void)u; apply_preset(&k_presets[1]);}
static void cb_preset2(sf_ctx_t *c, void *u){(void)c;(void)u; apply_preset(&k_presets[2]);}
static void cb_preset3(sf_ctx_t *c, void *u){(void)c;(void)u; apply_preset(&k_presets[3]);}
static void cb_preset4(sf_ctx_t *c, void *u){(void)c;(void)u; apply_preset(&k_presets[4]);}
static void (*k_preset_cbs[N_PRESETS])(sf_ctx_t*,void*) = {
    cb_preset0, cb_preset1, cb_preset2, cb_preset3, cb_preset4
};

/* ============================================================
   BUILD UI
   ============================================================ */
static void build_ui(void) {
    sf_ui_add_panel(&g_ctx, "Tree Generator",
        (sf_ivec2_t){0, PY0},
        (sf_ivec2_t){PX1, H});

#define ROW_DF(row, lbl, tgt, step) do { \
    sf_ui_add_label(&g_ctx, lbl, (sf_ivec2_t){LX, RY(row)+2}, 0); \
    sf_ui_add_drag_float(&g_ctx, (sf_ivec2_t){IX,RY(row)}, \
        (sf_ivec2_t){IX1,RY(row)+12}, &tgt, step, NULL, NULL); \
} while(0)

    ROW_DF( 0, "Seed",     p_seed,   1.0f);
    ROW_DF( 1, "Depth",    p_depth,  0.5f);
    ROW_DF( 2, "Branches", p_branch, 1.0f);
    ROW_DF( 3, "Angle",    p_angle,  1.0f);
    ROW_DF( 4, "Length",   p_len,    0.01f);
    ROW_DF( 5, "Taper",    p_taper,  0.01f);
    ROW_DF( 6, "Gravity",  p_grav,   0.01f);
    ROW_DF( 7, "Wiggle",   p_wiggle, 0.01f);
    ROW_DF( 8, "Twist",    p_twist,  1.0f);
    ROW_DF( 9, "Trunk R",  p_tr,     0.01f);
    ROW_DF(10, "Trunk L",  p_tl,     0.1f);
    ROW_DF(11, "Leaf Scl", p_ls,     0.05f);
    ROW_DF(12, "Leaf Cnt", p_ld,     1.0f);

#undef ROW_DF

    /* Bark texture dropdown */
    sf_ui_add_label(&g_ctx, "Bark Tex", (sf_ivec2_t){LX, RY(13)+2}, 0);
    if (g_tnc > 0)
        sf_ui_add_dropdown(&g_ctx,
            (sf_ivec2_t){LX,  RY(14)},
            (sf_ivec2_t){IX1, RY(14)+12},
            g_titem, g_tnc, &g_tsel, NULL, NULL);

    /* Leaf sprite dropdown (preview blitted manually after sf_ui_render at RY(17)) */
    sf_ui_add_label(&g_ctx, "Leaf Spr", (sf_ivec2_t){LX, RY(15)+2}, 0);
    if (g_snc > 0)
        sf_ui_add_dropdown(&g_ctx,
            (sf_ivec2_t){LX,  RY(16)},
            (sf_ivec2_t){IX1, RY(16)+12},
            g_sitem, g_snc, &g_ssel, NULL, NULL);

    /* Save button */
    sf_ui_add_button(&g_ctx, "Save",
        (sf_ivec2_t){LX,  RY(20)},
        (sf_ivec2_t){IX1, RY(20)+14},
        cb_save, NULL);

    /* SFF path text input */
    sf_ui_add_label(&g_ctx, "SFF path", (sf_ivec2_t){LX, RY(21)+2}, 0);
    sf_ui_add_text_input(&g_ctx,
        (sf_ivec2_t){LX,  RY(22)},
        (sf_ivec2_t){IX1, RY(22)+12},
        g_sff_path, (int)sizeof(g_sff_path), NULL, NULL);

    /* Preset buttons */
    sf_ui_add_label(&g_ctx, "Presets", (sf_ivec2_t){LX, RY(23)+2}, 0);

    int pw = (IX1 - LX - 2) / 3;
    for (int i = 0; i < N_PRESETS; i++) {
        int row = 24 + i/3;
        int col = i % 3;
        int bx0 = LX + col*(pw+1);
        int bx1 = bx0 + pw;
        sf_ui_add_button(&g_ctx, k_presets[i].name,
            (sf_ivec2_t){bx0, RY(row)},
            (sf_ivec2_t){bx1, RY(row)+14},
            k_preset_cbs[i], NULL);
    }
}

/* ============================================================
   MAIN
   ============================================================ */
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window   *win = SDL_CreateWindow("sf_trees — Procedural Tree Generator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture  *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, W, H);

    sf_init(&g_ctx, W, H);
    sf_set_logger(&g_ctx, sf_logger_console, NULL);
    sf_camera_set_psp(&g_ctx, &g_ctx.main_camera, 60.f, 0.1f, 200.f);
    update_camera();

    sf_light_t *key = sf_add_light(&g_ctx, "key",
        SF_LIGHT_DIR, (sf_fvec3_t){1.0f, 0.95f, 0.88f}, 1.3f);
    if (key) sf_frame_look_at(key->frame, (sf_fvec3_t){ 1.f, -1.f, -0.5f});

    sf_light_t *fill = sf_add_light(&g_ctx, "fill",
        SF_LIGHT_DIR, (sf_fvec3_t){0.45f, 0.55f, 0.65f}, 0.5f);
    if (fill) sf_frame_look_at(fill->frame, (sf_fvec3_t){-1.f, -0.3f, 1.f});

    scan_bark_textures();
    load_leaf_sprites();

    g_obj  = sf_obj_create_empty(&g_ctx, "tree", MAX_V, MAX_UV, MAX_F);
    g_enti = sf_add_enti(&g_ctx, g_obj, "tree_enti");
    if (g_enti) sf_enti_set_pos(&g_ctx, g_enti, 0.f, 0.f, 0.f);

    build_ui();

    /* Initial tree (Oak preset) */
    apply_preset(&k_presets[0]);
    generate_tree();
    apply_bark_tex();

    SDL_Event ev;
    while (sf_running(&g_ctx)) {
        sf_input_cycle_state(&g_ctx);

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { sf_stop(&g_ctx); break; }

            /* LMB orbit — only right of the UI panel */
            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT
                    && ev.button.x >= PX1) {
                g_drag = true; g_lmx = ev.button.x; g_lmy = ev.button.y;
            } else if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                g_drag = false;
            } else if (ev.type == SDL_MOUSEMOTION && g_drag) {
                float dx = (float)(ev.motion.x - g_lmx) * 0.007f;
                float dy = (float)(ev.motion.y - g_lmy) * 0.007f;
                g_yaw   -= dx;   /* inverted for natural feel */
                g_pitch -= dy;
                if (g_pitch >  1.4f) g_pitch =  1.4f;
                if (g_pitch < -0.1f) g_pitch = -0.1f;
                g_lmx = ev.motion.x; g_lmy = ev.motion.y;
                update_camera();
            } else if (ev.type == SDL_MOUSEWHEEL) {
                g_dist -= ev.wheel.y * 0.8f;
                if (g_dist <  2.f) g_dist =  2.f;
                if (g_dist > 60.f) g_dist = 60.f;
                update_camera();
            }

            sf_sdl_process_event(&g_ctx, &ev);
        }

        if (sf_key_pressed(&g_ctx, SF_KEY_ESC)) sf_stop(&g_ctx);

        /* Real-time update: regenerate when any parameter changes */
        {
            static float last_p[13] = {-1e30f};
            static int   last_tsel  = -1;
            static int   last_ssel  = -1;

            float cur[13] = {p_seed,p_depth,p_branch,p_angle,p_len,p_taper,
                             p_grav,p_wiggle,p_twist,p_tr,p_tl,p_ls,p_ld};
            if (memcmp(cur, last_p, sizeof(cur)) != 0) {
                memcpy(last_p, cur, sizeof(cur));
                generate_tree();
                apply_bark_tex();
            }
            if (g_tsel != last_tsel) {
                last_tsel = g_tsel;
                apply_bark_tex();
            }
            if (g_ssel != last_ssel) {
                last_ssel = g_ssel;
                if (g_ssel >= 0 && g_ssel < g_snc && g_sprites[g_ssel])
                    g_leaf = g_sprites[g_ssel];
            }
        }

        sf_time_update(&g_ctx);
        sf_ui_update(&g_ctx, g_ctx.ui);
        sf_render_ctx(&g_ctx);

        /* Leaf billboards */
        if (g_leaf) {
            for (int i = 0; i < g_nleaf; i++)
                sf_draw_sprite(&g_ctx, &g_ctx.main_camera,
                    g_leaf, g_leaves[i].p, 0.f, g_leaves[i].s);
        }

        sf_draw_debug_perf(&g_ctx, &g_ctx.main_camera);
        sf_ui_render(&g_ctx, &g_ctx.main_camera, g_ctx.ui);

        /* Sprite preview (40x40) blitted on top of panel after UI render */
        if (g_snc > 0 && g_ssel >= 0 && g_ssel < g_snc) {
            sf_sprite_t *spr = g_sprites[g_ssel];
            if (spr && spr->frame_count > 0 && spr->frames[0])
                blit_preview(spr->frames[0], LX, RY(17), 40, 40);
        }

        SDL_UpdateTexture(tex, NULL,
            g_ctx.main_camera.buffer,
            W * sizeof(sf_pkd_clr_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    sf_destroy(&g_ctx);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
