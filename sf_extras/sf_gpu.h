#ifndef SF_GPU_H
#define SF_GPU_H

#include "../saffron.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>

typedef struct {
  GLuint                             vao;
  GLuint                             vbo;
  int                                tri_cnt;
} sf_gpu_mesh_t;

typedef struct {
  EGLDisplay                         display;
  EGLContext                         context;
  EGLSurface                         surface;
  GLuint                             prog;
  GLuint                             fbo;
  GLuint                             color_tex;
  GLuint                             depth_rb;
  int                                fbo_w, fbo_h;
  GLuint                             white_tex;
  sf_gpu_mesh_t                      meshes[SF_MAX_OBJS];
  GLuint                             textures[SF_MAX_TEXTURES];
  GLint                              u_mv;
  GLint                              u_p;
  GLint                              u_nm;
  GLint                              u_tex;
  GLint                              u_has_tex;
  GLint                              u_light_cnt;
  GLint                              u_light_pos;
  GLint                              u_light_dir;
  GLint                              u_light_clr;
  GLint                              u_light_int;
  GLint                              u_light_type;
} sf_gpu_t;

int   sf_gpu_init        (sf_gpu_t *gpu);
void  sf_gpu_upload_obj  (sf_gpu_t *gpu, sf_obj_t *obj);
void  sf_gpu_upload_tex  (sf_gpu_t *gpu, sf_tex_t *tex);
void  sf_gpu_render_ctx  (sf_gpu_t *gpu, sf_ctx_t *ctx);
void  sf_gpu_render_cam  (sf_gpu_t *gpu, sf_ctx_t *ctx, sf_cam_t *cam);
void  sf_gpu_destroy     (sf_gpu_t *gpu);

#endif

#ifdef SAFFRON_GPU_IMPLEMENTATION

static const char *_sfg_vert_src =
  "#version 300 es\n"
  "precision highp float;\n"
  "layout(location = 0) in vec3 a_pos;\n"
  "layout(location = 1) in vec3 a_norm;\n"
  "layout(location = 2) in vec2 a_uv;\n"
  "uniform mat4 u_mv;\n"
  "uniform mat4 u_p;\n"
  "uniform mat3 u_nm;\n"
  "out vec3 v_pos;\n"
  "flat out vec3 v_norm;\n"
  "out vec2 v_uv;\n"
  "void main() {\n"
  "  vec4 vp = u_mv * vec4(a_pos, 1.0);\n"
  "  v_pos = vp.xyz;\n"
  "  v_norm = normalize(u_nm * a_norm);\n"
  "  v_uv = a_uv;\n"
  "  gl_Position = u_p * vp;\n"
  "}\n";

static const char *_sfg_frag_src =
  "#version 300 es\n"
  "precision highp float;\n"
  "in vec3 v_pos;\n"
  "flat in vec3 v_norm;\n"
  "in vec2 v_uv;\n"
  "uniform sampler2D u_tex;\n"
  "uniform int u_has_tex;\n"
  "uniform int u_light_cnt;\n"
  "uniform vec3 u_light_pos[10];\n"
  "uniform vec3 u_light_dir[10];\n"
  "uniform vec3 u_light_clr[10];\n"
  "uniform float u_light_int[10];\n"
  "uniform int u_light_type[10];\n"
  "out vec4 frag_color;\n"
  "void main() {\n"
  "  vec4 tc = (u_has_tex != 0) ? texture(u_tex, v_uv) : vec4(1.0);\n"
  "  if (tc.a < 0.01) discard;\n"
  "  vec3 N = normalize(v_norm);\n"
  "  vec3 light = vec3(0.1);\n"
  "  for (int i = 0; i < u_light_cnt; i++) {\n"
  "    vec3 L;\n"
  "    float atten = 1.0;\n"
  "    if (u_light_type[i] == 0) {\n"
  "      L = u_light_dir[i];\n"
  "    } else {\n"
  "      vec3 d = u_light_pos[i] - v_pos;\n"
  "      float ds = dot(d, d);\n"
  "      float dl = sqrt(ds);\n"
  "      L = d / dl;\n"
  "      atten = 1.0 / (1.0 + 0.09 * dl + 0.032 * ds);\n"
  "    }\n"
  "    float NdL = max(dot(N, L), 0.0);\n"
  "    light += u_light_clr[i] * u_light_int[i] * NdL * atten;\n"
  "  }\n"
  "  light = clamp(light, 0.0, 1.0);\n"
  "  vec3 c = tc.rgb * light;\n"
  "  c = pow(c, vec3(1.0 / 2.2));\n"
  "  frag_color = vec4(c.b, c.g, c.r, 1.0);\n"
  "}\n";

