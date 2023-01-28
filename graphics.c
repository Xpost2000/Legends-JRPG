/* 
   NOTE:
   
   consider making this easier for multithreading by allowing things to blit in tiled sets which
   allows for multithreading.
*/
#include "graphics_def.c"

/* doesn't take an allocator because of the way stb image works... */
struct image_buffer image_buffer_load_from_file(string filepath) {
    s32 width;
    s32 height;
    s32 components;

#ifndef EXPERIMENTAL_VFS
    u8* image_buffer = stbi_load(filepath.data, &width, &height, &components, 4);
#else
    /* STBIDEF stbi_uc *stbi_load_from_memory   (stbi_uc           const *buffer, int len   , int *x, int *y, int *channels_in_file, int desired_channels); */
    u8* image_buffer = NULL;
    {
        struct file_buffer buffer = read_entire_file(memory_arena_allocator(&game_arena), filepath);
        _debugprintf("%p, %d", buffer.buffer, (s32)buffer.length);
        image_buffer = stbi_load_from_memory(buffer.buffer, buffer.length, &width, &height, &components, 4);
        file_buffer_free(&buffer);
    }
#endif
    _debugprintf("tried to load: \"%.*s\"", filepath.length, filepath.data);
    if (!image_buffer) {
        _debugprintf("Failed to load \"%.*s\"", filepath.length, filepath.data);
    }
    assertion(image_buffer && "image load failed!");
    struct image_buffer result = (struct image_buffer) {
        .pixels = image_buffer,
        .width  = width,
        .height = height,
    };
    return result;
}

void image_buffer_write_to_disk(struct image_buffer* image, string as) {
    char filename[256] = {};
#if 0
    snprintf(filename, 256, "%s.bmp", as.data);
    stbi_write_bmp(filename, image->width, image->height, 4, image->pixels);
#else
    snprintf(filename, 256, "%s.jpg", as.data);
    stbi_write_jpg(filename, image->width, image->height, 4, image->pixels, 3);
#endif
    _debugprintf("screenshot produced.");
}

void image_buffer_free(struct image_buffer* image) {
    if (image->pixels) {
        system_heap_memory_deallocate(image->pixels);
        image->pixels = 0;
    }
}

struct font_cache font_cache_load_bitmap_font(string filepath, s32 tile_width, s32 tile_height, s32 atlas_rows, s32 atlas_columns) {
    struct font_cache result = {
        .type = FONT_CACHE_ATLAS_FIXED_ASCII,
        .tile_width = tile_width,
        .tile_height = tile_height,
        .atlas_rows = atlas_rows,
        .atlas_cols = atlas_columns
    };

    struct image_buffer atlas_image = image_buffer_load_from_file(filepath);

    result.width  = atlas_image.width;
    result.height = atlas_image.height;
    result.pixels = atlas_image.pixels;

    return result;
}

void font_cache_free(struct font_cache* font_cache) {
    image_buffer_free((struct image_buffer*) font_cache); 
}

f32 font_cache_text_height(struct font_cache* font_cache) {
    return font_cache->tile_height;
}

f32 font_cache_calculate_height_of(struct font_cache* font_cache, string str, f32 width_bounds, f32 scale) {
    f32 font_height = font_cache_text_height(font_cache);

    s32 units = 1;

    if (width_bounds == 0.0f) {
        for (s32 index = 0; index < str.length; ++index) {
            if (str.data[index] == '\n')
                units++;
        }
    } else {
        f32 cursor_x = 0;
        for (s32 index = 0; index < str.length; ++index) {
            cursor_x += font_cache->tile_width * scale;

            if (str.data[index] == '\n') {
                cursor_x = 0;
                units++;
            }

            if (cursor_x >= width_bounds) {
                cursor_x = 0;
                units++;
            }
        }
    }

    return units * font_height * scale;
}

