struct software_framebuffer {
    Image_Buffer_Base;

    s32 scissor_x;
    s32 scissor_y;
    s32 scissor_w;
    s32 scissor_h;
};

struct software_framebuffer software_framebuffer_create(u32 width, u32 height);
struct software_framebuffer software_framebuffer_create_from_arena(struct memory_arena* arena, u32 width, u32 height);
void                        software_framebuffer_finish(struct software_framebuffer* framebuffer);
void                        software_framebuffer_copy_into(struct software_framebuffer* target, struct software_framebuffer* source);
void                        software_framebuffer_clear_scissor(struct software_framebuffer* framebuffer);
void                        software_framebuffer_set_scissor(struct software_framebuffer* framebuffer, s32 x, s32 y, s32 w, s32 h);
void                        software_framebuffer_clear_buffer(struct software_framebuffer* framebuffer, union color32u8 rgba);
void                        software_framebuffer_draw_quad(struct software_framebuffer* framebuffer, struct rectangle_f32 destination, union color32u8 rgba, u8 blend_mode);
void                        software_framebuffer_draw_image_ex(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags, u8 blend_mode);
void                        software_framebuffer_draw_image_ex_clipped(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags, u8 blend_mode, struct rectangle_f32 clip_rect, shader_fn shader, void* shader_ctx);
void                        software_framebuffer_draw_glyph(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, v2f32 xy, char glyph, union color32f32 modulation, u8 blend_mode);
/* only thin lines */
void                        software_framebuffer_draw_line(struct software_framebuffer* framebuffer, v2f32 start, v2f32 end, union color32u8 rgba, u8 blend_mode);
void                        software_framebuffer_draw_text(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, v2f32 xy, string text, union color32f32 modulation, u8 blend_mode);
/* TODO make command buffer version */
void                        software_framebuffer_draw_text_bounds(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, v2f32 xy, f32 bounds_w, string cstring, union color32f32 modulation, u8 blend_mode);
void                        software_framebuffer_draw_text_bounds_centered(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, struct rectangle_f32 bounds, string text, union color32f32 modulation, u8 blend_mode);
void                        software_framebuffer_kernel_convolution_ex(struct memory_arena* arena, struct software_framebuffer* framebuffer, f32* kernel, s16 width, s16 height, f32 divisor, f32 blend_t, s32 passes);
void                        software_framebuffer_run_shader(struct software_framebuffer* framebuffer, struct rectangle_f32 src_rect, shader_fn shader, void* context);

/*
  This is intended for threading work, and kernel convolution samples from the neighbors which
  requires the entire unaltered image. I don't have enough stack space to copy the framebuffer every single time
  on the thread 

  (1.2 mb per thread to hold 640x480!!!)
  
  So we'll just pass one singular copy from the above function, to avoid the time to copy multiple slices of the framebuffer which
  would ruin the effect anyways.
*/
void software_framebuffer_kernel_convolution_ex_bounded(struct software_framebuffer before, struct software_framebuffer* framebuffer, f32* kernel, s16 kernel_width, s16 kernel_height, f32 divisor, f32 blend_t, s32 passes, struct rectangle_f32 clip);
