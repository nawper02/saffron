/* sf_gles.h
 * GPU backend for saffron, targeting OpenGL ES 3.0 over EGL (offscreen).
 * Companion to the CPU rasterizer in saffron.h.
 *
 * Validated on Mesa desktop and Qualcomm Adreno 650 (RB5, Freedreno).
 * Single-header, STB style.
 *
 *   #define SAFFRON_GLES_IMPLEMENTATION
 *   #include "sf_extras/sf_gles.h"
 *   #undef  SAFFRON_GLES_IMPLEMENTATION
 *
 * The CPU and GPU paths share the same sf_ctx_t.  Entities, textures,
 * cameras, lights, and frames are read directly from the context every
 * frame; the GPU backend only owns hardware resources (EGL context,
 * default shader program, render targets, VBO/VAO/texture caches keyed
 * by saffron id).
 *
 * Out of scope (deliberately): skybox, fog, wireframe, particles, UI,
 * debug overlay.  Those still live in the CPU path; this backend just
 * paints the 3D world.  See the "Extension points" section near the
 * bottom of the header for the seams a custom pipeline can hook into.
 */

#ifndef SF_GLES_H
#define SF_GLES_H

#include "saffron.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SF_GLES_DEFINES */
#define SF_GLES_MAX_LIGHTS            16
#define SF_GLES_VERT_FLOATS           8       /* pos3 + normal3 + uv2 */

/* SF_GLES_TYPES */

typedef struct {
  int32_t   obj_id;       /* saffron sf_obj_t.id */
  GLuint    vbo;
  GLuint    vao;
  int       vert_count;   /* 3 * f_cnt */
  bool      has_normals;  /* true => smooth shading from obj->vn */
} sf_gles_obj_cache_t;

typedef struct {
  int32_t   tex_id;       /* saffron sf_tex_t.id */
  GLuint    gl_tex;
} sf_gles_tex_cache_t;

typedef struct {
  GLuint    program;
  GLint     u_mvp;
  GLint     u_mv;
  GLint     u_tex;
  GLint     u_has_tex;
  GLint     u_uv_scale;
  GLint     u_light_count;
  GLint     u_light_pos      [SF_GLES_MAX_LIGHTS];
  GLint     u_light_color    [SF_GLES_MAX_LIGHTS];
  GLint     u_light_intensity[SF_GLES_MAX_LIGHTS];
  GLint     u_light_is_dir   [SF_GLES_MAX_LIGHTS];
} sf_gles_prog_t;

typedef struct {
  int       w, h;
  GLuint    fbo;
  GLuint    color_tex;
  GLuint    depth_rbo;
  bool      initialized;
} sf_gles_tgt_t;

typedef struct {
  /* EGL */
  EGLDisplay              display;
  EGLContext              context;
  EGLSurface              dummy_surface;  /* 1x1 PBuffer for eglMakeCurrent */
  EGLConfig               config;

  /* Default lit/textured shader. */
  sf_gles_prog_t          prog_default;
  sf_gles_prog_t         *prog_active;    /* NULL => prog_default */
  GLuint                  white_tex;      /* 1x1 fallback */

  /* Resource caches.  Sized to saffron's caps so ids map 1:1. */
  sf_gles_obj_cache_t     obj_cache[SF_MAX_OBJS];
  int                     obj_cache_count;
  sf_gles_tex_cache_t     tex_cache[SF_MAX_TEXTURES];
  int                     tex_cache_count;

  sf_ctx_t               *owner_ctx;      /* for SF_LOG; non-owning */
  bool                    initialized;
} sf_gles_t;

/* SF_GLES_HEADERS */

/* === Renderer lifecycle ============================================ */
bool   sf_gles_init           (sf_gles_t *gl, sf_ctx_t *ctx);
void   sf_gles_destroy        (sf_gles_t *gl);

/* === Render targets ================================================ */
bool   sf_gles_tgt_init       (sf_gles_t *gl, sf_gles_tgt_t *tgt, int w, int h);
void   sf_gles_tgt_destroy    (sf_gles_t *gl, sf_gles_tgt_t *tgt);
/* out_rgba is w*h*4 bytes, top-row-first (suitable for stb_image_write). */
void   sf_gles_tgt_readback   (sf_gles_t *gl, sf_gles_tgt_t *tgt, uint8_t *out_rgba);

