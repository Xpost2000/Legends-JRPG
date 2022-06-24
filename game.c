struct image_buffer        test_image;
static struct memory_arena game_arena = {};
struct font_cache          game_font;


void game_initialize(void) {
    game_arena = memory_arena_create_from_heap("Game Memory", Megabyte(8));
    scratch_arena = memory_arena_create_from_heap("Scratch Buffer", Megabyte(4));
    test_image = image_buffer_load_from_file(string_literal("./res/a.png"));
    game_font  = font_cache_load_bitmap_font(string_literal("./res/gnshbmpfonts/gnsh-bitmapfont-colour1.png"), 5, 12, 5, 20);
}

void game_deinitialize(void) {
    font_cache_free(&game_font);
    image_buffer_free(&test_image);
    memory_arena_finish(&game_arena);
    memory_arena_finish(&scratch_arena);
}

void game_postprocess_blur(struct software_framebuffer* framebuffer, s32 quality_scale, f32 t) {
    f32 box_blur[] = {
        1,1,1,
        1,1,1,
        1,1,1,
    };

    if (t > 0.0) {
        struct software_framebuffer scaled_down = software_framebuffer_create(&scratch_arena, framebuffer->width/quality_scale, framebuffer->height/quality_scale);
        software_framebuffer_copy_into(&scaled_down, framebuffer);
        software_framebuffer_kernel_convolution_ex(&scratch_arena, &scaled_down, box_blur, 3, 3, 9, t, 1);
        software_framebuffer_draw_image_ex(framebuffer, &scaled_down, RECTANGLE_F32_NULL, RECTANGLE_F32_NULL, color32f32(1,1,1,1), 0);
    }
}

void game_postprocess_grayscale(struct software_framebuffer* framebuffer, f32 t) {
    for (s32 y_cursor = 0; y_cursor < framebuffer->height; ++y_cursor) {
        for (s32 x_cursor = 0; x_cursor < framebuffer->width; ++x_cursor) {
            u8 r = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 0];
            u8 g = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 1];
            u8 b = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 2];
            f32 average = (r + g + b) / 3.0f;
            framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 0] = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 0] * (1 - t) + average * t;
            framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 1] = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 1] * (1 - t) + average * t;
            framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 2] = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 2] * (1 - t) + average * t;
        }
    }
}

void update_and_render_game(struct software_framebuffer* framebuffer, float dt) {
    static f32 x = 0;
    static f32 y = 0;

    if (is_key_down(KEY_W)) {
        y -= 80 * dt;
    } else if (is_key_down(KEY_S)) {
        y += 80 * dt;
    }
    if (is_key_down(KEY_A)) {
        x -= 80 * dt;
    } else if (is_key_down(KEY_D)) {
        x += 80 * dt;
    }

    if (is_key_pressed(KEY_F12)) {
        image_buffer_write_to_disk(framebuffer, string_literal("scr"));
    }

    struct render_commands commands = render_commands(
        /* camera_centered(v2f32(0, 0), */
        /*                 ((sinf(global_elapsed_time)+1)/2.0)+0.4) */
        camera_centered(v2f32(0, 0), 2)
    );
    {
        commands.should_clear_buffer = true;
        commands.clear_buffer_color  = color32u8(0, 128, 0, 255);

        render_commands_push_quad(&commands, rectangle_f32(-50, 450, 100, 100), color32u8(255, 0, 0, 255));
        render_commands_push_image(&commands, &test_image, rectangle_f32(x, y, 96, 96), RECTANGLE_F32_NULL, color32f32(1,1,1,1), 0);
        render_commands_push_quad(&commands, rectangle_f32(100, 0, 400, 400), color32u8(0, 0, 255, 128));
        render_commands_push_quad(&commands, rectangle_f32(40, 0, 200, 200), color32u8(255, 0, 255, 128));
        render_commands_push_line(&commands, v2f32(200, 200), v2f32(400, 400), color32u8(0, 0, 255, 255));

        render_commands_push_text(&commands, &game_font, 8, v2f32(0, 0), string_literal("Hello World\nSad"), color32f32(1,1,1,1));
    }
    software_framebuffer_render_commands(framebuffer, &commands);
    static bool blur = true;

    if (is_key_pressed(KEY_E)) {
        blur ^= 1;
    }

    static f32 test_t = 0;
    if (is_key_down(KEY_LEFT)) {
        test_t -= dt;
        if (test_t < 0) test_t = 0;
    } else if (is_key_down(KEY_RIGHT)) {
        test_t += dt;
        if (test_t > 1) test_t = 1;
    }

    if (blur) {
        game_postprocess_blur(framebuffer, 8, test_t);
        game_postprocess_grayscale(framebuffer, test_t);
    }
}