static GLuint _sfg_compile(GLenum type, const char *src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, NULL);
  glCompileShader(s);
  GLint ok;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(s, 512, NULL, log);
    fprintf(stderr, "sf_gpu: shader error: %s\n", log);
  }
  return s;
}

static void _sfg_make_fbo(sf_gpu_t *gpu, int w, int h) {
  if (gpu->fbo && gpu->fbo_w == w && gpu->fbo_h == h) return;

  if (gpu->fbo) {
    glDeleteFramebuffers(1, &gpu->fbo);
    glDeleteTextures(1, &gpu->color_tex);
    glDeleteRenderbuffers(1, &gpu->depth_rb);
  }

  glGenTextures(1, &gpu->color_tex);
  glBindTexture(GL_TEXTURE_2D, gpu->color_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glGenRenderbuffers(1, &gpu->depth_rb);
  glBindRenderbuffer(GL_RENDERBUFFER, gpu->depth_rb);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);

  glGenFramebuffers(1, &gpu->fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, gpu->fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gpu->color_tex, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gpu->depth_rb);

  gpu->fbo_w = w;
  gpu->fbo_h = h;
}

int sf_gpu_init(sf_gpu_t *gpu) {
  memset(gpu, 0, sizeof(sf_gpu_t));

  gpu->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (gpu->display == EGL_NO_DISPLAY) return -1;
  if (!eglInitialize(gpu->display, NULL, NULL)) return -1;

  EGLint cfg_attr[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 24,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
    EGL_NONE
  };
  EGLConfig config;
  EGLint num;
  if (!eglChooseConfig(gpu->display, cfg_attr, &config, 1, &num) || num == 0) return -1;

  EGLint ctx_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
  gpu->context = eglCreateContext(gpu->display, config, EGL_NO_CONTEXT, ctx_attr);
  if (gpu->context == EGL_NO_CONTEXT) return -1;

  EGLint pb_attr[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
  gpu->surface = eglCreatePbufferSurface(gpu->display, config, pb_attr);
  eglMakeCurrent(gpu->display, gpu->surface, gpu->surface, gpu->context);

  GLuint vs = _sfg_compile(GL_VERTEX_SHADER, _sfg_vert_src);
  GLuint fs = _sfg_compile(GL_FRAGMENT_SHADER, _sfg_frag_src);
  gpu->prog = glCreateProgram();
  glAttachShader(gpu->prog, vs);
  glAttachShader(gpu->prog, fs);
  glLinkProgram(gpu->prog);
  glDeleteShader(vs);
  glDeleteShader(fs);

  GLint ok;
  glGetProgramiv(gpu->prog, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetProgramInfoLog(gpu->prog, 512, NULL, log);
    fprintf(stderr, "sf_gpu: link error: %s\n", log);
    return -1;
  }

  gpu->u_mv         = glGetUniformLocation(gpu->prog, "u_mv");
  gpu->u_p          = glGetUniformLocation(gpu->prog, "u_p");
  gpu->u_nm         = glGetUniformLocation(gpu->prog, "u_nm");
  gpu->u_tex        = glGetUniformLocation(gpu->prog, "u_tex");
  gpu->u_has_tex    = glGetUniformLocation(gpu->prog, "u_has_tex");
  gpu->u_light_cnt  = glGetUniformLocation(gpu->prog, "u_light_cnt");
  gpu->u_light_pos  = glGetUniformLocation(gpu->prog, "u_light_pos");
  gpu->u_light_dir  = glGetUniformLocation(gpu->prog, "u_light_dir");
  gpu->u_light_clr  = glGetUniformLocation(gpu->prog, "u_light_clr");
  gpu->u_light_int  = glGetUniformLocation(gpu->prog, "u_light_int");
  gpu->u_light_type = glGetUniformLocation(gpu->prog, "u_light_type");

  uint8_t white[] = {255, 255, 255, 255};
  glGenTextures(1, &gpu->white_tex);
  glBindTexture(GL_TEXTURE_2D, gpu->white_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  return 0;
}

void sf_gpu_upload_obj(sf_gpu_t *gpu, sf_obj_t *obj) {
  sf_gpu_mesh_t *m = &gpu->meshes[obj->id];
  m->tri_cnt = obj->f_cnt;
  int vert_cnt = obj->f_cnt * 3;

  float *buf = (float*)malloc(vert_cnt * 8 * sizeof(float));

  for (int i = 0; i < obj->f_cnt; i++) {
    sf_face_t *face = &obj->f[i];
    sf_fvec3_t v0 = obj->v[face->idx[0].v];
    sf_fvec3_t v1 = obj->v[face->idx[1].v];
    sf_fvec3_t v2 = obj->v[face->idx[2].v];

    sf_fvec3_t e1 = { v1.x - v0.x, v1.y - v0.y, v1.z - v0.z };
    sf_fvec3_t e2 = { v2.x - v0.x, v2.y - v0.y, v2.z - v0.z };
    sf_fvec3_t fn = {
      e1.y * e2.z - e1.z * e2.y,
      e1.z * e2.x - e1.x * e2.z,
      e1.x * e2.y - e1.y * e2.x
    };
    float len = sqrtf(fn.x*fn.x + fn.y*fn.y + fn.z*fn.z);
    if (len > 0.0f) { float il = 1.0f / len; fn.x *= il; fn.y *= il; fn.z *= il; }

    for (int j = 0; j < 3; j++) {
      int vi = face->idx[j].v;
      int ti = face->idx[j].vt;
      int base = (i * 3 + j) * 8;
      buf[base + 0] = obj->v[vi].x;
      buf[base + 1] = obj->v[vi].y;
      buf[base + 2] = obj->v[vi].z;
      buf[base + 3] = fn.x;
      buf[base + 4] = fn.y;
      buf[base + 5] = fn.z;
      buf[base + 6] = (ti >= 0 && ti < obj->vt_cnt) ? obj->vt[ti].x : 0.0f;
      buf[base + 7] = (ti >= 0 && ti < obj->vt_cnt) ? obj->vt[ti].y : 0.0f;
    }
  }

  glGenVertexArrays(1, &m->vao);
  glGenBuffers(1, &m->vbo);
  glBindVertexArray(m->vao);
  glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
  glBufferData(GL_ARRAY_BUFFER, vert_cnt * 8 * sizeof(float), buf, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 32, (void*)12);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 32, (void*)24);
  glEnableVertexAttribArray(2);

  glBindVertexArray(0);
  free(buf);
}

void sf_gpu_upload_tex(sf_gpu_t *gpu, sf_tex_t *tex) {
  GLuint t;
  glGenTextures(1, &t);
  glBindTexture(GL_TEXTURE_2D, t);

  uint8_t *rgba = (uint8_t*)malloc(tex->w * tex->h * 4);
  for (int i = 0; i < tex->w * tex->h; i++) {
    sf_pkd_clr_t p = tex->px[i];
    rgba[i*4 + 0] = (p >> 16) & 0xFF;
    rgba[i*4 + 1] = (p >> 8)  & 0xFF;
    rgba[i*4 + 2] = (p)       & 0xFF;
    rgba[i*4 + 3] = (p >> 24) & 0xFF;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->w, tex->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  gpu->textures[tex->id] = t;
  free(rgba);
}

static void _sfg_transpose_mat4(const sf_fmat4_t *src, float *dst) {
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++)
      dst[c * 4 + r] = src->m[r][c];
}

static void _sfg_normal_mat(const sf_fmat4_t *mv, float *nm) {
  float a00 = mv->m[0][0], a01 = mv->m[0][1], a02 = mv->m[0][2];
  float a10 = mv->m[1][0], a11 = mv->m[1][1], a12 = mv->m[1][2];
  float a20 = mv->m[2][0], a21 = mv->m[2][1], a22 = mv->m[2][2];
  float det = a00*(a11*a22 - a12*a21) - a01*(a10*a22 - a12*a20) + a02*(a10*a21 - a11*a20);
  float id = (det != 0.0f) ? 1.0f / det : 0.0f;
  nm[0] = (a11*a22 - a12*a21) * id;
  nm[1] = (a12*a20 - a10*a22) * id;
  nm[2] = (a10*a21 - a11*a20) * id;
  nm[3] = (a02*a21 - a01*a22) * id;
  nm[4] = (a00*a22 - a02*a20) * id;
  nm[5] = (a01*a20 - a00*a21) * id;
  nm[6] = (a01*a12 - a02*a11) * id;
  nm[7] = (a02*a10 - a00*a12) * id;
  nm[8] = (a00*a11 - a01*a10) * id;
}

void sf_gpu_render_cam(sf_gpu_t *gpu, sf_ctx_t *ctx, sf_cam_t *cam) {
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

  _sfg_make_fbo(gpu, cam->w, cam->h);
  glBindFramebuffer(GL_FRAMEBUFFER, gpu->fbo);
  glViewport(0, 0, cam->w, cam->h);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(gpu->prog);

  float p_gl[16];
  _sfg_transpose_mat4(&cam->P, p_gl);
  glUniformMatrix4fv(gpu->u_p, 1, GL_FALSE, p_gl);

  float light_pos[30], light_dir[30], light_clr[30];
  float light_int[10];
  int light_type[10];
  int lv_cnt = 0;
  for (int l = 0; l < ctx->light_count && l < SF_MAX_LIGHTS; l++) {
    sf_light_t *light = &ctx->lights[l];
    if (!light->frame) continue;
    sf_fmat4_t lM = light->frame->global_M;
    sf_fvec3_t lp_w = {lM.m[3][0], lM.m[3][1], lM.m[3][2]};
    sf_fvec3_t lp_v = sf_fmat4_mul_vec3(cam->V, lp_w);
    light_pos[lv_cnt*3+0] = lp_v.x;
    light_pos[lv_cnt*3+1] = lp_v.y;
    light_pos[lv_cnt*3+2] = lp_v.z;
    light_type[lv_cnt] = (light->type == SF_LIGHT_DIR) ? 0 : 1;
    light_int[lv_cnt] = light->intensity;
    light_clr[lv_cnt*3+0] = light->color.x;
    light_clr[lv_cnt*3+1] = light->color.y;
    light_clr[lv_cnt*3+2] = light->color.z;
    if (light->type == SF_LIGHT_DIR) {
      sf_fvec3_t dir_w = {-lM.m[2][0], -lM.m[2][1], -lM.m[2][2]};
      sf_fvec3_t end_v = sf_fmat4_mul_vec3(cam->V, sf_fvec3_add(lp_w, dir_w));
      sf_fvec3_t dv = sf_fvec3_norm(sf_fvec3_sub(end_v, lp_v));
      light_dir[lv_cnt*3+0] = dv.x;
      light_dir[lv_cnt*3+1] = dv.y;
      light_dir[lv_cnt*3+2] = dv.z;
    }
    lv_cnt++;
  }
  glUniform1i(gpu->u_light_cnt, lv_cnt);
  if (lv_cnt > 0) {
    glUniform3fv(gpu->u_light_pos, lv_cnt, light_pos);
    glUniform3fv(gpu->u_light_dir, lv_cnt, light_dir);
    glUniform3fv(gpu->u_light_clr, lv_cnt, light_clr);
    glUniform1fv(gpu->u_light_int, lv_cnt, light_int);
    glUniform1iv(gpu->u_light_type, lv_cnt, light_type);
  }

  for (int i = 0; i < ctx->enti_count; i++) {
    sf_enti_t *enti = &ctx->entities[i];
    if (!enti || !enti->frame) continue;
    sf_gpu_mesh_t *mesh = &gpu->meshes[enti->obj.id];
    if (!mesh->vao) continue;

    sf_fmat4_t M = enti->frame->global_M;
    sf_fmat4_t MV = sf_fmat4_mul_fmat4(M, cam->V);

    float mv_gl[16];
    _sfg_transpose_mat4(&MV, mv_gl);
    glUniformMatrix4fv(gpu->u_mv, 1, GL_FALSE, mv_gl);

    float nm[9];
    _sfg_normal_mat(&MV, nm);
    glUniformMatrix3fv(gpu->u_nm, 1, GL_FALSE, nm);

    if (enti->tex && gpu->textures[enti->tex->id]) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, gpu->textures[enti->tex->id]);
      glUniform1i(gpu->u_tex, 0);
      glUniform1i(gpu->u_has_tex, 1);
    } else {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, gpu->white_tex);
      glUniform1i(gpu->u_tex, 0);
      glUniform1i(gpu->u_has_tex, 0);
    }

    glBindVertexArray(mesh->vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh->tri_cnt * 3);
  }

  glReadPixels(0, 0, cam->w, cam->h, GL_RGBA, GL_UNSIGNED_BYTE, cam->buffer);

  int stride = cam->w;
  uint32_t *buf = (uint32_t*)cam->buffer;
  for (int y = 0; y < cam->h / 2; y++) {
    int opp = cam->h - 1 - y;
    for (int x = 0; x < cam->w; x++) {
      uint32_t tmp = buf[y * stride + x];
      buf[y * stride + x] = buf[opp * stride + x];
      buf[opp * stride + x] = tmp;
    }
  }

  sf_render_emitrs(ctx, cam);

  sf_event_t ev_end;
  ev_end.type = SF_EVT_RENDER_END;
  sf_event_trigger(ctx, &ev_end);
}

