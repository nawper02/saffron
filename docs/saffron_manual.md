# Saffron Manual

> Auto-generated from `saffron.h` — run `python3 docs/gen_docs.py` to update.


## Table of Contents

- [Constants & Limits](#constants--limits)
- [Types & Enumerations](#types--enumerations)
- [Core](#core)
- [Memory / Arena](#memory--arena)
- [Events & Input](#events--input)
- [Scene](#scene)
- [Frames](#frames)
- [Drawing](#drawing)
- [UI](#ui)
- [Mesh Authoring](#mesh-authoring)
- [Picking / Raycasting](#picking--raycasting)
- [Logging](#logging)
- [Math](#math)

---

## Quick Reference

| Function | Section |
|----------|---------|
| `sf_init` | Core |
| `sf_destroy` | Core |
| `sf_running` | Core |
| `sf_stop` | Core |
| `sf_render_enti` | Core |
| `sf_render_ctx` | Core |
| `sf_render_cam` | Core |
| `sf_render_emitrs` | Core |
| `sf_update_emitrs` | Core |
| `sf_time_update` | Core |
| `sf_arena_init` | Memory / Arena |
| `sf_arena_alloc` | Memory / Arena |
| `sf_arena_save` | Memory / Arena |
| `sf_arena_restore` | Memory / Arena |
| `sf_event_reg` | Events & Input |
| `sf_event_trigger` | Events & Input |
| `sf_input_cycle_state` | Events & Input |
| `sf_input_set_key` | Events & Input |
| `sf_input_set_mouse_p` | Events & Input |
| `sf_input_set_mouse_b` | Events & Input |
| `sf_input_set_wheel` | Events & Input |
| `sf_input_set_text` | Events & Input |
| `sf_key_down` | Events & Input |
| `sf_key_pressed` | Events & Input |
| `sf_load_texture_bmp` | Scene |
| `sf_load_sprite` | Scene |
| `sf_add_emitr` | Scene |
| `sf_load_obj` | Scene |
| `sf_add_enti` | Scene |
| `sf_add_cam` | Scene |
| `sf_add_light` | Scene |
| `sf_enti_set_pos` | Scene |
| `sf_enti_move` | Scene |
| `sf_enti_set_rot` | Scene |
| `sf_enti_rotate` | Scene |
| `sf_enti_set_scale` | Scene |
| `sf_enti_set_tex` | Scene |
| `sf_obj_recenter` | Scene |
| `sf_camera_set_psp` | Scene |
| `sf_camera_set_pos` | Scene |
| `sf_camera_move_loc` | Scene |
| `sf_camera_look_at` | Scene |
| `sf_camera_add_yp` | Scene |
| `sf_load_sff` | Scene |
| `sf_save_sff` | Scene |
| `sf_get_root` | Frames |
| `sf_add_frame` | Frames |
| `sf_update_frames` | Frames |
| `sf_frame_look_at` | Frames |
| `sf_frame_set_parent` | Frames |
| `sf_remove_frame` | Frames |
| `sf_fill` | Drawing |
| `sf_pixel` | Drawing |
| `sf_pixel_depth` | Drawing |
| `sf_line` | Drawing |
| `sf_rect` | Drawing |
| `sf_tri` | Drawing |
| `sf_tri_tex` | Drawing |
| `sf_put_text` | Drawing |
| `sf_clear_depth` | Drawing |
| `sf_draw_cam_pip` | Drawing |
| `sf_draw_debug_ovrlay` | Drawing |
| `sf_draw_debug_axes` | Drawing |
| `sf_draw_debug_frames` | Drawing |
| `sf_draw_debug_lights` | Drawing |
| `sf_draw_debug_cams` | Drawing |
| `sf_draw_debug_perf` | Drawing |
| `sf_draw_sprite` | Drawing |
| `sf_ui_create` | UI |
| `sf_ui_update` | UI |
| `sf_ui_render` | UI |
| `sf_ui_clear` | UI |
| `sf_ui_get_by_name` | UI |
| `sf_ui_set_callback` | UI |
| `sf_ui_add_button` | UI |
| `sf_ui_add_slider` | UI |
| `sf_ui_add_checkbox` | UI |
| `sf_ui_add_label` | UI |
| `sf_ui_add_text_input` | UI |
| `sf_ui_add_drag_float` | UI |
| `sf_ui_add_dropdown` | UI |
| `sf_ui_add_panel` | UI |
| `sf_save_sfui` | UI |
| `sf_load_sfui` | UI |
| `sf_obj_create_empty` | Mesh Authoring |
| `sf_obj_add_vert` | Mesh Authoring |
| `sf_obj_add_uv` | Mesh Authoring |
| `sf_obj_add_face` | Mesh Authoring |
| `sf_obj_add_face_uv` | Mesh Authoring |
| `sf_obj_recompute_bs` | Mesh Authoring |
| `sf_obj_make_plane` | Mesh Authoring |
| `sf_obj_make_box` | Mesh Authoring |
| `sf_obj_make_sphere` | Mesh Authoring |
| `sf_obj_make_cyl` | Mesh Authoring |
| `sf_obj_make_heightmap` | Mesh Authoring |
| `sf_obj_save_obj` | Mesh Authoring |
| `sf_noise_fbm` | Mesh Authoring |
| `sf_ray_from_screen` | Picking / Raycasting |
| `sf_raycast_entities` | Picking / Raycasting |
| `sf_ray_triangle` | Picking / Raycasting |
| `sf_ray_plane_y` | Picking / Raycasting |
| `sf_ray_aabb` | Picking / Raycasting |
| `sf_set_logger` | Logging |
| `sf_logger_console` | Logging |
| `sf_fmat4_mul_fmat4` | Math |
| `sf_fmat4_mul_vec3` | Math |
| `sf_fvec3_sub` | Math |
| `sf_fvec3_add` | Math |
| `sf_fvec3_norm` | Math |
| `sf_fvec3_cross` | Math |
| `sf_fvec3_dot` | Math |
| `sf_make_tsl_fmat4` | Math |
| `sf_make_rot_fmat4` | Math |
| `sf_make_psp_fmat4` | Math |
| `sf_make_idn_fmat4` | Math |
| `sf_make_view_fmat4` | Math |
| `sf_make_scale_fmat4` | Math |

---


## Constants & Limits

### Capacity Limits

| Name | Value |
|------|-------|
| `SF_ARENA_SIZE` | `67108864` |
| `SF_MAX_OBJS` | `128` |
| `SF_MAX_ENTITIES` | `1024` |
| `SF_MAX_LIGHTS` | `32` |
| `SF_MAX_TEXTURES` | `64` |
| `SF_MAX_CAMS` | `8` |
| `SF_MAX_CB_PER_EVT` | `4` |
| `SF_MAX_UI_ELEMENTS` | `512` |
| `SF_MAX_TEXT_INPUT_LEN` | `128` |
| `SF_MAX_DROPDOWN_ITEMS` | `64` |
| `SF_MAX_FRAMES` | `512` |
| `SF_MAX_SPRITES` | `20` |
| `SF_MAX_EMITRS` | `10` |
| `SF_MAX_SPRITE_FRAMES` | `16` |
| `SF_PERF_HIST_SIZE` | `64` |
| `SF_PI` | `3.14159265359f` |
| `SF_NANOS_PER_SEC` | `1000000000ULL` |
| `SF_ASSET_PATH` | `"/usr/local/share/saffron/sf_assets"` |

### Built-in Colors

| Name | Value |
|------|-------|
| `SF_CLR_RED` | `((sf_pkd_clr_t)0xFFFF0000)` |
| `SF_CLR_GREEN` | `((sf_pkd_clr_t)0xFF00FF00)` |
| `SF_CLR_BLUE` | `((sf_pkd_clr_t)0xFF0000FF)` |
| `SF_CLR_BLACK` | `((sf_pkd_clr_t)0xFF000000)` |
| `SF_CLR_WHITE` | `((sf_pkd_clr_t)0xFFFFFFFF)` |

### Macros

| Name | Expansion |
|------|-----------|
| `SF_LOG_INDENT` | `"            "` |

## Types & Enumerations

### Primitive Typedefs

| Name | Underlying |
|------|------------|
| `sf_log_fn` | `void (*sf_log_fn )(const char* message, void* userdata)` |
| `sf_pkd_clr_t` | `uint32_t` |
| `sf_height_fn` | `float (*sf_height_fn )(float x, float z, void *ud)` |
| `sf_event_cb` | `void (*sf_event_cb )(struct sf_ctx_t_ *ctx, const sf_event_t *event, void *userdata)` |
| `sf_ui_cb` | `void (*sf_ui_cb)(struct sf_ctx_t_ *ctx, void *userdata)` |

### Enumerations

**`sf_run_state_t`** — `SF_RUN_STATE_INIT`, `SF_RUN_STATE_RUNNING`, `SF_RUN_STATE_ABORT`, `SF_RUN_STATE_STOPPED`

**`sf_log_level_t`** — `SF_LOG_DEBUG`, `SF_LOG_INFO`, `SF_LOG_WARN`, `SF_LOG_ERROR`

**`sf_convention_t`** — `SF_CONV_DEFAULT`, `SF_CONV_NED`, `SF_CONV_FLU`, `SF_CONV_MAX`

**`sf_light_type_t`** — `SF_LIGHT_DIR`, `SF_LIGHT_POINT`

**`sf_emitr_type_t`** — `SF_EMITR_DIR`, `SF_EMITR_OMNI`, `SF_EMITR_VOLUME`

**`sf_key_t`** — `SF_KEY_UNKNOWN`, `SF_KEY_A`, `SF_KEY_B`, `SF_KEY_C`, `SF_KEY_D`, `SF_KEY_E`, `SF_KEY_F`, `SF_KEY_G`, `SF_KEY_H`, `SF_KEY_I`, `SF_KEY_J`, `SF_KEY_K`, `SF_KEY_L`, `SF_KEY_M`, `SF_KEY_N`, `SF_KEY_O`, `SF_KEY_P`, `SF_KEY_Q`, `SF_KEY_R`, `SF_KEY_S`, `SF_KEY_T`, `SF_KEY_U`, `SF_KEY_V`, `SF_KEY_W`, `SF_KEY_X`, `SF_KEY_Y`, `SF_KEY_Z`, `SF_KEY_0`, `SF_KEY_1`, `SF_KEY_2`, `SF_KEY_3`, `SF_KEY_4`, `SF_KEY_5`, `SF_KEY_6`, `SF_KEY_7`, `SF_KEY_8`, `SF_KEY_9`, `SF_KEY_SPACE`, `SF_KEY_LSHIFT`, `SF_KEY_UP`, `SF_KEY_DOWN`, `SF_KEY_LEFT`, `SF_KEY_RIGHT`, `SF_KEY_BACKSPACE`, `SF_KEY_RETURN`, `SF_KEY_ESC`, `SF_KEY_TAB`, `SF_KEY_DEL`, `SF_KEY_HOME`, `SF_KEY_END`, `SF_KEY_LCTRL`, `SF_KEY_MAX`

**`sf_mouse_btn_t`** — `SF_MOUSE_LEFT`, `SF_MOUSE_RIGHT`, `SF_MOUSE_MIDDLE`, `SF_MOUSE_MAX`

**`sf_event_type_t`** — `SF_EVT_RENDER_START`, `SF_EVT_RENDER_END`, `SF_EVT_KEY_DOWN`, `SF_EVT_KEY_UP`, `SF_EVT_MOUSE_MOVE`, `SF_EVT_MOUSE_DOWN`, `SF_EVT_MOUSE_UP`, `SF_EVT_MOUSE_WHEEL`, `SF_EVT_TEXT_INPUT`, `SF_EVT_MAX`

**`sf_ui_type_t`** — `SF_UI_BUTTON`, `SF_UI_SLIDER`, `SF_UI_CHECKBOX`, `SF_UI_LABEL`, `SF_UI_TEXT_INPUT`, `SF_UI_DRAG_FLOAT`, `SF_UI_DROPDOWN`, `SF_UI_PANEL`


### Structs

**`sf_ctx_t`** — fields: 

**`sf_unpkd_clr_t`** — fields: `r`, `g`, `b`, `a`

**`sf_ivec2_t`** — fields: `x`, `y`

**`sf_fvec2_t`** — fields: `x`, `y`

**`sf_svec3_t`** — fields: `x`, `y`, `z`

**`sf_fvec3_t`** — fields: `x`, `y`, `z`

**`sf_ivec3_t`** — fields: `x`, `y`, `z`

**`sf_fmat4_t`** — fields: `m`

**`sf_ray_t`** — fields: `o`, `d`

**`sf_frame_t`** — fields: 

**`sf_cam_t`** — fields: `id`, `name`, `w`, `h`, `buffer_size`, `buffer`, `z_buffer`, `fov`, `near_plane`, `far_plane`, `is_proj_dirty`, `V`, `P`, `frame`

**`sf_tex_t`** — fields: `px`, `w`, `h`, `w_mask`, `h_mask`, `id`, `name`

**`sf_vtx_idx_t`** — fields: `v`, `vt`, `vn`

**`sf_face_t`** — fields: `idx`

**`sf_obj_t`** — fields: `v`, `vt`, `vn`, `f`, `v_cnt`, `vt_cnt`, `vn_cnt`, `f_cnt`, `id`, `name`, `bs_center`, `bs_radius`, `src_path`, `v_cap`, `vt_cap`, `f_cap`

**`sf_enti_t`** — fields: `obj`, `id`, `tex`, `tex_scale`, `name`, `frame`

**`sf_light_t`** — fields: `type`, `color`, `intensity`, `frame`, `name`, `id`

**`sf_sprite_t`** — fields: `id`, `name`, `SF_MAX_SPRITE_FRAMES`, `frame_count`, `frame_duration`, `base_scale`

**`sf_particle_t`** — fields: `pos`, `vel`, `life`, `max_life`, `anim_time`, `active`

**`sf_emitr_t`** — fields: `id`, `name`, `type`, `sprite`, `frame`, `particles`, `max_particles`, `spawn_rate`, `spawn_acc`, `particle_life`, `speed`, `dir`, `spread`, `volume_size`

**`sf_arena_t`** — fields: `size`, `offset`, `buffer`

**`mouse_move`** — fields: `type`, `key`, `x`, `y`, `dx`, `dy`, `btn`, `x`, `y`, `dy`, `text`

**`sf_callback_entry_t`** — fields: `cb`, `userdata`

**`sf_input_state_t`** — fields: `SF_KEY_MAX`, `SF_KEY_MAX`, `mouse_x`, `mouse_y`, `mouse_dx`, `mouse_dy`, `wheel_dy`, `SF_MOUSE_MAX`, `SF_MOUSE_MAX`

**`sf_ui_style_t`** — fields: `color_base`, `color_hover`, `color_active`, `color_text`, `draw_outline`

**`sf_ui_lmn_t`** — fields: 

**`sf_ui_t`** — fields: `elements`, `count`, `default_style`, `focused`, `active_panel`


## Core

### `sf_init`

```c
void sf_init (sf_ctx_t *ctx, int w, int h);
```

### `sf_destroy`

```c
void sf_destroy (sf_ctx_t *ctx);
```

### `sf_running`

```c
bool sf_running (sf_ctx_t *ctx);
```

### `sf_stop`

```c
void sf_stop (sf_ctx_t *ctx);
```

### `sf_render_enti`

```c
void sf_render_enti (sf_ctx_t *ctx, sf_cam_t *cam, sf_enti_t *enti);
```

### `sf_render_ctx`

```c
void sf_render_ctx (sf_ctx_t *ctx);
```

### `sf_render_cam`

```c
void sf_render_cam (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_render_emitrs`

```c
void sf_render_emitrs (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_update_emitrs`

```c
void sf_update_emitrs (sf_ctx_t *ctx);
```

### `sf_time_update`

```c
void sf_time_update (sf_ctx_t *ctx);
```


## Memory / Arena

### `sf_arena_init`

```c
sf_arena_t sf_arena_init (sf_ctx_t *ctx, size_t size);
```

### `sf_arena_alloc`

```c
void* sf_arena_alloc (sf_ctx_t *ctx, sf_arena_t *arena, size_t size);
```

### `sf_arena_save`

```c
size_t sf_arena_save (sf_ctx_t *ctx, sf_arena_t *arena);
```

### `sf_arena_restore`

```c
void sf_arena_restore (sf_ctx_t *ctx, sf_arena_t *arena, size_t mark);
```


## Events & Input

### `sf_event_reg`

```c
void sf_event_reg (sf_ctx_t *ctx, sf_event_type_t type, sf_event_cb cb, void *userdata);
```

### `sf_event_trigger`

```c
void sf_event_trigger (sf_ctx_t *ctx, const sf_event_t *event);
```

### `sf_input_cycle_state`

```c
void sf_input_cycle_state (sf_ctx_t *ctx);
```

### `sf_input_set_key`

```c
void sf_input_set_key (sf_ctx_t *ctx, sf_key_t key, bool is_down);
```

### `sf_input_set_mouse_p`

```c
void sf_input_set_mouse_p (sf_ctx_t *ctx, int x, int y);
```

### `sf_input_set_mouse_b`

```c
void sf_input_set_mouse_b (sf_ctx_t *ctx, sf_mouse_btn_t btn, bool is_down);
```

### `sf_input_set_wheel`

```c
void sf_input_set_wheel (sf_ctx_t *ctx, int dy);
```

### `sf_input_set_text`

```c
void sf_input_set_text (sf_ctx_t *ctx, const char *txt);
```

### `sf_key_down`

```c
bool sf_key_down (sf_ctx_t *ctx, sf_key_t key);
```

### `sf_key_pressed`

```c
bool sf_key_pressed (sf_ctx_t *ctx, sf_key_t key);
```


## Scene

### `sf_load_texture_bmp`

```c
sf_tex_t* sf_load_texture_bmp (sf_ctx_t *ctx, const char *filename, const char *texname);
```

### `sf_load_sprite`

```c
sf_sprite_t* sf_load_sprite (sf_ctx_t *ctx, const char *spritename, float duration, float scale, int frame_count, ...);
```

### `sf_add_emitr`

```c
sf_emitr_t* sf_add_emitr (sf_ctx_t *ctx, const char *emitrname, sf_emitr_type_t type, sf_sprite_t *sprite, int max_p);
```

### `sf_load_obj`

```c
sf_obj_t* sf_load_obj (sf_ctx_t *ctx, const char *filename, const char *objname);
```

### `sf_add_enti`

```c
sf_enti_t* sf_add_enti (sf_ctx_t *ctx, sf_obj_t *obj, const char *entiname);
```

### `sf_add_cam`

```c
sf_cam_t* sf_add_cam (sf_ctx_t *ctx, const char *camname, int w, int h, float fov);
```

### `sf_add_light`

```c
sf_light_t* sf_add_light (sf_ctx_t *ctx, const char *lightname, sf_light_type_t type, sf_fvec3_t color, float intensity);
```

### `sf_enti_set_pos`

```c
void sf_enti_set_pos (sf_ctx_t *ctx, sf_enti_t *enti, float x, float y, float z);
```

### `sf_enti_move`

```c
void sf_enti_move (sf_ctx_t *ctx, sf_enti_t *enti, float dx, float dy, float dz);
```

### `sf_enti_set_rot`

```c
void sf_enti_set_rot (sf_ctx_t *ctx, sf_enti_t *enti, float rx, float ry, float rz);
```

### `sf_enti_rotate`

```c
void sf_enti_rotate (sf_ctx_t *ctx, sf_enti_t *enti, float drx, float dry, float drz);
```

### `sf_enti_set_scale`

```c
void sf_enti_set_scale (sf_ctx_t *ctx, sf_enti_t *enti, float sx, float sy, float sz);
```

### `sf_enti_set_tex`

```c
void sf_enti_set_tex (sf_ctx_t *ctx, const char *entiname, const char *texname);
```

### `sf_obj_recenter`

```c
void sf_obj_recenter (sf_obj_t *obj);
```

### `sf_camera_set_psp`

```c
void sf_camera_set_psp (sf_ctx_t *ctx, sf_cam_t *cam, float fov, float near_plane, float far_plane);
```

### `sf_camera_set_pos`

```c
void sf_camera_set_pos (sf_ctx_t *ctx, sf_cam_t *cam, float x, float y, float z);
```

### `sf_camera_move_loc`

```c
void sf_camera_move_loc (sf_ctx_t *ctx, sf_cam_t *cam, float fwd, float right, float up);
```

### `sf_camera_look_at`

```c
void sf_camera_look_at (sf_ctx_t *ctx, sf_cam_t *cam, sf_fvec3_t target);
```

### `sf_camera_add_yp`

```c
void sf_camera_add_yp (sf_ctx_t *ctx, sf_cam_t *cam, float yaw_offset, float pitch_offset);
```

### `sf_load_sff`

```c
void sf_load_sff (sf_ctx_t *ctx, const char *filename, const char *worldname);
```

### `sf_save_sff`

```c
bool sf_save_sff (sf_ctx_t *ctx, const char *filepath);
```


## Frames

### `sf_get_root`

```c
sf_frame_t* sf_get_root (sf_ctx_t *ctx, sf_convention_t conv);
```

### `sf_add_frame`

```c
sf_frame_t* sf_add_frame (sf_ctx_t *ctx, sf_frame_t *parent);
```

### `sf_update_frames`

```c
void sf_update_frames (sf_ctx_t *ctx);
```

### `sf_frame_look_at`

```c
void sf_frame_look_at (sf_frame_t *f, sf_fvec3_t target);
```

### `sf_frame_set_parent`

```c
void sf_frame_set_parent (sf_frame_t *child, sf_frame_t *new_parent);
```

### `sf_remove_frame`

```c
void sf_remove_frame (sf_ctx_t *ctx, sf_frame_t *f);
```


## Drawing

### `sf_fill`

```c
void sf_fill (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c);
```

### `sf_pixel`

```c
void sf_pixel (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0);
```

### `sf_pixel_depth`

```c
void sf_pixel_depth (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, float z);
```

### `sf_line`

```c
void sf_line (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1);
```

### `sf_rect`

```c
void sf_rect (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1);
```

### `sf_tri`

```c
void sf_tri (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, bool use_depth);
```

### `sf_tri_tex`

```c
void sf_tri_tex (sf_ctx_t *ctx, sf_cam_t *cam, sf_tex_t *tex, sf_fvec3_t v0, sf_fvec3_t v1, sf_fvec3_t v2, sf_fvec3_t uvz0, sf_fvec3_t uvz1, sf_fvec3_t uvz2, sf_fvec3_t l_int);
```

### `sf_put_text`

```c
void sf_put_text (sf_ctx_t *ctx, sf_cam_t *cam, const char *text, sf_ivec2_t p, sf_pkd_clr_t c, int scale);
```

### `sf_clear_depth`

```c
void sf_clear_depth (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_draw_cam_pip`

```c
void sf_draw_cam_pip (sf_ctx_t *ctx, sf_cam_t *dest, sf_cam_t *src, sf_ivec2_t pos);
```

### `sf_draw_debug_ovrlay`

```c
void sf_draw_debug_ovrlay (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_draw_debug_axes`

```c
void sf_draw_debug_axes (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_draw_debug_frames`

```c
void sf_draw_debug_frames (sf_ctx_t *ctx, sf_cam_t *cam, float axis_size);
```

### `sf_draw_debug_lights`

```c
void sf_draw_debug_lights (sf_ctx_t *ctx, sf_cam_t *cam, float size);
```

### `sf_draw_debug_cams`

```c
void sf_draw_debug_cams (sf_ctx_t *ctx, sf_cam_t *view_cam, float ray_len);
```

### `sf_draw_debug_perf`

```c
void sf_draw_debug_perf (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_draw_sprite`

```c
void sf_draw_sprite (sf_ctx_t *ctx, sf_cam_t *cam, sf_sprite_t *spr, sf_fvec3_t pos_w, float anim_time, float scale_mult);
```


## UI

### `sf_ui_create`

```c
sf_ui_t* sf_ui_create (sf_ctx_t *ctx);
```

### `sf_ui_update`

```c
void sf_ui_update (sf_ctx_t *ctx, sf_ui_t *ui);
```

### `sf_ui_render`

```c
void sf_ui_render (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_t *ui);
```

### `sf_ui_clear`

```c
void sf_ui_clear (sf_ctx_t *ctx);
```

### `sf_ui_get_by_name`

```c
sf_ui_lmn_t* sf_ui_get_by_name (sf_ui_t *ui, const char *name);
```

### `sf_ui_set_callback`

```c
void sf_ui_set_callback (sf_ui_lmn_t *el, sf_ui_cb cb, void *userdata);
```

### `sf_ui_add_button`

```c
sf_ui_lmn_t* sf_ui_add_button (sf_ctx_t *ctx, const char *text, sf_ivec2_t v0, sf_ivec2_t v1, sf_ui_cb cb, void *userdata);
```

### `sf_ui_add_slider`

```c
sf_ui_lmn_t* sf_ui_add_slider (sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, float min_val, float max_val, float init_val, sf_ui_cb cb, void *userdata);
```

### `sf_ui_add_checkbox`

```c
sf_ui_lmn_t* sf_ui_add_checkbox (sf_ctx_t *ctx, const char *text, sf_ivec2_t v0, sf_ivec2_t v1, bool init_state, sf_ui_cb cb, void *userdata);
```

### `sf_ui_add_label`

```c
sf_ui_lmn_t* sf_ui_add_label (sf_ctx_t *ctx, const char *text, sf_ivec2_t pos, sf_pkd_clr_t color);
```

### `sf_ui_add_text_input`

```c
sf_ui_lmn_t* sf_ui_add_text_input (sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, char *buf, int buflen, sf_ui_cb cb, void *ud);
```

### `sf_ui_add_drag_float`

```c
sf_ui_lmn_t* sf_ui_add_drag_float (sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, float *target, float step, sf_ui_cb cb, void *ud);
```

### `sf_ui_add_dropdown`

```c
sf_ui_lmn_t* sf_ui_add_dropdown (sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1, const char **items, int n, int *selected, sf_ui_cb cb, void *ud);
```

### `sf_ui_add_panel`

```c
sf_ui_lmn_t* sf_ui_add_panel (sf_ctx_t *ctx, const char *title, sf_ivec2_t v0, sf_ivec2_t v1);
```

### `sf_save_sfui`

```c
bool sf_save_sfui (sf_ctx_t *ctx, sf_ui_t *ui, const char *filepath);
```

### `sf_load_sfui`

```c
sf_ui_t* sf_load_sfui (sf_ctx_t *ctx, const char *filepath);
```


## Mesh Authoring

### `sf_obj_create_empty`

```c
sf_obj_t* sf_obj_create_empty (sf_ctx_t *ctx, const char *objname, int max_v, int max_vt, int max_f);
```

### `sf_obj_add_vert`

```c
int sf_obj_add_vert (sf_obj_t *obj, sf_fvec3_t p);
```

### `sf_obj_add_uv`

```c
int sf_obj_add_uv (sf_obj_t *obj, sf_fvec2_t uv);
```

### `sf_obj_add_face`

```c
int sf_obj_add_face (sf_obj_t *obj, int i0, int i1, int i2);
```

### `sf_obj_add_face_uv`

```c
int sf_obj_add_face_uv (sf_obj_t *obj, int v0, int v1, int v2, int t0, int t1, int t2);
```

### `sf_obj_recompute_bs`

```c
void sf_obj_recompute_bs (sf_obj_t *obj);
```

### `sf_obj_make_plane`

```c
sf_obj_t* sf_obj_make_plane (sf_ctx_t *ctx, const char *objname, float sx, float sz, int res);
```

### `sf_obj_make_box`

```c
sf_obj_t* sf_obj_make_box (sf_ctx_t *ctx, const char *objname, float sx, float sy, float sz);
```

### `sf_obj_make_sphere`

```c
sf_obj_t* sf_obj_make_sphere (sf_ctx_t *ctx, const char *objname, float radius, int segs);
```

### `sf_obj_make_cyl`

```c
sf_obj_t* sf_obj_make_cyl (sf_ctx_t *ctx, const char *objname, float radius, float height, int segs);
```

### `sf_obj_make_heightmap`

```c
sf_obj_t* sf_obj_make_heightmap(sf_ctx_t *ctx, const char *objname, float size_x, float size_z, int res, sf_height_fn fn, void *ud);
```

### `sf_obj_save_obj`

```c
bool sf_obj_save_obj (sf_ctx_t *ctx, sf_obj_t *obj, const char *filepath);
```

### `sf_noise_fbm`

```c
float sf_noise_fbm (float x, float z, int oct, float lac, float gain, uint32_t seed);
```


## Picking / Raycasting

### `sf_ray_from_screen`

```c
sf_ray_t sf_ray_from_screen (sf_ctx_t *ctx, sf_cam_t *cam, int sx, int sy);
```

### `sf_raycast_entities`

```c
sf_enti_t* sf_raycast_entities (sf_ctx_t *ctx, sf_ray_t ray, float *out_t);
```

### `sf_ray_triangle`

```c
bool sf_ray_triangle (sf_ray_t r, sf_fvec3_t a, sf_fvec3_t b, sf_fvec3_t c, float *out_t);
```

### `sf_ray_plane_y`

```c
bool sf_ray_plane_y (sf_ray_t r, float y, sf_fvec3_t *out);
```

### `sf_ray_aabb`

```c
bool sf_ray_aabb (sf_ray_t r, sf_fvec3_t bmin, sf_fvec3_t bmax, float *out_t);
```


## Logging

### `sf_set_logger`

```c
void sf_set_logger (sf_ctx_t *ctx, sf_log_fn log_cb, void* userdata);
```

### `sf_logger_console`

```c
void sf_logger_console (const char* message, void* userdata);
```


## Math

### `sf_fmat4_mul_fmat4`

```c
sf_fmat4_t sf_fmat4_mul_fmat4 (sf_fmat4_t m0, sf_fmat4_t m1);
```

### `sf_fmat4_mul_vec3`

```c
sf_fvec3_t sf_fmat4_mul_vec3 (sf_fmat4_t m, sf_fvec3_t v);
```

### `sf_fvec3_sub`

```c
sf_fvec3_t sf_fvec3_sub (sf_fvec3_t v0, sf_fvec3_t v1);
```

### `sf_fvec3_add`

```c
sf_fvec3_t sf_fvec3_add (sf_fvec3_t v0, sf_fvec3_t v1);
```

### `sf_fvec3_norm`

```c
sf_fvec3_t sf_fvec3_norm (sf_fvec3_t v);
```

### `sf_fvec3_cross`

```c
sf_fvec3_t sf_fvec3_cross (sf_fvec3_t v0, sf_fvec3_t v1);
```

### `sf_fvec3_dot`

```c
float sf_fvec3_dot (sf_fvec3_t v0, sf_fvec3_t v1);
```

### `sf_make_tsl_fmat4`

```c
sf_fmat4_t sf_make_tsl_fmat4 (float x, float y, float z);
```

### `sf_make_rot_fmat4`

```c
sf_fmat4_t sf_make_rot_fmat4 (sf_fvec3_t angles);
```

### `sf_make_psp_fmat4`

```c
sf_fmat4_t sf_make_psp_fmat4 (float fov_deg, float aspect, float near, float far);
```

### `sf_make_idn_fmat4`

```c
sf_fmat4_t sf_make_idn_fmat4 (void);
```

### `sf_make_view_fmat4`

```c
sf_fmat4_t sf_make_view_fmat4 (sf_fvec3_t eye, sf_fvec3_t target, sf_fvec3_t up);
```

### `sf_make_scale_fmat4`

```c
sf_fmat4_t sf_make_scale_fmat4 (sf_fvec3_t scale);
```


---

*Generated by `docs/gen_docs.py`*
