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
| `sf_render_skybox` | Core |
| `sf_render_fog` | Core |
| `sf_render_depth` | Core |
| `sf_update_emitrs` | Core |
| `sf_time_update` | Core |
| `sf_arena_init` | Memory / Arena |
| `sf_arena_alloc` | Memory / Arena |
| `sf_arena_save` | Memory / Arena |
| `sf_arena_restore` | Memory / Arena |
| `_sf_obj_memusg` | Memory / Arena |
| `_sf_arena_strdup` | Memory / Arena |
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
| `sf_get_texture_` | Scene |
| `sf_load_sprite` | Scene |
| `sf_get_sprite_` | Scene |
| `sf_add_emitr` | Scene |
| `sf_get_emitr_` | Scene |
| `sf_load_obj` | Scene |
| `sf_get_obj_` | Scene |
| `sf_add_enti` | Scene |
| `sf_get_enti_` | Scene |
| `sf_add_cam` | Scene |
| `sf_get_cam_` | Scene |
| `sf_add_light` | Scene |
| `sf_get_light_` | Scene |
| `sf_set_fog` | Scene |
| `sf_load_skybox` | Scene |
| `sf_get_skybox_` | Scene |
| `sf_set_active_skybox` | Scene |
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
| `_sf_sff_read_kv` | Scene |
| `_sf_sff_trim` | Scene |
| `_sf_sff_prse_vec3` | Scene |
| `_sf_sff_prse_list` | Scene |
| `_sf_sff_get_frame_` | Scene |
| `_sf_sff_prse_frame` | Scene |
| `_sf_sff_prse_cam` | Scene |
| `_sf_sff_prse_enti` | Scene |
| `_sf_sff_prse_light` | Scene |
| `_sf_sff_prse_sprit` | Scene |
| `_sf_sff_prse_emitr` | Scene |
| `sf_get_root` | Frames |
| `sf_add_frame` | Frames |
| `sf_update_frames` | Frames |
| `sf_frame_look_at` | Frames |
| `sf_frame_set_parent` | Frames |
| `sf_remove_frame` | Frames |
| `_sf_set_up_frames` | Frames |
| `_sf_calc_frame_tree` | Frames |
| `_sf_write_frame_ref` | Frames |
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
| `sf_draw_bill` | Drawing |
| `sf_add_bill` | Drawing |
| `sf_clear_bills` | Drawing |
| `_sf_sff_prse_bill` | Drawing |
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
| `_sf_ui_find_prnt_pnl` | UI |
| `_sf_ui_eff_visible` | UI |
| `_sf_draw_button` | UI |
| `_sf_draw_slider` | UI |
| `_sf_draw_checkbox` | UI |
| `_sf_draw_label` | UI |
| `_sf_draw_text_input` | UI |
| `_sf_draw_drag_float` | UI |
| `_sf_draw_dropdown` | UI |
| `_sf_draw_drpdwn_popup` | UI |
| `_sf_draw_panel` | UI |
| `_sf_update_text_input` | UI |
| `_sf_update_drag_float` | UI |
| `_sf_update_dropdown` | UI |
| `_sf_update_panel` | UI |
| `_sf_update_button` | UI |
| `_sf_update_checkbox` | UI |
| `_sf_update_slider` | UI |
| `_sf_ui_type_str` | UI |
| `_sf_ui_type_from_str` | UI |
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
| `sf_log_` | Logging |
| `sf_set_logger` | Logging |
| `sf_logger_console` | Logging |
| `_sf_log_lvl_to_str` | Logging |
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
| `_sf_vec_to_index` | Math |
| `_sf_swap_svec2` | Math |
| `_sf_swap_fvec3` | Math |
| `_sf_lerp_f` | Math |
| `_sf_lerp_fvec3` | Math |
| `_sf_intersect_near` | Math |
| `_sf_project_vertex` | Math |
| `_sf_hash_2d` | Math |
| `_sf_smooth_noise` | Math |

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
| `SF_MAX_SKYBOXES` | `4` |
| `SF_SKYBOX_SPAN` | `128` |
| `SF_MAX_SPRITE_FRAMES` | `16` |
| `SF_MAX_BILLS` | `8192` |
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

