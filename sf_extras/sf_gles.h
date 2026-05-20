/*
 * sf_gles.h  --  Hardware-accelerated 3D renderer for saffron, targeting
 *                OpenGL ES 3.0 + EGL (offscreen).  Built for the Qualcomm RB5
 *                (Adreno 650 / Freedreno), works on any Mesa desktop too.
 *
 * This is an alternative to saffron's CPU rasterizer.  It renders entities
 * registered in an sf_ctx_t through a single textured + diffuse-lit GLES
 * pipeline, into an FBO sized to the requested camera.  The result is read
 * back as an RGBA byte buffer that the caller can save (e.g. PNG) or upload
 * to a window.
 *
 *   USAGE
 *
 *     #define SAFFRON_GLES_IMPLEMENTATION
 *     #include "sf_extras/sf_gles.h"
 *     #undef  SAFFRON_GLES_IMPLEMENTATION
 *
 *     sf_gles_t g;
 *     sf_gles_init(&g, width, height);
 *     // ... populate sf_ctx_t with entities, lights, camera ...
 *     sf_gles_render_ctx(&g, &ctx, &ctx.main_camera);
 *     uint8_t *rgba = malloc(width*height*4);
 *     sf_gles_readback_rgba(&g, width, height, rgba);
 *     // hand `rgba` to stbi_write_png() or a network sender
 *     sf_gles_destroy(&g);
 *
 *   SCOPE
 *
 *     - Renders sf_enti_t entities with textured / flat diffuse shading.
 *     - Supports SF_MAX_LIGHTS_GL point + directional lights.
 *     - Skips skybox, fog, wireframe, UI, debug overlay, particles.
 *       (Those live in the CPU path; this backend just paints the world.)
 */

#ifndef SF_GLES_H
#define SF_GLES_H

#include "saffron.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <stdint.h>
#include <stdbool.h>

#define SF_GLES_MAX_OBJS     SF_MAX_OBJS
#define SF_GLES_MAX_TEXTURES SF_MAX_TEXTURES
#define SF_GLES_MAX_LIGHTS   8

typedef struct {
  int32_t  obj_id;
  GLuint   vbo;
  int      vert_count;   /* 3 * f_cnt, flat-shaded interleaved verts */
} sf_gles_obj_cache_t;

typedef struct {
  int32_t  tex_id;
  GLuint   gl_tex;
} sf_gles_tex_cache_t;

typedef struct {
  /* EGL */
  EGLDisplay           display;
  EGLContext           context;
  EGLSurface           surface;
  EGLConfig            config;

  /* FBO target */
  int                  fb_w, fb_h;
  GLuint               fbo;
  GLuint               color_tex;
  GLuint               depth_rbo;

  /* Program + uniforms */
  GLuint               program;
  GLint                u_mvp, u_mv;
  GLint                u_tex, u_has_tex;
  GLint                u_light_count;
  GLint                u_light_pos[SF_GLES_MAX_LIGHTS];
  GLint                u_light_color[SF_GLES_MAX_LIGHTS];
  GLint                u_light_intensity[SF_GLES_MAX_LIGHTS];
  GLint                u_light_is_dir[SF_GLES_MAX_LIGHTS];
  GLuint               white_tex;     /* 1x1 fallback */

  /* Caches keyed by saffron id */
  sf_gles_obj_cache_t  obj_cache[SF_GLES_MAX_OBJS];
  int                  obj_cache_count;
  sf_gles_tex_cache_t  tex_cache[SF_GLES_MAX_TEXTURES];
  int                  tex_cache_count;

  bool                 initialized;
} sf_gles_t;

bool  sf_gles_init        (sf_gles_t *g, int w, int h);
void  sf_gles_destroy     (sf_gles_t *g);
void  sf_gles_render_ctx  (sf_gles_t *g, sf_ctx_t *ctx, sf_cam_t *cam);
/* out_rgba is w*h*4 bytes, top-row-first (suitable for stbi_write_png). */
void  sf_gles_readback_rgba(sf_gles_t *g, int w, int h, uint8_t *out_rgba);

#endif /* SF_GLES_H */