void sf_gpu_render_ctx(sf_gpu_t *gpu, sf_ctx_t *ctx) {
  sf_update_frames(ctx);
  sf_update_emitrs(ctx);
  sf_gpu_render_cam(gpu, ctx, &ctx->camera);
  for (int i = 0; i < ctx->cam_count; ++i) {
    sf_gpu_render_cam(gpu, ctx, &ctx->cameras[i]);
  }
}

void sf_gpu_destroy(sf_gpu_t *gpu) {
  for (int i = 0; i < SF_MAX_OBJS; i++) {
    if (gpu->meshes[i].vao) {
      glDeleteVertexArrays(1, &gpu->meshes[i].vao);
      glDeleteBuffers(1, &gpu->meshes[i].vbo);
    }
  }
  for (int i = 0; i < SF_MAX_TEXTURES; i++) {
    if (gpu->textures[i]) glDeleteTextures(1, &gpu->textures[i]);
  }
  if (gpu->white_tex) glDeleteTextures(1, &gpu->white_tex);
  if (gpu->fbo) {
    glDeleteFramebuffers(1, &gpu->fbo);
    glDeleteTextures(1, &gpu->color_tex);
    glDeleteRenderbuffers(1, &gpu->depth_rb);
  }
  if (gpu->prog) glDeleteProgram(gpu->prog);
  if (gpu->display != EGL_NO_DISPLAY) {
    eglMakeCurrent(gpu->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (gpu->surface) eglDestroySurface(gpu->display, gpu->surface);
    if (gpu->context) eglDestroyContext(gpu->display, gpu->context);
    eglTerminate(gpu->display);
  }
}

#endif