**`sf_render_mode_t`** — `SF_RENDER_NORMAL`, `SF_RENDER_WIREFRAME`, `SF_RENDER_DEPTH`, `SF_RENDER_MODE_COUNT`


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

**`sf_sprite_t`** — fields: `id`, `name`, `SF_MAX_SPRITE_FRAMES`, `frame_count`, `frame_duration`, `base_scale`, `opacity`, `opacity`

**`sf_bill_t`** — fields: `name`, `sprite`, `pos`, `scale`, `opacity`, `angle`, `rotation`, `normal`, `billboard`, `quad`

**`sf_skybox_t`** — fields: `id`, `name`, `tex`

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

Initialize the engine context: allocate arena, all scene arrays, main camera pixel/z buffers, and default UI.

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

Rasterize one entity into cam: frustum-cull, light, near-clip, then draw textured or flat triangles.

```c
void sf_render_enti (sf_ctx_t *ctx, sf_cam_t *cam, sf_enti_t *enti);
```

### `sf_render_ctx`

```c
void sf_render_ctx (sf_ctx_t *ctx);
```

### `sf_render_cam`

Clear and render a single camera: fire RENDER_START/END events, rebuild projection if dirty, draw all entities.

```c
void sf_render_cam (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_render_emitrs`

```c
void sf_render_emitrs (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_render_skybox`

Fill the camera buffer with an equirectangular sky panorama using span interpolation.
Exact UVs are computed every SF_SKYBOX_SPAN pixels and linearly interpolated between.

```c
void sf_render_skybox (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_render_fog`

Draw all active particles from every emitter as sprites into cam.

```c
void sf_render_fog (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_render_depth`

```c
void sf_render_depth (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_update_emitrs`

Advance particle lifetimes, move them by velocity, and spawn new particles according to rate.

```c
void sf_update_emitrs (sf_ctx_t *ctx);
```

### `sf_time_update`

Sample the high-resolution clock to compute delta_time, elapsed_time, smoothed FPS, and frame count.

```c
void sf_time_update (sf_ctx_t *ctx);
```


## Memory / Arena

### `sf_arena_init`

Allocate a new arena of the given byte size; all subsequent allocs bump a single pointer.

```c
sf_arena_t sf_arena_init (sf_ctx_t *ctx, size_t size);
```

### `sf_arena_alloc`

Add a clickable button with a label and optional click callback.

```c
void* sf_arena_alloc (sf_ctx_t *ctx, sf_arena_t *arena, size_t size);
```

### `sf_arena_save`

```c
size_t sf_arena_save (sf_ctx_t *ctx, sf_arena_t *arena);
```

### `sf_arena_restore`

Rewind arena to a previously saved mark, freeing everything allocated since.

```c
void sf_arena_restore (sf_ctx_t *ctx, sf_arena_t *arena, size_t mark);
```

### `_sf_obj_memusg`

Return the total arena bytes consumed by an obj's vertex, UV, normal, and face arrays.

```c
size_t _sf_obj_memusg (sf_obj_t *obj);
```

### `_sf_arena_strdup`

```c
char* _sf_arena_strdup (sf_ctx_t *ctx, const char *s);
```


## Events & Input

### `sf_event_reg`

```c
void sf_event_reg (sf_ctx_t *ctx, sf_event_type_t type, sf_event_cb cb, void *userdata);
```

### `sf_event_trigger`

Return true while the key is held down this frame.

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

Return true only on the frame the key transitions from up to down.

```c
bool sf_key_pressed (sf_ctx_t *ctx, sf_key_t key);
```


## Scene

### `sf_load_texture_bmp`

Load a 24-bit BMP file into the texture pool, applying gamma correction and treating magenta as transparent.

```c
sf_tex_t* sf_load_texture_bmp (sf_ctx_t *ctx, const char *filename, const char *texname);
```

### `sf_get_texture_`

```c
sf_tex_t* sf_get_texture_ (sf_ctx_t *ctx, const char *texname, bool should_log_failure);
```

### `sf_load_sprite`

```c
sf_sprite_t* sf_load_sprite (sf_ctx_t *ctx, const char *spritename, float duration, float scale, int frame_count, ...);
```

### `sf_get_sprite_`

```c
sf_sprite_t* sf_get_sprite_ (sf_ctx_t *ctx, const char *spritename, bool should_log_failure);
```

