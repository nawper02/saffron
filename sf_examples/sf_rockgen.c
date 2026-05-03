/* sf_rockgen — Procedural Rock Generator for Saffron
 *
 * Generates rocks by subdividing an icosphere, displacing vertices with
 * fractal value noise, then applying shape modifiers (flatness, elongation,
 * pointiness).  Follows the same patterns as sf_treegen. */

#define SAFFRON_IMPLEMENTATION
#include "saffron.h"
#undef SAFFRON_IMPLEMENTATION

#define SAFFRON_SDL_IMPLEMENTATION
#include "sf_extras/sf_sdl.h"
#undef SAFFRON_SDL_IMPLEMENTATION

#include <SDL2/SDL.h>
#include <dirent.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
   CONSTANTS
   ============================================================ */
#define W        800
#define H        600
#define MAX_TEX  64

/* Icosphere limits (subdiv 4 → 2562 unique verts, 5120 faces) */
#define MAX_ISO_V  3000
#define MAX_ISO_F  6000

/* Final obj: one unique vert per face corner (no UV seams) */
#define MAX_OBJ_V  16000
#define MAX_OBJ_UV 16000
#define MAX_OBJ_F   6000

/* Midpoint cache for subdivision (open-addressing hash) */
#define MID_SZ 65536

/* UI panel */
#define PX1      222
#define PY0       18
#define LX          5
#define LX_TXT    (LX + 2)
#define IX         85
#define IX1       195
#define RH         15
#define RY(n)     (PY0 + 22 + (n)*RH)

/* Texture preview */
#define PREV_SZ   48
#define PREV_Y    (PY0 + 2)
#define TEX_PX    (W - PREV_SZ - 4)

/* ============================================================
   GLOBALS
   ============================================================ */
static sf_ctx_t    g_ctx;
static sf_obj_t   *g_obj  = NULL;
static sf_enti_t  *g_enti = NULL;

/* Camera orbit */
static float g_yaw   =  0.5f;
static float g_pitch =  0.35f;
static float g_dist  =  5.0f;
static bool  g_drag  = false;
static int   g_lmx, g_lmy;

/* Rock parameters */
static float p_seed    =  1.0f;
static float p_subdiv  =  3.0f;
static float p_rough   =  0.30f;
static float p_freq    =  2.0f;
static float p_octaves =  4.0f;
static float p_persist =  0.50f;
static float p_flat    =  0.20f;
static float p_elongx  =  1.0f;
static float p_elongz  =  1.0f;
static float p_pointy  =  0.0f;
static float p_bump    =  0.5f;

/* Stone textures */
static char        g_tname[MAX_TEX][64];
static const char *g_titem[MAX_TEX];
static int         g_tnc  = 0;
static int         g_tsel = 0;

/* SFF save path */
static char g_sff_path[SF_MAX_TEXT_INPUT_LEN] = "rock_out.sff";

/* ============================================================
   PRESETS
   ============================================================ */
typedef struct {
    const char *name;
    float subdiv, rough, freq, octaves, persist;
    float flat, elongx, elongz, pointy, bump;
} rock_preset_t;

static const rock_preset_t k_presets[] = {
    /* name       sub  rough  freq  oct  per   flat  ex    ez   pointy bump */
    {"Cobble",    3,   0.20f, 2.0f,  4, 0.50f, 0.30f, 1.0f, 1.0f,  0.0f, 0.30f},
    {"Boulder",   3,   0.40f, 1.5f,  5, 0.55f, 0.10f, 1.2f, 0.9f,  0.0f, 0.70f},
    {"Slab",      3,   0.15f, 2.5f,  3, 0.45f, 0.65f, 1.4f, 1.2f, -0.3f, 0.20f},
    {"Spire",     3,   0.25f, 1.8f,  4, 0.50f, 0.00f, 0.6f, 0.6f,  1.2f, 0.40f},
    {"Pebble",    3,   0.08f, 3.0f,  2, 0.40f, 0.10f, 1.1f, 0.9f,  0.0f, 0.10f},
};
#define N_PRESETS 5

/* ============================================================
   HASH-BASED VALUE NOISE (no external deps)
   ============================================================ */