f32 font_cache_text_width(struct font_cache* font_cache, string text, f32 scale) {
    /* NOTE font_caches are monospaced */
    return font_cache->tile_width * text.length * scale;
}

struct render_commands render_commands(struct memory_arena* arena, s32 capacity, struct camera camera) {
    struct render_commands result = {};
    result.camera = camera;
    result.command_capacity = capacity;
    result.commands         = memory_arena_push(arena, capacity * sizeof(*result.commands));
    return result;
}

struct render_command* render_commands_new_command(struct render_commands* commands, s16 type) {
    struct render_command* command = &commands->commands[commands->command_count++];
    command->type = type;
    return command;
}

void render_commands_set_shader(struct render_commands* commands, shader_fn shader, void* context) {
    struct render_command* last_command = &commands->commands[commands->command_count - 1];
    last_command->shader = shader;
    last_command->shader_ctx = context;
}

void render_commands_push_quad(struct render_commands* commands, struct rectangle_f32 destination, union color32u8 rgba, u8 blend_mode) {
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_QUAD);
    command->destination   = destination;
    command->flags         = 0;
    command->modulation_u8 = rgba;
    command->blend_mode    = blend_mode;
}

void render_commands_push_image(struct render_commands* commands, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 source, union color32f32 rgba, u32 flags, u8 blend_mode){
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_IMAGE);
    command->destination   = destination;
    command->image         = image;
    command->source        = source;
    command->flags         = flags;
    command->modulation_u8 = color32f32_to_color32u8(rgba);
    command->blend_mode    = blend_mode;
}

void render_commands_push_line(struct render_commands* commands, v2f32 start, v2f32 end, union color32u8 rgba, u8 blend_mode) {
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_LINE);
    command->start         = start;
    command->end           = end;
    command->flags         = 0;
    command->modulation_u8 = rgba;
    command->blend_mode    = blend_mode;
}

void render_commands_push_text(struct render_commands* commands, struct font_cache* font, f32 scale, v2f32 xy, string text, union color32f32 rgba, u8 blend_mode) {
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_TEXT);
    command->font          = font;
    command->xy            = xy;
    command->scale         = scale;
    command->text          = text;
    command->modulation_u8 = color32f32_to_color32u8(rgba);
    command->flags         = 0;
    command->blend_mode    = blend_mode;
}

void render_commands_clear(struct render_commands* commands) {
    commands->command_count = 0;
}

void sort_render_commands(struct render_commands* commands) {
    /* NOTE: Not needed? I seem to know the explicit order of everything I'm drawing... */
}

struct graphics_assets graphics_assets_create(struct memory_arena* arena, u32 font_limit, u32 image_limit) {
    struct graphics_assets assets = {
        .font_capacity  = font_limit,
        .image_capacity = image_limit,
    };

    assets.arena              = arena;
    assets.images             = memory_arena_push(arena, sizeof(*assets.images) * image_limit);
    assets.image_file_strings = memory_arena_push(arena, sizeof(*assets.image_file_strings) *image_limit);
    assets.fonts              = memory_arena_push(arena, sizeof(*assets.fonts)  * font_limit);

    return assets;
}

void graphics_assets_finish(struct graphics_assets* assets) {
    for (unsigned image_index = 0; image_index < assets->image_count; ++image_index) {
        struct image_buffer* image = assets->images + image_index;
        _debugprintf("destroying img: %p (%d) (%dx%d %p)", image, image_index, image->width, image->height, image->pixels);
        image_buffer_free(image);
    }
    for (unsigned font_index = 0; font_index < assets->font_count; ++font_index) {
        struct font_cache* font = assets->fonts + font_index;
        _debugprintf("destroying font: %p (%d)", font, font_index);
        font_cache_free(font);
    }
}