### `sf_add_emitr`

```c
sf_emitr_t* sf_add_emitr (sf_ctx_t *ctx, const char *emitrname, sf_emitr_type_t type, sf_sprite_t *sprite, int max_p);
```

### `sf_get_emitr_`

```c
sf_emitr_t* sf_get_emitr_ (sf_ctx_t *ctx, const char *emitrname, bool should_log_failure);
```

### `sf_load_obj`

```c
sf_obj_t* sf_load_obj (sf_ctx_t *ctx, const char *filename, const char *objname);
```

### `sf_get_obj_`

Linear search for a mesh by name; use the sf_get_obj() macro instead.

```c
sf_obj_t* sf_get_obj_ (sf_ctx_t *ctx, const char *objname, bool should_log_failure);
```

### `sf_add_enti`

```c
sf_enti_t* sf_add_enti (sf_ctx_t *ctx, sf_obj_t *obj, const char *entiname);
```

### `sf_get_enti_`

```c
sf_enti_t* sf_get_enti_ (sf_ctx_t *ctx, const char *entiname, bool should_log_failure);
```

### `sf_add_cam`

```c
sf_cam_t* sf_add_cam (sf_ctx_t *ctx, const char *camname, int w, int h, float fov);
```

### `sf_get_cam_`

```c
sf_cam_t* sf_get_cam_ (sf_ctx_t *ctx, const char *camname, bool should_log_failure);
```

### `sf_add_light`

```c
sf_light_t* sf_add_light (sf_ctx_t *ctx, const char *lightname, sf_light_type_t type, sf_fvec3_t color, float intensity);
```

### `sf_get_light_`

```c
sf_light_t* sf_get_light_ (sf_ctx_t *ctx, const char *lightname, bool should_log_failure);
```

### `sf_set_fog`

```c
void sf_set_fog (sf_ctx_t *ctx, sf_fvec3_t color, float start, float end);
```

### `sf_load_skybox`

```c
sf_skybox_t* sf_load_skybox (sf_ctx_t *ctx, const char *filename, const char *skyboxname);
```

### `sf_get_skybox_`

```c
sf_skybox_t* sf_get_skybox_ (sf_ctx_t *ctx, const char *skyboxname, bool should_log_failure);
```

### `sf_set_active_skybox`

```c
void sf_set_active_skybox (sf_ctx_t *ctx, sf_skybox_t *skybox);
```

### `sf_enti_set_pos`

Set an entity's world position directly.

```c
void sf_enti_set_pos (sf_ctx_t *ctx, sf_enti_t *enti, float x, float y, float z);
```

### `sf_enti_move`

Translate an entity by a world-space delta.

```c
void sf_enti_move (sf_ctx_t *ctx, sf_enti_t *enti, float dx, float dy, float dz);
```

### `sf_enti_set_rot`

Set an entity's Euler rotation (radians) directly.

```c
void sf_enti_set_rot (sf_ctx_t *ctx, sf_enti_t *enti, float rx, float ry, float rz);
```

### `sf_enti_rotate`

Add delta Euler angles (radians) to an entity's rotation.

```c
void sf_enti_rotate (sf_ctx_t *ctx, sf_enti_t *enti, float drx, float dry, float drz);
```

### `sf_enti_set_scale`

Set an entity's per-axis scale.

```c
void sf_enti_set_scale (sf_ctx_t *ctx, sf_enti_t *enti, float sx, float sy, float sz);
```

### `sf_enti_set_tex`

Assign a texture to an entity by name.

```c
void sf_enti_set_tex (sf_ctx_t *ctx, const char *entiname, const char *texname);
```

### `sf_obj_recenter`

```c
void sf_obj_recenter (sf_obj_t *obj);
```

### `sf_camera_set_psp`

Update a camera's perspective parameters and mark the projection matrix dirty.

```c
void sf_camera_set_psp (sf_ctx_t *ctx, sf_cam_t *cam, float fov, float near_plane, float far_plane);
```

### `sf_camera_set_pos`

Set a camera's world position directly.

```c
void sf_camera_set_pos (sf_ctx_t *ctx, sf_cam_t *cam, float x, float y, float z);
```

### `sf_camera_move_loc`

Move a camera along its own local axes (forward, right, up).

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