/* === Rendering (mirrors saffron's CPU calls) ======================= */
void   sf_gles_clear          (sf_gles_t *gl, sf_gles_tgt_t *tgt, sf_fvec3_t color);
void   sf_gles_render_ctx     (sf_gles_t *gl, sf_ctx_t *ctx, sf_cam_t *cam, sf_gles_tgt_t *tgt);
void   sf_gles_render_enti    (sf_gles_t *gl, sf_ctx_t *ctx, sf_cam_t *cam, sf_gles_tgt_t *tgt, sf_enti_t *enti);

/* === Resource cache (lazy by default; explicit when needed) ======== */
void   sf_gles_upload_obj     (sf_gles_t *gl, sf_obj_t *obj);
void   sf_gles_upload_tex     (sf_gles_t *gl, sf_tex_t *tex);
void   sf_gles_invalidate_obj (sf_gles_t *gl, sf_obj_t *obj);
void   sf_gles_invalidate_tex (sf_gles_t *gl, sf_tex_t *tex);

/* === Extension points ============================================== */
/* Raw GL handles so a downstream user can drive their own passes
 * (skybox, lines, sprites, post-FX) against the same target / VBOs. */
GLuint sf_gles_tgt_fbo        (sf_gles_tgt_t *tgt);
GLuint sf_gles_tgt_color      (sf_gles_tgt_t *tgt);
GLuint sf_gles_obj_vao        (sf_gles_t *gl, sf_obj_t *obj);
GLuint sf_gles_obj_vbo        (sf_gles_t *gl, sf_obj_t *obj);
int    sf_gles_obj_vert_count (sf_gles_t *gl, sf_obj_t *obj);
GLuint sf_gles_tex_handle     (sf_gles_t *gl, sf_tex_t *tex);

/* Compile a custom GLSL ES 3.00 program and use it for subsequent draws.
 * Pass NULL to revert to the default lit/textured program.  The shader
 * must declare the same attribute locations as the default (see top of
 * the implementation for the spec).  prog_destroy releases GL state. */
bool   sf_gles_prog_compile   (sf_gles_prog_t *p, const char *vs_src, const char *fs_src);
void   sf_gles_prog_destroy   (sf_gles_prog_t *p);
void   sf_gles_use_prog       (sf_gles_t *gl, sf_gles_prog_t *p);

#ifdef __cplusplus
}
#endif

#endif /* SF_GLES_H */


/* ================================================================== */
/* IMPLEMENTATION                                                     */
/* ================================================================== */
#ifdef SAFFRON_GLES_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* SF_GLES_INTERNAL_LOG -- prefer ctx logger if available */
#define SF_GLES_LOG(gl, level, ...)                                     \
  do {                                                                  \
    if ((gl) && (gl)->owner_ctx)  SF_LOG((gl)->owner_ctx, level, __VA_ARGS__); \
    else                          fprintf(stderr, "[sf_gles] " __VA_ARGS__);   \
  } while (0)

/* SF_GLES_DEFAULT_SHADERS
 *
 * Attribute layout (custom programs must match):
 *   location=0  vec3 a_pos
 *   location=1  vec3 a_normal     (view-space lighting input)
 *   location=2  vec2 a_uv
 *
 * Required uniforms in custom programs are documented near the
 * use site -- the default set below is what sf_gles_render_enti
 * binds before each draw. */
static const char *SF_GLES_VS_DEFAULT =
  "#version 300 es\n"
  "layout(location=0) in vec3 a_pos;\n"
  "layout(location=1) in vec3 a_normal;\n"
  "layout(location=2) in vec2 a_uv;\n"
  "uniform mat4 u_mvp;\n"
  "uniform mat4 u_mv;\n"
  "uniform vec2 u_uv_scale;\n"
  "out vec3 v_pos_v;\n"
  "out vec3 v_normal_v;\n"
  "out vec2 v_uv;\n"
  "void main(){\n"
  "  v_pos_v    = (u_mv * vec4(a_pos, 1.0)).xyz;\n"
  /* Saffron transforms are uniform-ish; skip inverse-transpose for now. */
  "  v_normal_v = normalize((u_mv * vec4(a_normal, 0.0)).xyz);\n"
  "  v_uv       = a_uv * u_uv_scale;\n"
  "  gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
  "}\n";

