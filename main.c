/*
  TODO
  Key input,
  and basic world collision detection
*/

#include <SDL2/SDL.h>

#include "common.c"
#include "memory_arena.c"

#include "graphics.c"

/* While this is *software* rendered, we still blit using a hardware method since it's still faster than manual texture blits. */
static SDL_Window*                 global_game_window          = NULL;
static bool                        global_game_running         = true;
static SDL_Renderer*               global_game_sdl_renderer    = NULL;
static SDL_Texture*                global_game_texture_surface = NULL;
static struct software_framebuffer global_default_framebuffer;
struct image_buffer                test_image;

void swap_framebuffers_onto_screen(void) {
    {
        void* locked_pixel_region;
        s32   _pitch; unused(_pitch);
        SDL_LockTexture(global_game_texture_surface, 0, &locked_pixel_region, &_pitch);
        memory_copy(global_default_framebuffer.pixels, locked_pixel_region, global_default_framebuffer.width * global_default_framebuffer.height * sizeof(u32), 0);
        SDL_UnlockTexture(global_game_texture_surface);
    }

    SDL_RenderCopy(global_game_sdl_renderer, global_game_texture_surface, 0, 0);
    SDL_RenderPresent(global_game_sdl_renderer);
}

void update_and_render_game(struct software_framebuffer* framebuffer, float dt) {
    static f32 x = 0;
    static f32 dir = 1;
    software_framebuffer_clear_buffer(framebuffer, color32u8(0, 255, 0, 255));
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(-50, 450, 100, 100), color32u8(255, 0, 0, 255));
    software_framebuffer_draw_image_ex(framebuffer, test_image, rectangle_f32(x, 5, 96, 96), RECTANGLE_F32_NULL, color32f32(1,1,1,1), 0); 
    software_framebuffer_draw_image_ex(framebuffer, test_image, rectangle_f32(x, 100, 96, 96), RECTANGLE_F32_NULL, color32f32(1,1,1,1), 0); 
    software_framebuffer_draw_image_ex(framebuffer, test_image, rectangle_f32(x, 200, 96, 96), RECTANGLE_F32_NULL, color32f32(1,1,1,1), 0); 
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(100, 0, 400, 400), color32u8(0, 0, 255, 128));
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(40, 0, 200, 200), color32u8(255, 0, 255, 128));

    x += 100 * dir * dt;
    if (x + 96 > 640 || x < 0) dir *= -1;
}

int main(int argc, char** argv) {
    struct memory_arena game_arena = memory_arena_create_from_heap("Game Memory", Megabyte(256));

    SDL_Init(SDL_INIT_VIDEO);

    const u32 SCREEN_WIDTH  = 320;
    const u32 SCREEN_HEIGHT = 200;

    global_game_window          = SDL_CreateWindow("RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    global_game_sdl_renderer    = SDL_CreateRenderer(global_game_window, -1, SDL_RENDERER_ACCELERATED);
    global_default_framebuffer  = software_framebuffer_create(&game_arena, SCREEN_WIDTH, SCREEN_HEIGHT);
    global_game_texture_surface = SDL_CreateTexture(global_game_sdl_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, global_default_framebuffer.width, global_default_framebuffer.height);

    test_image = image_buffer_load_from_file("./res/a.png");

    f32 last_elapsed_delta_time = (1.0 / 60.0f);
    while (global_game_running) {
        u32 start_frame_time = SDL_GetTicks();

        {
            SDL_Event current_event;
            while (SDL_PollEvent(&current_event)) {
                switch (current_event.type) {
                    case SDL_QUIT: {
                        global_game_running = false;
                    } break;
                        
                    case SDL_KEYUP:
                    case SDL_KEYDOWN: {
                        bool is_keydown = (current_event.type == SDL_KEYDOWN);
                    } break;

                    case SDL_MOUSEWHEEL:
                    case SDL_MOUSEMOTION:
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP: {
                        
                    } break;

                    default: {
                        
                    } break;
                }
            }
        }


        update_and_render_game(&global_default_framebuffer, last_elapsed_delta_time);
        swap_framebuffers_onto_screen();

        last_elapsed_delta_time = (SDL_GetTicks() - start_frame_time) / 1000.0f;
    }

    image_buffer_free(&test_image);
    memory_arena_finish(&game_arena);

    SDL_Quit();
    assertion(system_heap_memory_leak_check());
    return 0;
}