Serialize the current scene (cameras, objects, entities, lights) to a .sff file.

```c
void sf_load_sff (sf_ctx_t *ctx, const char *filename, const char *worldname);
```

### `sf_save_sff`

```c
bool sf_save_sff (sf_ctx_t *ctx, const char *filepath);
```

### `_sf_sff_read_kv`

```c
bool _sf_sff_read_kv (FILE *f, char *key, size_t ksz, char *val, size_t vsz);
```

### `_sf_sff_trim`

Trim leading and trailing whitespace (including newlines) from a string in place.

```c
void _sf_sff_trim (char *s);
```

### `_sf_sff_prse_vec3`

```c
sf_fvec3_t _sf_sff_prse_vec3 (const char *s);
```

### `_sf_sff_prse_list`

```c
int _sf_sff_prse_list (const char *s, char out[][64], int max);
```

### `_sf_sff_get_frame_`

Parse a "billboard name { ... }" block from a .sff file.

```c
sf_frame_t* _sf_sff_get_frame_ (sf_ctx_t *ctx, const char *name);
```

### `_sf_sff_prse_frame`

```c
void _sf_sff_prse_frame (sf_ctx_t *ctx, FILE *f, const char *name, int *frame_count);
```

### `_sf_sff_prse_cam`

```c
void _sf_sff_prse_cam (sf_ctx_t *ctx, FILE *f, const char *name, int *cam_count);
```

### `_sf_sff_prse_enti`

```c
void _sf_sff_prse_enti (sf_ctx_t *ctx, FILE *f, const char *name, int *enti_count);
```

### `_sf_sff_prse_light`

```c
void _sf_sff_prse_light (sf_ctx_t *ctx, FILE *f, const char *name, int *light_count);
```

### `_sf_sff_prse_sprit`

```c
void _sf_sff_prse_sprit (sf_ctx_t *ctx, FILE *f, const char *name, int *sprite_count);
```

### `_sf_sff_prse_emitr`

```c
void _sf_sff_prse_emitr (sf_ctx_t *ctx, FILE *f, const char *name, int *emitr_count);
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

Recursively unlink a frame and all its children, returning them to the free list.

```c
void sf_remove_frame (sf_ctx_t *ctx, sf_frame_t *f);
```

### `_sf_set_up_frames`

Create the three convention roots (DEFAULT, NED, FLU) with their fixed basis matrices.

```c
void _sf_set_up_frames (sf_ctx_t *ctx);
```

### `_sf_calc_frame_tree`

Orient a frame to face a world-space target point (sets yaw/pitch; rolls to zero).

```c
void _sf_calc_frame_tree (sf_frame_t *f, sf_fmat4_t parent_global_M, bool force_dirty);
```

### `_sf_write_frame_ref`

```c
void _sf_write_frame_ref (FILE *f, sf_frame_t *fr, sf_ctx_t *ctx);
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

Draw a Bresenham line between two screen-space points.

```c
void sf_line (sf_ctx_t *ctx, sf_cam_t *cam, sf_pkd_clr_t c, sf_ivec2_t v0, sf_ivec2_t v1);
```

### `sf_rect`

Fill a screen-space axis-aligned rectangle with a solid color.

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

Render the collapsed dropdown header showing the selected item label and an arrow.

```c
void sf_put_text (sf_ctx_t *ctx, sf_cam_t *cam, const char *text, sf_ivec2_t p, sf_pkd_clr_t c, int scale);
```

### `sf_clear_depth`

Reset the z-buffer to maximum depth (0x7F7F7F7F ≈ far).

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

Draw the world-space X/Y/Z axes as RGB lines in a corner compass widget.

```c
void sf_draw_debug_axes (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_draw_debug_frames`

```c
void sf_draw_debug_frames (sf_ctx_t *ctx, sf_cam_t *cam, float axis_size);
```

### `sf_draw_debug_lights`

Draw a star (point) or arrow (dir) gizmo for each light in the scene.

```c
void sf_draw_debug_lights (sf_ctx_t *ctx, sf_cam_t *cam, float size);
```

### `sf_draw_debug_cams`

Draw frustum wireframes for every camera except view_cam itself.

```c
void sf_draw_debug_cams (sf_ctx_t *ctx, sf_cam_t *view_cam, float ray_len);
```