static const char *SF_GLES_FS_DEFAULT =
  "#version 300 es\n"
  "precision mediump float;\n"
  "in vec3 v_pos_v;\n"
  "in vec3 v_normal_v;\n"
  "in vec2 v_uv;\n"
  "uniform sampler2D u_tex;\n"
  "uniform int       u_has_tex;\n"
  "uniform int       u_light_count;\n"
  "uniform vec3      u_light_pos      [16];\n"
  "uniform vec3      u_light_color    [16];\n"
  "uniform float     u_light_intensity[16];\n"
  "uniform int       u_light_is_dir   [16];\n"
  "out vec4 frag;\n"
  "void main(){\n"
  "  vec3 n = normalize(v_normal_v);\n"
  "  vec3 lit = vec3(0.1);\n"
  "  for (int i = 0; i < u_light_count; i++) {\n"
  "    vec3 ldir; float atten = 1.0;\n"
  "    if (u_light_is_dir[i] == 1) {\n"
  "      ldir = normalize(u_light_pos[i]);\n"
  "    } else {\n"
  "      vec3 diff = u_light_pos[i] - v_pos_v;\n"
  "      float dist = length(diff);\n"
  "      ldir  = diff / max(dist, 0.0001);\n"
  "      atten = 1.0 / (1.0 + 0.09*dist + 0.032*dist*dist);\n"
  "    }\n"
  "    float d = max(dot(n, ldir), 0.0);\n"
  "    lit += u_light_color[i] * u_light_intensity[i] * d * atten;\n"
  "  }\n"
  "  lit = clamp(lit, 0.0, 1.0);\n"
  "  vec3 base = (u_has_tex == 1) ? texture(u_tex, v_uv).rgb : vec3(0.8);\n"
  "  frag = vec4(lit * base, 1.0);\n"
  "}\n";

/* SF_GLES_PROG ---------------------------------------------------- */

static GLuint sf__gles_compile(GLenum stage, const char *src) {
  GLuint s = glCreateShader(stage);
  glShaderSource(s, 1, &src, NULL);
  glCompileShader(s);
  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[2048] = {0};
    glGetShaderInfoLog(s, sizeof log, NULL, log);
    fprintf(stderr, "[sf_gles] shader compile failed: %s\n", log);
    glDeleteShader(s);
    return 0;
  }
  return s;
}

static GLuint sf__gles_link(GLuint vs, GLuint fs) {
  GLuint p = glCreateProgram();
  glAttachShader(p, vs);
  glAttachShader(p, fs);
  glLinkProgram(p);
  GLint ok = 0;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[2048] = {0};
    glGetProgramInfoLog(p, sizeof log, NULL, log);
    fprintf(stderr, "[sf_gles] link failed: %s\n", log);
    glDeleteProgram(p);
    return 0;
  }
  return p;
}

static void sf__gles_resolve_uniforms(sf_gles_prog_t *p) {
  p->u_mvp         = glGetUniformLocation(p->program, "u_mvp");
  p->u_mv          = glGetUniformLocation(p->program, "u_mv");
  p->u_tex         = glGetUniformLocation(p->program, "u_tex");
  p->u_has_tex     = glGetUniformLocation(p->program, "u_has_tex");
  p->u_uv_scale    = glGetUniformLocation(p->program, "u_uv_scale");
  p->u_light_count = glGetUniformLocation(p->program, "u_light_count");
  for (int i = 0; i < SF_GLES_MAX_LIGHTS; i++) {
    char buf[64];
    snprintf(buf, sizeof buf, "u_light_pos[%d]", i);       p->u_light_pos[i]       = glGetUniformLocation(p->program, buf);
    snprintf(buf, sizeof buf, "u_light_color[%d]", i);     p->u_light_color[i]     = glGetUniformLocation(p->program, buf);
    snprintf(buf, sizeof buf, "u_light_intensity[%d]", i); p->u_light_intensity[i] = glGetUniformLocation(p->program, buf);
    snprintf(buf, sizeof buf, "u_light_is_dir[%d]", i);    p->u_light_is_dir[i]    = glGetUniformLocation(p->program, buf);
  }
}

