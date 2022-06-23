struct image_buffer        test_image;
static struct memory_arena game_arena = {};
struct font_cache          game_font;


void game_initialize(void) {
    game_arena = memory_arena_create_from_heap("Game Memory", Megabyte(8));
    scratch_arena = memory_arena_create_from_heap("Scratch Buffer", Megabyte(4));
    test_image = image_buffer_load_from_file("./res/a.png");
    game_font  = font_cache_load_bitmap_font("./res/gnshbmpfonts/gnsh-bitmapfont-colour1.png", 5, 12, 5, 20);
}

void game_deinitialize(void) {
    font_cache_free(&game_font);
    image_buffer_free(&test_image);
    memory_arena_finish(&game_arena);
    memory_arena_finish(&scratch_arena);
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
        image_buffer_write_to_disk(framebuffer, "scr");
    }

    software_framebuffer_clear_buffer(framebuffer, color32u8(0, 255, 0, 255));
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(-50, 450, 100, 100), color32u8(255, 0, 0, 255));
    software_framebuffer_draw_image_ex(framebuffer, &test_image, rectangle_f32(x, y, 96, 96), RECTANGLE_F32_NULL, color32f32(1,1,1,1), 0);
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(100, 0, 400, 400), color32u8(0, 0, 255, 128));
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(40, 0, 200, 200), color32u8(255, 0, 255, 128));
    software_framebuffer_draw_line(framebuffer, v2f32(200, 200), v2f32(400, 400), color32u8(0, 0, 255, 255));

    software_framebuffer_draw_text(framebuffer, &game_font, 8, v2f32(0, 0), "Hello World\nsad", color32f32(1,1,1,1));
}
