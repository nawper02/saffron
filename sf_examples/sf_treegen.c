/* sf_treegen — Procedural Tree Generator for Saffron */
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
#define W        800
#define H        600
#define SEGS     6
#define MAX_V    32000
#define MAX_UV   32000
#define MAX_F    32000
#define MAX_TEX  64
#define MAX_SPR  32
#define ICON_SZ  13            /* slider icon size (px)              */

/* UI panel */
#define PX1    222
#define PY0    18
#define LX     5
#define LX_TXT (LX + ICON_SZ + 2)   /* label x, after icon            */
#define IX     78
#define IX1    218
#define RH     15
#define RY(n)  (PY0 + 22 + (n)*RH)

/* Top-right texture previews (just below stats bar) */
#define PREV_SZ   48
#define PREV_Y    (PY0 + 2)
#define BARK_PX   (W - PREV_SZ - 4)
#define LEAF_PX   (W - PREV_SZ*2 - 8)

/* ============================================================
   GLOBALS
   ============================================================ */
static sf_ctx_t      g_ctx;
static sf_obj_t     *g_obj  = NULL;
static sf_enti_t    *g_enti = NULL;
static sf_sprite_t  *g_leaf = NULL;

/* Camera orbit */
static float   g_yaw   =  0.5f;
static float   g_pitch =  0.35f;
static float   g_dist  = 15.0f;
static bool    g_drag  = false;
static int     g_lmx, g_lmy;

/* Tree parameters */
static float p_seed   = 42.0f;
static float p_depth  =  4.0f;
static float p_branch =  3.0f;
static float p_angle  = 32.0f;
static float p_len    =  0.70f;
static float p_taper  =  0.65f;
static float p_grav   =  0.15f;
static float p_wiggle =  0.12f;
static float p_twist  =137.5f;
static float p_tr     =  0.22f;
static float p_tl     =  2.5f;
static float p_ls     =  1.0f;   /* leaf scale   */
static float p_ld     =  4.0f;   /* leaf count   */
static float p_lo     =  0.85f;  /* leaf opacity */

/* Bark textures */
static char       g_tname[MAX_TEX][64];
static const char*g_titem[MAX_TEX];
static int        g_tnc = 0;
static int        g_tsel = 0;

/* Leaf sprites */
static char        g_sname[MAX_SPR][64];
static const char *g_sitem[MAX_SPR];
static sf_sprite_t*g_sprites[MAX_SPR];
static int         g_snc = 0;
static int         g_ssel = 0;

/* SFF save path */
static char g_sff_path[SF_MAX_TEXT_INPUT_LEN] = "tree_out.sff";

/* 3D leaf orientation toggle */
static bool p_rand3d = false;

/* Slider icons (one per drag-float row, 14 total) */
static const char *k_row_icon_bmp[14] = {
    "seed.bmp",        /* Seed      */
    "depth.bmp",       /* Depth     */
    "branches.bmp",    /* Branches  */
    "angle.bmp",       /* Angle     */
    "length.bmp",      /* Length    */
    "taper.bmp",       /* Taper     */
    "gravity.bmp",     /* Gravity   */
    "wiggle.bmp",      /* Wiggle    */
    "twist.bmp",       /* Twist     */
    "trunk_r.bmp",     /* Trunk R   */
    "trunk_l.bmp",     /* Trunk L   */
    "leaf_scl.bmp",    /* Leaf Scl  */
    "leaf_cnt.bmp",    /* Leaf Cnt  */
    "leaf_opacity.bmp",/* Leaf Opa  */
};
static sf_tex_t *g_row_icon[14] = {0};