bool sf_gles_prog_compile(sf_gles_prog_t *p, const char *vs_src, const char *fs_src) {
  memset(p, 0, sizeof *p);
  GLuint vs = sf__gles_compile(GL_VERTEX_SHADER,   vs_src);
  GLuint fs = sf__gles_compile(GL_FRAGMENT_SHADER, fs_src);
  if (!vs || !fs) { if (vs) glDeleteShader(vs); if (fs) glDeleteShader(fs); return false; }
  p->program = sf__gles_link(vs, fs);
  glDeleteShader(vs); glDeleteShader(fs);
  if (!p->program) return false;
  sf__gles_resolve_uniforms(p);
  return true;
}

void sf_gles_prog_destroy(sf_gles_prog_t *p) {
  if (!p || !p->program) return;
  glDeleteProgram(p->program);
  memset(p, 0, sizeof *p);
}

void sf_gles_use_prog(sf_gles_t *gl, sf_gles_prog_t *p) {
  gl->prog_active = p;  /* lazy bind: render_enti calls glUseProgram */
}

/* SF_GLES_EGL ---------------------------------------------------- */

static bool sf__gles_egl_init(sf_gles_t *gl) {
  gl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (gl->display == EGL_NO_DISPLAY) { SF_GLES_LOG(gl, SF_LOG_ERROR, "eglGetDisplay failed\n"); return false; }
  if (!eglInitialize(gl->display, NULL, NULL)) { SF_GLES_LOG(gl, SF_LOG_ERROR, "eglInitialize failed\n"); return false; }
  if (!eglBindAPI(EGL_OPENGL_ES_API)) { SF_GLES_LOG(gl, SF_LOG_ERROR, "eglBindAPI failed\n"); return false; }

  EGLint cfg_attrs[] = {
    EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE,   8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE,  8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 24,
    EGL_NONE
  };
  EGLint num_cfg = 0;
  if (!eglChooseConfig(gl->display, cfg_attrs, &gl->config, 1, &num_cfg) || num_cfg < 1) {
    SF_GLES_LOG(gl, SF_LOG_ERROR, "eglChooseConfig failed (n=%d)\n", num_cfg);
    return false;
  }
  EGLint ctx_attrs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
  gl->context = eglCreateContext(gl->display, gl->config, EGL_NO_CONTEXT, ctx_attrs);
  if (gl->context == EGL_NO_CONTEXT) { SF_GLES_LOG(gl, SF_LOG_ERROR, "eglCreateContext failed\n"); return false; }

  EGLint surf_attrs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
  gl->dummy_surface = eglCreatePbufferSurface(gl->display, gl->config, surf_attrs);
  if (gl->dummy_surface == EGL_NO_SURFACE) { SF_GLES_LOG(gl, SF_LOG_ERROR, "eglCreatePbufferSurface failed\n"); return false; }

  if (!eglMakeCurrent(gl->display, gl->dummy_surface, gl->dummy_surface, gl->context)) {
    SF_GLES_LOG(gl, SF_LOG_ERROR, "eglMakeCurrent failed\n");
    return false;
  }
  return true;
}

/* SF_GLES_RENDERER ----------------------------------------------- */

bool sf_gles_init(sf_gles_t *gl, sf_ctx_t *ctx) {
  memset(gl, 0, sizeof *gl);
  gl->owner_ctx = ctx;

  if (!sf__gles_egl_init(gl)) return false;

  if (!sf_gles_prog_compile(&gl->prog_default, SF_GLES_VS_DEFAULT, SF_GLES_FS_DEFAULT)) {
    SF_GLES_LOG(gl, SF_LOG_ERROR, "default program failed to build\n");
    return false;
  }

  /* 1x1 white fallback texture. */
  glGenTextures(1, &gl->white_tex);
  glBindTexture(GL_TEXTURE_2D, gl->white_tex);
  uint8_t white[4] = {255,255,255,255};
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  gl->initialized = true;
  SF_GLES_LOG(gl, SF_LOG_INFO,
              SF_LOG_INDENT "vendor   : %s\n"
              SF_LOG_INDENT "renderer : %s\n"
              SF_LOG_INDENT "version  : %s\n",
              (const char*)glGetString(GL_VENDOR),
              (const char*)glGetString(GL_RENDERER),
              (const char*)glGetString(GL_VERSION));
  return true;
}