/* ------------------------------------------------------------------ */
/* IMPLEMENTATION                                                     */
/* ------------------------------------------------------------------ */
#ifdef SAFFRON_GLES_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SF_GLES_LOG(...) fprintf(stderr, "[sf_gles] " __VA_ARGS__)

static const char *SF_GLES_VS =
  "#version 300 es\n"
  "layout(location=0) in vec3 a_pos;\n"
  "layout(location=1) in vec3 a_normal;\n"
  "layout(location=2) in vec2 a_uv;\n"
  "uniform mat4 u_mvp;\n"
  "uniform mat4 u_mv;\n"
  "out vec3 v_pos_view;\n"
  "out vec3 v_normal_view;\n"
  "out vec2 v_uv;\n"
  "void main(){\n"
  "  vec4 pv = u_mv * vec4(a_pos, 1.0);\n"
  "  v_pos_view = pv.xyz;\n"
  /* normals: only model+view rotation; saffron uses uniform-ish scales,
     so MV's upper-3x3 is good enough without an inverse-transpose. */
  "  v_normal_view = normalize((u_mv * vec4(a_normal, 0.0)).xyz);\n"
  "  v_uv = a_uv;\n"
  "  gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
  "}\n";

static const char *SF_GLES_FS =
  "#version 300 es\n"
  "precision mediump float;\n"
  "in vec3 v_pos_view;\n"
  "in vec3 v_normal_view;\n"
  "in vec2 v_uv;\n"
  "uniform sampler2D u_tex;\n"
  "uniform int       u_has_tex;\n"
  "uniform int       u_light_count;\n"
  "uniform vec3      u_light_pos[8];\n"
  "uniform vec3      u_light_color[8];\n"
  "uniform float     u_light_intensity[8];\n"
  "uniform int       u_light_is_dir[8];\n"
  "out vec4 frag;\n"
  "void main(){\n"
  "  vec3 n = normalize(v_normal_view);\n"
  "  vec3 lit = vec3(0.1);\n"
  "  for (int i = 0; i < u_light_count; i++) {\n"
  "    vec3 ldir; float atten = 1.0;\n"
  "    if (u_light_is_dir[i] == 1) {\n"
  "      ldir = normalize(u_light_pos[i]);\n"   /* dir stored in pos slot */
  "    } else {\n"
  "      vec3 diff = u_light_pos[i] - v_pos_view;\n"
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

static GLuint sf_gles_compile(GLenum stage, const char *src) {
  GLuint s = glCreateShader(stage);
  glShaderSource(s, 1, &src, NULL);
  glCompileShader(s);
  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[2048] = {0};
    glGetShaderInfoLog(s, sizeof log, NULL, log);
    SF_GLES_LOG("shader compile failed: %s\n", log);
    glDeleteShader(s);
    return 0;
  }
  return s;
}

static GLuint sf_gles_link(GLuint vs, GLuint fs) {
  GLuint p = glCreateProgram();
  glAttachShader(p, vs);
  glAttachShader(p, fs);
  glLinkProgram(p);
  GLint ok = 0;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[2048] = {0};
    glGetProgramInfoLog(p, sizeof log, NULL, log);
    SF_GLES_LOG("link failed: %s\n", log);
    glDeleteProgram(p);
    return 0;
  }
  return p;
}

static bool sf_gles_init_egl(sf_gles_t *g) {
  g->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (g->display == EGL_NO_DISPLAY) { SF_GLES_LOG("eglGetDisplay failed\n"); return false; }
  if (!eglInitialize(g->display, NULL, NULL)) { SF_GLES_LOG("eglInitialize failed\n"); return false; }
  if (!eglBindAPI(EGL_OPENGL_ES_API)) { SF_GLES_LOG("eglBindAPI failed\n"); return false; }

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
  if (!eglChooseConfig(g->display, cfg_attrs, &g->config, 1, &num_cfg) || num_cfg < 1) {
    SF_GLES_LOG("eglChooseConfig failed (num=%d)\n", num_cfg); return false;
  }
  EGLint ctx_attrs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
  g->context = eglCreateContext(g->display, g->config, EGL_NO_CONTEXT, ctx_attrs);
  if (g->context == EGL_NO_CONTEXT) { SF_GLES_LOG("eglCreateContext failed\n"); return false; }

  EGLint surf_attrs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
  g->surface = eglCreatePbufferSurface(g->display, g->config, surf_attrs);
  if (g->surface == EGL_NO_SURFACE) { SF_GLES_LOG("eglCreatePbufferSurface failed\n"); return false; }

  if (!eglMakeCurrent(g->display, g->surface, g->surface, g->context)) {
    SF_GLES_LOG("eglMakeCurrent failed\n"); return false;
  }
  return true;
}