image_id graphics_assets_load_image(struct graphics_assets* assets, string path) {
    for (s32 index = 0; index < assets->image_count; ++index) {
        string filepath = assets->image_file_strings[index];

        if (string_equal(path, filepath)) {
            return (image_id){.index = index+1};
        }
    }

    image_id new_id = (image_id) { .index = assets->image_count + 1 };
    _debugprintf("img loaded: %.*s", path.length, path.data);

    struct image_buffer* new_image           = &assets->images[assets->image_count];
    string*              new_filepath_string = &assets->image_file_strings[assets->image_count++];
    *new_filepath_string                     = memory_arena_push_string(assets->arena, path);
    *new_image                               = image_buffer_load_from_file(*new_filepath_string);

    return new_id;
}

font_id graphics_assets_load_bitmap_font(struct graphics_assets* assets, string path, s32 tile_width, s32 tile_height, s32 atlas_rows, s32 atlas_columns) {
    font_id new_id = (font_id) { .index = assets->font_count + 1 };

    struct font_cache* new_font   = &assets->fonts[assets->font_count++];
    *new_font                     = font_cache_load_bitmap_font(path, tile_width, tile_height, atlas_rows, atlas_columns);

    return new_id;
}

struct font_cache* graphics_assets_get_font_by_id(struct graphics_assets* assets, font_id font) {
    assertion(font.index > 0 && font.index <= assets->font_count);
    return &assets->fonts[font.index-1];
}

struct image_buffer* graphics_assets_get_image_by_id(struct graphics_assets* assets, image_id image) {
    assertion(image.index > 0 && image.index <= assets->image_count);
    return &assets->images[image.index-1];
}

image_id graphics_assets_get_image_by_filepath(struct graphics_assets* assets, string filepath) {
    assertion(filepath.data && filepath.length && "Bad string?");
    return graphics_assets_load_image(assets, filepath);
}

struct lightmask_buffer lightmask_buffer_create(u32 buffer_width, u32 buffer_height) {
    struct lightmask_buffer result;
    result.width       = buffer_width;
    result.height      = buffer_height;
    result.mask_buffer = system_heap_memory_allocate(buffer_width * buffer_height);
    lightmask_buffer_clear(&result);
    return result;
}

void lightmask_buffer_clear(struct lightmask_buffer* buffer) {
    zero_memory(buffer->mask_buffer, buffer->width*buffer->height);
}

static inline u8 lightmask_buffer_get_pixel(struct lightmask_buffer* buffer, s32 x, s32 y) {
    u32 index = y * buffer->width + x;
    return buffer->mask_buffer[index];
}

static inline void lightmask_buffer_put_pixel(struct lightmask_buffer* buffer, s32 x, s32 y, u8 value, u8 blend_mode) {
    u32 index = y * buffer->width + x;

    switch (blend_mode) {
        case LIGHTMASK_BLEND_NONE: {
            buffer->mask_buffer[index] = value;
        } break;
        case LIGHTMASK_BLEND_OR: {
            if (lightmask_buffer_get_pixel(buffer, x, y) == 255) {
                // ...
            } else {
                buffer->mask_buffer[index] = value;
            }
        } break;
    }
}