void sf_gles_destroy(sf_gles_t *gl) {
  if (!gl || !gl->initialized) return;
  for (int i = 0; i < gl->obj_cache_count; i++) {
    glDeleteBuffers     (1, &gl->obj_cache[i].vbo);
    glDeleteVertexArrays(1, &gl->obj_cache[i].vao);
  }
  for (int i = 0; i < gl->tex_cache_count; i++) {
    glDeleteTextures(1, &gl->tex_cache[i].gl_tex);
  }
  glDeleteTextures(1, &gl->white_tex);
  sf_gles_prog_destroy(&gl->prog_default);

  eglMakeCurrent  (gl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface(gl->display, gl->dummy_surface);
  eglDestroyContext(gl->display, gl->context);
  eglTerminate    (gl->display);
  memset(gl, 0, sizeof *gl);
}

/* SF_GLES_TARGETS ------------------------------------------------ */

bool sf_gles_tgt_init(sf_gles_t *gl, sf_gles_tgt_t *tgt, int w, int h) {
  memset(tgt, 0, sizeof *tgt);
  tgt->w = w; tgt->h = h;

  glGenTextures    (1, &tgt->color_tex);
  glBindTexture    (GL_TEXTURE_2D, tgt->color_tex);
  glTexImage2D     (GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri  (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri  (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenRenderbuffers   (1, &tgt->depth_rbo);
  glBindRenderbuffer   (GL_RENDERBUFFER, tgt->depth_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);

  glGenFramebuffers        (1, &tgt->fbo);
  glBindFramebuffer        (GL_FRAMEBUFFER, tgt->fbo);
  glFramebufferTexture2D   (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,    tgt->color_tex, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER,  tgt->depth_rbo);

  GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (st != GL_FRAMEBUFFER_COMPLETE) {
    SF_GLES_LOG(gl, SF_LOG_ERROR, "FBO incomplete: 0x%x\n", st);
    return false;
  }
  tgt->initialized = true;
  return true;
}

void sf_gles_tgt_destroy(sf_gles_t *gl, sf_gles_tgt_t *tgt) {
  (void)gl;
  if (!tgt || !tgt->initialized) return;
  glDeleteFramebuffers (1, &tgt->fbo);
  glDeleteRenderbuffers(1, &tgt->depth_rbo);
  glDeleteTextures     (1, &tgt->color_tex);
  memset(tgt, 0, sizeof *tgt);
}

void sf_gles_tgt_readback(sf_gles_t *gl, sf_gles_tgt_t *tgt, uint8_t *out_rgba) {
  if (!tgt || !tgt->initialized || !out_rgba) return;
  glBindFramebuffer(GL_FRAMEBUFFER, tgt->fbo);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  /* GL gives bottom-up; flip in place via a single tmp row. */
  size_t row = (size_t)tgt->w * 4;
  glReadPixels(0, 0, tgt->w, tgt->h, GL_RGBA, GL_UNSIGNED_BYTE, out_rgba);
  uint8_t *tmp = (uint8_t*)malloc(row);
  if (!tmp) { SF_GLES_LOG(gl, SF_LOG_ERROR, "readback flip OOM\n"); return; }
  for (int y = 0; y < tgt->h / 2; y++) {
    uint8_t *a = out_rgba + y * row;
    uint8_t *b = out_rgba + (tgt->h - 1 - y) * row;
    memcpy(tmp, a, row);
    memcpy(a,   b, row);
    memcpy(b, tmp, row);
  }
  free(tmp);
}

GLuint sf_gles_tgt_fbo  (sf_gles_tgt_t *tgt) { return tgt ? tgt->fbo       : 0; }
GLuint sf_gles_tgt_color(sf_gles_tgt_t *tgt) { return tgt ? tgt->color_tex : 0; }

/* SF_GLES_CACHE -------------------------------------------------- */

static sf_gles_obj_cache_t *sf__gles_find_obj(sf_gles_t *gl, int32_t id) {
  for (int i = 0; i < gl->obj_cache_count; i++)
    if (gl->obj_cache[i].obj_id == id) return &gl->obj_cache[i];
  return NULL;
}
static sf_gles_tex_cache_t *sf__gles_find_tex(sf_gles_t *gl, int32_t id) {
  for (int i = 0; i < gl->tex_cache_count; i++)
    if (gl->tex_cache[i].tex_id == id) return &gl->tex_cache[i];
  return NULL;
}

static sf_gles_obj_cache_t *sf__gles_upload_obj(sf_gles_t *gl, sf_obj_t *obj) {
  if (gl->obj_cache_count >= SF_MAX_OBJS) {
    SF_GLES_LOG(gl, SF_LOG_ERROR, "obj cache full (%d)\n", SF_MAX_OBJS);
    return NULL;
  }
  int vc = obj->f_cnt * 3;
  bool smooth = (obj->vn_cnt > 0);
  float *buf = (float*)malloc((size_t)vc * SF_GLES_VERT_FLOATS * sizeof(float));
  if (!buf) return NULL;

  for (int fi = 0; fi < obj->f_cnt; fi++) {
    sf_face_t face = obj->f[fi];
    sf_fvec3_t p[3] = { obj->v[face.idx[0].v], obj->v[face.idx[1].v], obj->v[face.idx[2].v] };

    /* Face normal as fallback for flat shading. */
    sf_fvec3_t a = sf_fvec3_sub(p[1], p[0]);
    sf_fvec3_t b = sf_fvec3_sub(p[2], p[0]);
    sf_fvec3_t fn = sf_fvec3_norm(sf_fvec3_cross(a, b));

    for (int k = 0; k < 3; k++) {
      sf_fvec3_t n = fn;
      if (smooth && face.idx[k].vn >= 0 && face.idx[k].vn < obj->vn_cnt) {
        n = obj->vn[face.idx[k].vn];
      }
      float u = 0.0f, v = 0.0f;
      if (obj->vt_cnt > 0 && face.idx[k].vt >= 0) {
        u = obj->vt[face.idx[k].vt].x;
        v = obj->vt[face.idx[k].vt].y;
      }
      float *o = &buf[(fi*3 + k) * SF_GLES_VERT_FLOATS];
      o[0] = p[k].x; o[1] = p[k].y; o[2] = p[k].z;
      o[3] = n.x;    o[4] = n.y;    o[5] = n.z;
      o[6] = u;      o[7] = v;
    }
  }

  sf_gles_obj_cache_t *c = &gl->obj_cache[gl->obj_cache_count++];
  c->obj_id      = obj->id;
  c->vert_count  = vc;
  c->has_normals = smooth;

  glGenVertexArrays(1, &c->vao);
  glGenBuffers     (1, &c->vbo);

  glBindVertexArray(c->vao);
  glBindBuffer     (GL_ARRAY_BUFFER, c->vbo);
  glBufferData     (GL_ARRAY_BUFFER, (GLsizeiptr)vc * SF_GLES_VERT_FLOATS * sizeof(float), buf, GL_STATIC_DRAW);

  GLsizei stride = SF_GLES_VERT_FLOATS * sizeof(float);
  glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)(0));
  glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float)));
  glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6*sizeof(float)));
  glBindVertexArray(0);

  free(buf);
  return c;
}