static uint32_t _h3(int32_t x, int32_t y, int32_t z, uint32_t s) {
    uint32_t h = s ^ ((uint32_t)x * 374761393u)
                   ^ ((uint32_t)y * 668265263u)
                   ^ ((uint32_t)z * 3266489917u);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float _s(float t) { return t*t*(3.f - 2.f*t); }

static float vn3(float x, float y, float z, uint32_t seed) {
    int32_t ix=(int32_t)floorf(x), iy=(int32_t)floorf(y), iz=(int32_t)floorf(z);
    float fx=x-ix, fy=y-iy, fz=z-iz;
    float ux=_s(fx), uy=_s(fy), uz=_s(fz);
#define NV(a,b,c) ((float)(_h3(ix+(a),iy+(b),iz+(c),seed)&0xFFFFu)/65535.f)
    return (1.f-uz)*((1.f-uy)*((1.f-ux)*NV(0,0,0)+ux*NV(1,0,0))
                        +uy  *((1.f-ux)*NV(0,1,0)+ux*NV(1,1,0)))
              +uz  *((1.f-uy)*((1.f-ux)*NV(0,0,1)+ux*NV(1,0,1))
                        +uy  *((1.f-ux)*NV(0,1,1)+ux*NV(1,1,1)));
#undef NV
}

static float fn3(float x, float y, float z, uint32_t seed, int oct, float persist) {
    float val=0.f, amp=1.f, max_amp=0.f, freq=1.f;
    for (int i=0; i<oct; i++) {
        val     += vn3(x*freq, y*freq, z*freq, seed+(uint32_t)i) * amp;
        max_amp += amp;
        amp     *= persist;
        freq    *= 2.f;
    }
    return val / max_amp;
}

/* ============================================================
   ICOSPHERE GENERATION
   ============================================================ */
static sf_fvec3_t g_iso_v[MAX_ISO_V];
static int        g_iso_vi;
static int        g_iso_f[MAX_ISO_F][3];
static int        g_iso_fi;

/* midpoint cache */
static int g_mid_a[MID_SZ], g_mid_b[MID_SZ], g_mid_val[MID_SZ];

static void mid_clear(void) { memset(g_mid_a, -1, sizeof(g_mid_a)); }

static int midpt(int a, int b) {
    if (a > b) { int t=a; a=b; b=t; }
    uint32_t slot = ((uint32_t)a*92837111u ^ (uint32_t)b*689287499u) & (MID_SZ-1);
    while (g_mid_a[slot] != -1) {
        if (g_mid_a[slot]==a && g_mid_b[slot]==b) return g_mid_val[slot];
        slot = (slot+1) & (MID_SZ-1);
    }
    if (g_iso_vi >= MAX_ISO_V) return 0;
    sf_fvec3_t m = {
        (g_iso_v[a].x+g_iso_v[b].x)*0.5f,
        (g_iso_v[a].y+g_iso_v[b].y)*0.5f,
        (g_iso_v[a].z+g_iso_v[b].z)*0.5f
    };
    float l = sqrtf(m.x*m.x + m.y*m.y + m.z*m.z);
    if (l > 0.f) { m.x/=l; m.y/=l; m.z/=l; }
    int idx = g_iso_vi;
    g_iso_v[g_iso_vi++] = m;
    g_mid_a[slot]=a; g_mid_b[slot]=b; g_mid_val[slot]=idx;
    return idx;
}

static void build_iso(int subdiv) {
    g_iso_vi = 0; g_iso_fi = 0;
    mid_clear();

    /* Icosahedron: 12 vertices on unit sphere via golden ratio */
    float t  = (1.f + sqrtf(5.f)) * 0.5f;
    float s  = 1.f / sqrtf(1.f + t*t);
    float ts = t * s;
    sf_fvec3_t iv[] = {
        {-s, ts,0},{s,ts, 0},{-s,-ts,0},{ s,-ts, 0},
        { 0,-s,ts},{0, s,ts},{ 0,-s,-ts},{ 0, s,-ts},
        {ts, 0,-s},{ts,0,  s},{-ts,0,-s},{-ts, 0,  s}
    };
    for (int i=0; i<12; i++) g_iso_v[g_iso_vi++] = iv[i];

    /* 20 initial faces (CCW winding outward) */
    static const int IF[20][3] = {
        {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
        {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
        {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
        {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}
    };
    for (int i=0; i<20; i++) {
        g_iso_f[g_iso_fi][0]=IF[i][0];
        g_iso_f[g_iso_fi][1]=IF[i][1];
        g_iso_f[g_iso_fi][2]=IF[i][2];
        g_iso_fi++;
    }

    /* Subdivide: each triangle → 4 */
    static int nf_buf[MAX_ISO_F][3];
    for (int lv=0; lv<subdiv; lv++) {
        int nf = g_iso_fi, nfi = 0;
        for (int i=0; i<nf; i++) {
            int a=g_iso_f[i][0], b=g_iso_f[i][1], c=g_iso_f[i][2];
            int ab=midpt(a,b), bc=midpt(b,c), ca=midpt(c,a);
            nf_buf[nfi][0]=a;  nf_buf[nfi][1]=ab; nf_buf[nfi][2]=ca; nfi++;
            nf_buf[nfi][0]=b;  nf_buf[nfi][1]=bc; nf_buf[nfi][2]=ab; nfi++;
            nf_buf[nfi][0]=c;  nf_buf[nfi][1]=ca; nf_buf[nfi][2]=bc; nfi++;
            nf_buf[nfi][0]=ab; nf_buf[nfi][1]=bc; nf_buf[nfi][2]=ca; nfi++;
        }
        g_iso_fi = nfi;
        memcpy(g_iso_f, nf_buf, nfi * sizeof(g_iso_f[0]));
        mid_clear();
    }
}

/* ============================================================
   ROCK GENERATION
   ============================================================ */
static void generate_rock(void) {
    if (!g_obj || !g_enti) return;
    g_obj->v_cnt = 0; g_obj->vt_cnt = 0; g_obj->f_cnt = 0; g_obj->src_path = NULL;

    int subdiv = (int)(p_subdiv + 0.5f);
    if (subdiv < 1) subdiv = 1;
    if (subdiv > 4) subdiv = 4;
    build_iso(subdiv);

    uint32_t seed = (uint32_t)(p_seed * 12345.f) + 1u;
    int oct = (int)(p_octaves + 0.5f);
    if (oct < 1) oct = 1; if (oct > 8) oct = 8;

    /* Displace each unique sphere vertex */
    static sf_fvec3_t dv[MAX_ISO_V];
    for (int i=0; i<g_iso_vi; i++) {
        sf_fvec3_t v = g_iso_v[i];  /* unit sphere position = noise input */

        /* Primary low-freq shape noise */
        float n  = fn3(v.x*p_freq,          v.y*p_freq,          v.z*p_freq,
                       seed, oct, p_persist);
        /* High-freq bump layer */
        float nb = fn3(v.x*p_freq*4.f+7.3f, v.y*p_freq*4.f+3.1f, v.z*p_freq*4.f+5.7f,
                       seed+99u, 2, 0.5f);

        float disp = (n*2.f-1.f)*p_rough + (nb*2.f-1.f)*p_rough*p_bump*0.25f;
        float r = 1.f + disp;
        v.x *= r; v.y *= r; v.z *= r;

        /* Shape modifiers (applied after displacement so noise stays isotropic) */
        v.x *= p_elongx;
        v.z *= p_elongz;
        v.y *= (1.f - p_flat * 0.8f);
        if (v.y > 0.f) v.y *= (1.f + p_pointy);

        dv[i] = v;
    }

    /* Emit per-face-unique verts into the obj (avoids UV seam from spherical mapping) */
    float tile = 1.5f;
    for (int i=0; i<g_iso_fi; i++) {
        int ai=g_iso_f[i][0], bi=g_iso_f[i][1], ci=g_iso_f[i][2];

        /* Use original sphere positions for UV: planar XZ mapping, no seam */
        sf_fvec3_t na=g_iso_v[ai], nb2=g_iso_v[bi], nc=g_iso_v[ci];
        sf_fvec2_t uva = { (na.x+1.f)*tile*0.5f, (na.z+1.f)*tile*0.5f };
        sf_fvec2_t uvb = { (nb2.x+1.f)*tile*0.5f, (nb2.z+1.f)*tile*0.5f };
        sf_fvec2_t uvc = { (nc.x+1.f)*tile*0.5f, (nc.z+1.f)*tile*0.5f };

        int bv = g_obj->v_cnt;
        int bt = g_obj->vt_cnt;
        sf_obj_add_vert(g_obj, dv[ai]);
        sf_obj_add_vert(g_obj, dv[bi]);
        sf_obj_add_vert(g_obj, dv[ci]);
        sf_obj_add_uv(g_obj, uva);
        sf_obj_add_uv(g_obj, uvb);
        sf_obj_add_uv(g_obj, uvc);
        sf_obj_add_face_uv(g_obj, bv, bv+1, bv+2, bt, bt+1, bt+2);
    }

    sf_obj_recompute_bs(g_obj);
    g_enti->obj.v_cnt     = g_obj->v_cnt;
    g_enti->obj.vt_cnt    = g_obj->vt_cnt;
    g_enti->obj.f_cnt     = g_obj->f_cnt;
    g_enti->obj.bs_center = g_obj->bs_center;
    g_enti->obj.bs_radius = g_obj->bs_radius;
}

/* ============================================================
   TEXTURE HELPERS
   ============================================================ */
static void scan_stone_textures(void) {
    const char *dirs[] = {
        SF_ASSET_PATH "/sf_textures/128x128/Stone",
        SF_ASSET_PATH "/sf_textures/128x128/Rock",
        SF_ASSET_PATH "/sf_textures/128x128/Misc",
        NULL
    };
    g_tnc = 0;
    for (int d=0; dirs[d] && g_tnc<MAX_TEX; d++) {
        DIR *dir = opendir(dirs[d]);
        if (!dir) continue;
        struct dirent *e;
        while ((e=readdir(dir)) && g_tnc<MAX_TEX) {
            if (e->d_name[0] == '.') continue;
            const char *dot = strrchr(e->d_name, '.');
            if (!dot || strcmp(dot, ".bmp") != 0) continue;
            bool dup = false;
            for (int i=0; i<g_tnc; i++)
                if (strcmp(g_tname[i], e->d_name)==0) { dup=true; break; }
            if (dup) continue;
            snprintf(g_tname[g_tnc], sizeof(g_tname[0]), "%s", e->d_name);
            g_titem[g_tnc] = g_tname[g_tnc];
            g_tnc++;
        }
        closedir(dir);
    }
}

static sf_tex_t *ensure_stone_tex(int idx) {
    if (idx < 0 || idx >= g_tnc) return NULL;
    char tname[64];
    snprintf(tname, sizeof(tname), "%s", g_tname[idx]);
    char *dot = strrchr(tname, '.'); if (dot) *dot = '\0';
    sf_tex_t *t = sf_get_texture_(&g_ctx, tname, false);
    if (!t) t = sf_load_texture_bmp(&g_ctx, g_tname[idx], tname);
    return t;
}

static void apply_stone_tex(void) {
    if (!g_enti) return;
    sf_tex_t *t = ensure_stone_tex(g_tsel);
    if (t) { g_enti->tex = t; g_enti->tex_scale = (sf_fvec2_t){1.f, 1.f}; }
}

/* Nearest-neighbour blit for texture preview */
static void blit_tex(sf_tex_t *t, int px, int py, int pw, int ph) {
    if (!t || !t->px) return;
    sf_pkd_clr_t *buf = g_ctx.main_camera.buffer;
    int bw = g_ctx.main_camera.w, bh = g_ctx.main_camera.h;
    for (int y=0; y<ph; y++) for (int x=0; x<pw; x++) {
        int bx=px+x, by=py+y;
        if (bx<0||bx>=bw||by<0||by>=bh) continue;
        buf[by*bw+bx] = t->px[(y*t->h/ph)*t->w + x*t->w/pw];
    }
}

/* ============================================================
   CAMERA
   ============================================================ */
static void update_camera(void) {
    float x = sinf(g_yaw) * cosf(g_pitch) * g_dist;
    float y = sinf(g_pitch) * g_dist;
    float z = cosf(g_yaw) * cosf(g_pitch) * g_dist;
    sf_camera_set_pos(&g_ctx, &g_ctx.main_camera, x, y, z);
    sf_camera_look_at(&g_ctx, &g_ctx.main_camera, (sf_fvec3_t){0.f, 0.f, 0.f});
}

/* ============================================================
   SAVE / INSTALL
   ============================================================ */
static bool copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb"); if (!in) return false;
    FILE *out = fopen(dst, "wb"); if (!out) { fclose(in); return false; }
    char buf[4096]; size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) fwrite(buf, 1, n, out);
    fclose(in); fclose(out);
    return true;
}

static void save_rock(void) {
    if (!g_obj) return;

    const char *sl = strrchr(g_sff_path, '/');
    char base[256]; snprintf(base, sizeof(base), "%s", sl ? sl+1 : g_sff_path);
    char *bdot = strrchr(base, '.'); if (bdot) *bdot = '\0';

    char dir[512]; snprintf(dir, sizeof(dir), "%s", g_sff_path);
    char *sl2 = strrchr(dir, '/');
    if (sl2) *sl2 = '\0'; else { dir[0]='.'; dir[1]='\0'; }

    char obj_path[512];
    snprintf(obj_path, sizeof(obj_path), "%s/%s.obj", dir, base);
    sf_obj_save_obj(&g_ctx, g_obj, obj_path);

    FILE *f = fopen(g_sff_path, "w");
    if (!f) { SF_LOG(&g_ctx, SF_LOG_ERROR, "Cannot write %s\n", g_sff_path); return; }

    fprintf(f, "# Saffron Rock Model\n\n");
    fprintf(f, "mesh %s \"%s.obj\"\n\n", g_obj->name, base);

    if (g_enti && g_enti->tex && g_enti->tex->name)
        fprintf(f, "texture %s \"%s.bmp\"\n\n", g_enti->tex->name, g_enti->tex->name);

    if (g_enti && g_enti->name) {
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
    SF_LOG(&g_ctx, SF_LOG_INFO, SF_LOG_INDENT "Saved rock: %s\n", g_sff_path);
}

static void install_rock(void) {
    save_rock();

    const char *sl = strrchr(g_sff_path, '/');
    char base[256]; snprintf(base, sizeof(base), "%s", sl ? sl+1 : g_sff_path);
    char *bdot = strrchr(base, '.'); if (bdot) *bdot = '\0';

    char dir[512]; snprintf(dir, sizeof(dir), "%s", g_sff_path);
    char *sl2 = strrchr(dir, '/');
    if (sl2) *sl2 = '\0'; else { dir[0]='.'; dir[1]='\0'; }

    char obj_src[512], sff_src[512];
    snprintf(obj_src, sizeof(obj_src), "%s/%s.obj", dir, base);
    snprintf(sff_src, sizeof(sff_src), "%s", g_sff_path);

    char obj_dst[512], sff_dst[512];
    snprintf(obj_dst, sizeof(obj_dst), SF_ASSET_PATH "/sf_objs/%s.obj", base);
    snprintf(sff_dst, sizeof(sff_dst), SF_ASSET_PATH "/sf_sff/%s.sff", base);

    bool ok1 = copy_file(obj_src, obj_dst);
    bool ok2 = copy_file(sff_src, sff_dst);
    if (ok1 && ok2)
        SF_LOG(&g_ctx, SF_LOG_INFO, SF_LOG_INDENT "Installed %s.obj + %s.sff\n", base, base);
    else
        SF_LOG(&g_ctx, SF_LOG_WARN,  SF_LOG_INDENT "Install: obj=%s sff=%s\n",
               ok1?"ok":"fail", ok2?"ok":"fail");
}

/* ============================================================
   UI CALLBACKS
   ============================================================ */
static void cb_save   (sf_ctx_t *c, void *u) { (void)c;(void)u; save_rock();    }
static void cb_install(sf_ctx_t *c, void *u) { (void)c;(void)u; install_rock(); }

static void apply_preset(const rock_preset_t *p) {
    p_subdiv=p->subdiv; p_rough=p->rough;   p_freq=p->freq;
    p_octaves=p->octaves; p_persist=p->persist;
    p_flat=p->flat;   p_elongx=p->elongx; p_elongz=p->elongz;
    p_pointy=p->pointy; p_bump=p->bump;
}
static void cb_p0(sf_ctx_t*c,void*u){(void)c;(void)u;apply_preset(&k_presets[0]);}
static void cb_p1(sf_ctx_t*c,void*u){(void)c;(void)u;apply_preset(&k_presets[1]);}
static void cb_p2(sf_ctx_t*c,void*u){(void)c;(void)u;apply_preset(&k_presets[2]);}
static void cb_p3(sf_ctx_t*c,void*u){(void)c;(void)u;apply_preset(&k_presets[3]);}
static void cb_p4(sf_ctx_t*c,void*u){(void)c;(void)u;apply_preset(&k_presets[4]);}
static void (*k_pcbs[N_PRESETS])(sf_ctx_t*,void*)={cb_p0,cb_p1,cb_p2,cb_p3,cb_p4};

/* ============================================================
   BUILD UI
   ============================================================ */
static void build_ui(void) {
    sf_ui_add_panel(&g_ctx, "Rock Generator",
        (sf_ivec2_t){0, PY0}, (sf_ivec2_t){PX1, H});

#define DF(row, lbl, tgt, step) do { \
    sf_ui_add_label(&g_ctx, lbl, (sf_ivec2_t){LX_TXT, RY(row)+2}, 0); \
    sf_ui_add_drag_float(&g_ctx, \
        (sf_ivec2_t){IX, RY(row)}, (sf_ivec2_t){IX1, RY(row)+12}, \
        &tgt, step, NULL, NULL); \
} while(0)

    DF( 0, "Seed",     p_seed,    1.00f);
    DF( 1, "Subdiv",   p_subdiv,  1.00f);
    DF( 2, "Roughness",p_rough,   0.01f);
    DF( 3, "Frequency",p_freq,    0.05f);
    DF( 4, "Octaves",  p_octaves, 1.00f);
    DF( 5, "Persist",  p_persist, 0.01f);
    DF( 6, "Flatness", p_flat,    0.01f);
    DF( 7, "Elong X",  p_elongx,  0.02f);
    DF( 8, "Elong Z",  p_elongz,  0.02f);
    DF( 9, "Pointy",   p_pointy,  0.05f);
    DF(10, "Bump",     p_bump,    0.02f);

#undef DF

    /* Texture dropdown */
    sf_ui_add_label(&g_ctx, "Stone Tex", (sf_ivec2_t){LX_TXT, RY(11)+2}, 0);
    if (g_tnc > 0)
        sf_ui_add_dropdown(&g_ctx,
            (sf_ivec2_t){LX,  RY(12)},
            (sf_ivec2_t){IX1, RY(12)+12},
            g_titem, g_tnc, &g_tsel, NULL, NULL);

    /* Save / Install */
    {
        int bw = (IX1 - LX - 2) / 2;
        sf_ui_add_button(&g_ctx, "Save",
            (sf_ivec2_t){LX,      RY(13)}, (sf_ivec2_t){LX+bw,   RY(13)+14},
            cb_save, NULL);
        sf_ui_add_button(&g_ctx, "Install",
            (sf_ivec2_t){LX+bw+2, RY(13)}, (sf_ivec2_t){IX1,      RY(13)+14},
            cb_install, NULL);
    }

    /* SFF path */
    sf_ui_add_label(&g_ctx, "SFF path", (sf_ivec2_t){LX_TXT, RY(14)+2}, 0);
    sf_ui_add_text_input(&g_ctx,
        (sf_ivec2_t){LX,  RY(15)}, (sf_ivec2_t){IX1, RY(15)+12},
        g_sff_path, (int)sizeof(g_sff_path), NULL, NULL);

    /* Presets */
    sf_ui_add_label(&g_ctx, "Presets", (sf_ivec2_t){LX_TXT, RY(16)+2}, 0);
    int pw = (IX1 - LX - 4) / 3;
    for (int i=0; i<N_PRESETS; i++) {
        int row = 17 + i/3;
        int col = i % 3;
        int bx0 = LX + col*(pw+2);
        sf_ui_add_button(&g_ctx, k_presets[i].name,
            (sf_ivec2_t){bx0,      RY(row)},
            (sf_ivec2_t){bx0+pw,   RY(row)+14},
            k_pcbs[i], NULL);
    }
}

/* ============================================================
   MAIN
   ============================================================ */
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window   *win = SDL_CreateWindow("sf_rockgen",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture  *sdl_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, W, H);

    sf_init(&g_ctx, W, H);
    sf_set_logger(&g_ctx, sf_logger_console, NULL);
    sf_camera_set_psp(&g_ctx, &g_ctx.main_camera, 60.f, 0.1f, 200.f);

    /* Shift projection so rock is centred in the open area to the right of the panel */
    {
        float aspect = (float)W / (float)H;
        g_ctx.main_camera.P = sf_make_psp_fmat4(60.f, aspect, 0.1f, 200.f);
        float open_cx = PX1 + (W - PX1) * 0.5f;
        g_ctx.main_camera.P.m[2][0] = 1.f - 2.f * open_cx / (float)W;
        g_ctx.main_camera.is_proj_dirty = false;
    }
    update_camera();

    /* Two-light setup: warm key + cool fill */
    sf_light_t *key = sf_add_light(&g_ctx, "key",
        SF_LIGHT_DIR, (sf_fvec3_t){1.0f, 0.95f, 0.85f}, 1.3f);
    if (key)  sf_frame_look_at(key->frame,  (sf_fvec3_t){ 1.f, -1.f, -0.5f});

    sf_light_t *fill = sf_add_light(&g_ctx, "fill",
        SF_LIGHT_DIR, (sf_fvec3_t){0.4f, 0.55f, 0.7f},  0.5f);
    if (fill) sf_frame_look_at(fill->frame, (sf_fvec3_t){-1.f, -0.3f,  1.f});

    scan_stone_textures();

    g_obj  = sf_obj_create_empty(&g_ctx, "rock", MAX_OBJ_V, MAX_OBJ_UV, MAX_OBJ_F);
    g_enti = sf_add_enti(&g_ctx, g_obj, "rock_enti");
    if (g_enti) sf_enti_set_pos(&g_ctx, g_enti, 0.f, 0.f, 0.f);

    build_ui();
    apply_preset(&k_presets[0]);
    generate_rock();
    apply_stone_tex();

    SDL_Event ev;
    while (sf_running(&g_ctx)) {
        sf_input_cycle_state(&g_ctx);

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { sf_stop(&g_ctx); break; }

            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT
                    && ev.button.x >= PX1) {
                g_drag=true; g_lmx=ev.button.x; g_lmy=ev.button.y;
            } else if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                g_drag=false;
            } else if (ev.type == SDL_MOUSEMOTION && g_drag) {
                g_yaw   -= (float)(ev.motion.x - g_lmx) * 0.007f;
                g_pitch += (float)(ev.motion.y - g_lmy) * 0.007f;
                if (g_pitch >  1.4f) g_pitch =  1.4f;
                if (g_pitch < -0.3f) g_pitch = -0.3f;
                g_lmx=ev.motion.x; g_lmy=ev.motion.y;
                update_camera();
            } else if (ev.type == SDL_MOUSEWHEEL) {
                g_dist -= ev.wheel.y * 0.4f;
                if (g_dist < 1.f) g_dist = 1.f;
                if (g_dist > 30.f) g_dist = 30.f;
                update_camera();
            }
            sf_sdl_process_event(&g_ctx, &ev);
        }

        if (sf_key_pressed(&g_ctx, SF_KEY_ESC)) sf_stop(&g_ctx);

        /* Regenerate when any parameter changes */
        {
            static float lp[11] = {-1e30f};
            static int   ltsel  = -1;
            float cp[11] = {p_seed, p_subdiv, p_rough, p_freq, p_octaves,
                             p_persist, p_flat, p_elongx, p_elongz, p_pointy, p_bump};
            if (memcmp(cp, lp, sizeof(cp)) != 0) {
                memcpy(lp, cp, sizeof(cp));
                generate_rock();
                apply_stone_tex();
            }
            if (g_tsel != ltsel) {
                ltsel = g_tsel;
                apply_stone_tex();
            }
        }

        sf_time_update(&g_ctx);
        sf_ui_update(&g_ctx, g_ctx.ui);
        sf_render_ctx(&g_ctx);
        sf_draw_debug_perf(&g_ctx, &g_ctx.main_camera);
        sf_ui_render(&g_ctx, &g_ctx.main_camera, g_ctx.ui);

        /* Texture preview (top-right of scene view) */
        if (g_tnc > 0) {
            sf_tex_t *t = ensure_stone_tex(g_tsel);
            if (t) blit_tex(t, TEX_PX, PREV_Y, PREV_SZ, PREV_SZ);
        }

        SDL_UpdateTexture(sdl_tex, NULL,
            g_ctx.main_camera.buffer, W * sizeof(sf_pkd_clr_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, sdl_tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    sf_destroy(&g_ctx);
    SDL_DestroyTexture(sdl_tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
