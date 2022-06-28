static struct memory_arena game_arena = {};

image_id test_image;
font_id game_font;

void game_initialize(void) {
    game_arena = memory_arena_create_from_heap("Game Memory", Megabyte(16));
    graphics_assets = graphics_assets_create(&game_arena, 64, 512);
    scratch_arena = memory_arena_create_from_heap("Scratch Buffer", Megabyte(16));
    test_image = graphics_assets_load_image(&graphics_assets, string_literal("./res/a.png"));
    game_font  = graphics_assets_load_bitmap_font(&graphics_assets, string_literal("./res/gnshbmpfonts/gnsh-bitmapfont-colour1.png"), 5, 12, 5, 20);
}

void game_deinitialize(void) {
    graphics_assets_finish(&graphics_assets);
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
        struct software_framebuffer blur_buffer = software_framebuffer_create(&scratch_arena, framebuffer->width/quality_scale, framebuffer->height/quality_scale);
        software_framebuffer_copy_into(&blur_buffer, framebuffer);
        software_framebuffer_kernel_convolution_ex(&scratch_arena, &blur_buffer, box_blur, 3, 3, 12, t, 3);
        software_framebuffer_draw_image_ex(framebuffer, &blur_buffer, RECTANGLE_F32_NULL, RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
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

    {
        struct render_commands commands = render_commands(
            camera_centered(v2f32(sinf(global_elapsed_time) * 100, cosf(global_elapsed_time) * 100),
                            1)
        );
        {
            commands.should_clear_buffer = true;
            commands.clear_buffer_color  = color32u8(0, 128, 0, 255);

            render_commands_push_quad(&commands,  rectangle_f32(-50, 450, 100, 100), color32u8(255, 0, 0, 255), BLEND_MODE_ALPHA);
            render_commands_push_image(&commands, graphics_assets_get_image_by_id(&graphics_assets, test_image), rectangle_f32(x, y, 96, 96), RECTANGLE_F32_NULL, color32f32(1,0,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
            render_commands_push_quad(&commands,  rectangle_f32(100, 0, 400, 400), color32u8(0, 0, 255, 128), BLEND_MODE_ALPHA);
            render_commands_push_quad(&commands,  rectangle_f32(40, 0, 200, 200), color32u8(255, 0, 255, 128), BLEND_MODE_ALPHA);
            render_commands_push_line(&commands,  v2f32(200, 200), v2f32(400, 400), color32u8(0, 0, 255, 255), BLEND_MODE_ALPHA);

        }
        software_framebuffer_render_commands(framebuffer, &commands);
    }
    static bool blur = false;

    if (is_key_pressed(KEY_E)) {
        blur ^= 1;
    }

    static f32 test_t = 0.5;
    if (is_key_down(KEY_LEFT)) {
        test_t -= dt * 2;
        if (test_t < 0) test_t = 0;
    } else if (is_key_down(KEY_RIGHT)) {
        test_t += dt * 2;
        if (test_t > 1) test_t = 1;
    }

    if (blur) {
        game_postprocess_blur(framebuffer, 4, test_t);
        game_postprocess_grayscale(framebuffer, 0.35);
    }

    {
        struct render_commands commands = render_commands(camera(v2f32(0,0), 1));
        /* render_commands_push_text(&commands, &game_font, 4, v2f32(0, 16), string_literal("In the beginning.\nThere was nothing.\n"), color32f32(1,1,1,1)); */
        render_commands_push_text(&commands, graphics_assets_get_font_by_id(&graphics_assets, game_font), 4, v2f32(0, 16), string_literal("In the beginning.\nThere was nothing.\n"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        render_commands_push_text(&commands, graphics_assets_get_font_by_id(&graphics_assets, game_font), 2, v2f32(0, 100), string_literal("In the beginning.\nThere was nothing.\n"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        render_commands_push_text(&commands, graphics_assets_get_font_by_id(&graphics_assets, game_font), 1, v2f32(0, 140), string_literal("g"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        software_framebuffer_render_commands(framebuffer, &commands);
    }
}