static bool sf_gles_make_fbo(sf_gles_t *g, int w, int h) {
  g->fb_w = w; g->fb_h = h;

  glGenTextures(1, &g->color_tex);
  glBindTexture(GL_TEXTURE_2D, g->color_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glGenRenderbuffers(1, &g->depth_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, g->depth_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);

  glGenFramebuffers(1, &g->fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, g->fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g->color_tex, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g->depth_rbo);
  GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (st != GL_FRAMEBUFFER_COMPLETE) { SF_GLES_LOG("FBO incomplete: 0x%x\n", st); return false; }
  return true;
}

bool sf_gles_init(sf_gles_t *g, int w, int h) {
  memset(g, 0, sizeof *g);
  if (!sf_gles_init_egl(g))   return false;
  if (!sf_gles_make_fbo(g, w, h)) return false;

  GLuint vs = sf_gles_compile(GL_VERTEX_SHADER,   SF_GLES_VS);
  GLuint fs = sf_gles_compile(GL_FRAGMENT_SHADER, SF_GLES_FS);
  if (!vs || !fs) return false;
  g->program = sf_gles_link(vs, fs);
  glDeleteShader(vs); glDeleteShader(fs);
  if (!g->program) return false;

  g->u_mvp         = glGetUniformLocation(g->program, "u_mvp");
  g->u_mv          = glGetUniformLocation(g->program, "u_mv");
  g->u_tex         = glGetUniformLocation(g->program, "u_tex");
  g->u_has_tex     = glGetUniformLocation(g->program, "u_has_tex");
  g->u_light_count = glGetUniformLocation(g->program, "u_light_count");
  for (int i = 0; i < SF_GLES_MAX_LIGHTS; i++) {
    char buf[64];
    snprintf(buf, sizeof buf, "u_light_pos[%d]", i);       g->u_light_pos[i]       = glGetUniformLocation(g->program, buf);
    snprintf(buf, sizeof buf, "u_light_color[%d]", i);     g->u_light_color[i]     = glGetUniformLocation(g->program, buf);
    snprintf(buf, sizeof buf, "u_light_intensity[%d]", i); g->u_light_intensity[i] = glGetUniformLocation(g->program, buf);
    snprintf(buf, sizeof buf, "u_light_is_dir[%d]", i);    g->u_light_is_dir[i]    = glGetUniformLocation(g->program, buf);
  }

  /* 1x1 white texture for un-textured entities */
  glGenTextures(1, &g->white_tex);
  glBindTexture(GL_TEXTURE_2D, g->white_tex);
  uint8_t white[4] = {255,255,255,255};
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  g->initialized = true;
  return true;
}

void sf_gles_destroy(sf_gles_t *g) {
  if (!g || !g->initialized) return;
  for (int i = 0; i < g->obj_cache_count; i++) glDeleteBuffers(1, &g->obj_cache[i].vbo);
  for (int i = 0; i < g->tex_cache_count; i++) glDeleteTextures(1, &g->tex_cache[i].gl_tex);
  glDeleteTextures(1, &g->white_tex);
  glDeleteFramebuffers(1, &g->fbo);
  glDeleteRenderbuffers(1, &g->depth_rbo);
  glDeleteTextures(1, &g->color_tex);
  glDeleteProgram(g->program);
  eglMakeCurrent(g->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface(g->display, g->surface);
  eglDestroyContext(g->display, g->context);
  eglTerminate(g->display);
  g->initialized = false;
}

/* -- caches ------------------------------------------------------- */

static sf_gles_obj_cache_t *sf_gles_get_or_upload_obj(sf_gles_t *g, sf_obj_t *obj) {
  for (int i = 0; i < g->obj_cache_count; i++)
    if (g->obj_cache[i].obj_id == obj->id) return &g->obj_cache[i];
  if (g->obj_cache_count >= SF_GLES_MAX_OBJS) return NULL;

  /* Flatten faces -> 3*f_cnt verts, interleaved [pos.xyz, normal.xyz, uv.xy]. */
  int vc = obj->f_cnt * 3;
  float *buf = (float*)malloc((size_t)vc * 8 * sizeof(float));
  if (!buf) return NULL;
  for (int fi = 0; fi < obj->f_cnt; fi++) {
    sf_face_t face = obj->f[fi];
    sf_fvec3_t p[3] = { obj->v[face.idx[0].v], obj->v[face.idx[1].v], obj->v[face.idx[2].v] };
    /* Face normal (flat shading; matches saffron CPU lighting). */
    sf_fvec3_t a = sf_fvec3_sub(p[1], p[0]);
    sf_fvec3_t b = sf_fvec3_sub(p[2], p[0]);
    sf_fvec3_t fn = sf_fvec3_norm(sf_fvec3_cross(a, b));
    for (int k = 0; k < 3; k++) {
      float u = 0.0f, v = 0.0f;
      if (obj->vt_cnt > 0 && face.idx[k].vt >= 0) {
        u = obj->vt[face.idx[k].vt].x;
        v = obj->vt[face.idx[k].vt].y;
      }
      float *out = &buf[(fi*3 + k) * 8];
      out[0] = p[k].x; out[1] = p[k].y; out[2] = p[k].z;
      out[3] = fn.x;   out[4] = fn.y;   out[5] = fn.z;
      out[6] = u;      out[7] = v;
    }
  }

  sf_gles_obj_cache_t *c = &g->obj_cache[g->obj_cache_count++];
  c->obj_id = obj->id;
  c->vert_count = vc;
  glGenBuffers(1, &c->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, c->vbo);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vc * 8 * sizeof(float), buf, GL_STATIC_DRAW);
  free(buf);
  return c;
}

static sf_gles_tex_cache_t *sf_gles_get_or_upload_tex(sf_gles_t *g, sf_tex_t *tex) {
  if (!tex || !tex->px || tex->w <= 0 || tex->h <= 0) return NULL;
  for (int i = 0; i < g->tex_cache_count; i++)
    if (g->tex_cache[i].tex_id == tex->id) return &g->tex_cache[i];
  if (g->tex_cache_count >= SF_GLES_MAX_TEXTURES) return NULL;

  /* saffron stores sf_pkd_clr_t = 0xAARRGGBB (little-endian: B,G,R,A bytes).
     GL needs RGBA byte order, so swizzle. */
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

  sf_gles_tex_cache_t *c = &g->tex_cache[g->tex_cache_count++];
  c->tex_id = tex->id;
  glGenTextures(1, &c->gl_tex);
  glBindTexture(GL_TEXTURE_2D, c->gl_tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->w, tex->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  free(rgba);
  return c;
}

/* -- render ------------------------------------------------------- */

static sf_fmat4_t sf_gles_mat_mul(sf_fmat4_t a, sf_fmat4_t b) {
  /* column-major: result = a * b */
  return sf_fmat4_mul_fmat4(a, b);
}

void sf_gles_render_ctx(sf_gles_t *g, sf_ctx_t *ctx, sf_cam_t *cam) {
  if (!g || !g->initialized || !ctx || !cam) return;

  /* Keep saffron's frame graph + camera matrices in sync, the same way
     sf_render_cam does on the CPU path. */
  sf_update_frames(ctx);

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

  glBindFramebuffer(GL_FRAMEBUFFER, g->fbo);
  glViewport(0, 0, g->fb_w, g->fb_h);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glClearColor(0.05f, 0.07f, 0.10f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(g->program);

  /* Upload lights, transformed to view space (matches sf_render_enti). */
  int lc = ctx->light_count > SF_GLES_MAX_LIGHTS ? SF_GLES_MAX_LIGHTS : ctx->light_count;
  glUniform1i(g->u_light_count, lc);
  for (int i = 0; i < lc; i++) {
    sf_light_t *L = &ctx->lights[i];
    if (!L->frame) { glUniform1i(g->u_light_count, i); lc = i; break; }
    sf_fmat4_t lM = L->frame->global_M;
    sf_fvec3_t lp_w = { lM.m[3][0], lM.m[3][1], lM.m[3][2] };
    if (L->type == SF_LIGHT_DIR) {
      sf_fvec3_t dir_w = { -lM.m[2][0], -lM.m[2][1], -lM.m[2][2] };
      sf_fvec3_t end_v = sf_fmat4_mul_vec3(cam->V, sf_fvec3_add(lp_w, dir_w));
      sf_fvec3_t pos_v = sf_fmat4_mul_vec3(cam->V, lp_w);
      sf_fvec3_t dir_v = sf_fvec3_norm(sf_fvec3_sub(end_v, pos_v));
      /* shader expects the direction *from surface to light*; matches CPU. */
      glUniform3f(g->u_light_pos[i], dir_v.x, dir_v.y, dir_v.z);
      glUniform1i(g->u_light_is_dir[i], 1);
    } else {
      sf_fvec3_t pos_v = sf_fmat4_mul_vec3(cam->V, lp_w);
      glUniform3f(g->u_light_pos[i], pos_v.x, pos_v.y, pos_v.z);
      glUniform1i(g->u_light_is_dir[i], 0);
    }
    glUniform3f(g->u_light_color[i], L->color.x, L->color.y, L->color.z);
    glUniform1f(g->u_light_intensity[i], L->intensity);
  }

  /* Draw every entity. */
  for (int i = 0; i < ctx->enti_count; i++) {
    sf_enti_t *e = &ctx->entities[i];
    if (!e || !e->frame || e->obj.f_cnt <= 0) continue;

    sf_gles_obj_cache_t *oc = sf_gles_get_or_upload_obj(g, &e->obj);
    if (!oc) continue;

    sf_fmat4_t M  = e->frame->global_M;
    sf_fmat4_t MV = sf_gles_mat_mul(M, cam->V);
    sf_fmat4_t MVP = sf_gles_mat_mul(MV, cam->P);

    glUniformMatrix4fv(g->u_mv,  1, GL_FALSE, &MV.m[0][0]);
    glUniformMatrix4fv(g->u_mvp, 1, GL_FALSE, &MVP.m[0][0]);

    glActiveTexture(GL_TEXTURE0);
    sf_gles_tex_cache_t *tc = sf_gles_get_or_upload_tex(g, e->tex);
    if (tc) {
      glBindTexture(GL_TEXTURE_2D, tc->gl_tex);
      glUniform1i(g->u_has_tex, 1);
    } else {
      glBindTexture(GL_TEXTURE_2D, g->white_tex);
      glUniform1i(g->u_has_tex, 0);
    }
    glUniform1i(g->u_tex, 0);

    glBindBuffer(GL_ARRAY_BUFFER, oc->vbo);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(0));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glDrawArrays(GL_TRIANGLES, 0, oc->vert_count);
  }

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glFinish();
}

void sf_gles_readback_rgba(sf_gles_t *g, int w, int h, uint8_t *out_rgba) {
  if (!g || !out_rgba) return;
  if (w != g->fb_w || h != g->fb_h) {
    SF_GLES_LOG("readback size mismatch (%dx%d vs fbo %dx%d)\n", w, h, g->fb_w, g->fb_h);
    return;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, g->fbo);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  /* GL gives bottom-up. Read into a temp, then flip rows. */
  uint8_t *tmp = (uint8_t*)malloc((size_t)w * h * 4);
  if (!tmp) return;
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
  for (int y = 0; y < h; y++) {
    memcpy(&out_rgba[y * w * 4], &tmp[(h - 1 - y) * w * 4], (size_t)w * 4);
  }
  free(tmp);
}

#endif /* SAFFRON_GLES_IMPLEMENTATION */