/* Preset data */
typedef struct {
    const char *name;
    float depth,branch,angle,len,taper,grav,wiggle,twist,tr,tl,ls,ld,lo;
} preset_t;
static const preset_t k_presets[] = {
{"Oak",      4, 3, 35.f, .70f, .65f, .15f, .12f, 137.5f, .22f, 2.5f, 1.0f, 4, .85f},
{"Pine",     5, 2, 18.f, .76f, .72f,-.05f, .06f, 137.5f, .16f, 3.0f, 0.7f, 2, .80f},
{"Weeping",  4, 3, 42.f, .68f, .62f,  .4f, .15f, 137.5f, .20f, 2.5f, 1.2f, 4, .90f},
{"Birch",    5, 2, 28.f, .72f, .68f, .08f, .18f,  90.0f, .13f, 3.5f, 0.8f, 3, .85f},
{"Shrub",    3, 4, 48.f, .62f, .60f, .22f, .20f, 137.5f, .30f, 1.5f, 1.1f, 5, .90f},
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
static float rf2(void) { return rf()*2.f - 1.f; }

/* ============================================================
   MESH — ADD CYLINDER SEGMENT (outward CCW winding)
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
        sf_obj_add_face_uv(o, v00, v01, v11, t00, t01, t11);
        sf_obj_add_face_uv(o, v00, v11, v10, t00, t11, t10);
    }
}

/* ============================================================
   TREE GROWTH
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

    if (depth == 0) {
        int n = (int)(p_ld + .5f);
        for (int i = 0; i < n && g_ctx.bill_count < SF_MAX_BILLS; i++) {
            float sp = len * 0.9f;
            sf_fvec3_t lp = {
                end.x + rf2()*sp,
                end.y + rf() *sp * 0.8f,
                end.z + rf2()*sp
            };
            float ls = fminf(p_ls * (0.7f + rf()*0.3f), 2.0f);
            float lo = fminf(p_lo, 1.0f);
            float la = rf() * 2.f * SF_PI;  /* random screen-space rotation */
            char bname[32];
            snprintf(bname, sizeof(bname), "lf_%d", g_ctx.bill_count);
            sf_bill_t *bl = sf_add_bill(&g_ctx, g_leaf, bname, lp, ls, lo, la);
            if (bl && p_rand3d) {
                bl->normal.x = rf2(); bl->normal.y = rf2(); bl->normal.z = rf2();
                float nl = sqrtf(bl->normal.x*bl->normal.x + bl->normal.y*bl->normal.y + bl->normal.z*bl->normal.z);
                if (nl > 0.001f) { bl->normal.x /= nl; bl->normal.y /= nl; bl->normal.z /= nl; }
            }
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
    sf_clear_bills(&g_ctx);

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

/* Nearest-neighbour blit with magenta-keyed transparency. */
static void blit_keyed(sf_tex_t *t, int px, int py, int pw, int ph) {
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
            if ((c & 0x00FFFFFF) == 0x00FF00FF) continue;
            if ((c >> 24) == 0) continue;
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
   SAVE TREE — model-only .sff
   ============================================================ */
static void save_tree(void) {
    if (!g_obj) return;

    const char *sl = strrchr(g_sff_path, '/');
    char base[256];
    snprintf(base, sizeof(base), "%s", sl ? sl+1 : g_sff_path);
    char *bdot = strrchr(base, '.');
    if (bdot) *bdot = '\0';

    char dir[512];
    snprintf(dir, sizeof(dir), "%s", g_sff_path);
    char *sl2 = strrchr(dir, '/');
    if (sl2) *sl2 = '\0'; else { dir[0]='.'; dir[1]='\0'; }

    char obj_path[512];
    snprintf(obj_path, sizeof(obj_path), "%s/%s.obj", dir, base);
    sf_obj_save_obj(&g_ctx, g_obj, obj_path);

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
        fprintf(f, "}\n\n");
    }

    /* Billboard instances */
    for (int i = 0; i < g_ctx.bill_count; i++) {
        sf_bill_t *b = &g_ctx.bills[i];
        if (!b->sprite || !b->sprite->name) continue;
        fprintf(f, "billboard %s {\n", b->name[0] ? b->name : "bill");
        fprintf(f, "    sprite  = %s\n", b->sprite->name);
        fprintf(f, "    pos     = (%.4f, %.4f, %.4f)\n", b->pos.x, b->pos.y, b->pos.z);
        fprintf(f, "    scale   = %.4f\n", b->scale);
        fprintf(f, "    opacity = %.4f\n", b->opacity);
        fprintf(f, "    angle   = %.4f\n", b->angle);
        fprintf(f, "    normal  = (%.4f, %.4f, %.4f)\n", b->normal.x, b->normal.y, b->normal.z);
        fprintf(f, "}\n\n");
    }

    fclose(f);
    SF_LOG(&g_ctx, SF_LOG_INFO, SF_LOG_INDENT "Saved tree: %s\n", g_sff_path);
}

/* ============================================================
   UI CALLBACKS
   ============================================================ */
static void cb_save(sf_ctx_t *ctx, void *ud) { (void)ctx; (void)ud; save_tree(); }
static void cb_rand3d(sf_ctx_t *ctx, void *ud) { (void)ctx; (void)ud; p_rand3d = !p_rand3d; }

static void apply_preset(const preset_t *p) {
    p_depth  = p->depth;   p_branch = p->branch;
    p_angle  = p->angle;   p_len    = p->len;
    p_taper  = p->taper;   p_grav   = p->grav;
    p_wiggle = p->wiggle;  p_twist  = p->twist;
    p_tr     = p->tr;      p_tl     = p->tl;
    p_ls     = p->ls;      p_ld     = p->ld;
    p_lo     = p->lo;
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
    sf_ui_add_label(&g_ctx, lbl, (sf_ivec2_t){LX_TXT, RY(row)+2}, 0); \
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
    ROW_DF(13, "Leaf Opa", p_lo,     0.01f);

#undef ROW_DF

    /* Bark texture dropdown */
    sf_ui_add_label(&g_ctx, "Bark Tex", (sf_ivec2_t){LX_TXT, RY(14)+2}, 0);
    if (g_tnc > 0)
        sf_ui_add_dropdown(&g_ctx,
            (sf_ivec2_t){LX,  RY(15)},
            (sf_ivec2_t){IX1, RY(15)+12},
            g_titem, g_tnc, &g_tsel, NULL, NULL);

    /* Leaf sprite dropdown */
    sf_ui_add_label(&g_ctx, "Leaf Spr", (sf_ivec2_t){LX_TXT, RY(16)+2}, 0);
    if (g_snc > 0)
        sf_ui_add_dropdown(&g_ctx,
            (sf_ivec2_t){LX,  RY(17)},
            (sf_ivec2_t){IX1, RY(17)+12},
            g_sitem, g_snc, &g_ssel, NULL, NULL);
    /* sprite preview blitted at top-right after sf_ui_render */

    /* 3D leaf orientation toggle */
    sf_ui_add_checkbox(&g_ctx, "3D Leaves",
        (sf_ivec2_t){LX, RY(18)},
        (sf_ivec2_t){IX1, RY(18)+14},
        p_rand3d, cb_rand3d, NULL);

    /* Save button */
    sf_ui_add_button(&g_ctx, "Save",
        (sf_ivec2_t){LX,  RY(19)},
        (sf_ivec2_t){IX1, RY(19)+14},
        cb_save, NULL);

    /* SFF path */
    sf_ui_add_label(&g_ctx, "SFF path", (sf_ivec2_t){LX_TXT, RY(20)+2}, 0);
    sf_ui_add_text_input(&g_ctx,
        (sf_ivec2_t){LX,  RY(21)},
        (sf_ivec2_t){IX1, RY(21)+12},
        g_sff_path, (int)sizeof(g_sff_path), NULL, NULL);

    /* Preset buttons */
    sf_ui_add_label(&g_ctx, "Presets", (sf_ivec2_t){LX_TXT, RY(22)+2}, 0);

    int pw = (IX1 - LX - 2) / 3;
    for (int i = 0; i < N_PRESETS; i++) {
        int row = 23 + i/3;
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
    SDL_Window   *win = SDL_CreateWindow("sf_treegen",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture  *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, W, H);

    sf_init(&g_ctx, W, H);
    sf_set_logger(&g_ctx, sf_logger_console, NULL);
    sf_camera_set_psp(&g_ctx, &g_ctx.main_camera, 60.f, 0.1f, 200.f);
    /* Shift projection so scene centers in open area to the right of the panel */
    {
        float aspect = (float)W / (float)H;
        g_ctx.main_camera.P = sf_make_psp_fmat4(60.f, aspect, 0.1f, 200.f);
        float open_cx = PX1 + (W - PX1) * 0.5f;
        g_ctx.main_camera.P.m[2][0] = 1.f - 2.f * open_cx / (float)W;
        g_ctx.main_camera.is_proj_dirty = false;
    }
    update_camera();

    sf_light_t *key = sf_add_light(&g_ctx, "key",
        SF_LIGHT_DIR, (sf_fvec3_t){1.0f, 0.95f, 0.88f}, 1.3f);
    if (key) sf_frame_look_at(key->frame, (sf_fvec3_t){ 1.f, -1.f, -0.5f});

    sf_light_t *fill = sf_add_light(&g_ctx, "fill",
        SF_LIGHT_DIR, (sf_fvec3_t){0.45f, 0.55f, 0.65f}, 0.5f);
    if (fill) sf_frame_look_at(fill->frame, (sf_fvec3_t){-1.f, -0.3f, 1.f});

    scan_bark_textures();
    load_leaf_sprites();

    /* Load slider icons */
    for (int i = 0; i < 14; i++) {
        char iname[32];
        snprintf(iname, sizeof(iname), "ico_%d", i);
        g_row_icon[i] = sf_load_texture_bmp(&g_ctx, k_row_icon_bmp[i], iname);
    }

    g_obj  = sf_obj_create_empty(&g_ctx, "tree", MAX_V, MAX_UV, MAX_F);
    g_enti = sf_add_enti(&g_ctx, g_obj, "tree_enti");
    if (g_enti) sf_enti_set_pos(&g_ctx, g_enti, 0.f, 0.f, 0.f);

    build_ui();

    apply_preset(&k_presets[0]);
    generate_tree();
    apply_bark_tex();

    SDL_Event ev;
    while (sf_running(&g_ctx)) {
        sf_input_cycle_state(&g_ctx);

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { sf_stop(&g_ctx); break; }

            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT
                    && ev.button.x >= PX1) {
                g_drag = true; g_lmx = ev.button.x; g_lmy = ev.button.y;
            } else if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                g_drag = false;
            } else if (ev.type == SDL_MOUSEMOTION && g_drag) {
                float dx = (float)(ev.motion.x - g_lmx) * 0.007f;
                float dy = (float)(ev.motion.y - g_lmy) * 0.007f;
                g_yaw   -= dx;
                g_pitch += dy;   /* inverted up: drag up = pitch down */
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

        /* Real-time update */
        {
            static float last_p[14] = {-1e30f};
            static int   last_tsel  = -1;
            static int   last_ssel  = -1;
            static bool  last_rand3d = false;

            float cur[14] = {p_seed,p_depth,p_branch,p_angle,p_len,p_taper,
                             p_grav,p_wiggle,p_twist,p_tr,p_tl,p_ls,p_ld,p_lo};
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
                if (g_ssel >= 0 && g_ssel < g_snc && g_sprites[g_ssel]) {
                    g_leaf = g_sprites[g_ssel];
                    /* Regenerate so new sprite is used for bills */
                    generate_tree();
                    apply_bark_tex();
                }
            }
            if (p_rand3d != last_rand3d) {
                last_rand3d = p_rand3d;
                generate_tree();
                apply_bark_tex();
            }
        }

        sf_time_update(&g_ctx);
        sf_ui_update(&g_ctx, g_ctx.ui);
        sf_render_ctx(&g_ctx);

        /* Draw billboard leaves */
        for (int i = 0; i < g_ctx.bill_count; i++)
            sf_draw_bill(&g_ctx, &g_ctx.main_camera, &g_ctx.bills[i], 0.f);

        sf_draw_debug_perf(&g_ctx, &g_ctx.main_camera);
        sf_ui_render(&g_ctx, &g_ctx.main_camera, g_ctx.ui);

        /* Slider icons (blitted on top of UI, left of each label) */
        for (int i = 0; i < 14; i++) {
            if (g_row_icon[i])
                blit_keyed(g_row_icon[i], LX, RY(i)+1, ICON_SZ, ICON_SZ);
        }

        /* Texture previews: top-right corner of scene view */
        if (g_snc > 0 && g_ssel >= 0 && g_ssel < g_snc) {
            sf_sprite_t *spr = g_sprites[g_ssel];
            if (spr && spr->frame_count > 0 && spr->frames[0])
                blit_keyed(spr->frames[0], LEAF_PX, PREV_Y, PREV_SZ, PREV_SZ);
        }
        if (g_tnc > 0 && g_ssel >= 0) {
            sf_tex_t *bt = ensure_bark_tex(g_tsel);
            if (bt) blit_keyed(bt, BARK_PX, PREV_Y, PREV_SZ, PREV_SZ);
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