static sf_gles_tex_cache_t *sf__gles_upload_tex(sf_gles_t *gl, sf_tex_t *tex) {
  if (!tex || !tex->px || tex->w <= 0 || tex->h <= 0) return NULL;
  if (gl->tex_cache_count >= SF_MAX_TEXTURES) {
    SF_GLES_LOG(gl, SF_LOG_ERROR, "tex cache full (%d)\n", SF_MAX_TEXTURES);
    return NULL;
  }
  /* saffron stores sf_pkd_clr_t = 0xAARRGGBB; GL wants RGBA byte order. */
  int n = tex->w * tex->h;
  uint8_t *rgba = (uint8_t*)malloc((size_t)n * 4);
  if (!rgba) return NULL;
  for (int i = 0; i < n; i++) {
    uint32_t c = tex->px[i];
    rgba[i*4 + 0] = (uint8_t)((c >> 16) & 0xFF);
    rgba[i*4 + 1] = (uint8_t)((c >>  8) & 0xFF);
    rgba[i*4 + 2] = (uint8_t)((c >>  0) & 0xFF);
    rgba[i*4 + 3] = (uint8_t)((c >> 24) & 0xFF);
  }

  sf_gles_tex_cache_t *c = &gl->tex_cache[gl->tex_cache_count++];
  c->tex_id = tex->id;
  glGenTextures(1, &c->gl_tex);
  glBindTexture(GL_TEXTURE_2D, c->gl_tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, tex->w, tex->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  free(rgba);
  return c;
}

void sf_gles_upload_obj(sf_gles_t *gl, sf_obj_t *obj) {
  if (!obj) return;
  if (!sf__gles_find_obj(gl, obj->id)) sf__gles_upload_obj(gl, obj);
}
void sf_gles_upload_tex(sf_gles_t *gl, sf_tex_t *tex) {
  if (!tex) return;
  if (!sf__gles_find_tex(gl, tex->id)) sf__gles_upload_tex(gl, tex);
}

void sf_gles_invalidate_obj(sf_gles_t *gl, sf_obj_t *obj) {
  if (!obj) return;
  for (int i = 0; i < gl->obj_cache_count; i++) {
    if (gl->obj_cache[i].obj_id != obj->id) continue;
    glDeleteBuffers     (1, &gl->obj_cache[i].vbo);
    glDeleteVertexArrays(1, &gl->obj_cache[i].vao);
    gl->obj_cache[i] = gl->obj_cache[--gl->obj_cache_count];
    return;
  }
}
void sf_gles_invalidate_tex(sf_gles_t *gl, sf_tex_t *tex) {
  if (!tex) return;
  for (int i = 0; i < gl->tex_cache_count; i++) {
    if (gl->tex_cache[i].tex_id != tex->id) continue;
    glDeleteTextures(1, &gl->tex_cache[i].gl_tex);
    gl->tex_cache[i] = gl->tex_cache[--gl->tex_cache_count];
    return;
  }
}

GLuint sf_gles_obj_vao(sf_gles_t *gl, sf_obj_t *obj) {
  sf_gles_obj_cache_t *c = obj ? sf__gles_find_obj(gl, obj->id) : NULL;
  return c ? c->vao : 0;
}
GLuint sf_gles_obj_vbo(sf_gles_t *gl, sf_obj_t *obj) {
  sf_gles_obj_cache_t *c = obj ? sf__gles_find_obj(gl, obj->id) : NULL;
  return c ? c->vbo : 0;
}
int sf_gles_obj_vert_count(sf_gles_t *gl, sf_obj_t *obj) {
  sf_gles_obj_cache_t *c = obj ? sf__gles_find_obj(gl, obj->id) : NULL;
  return c ? c->vert_count : 0;
}
GLuint sf_gles_tex_handle(sf_gles_t *gl, sf_tex_t *tex) {
  sf_gles_tex_cache_t *c = tex ? sf__gles_find_tex(gl, tex->id) : NULL;
  return c ? c->gl_tex : 0;
}

/* SF_GLES_RENDER ------------------------------------------------- */

static void sf__gles_sync_cam(sf_cam_t *cam) {
  /* Same logic as sf_render_cam: refresh P on dirty, V from cam->frame. */
  if (cam->is_proj_dirty || (cam->P.m[0][0] == 0.0f && cam->P.m[1][1] == 0.0f)) {
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
}

static void sf__gles_upload_lights(sf_gles_t *gl, sf_ctx_t *ctx, sf_cam_t *cam) {
  sf_gles_prog_t *p = gl->prog_active ? gl->prog_active : &gl->prog_default;
  int lc = 0;
  for (int i = 0; i < ctx->light_count && lc < SF_GLES_MAX_LIGHTS; i++) {
    sf_light_t *L = &ctx->lights[i];
    if (!L->frame) continue;
    sf_fmat4_t lM = L->frame->global_M;
    sf_fvec3_t lp_w = { lM.m[3][0], lM.m[3][1], lM.m[3][2] };
    if (L->type == SF_LIGHT_DIR) {
      sf_fvec3_t dir_w = { -lM.m[2][0], -lM.m[2][1], -lM.m[2][2] };
      sf_fvec3_t end_v = sf_fmat4_mul_vec3(cam->V, sf_fvec3_add(lp_w, dir_w));
      sf_fvec3_t pos_v = sf_fmat4_mul_vec3(cam->V, lp_w);
      sf_fvec3_t dir_v = sf_fvec3_norm(sf_fvec3_sub(end_v, pos_v));
      glUniform3f(p->u_light_pos[lc], dir_v.x, dir_v.y, dir_v.z);
      glUniform1i(p->u_light_is_dir[lc], 1);
    } else {
      sf_fvec3_t pos_v = sf_fmat4_mul_vec3(cam->V, lp_w);
      glUniform3f(p->u_light_pos[lc], pos_v.x, pos_v.y, pos_v.z);
      glUniform1i(p->u_light_is_dir[lc], 0);
    }
    glUniform3f(p->u_light_color[lc], L->color.x, L->color.y, L->color.z);
    glUniform1f(p->u_light_intensity[lc], L->intensity);
    lc++;
  }
  glUniform1i(p->u_light_count, lc);
}

void sf_gles_clear(sf_gles_t *gl, sf_gles_tgt_t *tgt, sf_fvec3_t color) {
  (void)gl;
  if (!tgt) return;
  glBindFramebuffer(GL_FRAMEBUFFER, tgt->fbo);
  glViewport(0, 0, tgt->w, tgt->h);
  glClearColor(color.x, color.y, color.z, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void sf_gles_render_enti(sf_gles_t *gl, sf_ctx_t *ctx, sf_cam_t *cam, sf_gles_tgt_t *tgt, sf_enti_t *enti) {
  (void)ctx;
  if (!gl || !enti || !enti->frame || enti->obj.f_cnt <= 0 || !tgt) return;

  sf_gles_obj_cache_t *oc = sf__gles_find_obj(gl, enti->obj.id);
  if (!oc) oc = sf__gles_upload_obj(gl, &enti->obj);
  if (!oc) return;

  sf_gles_prog_t *p = gl->prog_active ? gl->prog_active : &gl->prog_default;

  sf_fmat4_t MV  = sf_fmat4_mul_fmat4(enti->frame->global_M, cam->V);
  sf_fmat4_t MVP = sf_fmat4_mul_fmat4(MV, cam->P);
  glUniformMatrix4fv(p->u_mv,  1, GL_FALSE, &MV.m[0][0]);
  glUniformMatrix4fv(p->u_mvp, 1, GL_FALSE, &MVP.m[0][0]);
  glUniform2f       (p->u_uv_scale, enti->tex_scale.x, enti->tex_scale.y);

  glActiveTexture(GL_TEXTURE0);
  sf_gles_tex_cache_t *tc = enti->tex ? sf__gles_find_tex(gl, enti->tex->id) : NULL;
  if (!tc && enti->tex) tc = sf__gles_upload_tex(gl, enti->tex);
  if (tc) {
    glBindTexture(GL_TEXTURE_2D, tc->gl_tex);
    glUniform1i  (p->u_has_tex, 1);
  } else {
    glBindTexture(GL_TEXTURE_2D, gl->white_tex);
    glUniform1i  (p->u_has_tex, 0);
  }
  glUniform1i(p->u_tex, 0);

  glBindVertexArray(oc->vao);
  glDrawArrays(GL_TRIANGLES, 0, oc->vert_count);
}

void sf_gles_render_ctx(sf_gles_t *gl, sf_ctx_t *ctx, sf_cam_t *cam, sf_gles_tgt_t *tgt) {
  if (!gl || !ctx || !cam || !tgt) return;

  sf_update_frames(ctx);
  sf__gles_sync_cam(cam);

  glBindFramebuffer(GL_FRAMEBUFFER, tgt->fbo);
  glViewport(0, 0, tgt->w, tgt->h);
  glEnable (GL_DEPTH_TEST); glDepthFunc(GL_LESS);
  glEnable (GL_CULL_FACE);  glCullFace(GL_BACK); glFrontFace(GL_CCW);
  glClearColor(0.05f, 0.07f, 0.10f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  sf_gles_prog_t *p = gl->prog_active ? gl->prog_active : &gl->prog_default;
  glUseProgram(p->program);

  sf__gles_upload_lights(gl, ctx, cam);

  for (int i = 0; i < ctx->enti_count; i++) {
    sf_gles_render_enti(gl, ctx, cam, tgt, &ctx->entities[i]);
    ctx->_perf_tri_count += ctx->entities[i].obj.f_cnt;
  }

  glBindVertexArray(0);
}

#endif /* SAFFRON_GLES_IMPLEMENTATION */