### `sf_draw_debug_perf`

Render a HUD bar showing FPS, delta time, triangle count, and arena usage.

```c
void sf_draw_debug_perf (sf_ctx_t *ctx, sf_cam_t *cam);
```

### `sf_draw_sprite`

Billboard a sprite frame at a world position with depth testing and alpha blending.

```c
void sf_draw_sprite (sf_ctx_t *ctx, sf_cam_t *cam, sf_sprite_t *spr, sf_fvec3_t pos_w, float anim_time, float scale_mult);
```

### `sf_draw_bill`

```c
void sf_draw_bill (sf_ctx_t *ctx, sf_cam_t *cam, sf_bill_t *bill, float anim_time);
```

### `sf_add_bill`

```c
sf_bill_t* sf_add_bill (sf_ctx_t *ctx, sf_sprite_t *spr, const char *name, sf_fvec3_t pos, float scale, float opacity, float angle);
```

### `sf_clear_bills`

Remove all billboard instances from the scene.

```c
void sf_clear_bills (sf_ctx_t *ctx);
```

### `_sf_sff_prse_bill`

```c
void _sf_sff_prse_bill (sf_ctx_t *ctx, FILE *f, const char *name, int *bill_count);
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

Remove all UI elements and clear focus without freeing arena memory.

```c
void sf_ui_clear (sf_ctx_t *ctx);
```

### `sf_ui_get_by_name`

Look up a UI element by its name string; returns NULL if not found.

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

### `_sf_ui_find_prnt_pnl`

```c
sf_ui_lmn_t* _sf_ui_find_prnt_pnl (sf_ctx_t *ctx, sf_ivec2_t v0, sf_ivec2_t v1);
```

### `_sf_ui_eff_visible`

```c
bool _sf_ui_eff_visible (sf_ui_lmn_t *el);
```

### `_sf_draw_button`

```c
void _sf_draw_button (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
```

### `_sf_draw_slider`

```c
void _sf_draw_slider (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
```

### `_sf_draw_checkbox`

```c
void _sf_draw_checkbox (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
```

### `_sf_draw_label`

```c
void _sf_draw_label (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
```

### `_sf_draw_text_input`

```c
void _sf_draw_text_input (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
```

### `_sf_draw_drag_float`

Render a drag-float widget showing the current float value centered in its box.

```c
void _sf_draw_drag_float (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
```

### `_sf_draw_dropdown`

```c
void _sf_draw_dropdown (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
```

### `_sf_draw_drpdwn_popup`

Render the open dropdown item list below the header, highlighting the hovered row.

```c
void _sf_draw_drpdwn_popup(sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
```

### `_sf_draw_panel`

Render a panel: dark background, header bar, and collapse indicator prefix.

```c
void _sf_draw_panel (sf_ctx_t *ctx, sf_cam_t *cam, sf_ui_lmn_t *el);
```

### `_sf_update_text_input`

Handle focus, character insertion/deletion, and caret movement for a text-input element.

```c
void _sf_update_text_input(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_pressed);
```

### `_sf_update_drag_float`

```c
void _sf_update_drag_float(sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed);
```

### `_sf_update_dropdown`

```c
void _sf_update_dropdown (sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_pressed);
```

### `_sf_update_panel`

```c
void _sf_update_panel (sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_pressed);
```

### `_sf_update_button`

```c
void _sf_update_button (sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released);
```

### `_sf_update_checkbox`

```c
void _sf_update_checkbox (sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released);
```

### `_sf_update_slider`

```c
void _sf_update_slider (sf_ctx_t *ctx, sf_ui_lmn_t *el, bool m_down, bool m_pressed, bool m_released);
```

### `_sf_ui_type_str`

```c
const char* _sf_ui_type_str (sf_ui_type_t t);
```

### `_sf_ui_type_from_str`

```c
int _sf_ui_type_from_str (const char *s);
```


## Mesh Authoring

### `sf_obj_create_empty`

```c
sf_obj_t* sf_obj_create_empty (sf_ctx_t *ctx, const char *objname, int max_v, int max_vt, int max_f);
```

### `sf_obj_add_vert`

Generate a UV-sphere mesh with the given radius and segment count.

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

Export a mesh to a Wavefront .obj file.

```c
int sf_obj_add_face_uv (sf_obj_t *obj, int v0, int v1, int v2, int t0, int t1, int t2);
```

### `sf_obj_recompute_bs`

Recompute the bounding-sphere center (centroid) and radius from the current vertex set.

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

Slab-method AABB intersection; writes nearest hit distance to out_t if non-NULL.

```c
bool sf_ray_aabb (sf_ray_t r, sf_fvec3_t bmin, sf_fvec3_t bmax, float *out_t);
```


## Logging

### `sf_log_`

```c
void sf_log_ (sf_ctx_t *ctx, sf_log_level_t level, const char* func, const char* fmt, ...);
```

### `sf_set_logger`

```c
void sf_set_logger (sf_ctx_t *ctx, sf_log_fn log_cb, void* userdata);
```

### `sf_logger_console`

Default log sink: write the formatted message to stdout.

```c
void sf_logger_console (const char* message, void* userdata);
```

### `_sf_log_lvl_to_str`

```c
const char* _sf_log_lvl_to_str (sf_log_level_t level);
```


## Math

### `sf_fmat4_mul_fmat4`

Multiply two 4×4 matrices and return the result.

```c
sf_fmat4_t sf_fmat4_mul_fmat4 (sf_fmat4_t m0, sf_fmat4_t m1);
```

### `sf_fmat4_mul_vec3`

Transform a vec3 by a 4×4 matrix with perspective divide.

```c
sf_fvec3_t sf_fmat4_mul_vec3 (sf_fmat4_t m, sf_fvec3_t v);
```

### `sf_fvec3_sub`

Subtract v1 from v0 component-wise.

```c
sf_fvec3_t sf_fvec3_sub (sf_fvec3_t v0, sf_fvec3_t v1);
```

### `sf_fvec3_add`

Add two vec3s component-wise.

```c
sf_fvec3_t sf_fvec3_add (sf_fvec3_t v0, sf_fvec3_t v1);
```

### `sf_fvec3_norm`

Build a 4×4 non-uniform scale matrix from a vec3 of scale factors.

```c
sf_fvec3_t sf_fvec3_norm (sf_fvec3_t v);
```

### `sf_fvec3_cross`

Compute the cross product of v0 and v1.

```c
sf_fvec3_t sf_fvec3_cross (sf_fvec3_t v0, sf_fvec3_t v1);
```

### `sf_fvec3_dot`

Compute the scalar dot product of v0 and v1.

```c
float sf_fvec3_dot (sf_fvec3_t v0, sf_fvec3_t v1);
```

### `sf_make_tsl_fmat4`

Build a 4×4 translation matrix for (x, y, z).

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

Build a 4×4 rotation matrix from Euler XYZ angles (radians), applied as Y→X→Z.

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

### `_sf_vec_to_index`

```c
uint32_t _sf_vec_to_index (sf_ctx_t *ctx, sf_cam_t *cam, sf_ivec2_t v);
```

### `_sf_swap_svec2`

```c
void _sf_swap_svec2 (sf_ivec2_t *v0, sf_ivec2_t *v1);
```

### `_sf_swap_fvec3`

```c
void _sf_swap_fvec3 (sf_fvec3_t *v0, sf_fvec3_t *v1);
```

### `_sf_lerp_f`

```c
float _sf_lerp_f (float a, float b, float t);
```

### `_sf_lerp_fvec3`

```c
sf_fvec3_t _sf_lerp_fvec3 (sf_fvec3_t a, sf_fvec3_t b, float t);
```

### `_sf_intersect_near`

Update frames and emitters, then render all cameras including the main camera.

```c
sf_fvec3_t _sf_intersect_near (sf_fvec3_t v0, sf_fvec3_t v1, float near);
```

### `_sf_project_vertex`

```c
sf_fvec3_t _sf_project_vertex (sf_ctx_t *ctx, sf_cam_t *cam, sf_fvec3_t v, sf_fmat4_t P);
```

### `_sf_hash_2d`

```c
float _sf_hash_2d (int x, int z, uint32_t seed);
```

### `_sf_smooth_noise`

Unproject a screen pixel into a world-space ray origin and direction.

```c
float _sf_smooth_noise (float x, float z, uint32_t seed);
```


---

*Generated by `docs/gen_docs.py`*