void lightmask_buffer_blit_image_clipped(struct lightmask_buffer* buffer, struct rectangle_f32 clip_rect, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, u8 flags, u8 blend_mode, u8 v) {
    if ((destination.x == 0) && (destination.y == 0) && (destination.w == 0) && (destination.h == 0)) {
        destination.w = buffer->width;
        destination.h = buffer->height;
    }

    if (!rectangle_f32_intersect(destination, clip_rect)) {
        return;
    }

    if ((src.x == 0) && (src.y == 0) && (src.w == 0) && (src.h == 0)) {
        src.w = image->width;
        src.h = image->height;
    }

    f32 scale_ratio_w = (f32)src.w  / destination.w;
    f32 scale_ratio_h = (f32)src.h  / destination.h;

    s32 start_x = clamp_s32((s32)destination.x, clip_rect.x, clip_rect.x+clip_rect.w);
    s32 start_y = clamp_s32((s32)destination.y, clip_rect.y, clip_rect.y+clip_rect.h);
    s32 end_x   = clamp_s32((s32)(destination.x + destination.w), clip_rect.x, clip_rect.x+clip_rect.w);
    s32 end_y   = clamp_s32((s32)(destination.y + destination.h), clip_rect.y, clip_rect.y+clip_rect.h);

    s32 unclamped_end_x = (s32)(destination.x + destination.w);
    s32 unclamped_end_y = (s32)(destination.y + destination.h);

    s32 stride       = buffer->width;
    s32 image_stride = image->width;

    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
            s32 image_sample_x = (s32)((src.x + src.w) - ((unclamped_end_x - x_cursor) * scale_ratio_w));
            s32 image_sample_y = (s32)((src.y + src.h) - ((unclamped_end_y - y_cursor) * scale_ratio_h));

            if ((flags & DRAW_IMAGE_FLIP_HORIZONTALLY))
                image_sample_x = (s32)(((unclamped_end_x - x_cursor) * scale_ratio_w) + src.x);

            if ((flags & DRAW_IMAGE_FLIP_VERTICALLY))
                image_sample_y = (s32)(((unclamped_end_y - y_cursor) * scale_ratio_h) + src.y);

            union color32f32 sampled_pixel = color32f32(image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 0] / 255.0f,
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 1] / 255.0f,
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 2] / 255.0f,
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 3] / 255.0f);

            if (sampled_pixel.a == 1.0) {
                lightmask_buffer_put_pixel(buffer, x_cursor, y_cursor, v, blend_mode);
            }
        }
    }
}

void lightmask_buffer_blit_rectangle_clipped(struct lightmask_buffer* buffer, struct rectangle_f32 clip_rect, struct rectangle_f32 destination, u8 blend_mode, u8 v) {
    s32 start_x = clamp_s32((s32)destination.x, clip_rect.x, clip_rect.x+clip_rect.w);
    s32 start_y = clamp_s32((s32)destination.y, clip_rect.y, clip_rect.y+clip_rect.h);
    s32 end_x   = clamp_s32((s32)(destination.x + destination.w), clip_rect.x, clip_rect.x+clip_rect.w);
    s32 end_y   = clamp_s32((s32)(destination.y + destination.h), clip_rect.y, clip_rect.y+clip_rect.h);

    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
            lightmask_buffer_put_pixel(buffer, x_cursor, y_cursor, v, blend_mode);
        }
    }
}

void lightmask_buffer_blit_image(struct lightmask_buffer* buffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, u8 flags, u8 blend_mode, u8 v) {
    lightmask_buffer_blit_image_clipped(buffer, rectangle_f32(0, 0, buffer->width, buffer->height), image, destination, src, flags, blend_mode, v);
}

void lightmask_buffer_blit_rectangle(struct lightmask_buffer* buffer, struct rectangle_f32 destination, u8 blend_mode, u8 v) {
    lightmask_buffer_blit_rectangle_clipped(buffer, rectangle_f32(0, 0, buffer->width, buffer->height), destination, blend_mode, v);
}

f32 lightmask_buffer_lit_percent(struct lightmask_buffer* buffer, s32 x, s32 y) {
    u32 index = y * buffer->width + x;
    u8 value = buffer->mask_buffer[index];

    return (value) / 255.0f;
}

bool lightmask_buffer_is_lit(struct lightmask_buffer* buffer, s32 x, s32 y) {
    u32 index = y * buffer->width + x;

    if (buffer->mask_buffer[index] == 255) {
        return 1;
    }

    return 0;
}

void lightmask_buffer_finish(struct lightmask_buffer* buffer) {
    if (buffer->mask_buffer) {
        system_heap_memory_deallocate(buffer->mask_buffer);
        buffer->mask_buffer = NULL;

        buffer->width  = 0;
        buffer->height = 0;
    }
}

#include "software_framebuffer.c"
#include "opengl_renderer.c"
