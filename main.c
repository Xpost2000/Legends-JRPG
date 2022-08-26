/* #define NO_POSTPROCESSING */
#define MULTITHREADED_EXPERIMENTAL

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define USE_SIMD_OPTIMIZATIONS
#endif

#include <SDL2/SDL.h>

#include "common.c"

static bool global_game_running = true;
static f32                         global_elapsed_time          = 0.0f;
static struct memory_arena         scratch_arena                = {};

/* target engine res */
local u32 SCREEN_WIDTH  = 0;
local u32 SCREEN_HEIGHT = 0;

local bool SCREEN_IS_FULLSCREEN = false;

/* real res */
/* consider a better way to decouple this from the editor logic. */
/* game logic is okay because we don't use the mouse for UI. (I mean I probably should allow it, but whatever.) */
local u32 REAL_SCREEN_WIDTH  = 640;
local u32 REAL_SCREEN_HEIGHT = 480;

#include "thread_pool.c"
#include "serializer.c"
#include "input.c"
#include "sdl_scancode_table.c"
#include "graphics.c"
#include "audio_none.c"

const char* _build_flags =
#ifdef USE_SIMD_OPTIMIZATIONS
    "using simd,"
#endif
#ifdef RELEASE
    "release,"
#else
    "debug,"
#endif
    ;

/* While this is *software* rendered, we still blit using a hardware method since it's still faster than manual texture blits. */
static SDL_Window*                 global_game_window           = NULL;
static SDL_Renderer*               global_game_sdl_renderer     = NULL;
static SDL_Texture*                global_game_texture_surface  = NULL;
static SDL_GameController*         global_controller_devices[4] = {};
static SDL_Haptic*                 global_haptic_devices[4]     = {};
static struct software_framebuffer global_default_framebuffer   = {};
static struct graphics_assets      graphics_assets              = {};


local void close_all_controllers(void) {
    for (unsigned controller_index = 0; controller_index < array_count(global_controller_devices); ++controller_index) {
        if (global_controller_devices[controller_index]) {
            SDL_GameControllerClose(global_controller_devices[controller_index]);
        }
    }
}

local void poll_and_register_controllers(void) {
    for (unsigned controller_index = 0; controller_index < array_count(global_controller_devices); ++controller_index) {
        SDL_GameController* controller = global_controller_devices[controller_index];

        if (controller) {
            if (!SDL_GameControllerGetAttached(controller)) {
                SDL_GameControllerClose(controller);
                global_controller_devices[controller_index] = NULL;
                _debugprintf("Controller at %d is bad", controller_index);
            }
        } else {
            if (SDL_IsGameController(controller_index)) {
                global_controller_devices[controller_index] = SDL_GameControllerOpen(controller_index);
                _debugprintf("Opened controller index %d (%s)\n", controller_index, SDL_GameControllerNameForIndex(controller_index));
            }
        }
    }
}

local void update_all_controller_inputs(void) {
    for (unsigned controller_index = 0; controller_index < array_count(global_controller_devices); ++controller_index) {
        SDL_GameController* controller = global_controller_devices[controller_index];

        if (!controller) {
            continue;
        }

        struct game_controller* gamepad = get_gamepad(controller_index);
        gamepad->_internal_controller_handle = controller;

        {
            gamepad->triggers.left  = (f32)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / (32767.0f);
            gamepad->triggers.right = (f32)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / (32767.0f);
        }

        {
            gamepad->buttons[BUTTON_RB] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
            gamepad->buttons[BUTTON_LB] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        }

        {
            gamepad->buttons[BUTTON_A] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);
            gamepad->buttons[BUTTON_B] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B);
            gamepad->buttons[BUTTON_X] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X);
            gamepad->buttons[BUTTON_Y] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y);
        }

        {
            gamepad->buttons[DPAD_UP]    = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
            gamepad->buttons[DPAD_DOWN]  = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
            gamepad->buttons[DPAD_LEFT]  = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT); 
            gamepad->buttons[DPAD_RIGHT] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT); 
        }

        {
            gamepad->buttons[BUTTON_RS] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
            gamepad->buttons[BUTTON_LS]  = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSTICK);
        }

        {
            gamepad->buttons[BUTTON_START] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START);
            gamepad->buttons[BUTTON_BACK]  = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK);
        }

        {
            {
                f32 axis_x = (f32)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) / (32767.0f);
                f32 axis_y = (f32)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) / (32767.0f);

                const f32 DEADZONE_X = 0.01;
                const f32 DEADZONE_Y = 0.03;
                if (fabs(axis_x) < DEADZONE_X) axis_x = 0;
                if (fabs(axis_y) < DEADZONE_Y) axis_y = 0;

                gamepad->left_stick.axes[0] = axis_x;
                gamepad->left_stick.axes[1] = axis_y;
            }

            {
                f32 axis_x = (f32)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) / (32767.0f);
                f32 axis_y = (f32)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) / (32767.0f);

                const f32 DEADZONE_X = 0.01;
                const f32 DEADZONE_Y = 0.03;
                if (fabs(axis_x) < DEADZONE_X) axis_x = 0;
                if (fabs(axis_y) < DEADZONE_Y) axis_y = 0;
                
                gamepad->right_stick.axes[0] = axis_x;
                gamepad->right_stick.axes[1] = axis_y;
            }
        }
    }
}

void controller_rumble(struct game_controller* controller, f32 x_magnitude, f32 y_magnitude, uint32_t ms) {
    SDL_GameController* sdl_controller = controller->_internal_controller_handle;
    x_magnitude = clamp_f32(x_magnitude, 0, 1);
    y_magnitude = clamp_f32(y_magnitude, 0, 1);
    SDL_GameControllerRumble(sdl_controller, (0xFFFF * x_magnitude), (0xFFFF * y_magnitude), ms);
}

local void set_window_transparency(f32 transparency) {
    SDL_SetWindowOpacity(global_game_window, transparency);
}

void swap_framebuffers_onto_screen(void) {
    {
        void* locked_pixel_region;
        s32   _pitch; unused(_pitch);
        SDL_LockTexture(global_game_texture_surface, 0, &locked_pixel_region, &_pitch);
        memory_copy(global_default_framebuffer.pixels, locked_pixel_region, global_default_framebuffer.width * global_default_framebuffer.height * sizeof(u32));
        SDL_UnlockTexture(global_game_texture_surface);
    }

    SDL_RenderCopy(global_game_sdl_renderer, global_game_texture_surface, 0, 0);
    SDL_RenderPresent(global_game_sdl_renderer);
}

local void change_resolution(s32 new_resolution_x, s32 new_resolution_y) {
    SDL_SetWindowSize(global_game_window, new_resolution_x, new_resolution_y);
}

local void toggle_fullscreen(void) {
    if (SCREEN_IS_FULLSCREEN) {
        SDL_SetWindowFullscreen(global_game_window, 0);
        SCREEN_IS_FULLSCREEN = false;
    } else {
        SDL_SetWindowFullscreen(global_game_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SCREEN_IS_FULLSCREEN = true;
    }
}

#include "game.c"

/* scale based off of vertical height */
const u32 ENGINE_BASE_VERTICAL_RESOLUTION = 480;

local v2f32 get_scaled_screen_resolution(v2f32 base_resolution) {
    f32 scale_factor = base_resolution.y / ENGINE_BASE_VERTICAL_RESOLUTION;

    base_resolution.x /= scale_factor;
    base_resolution.x = ceilf(base_resolution.x);

    return v2f32(base_resolution.x, ENGINE_BASE_VERTICAL_RESOLUTION);
}

local void initialize_framebuffer(void) {
    software_framebuffer_finish(&global_default_framebuffer);

    v2f32 framebuffer_resolution = get_scaled_screen_resolution(v2f32(REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT));

    /* I know these sound like constants. They really aren't... */
    SCREEN_WIDTH  = framebuffer_resolution.x;
    SCREEN_HEIGHT = framebuffer_resolution.y;


    _debugprintf("framebuffer resolution is: (%d, %d) vs (%d, %d) real resolution", SCREEN_WIDTH, SCREEN_HEIGHT, REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT);
    global_default_framebuffer  = software_framebuffer_create(framebuffer_resolution.x, framebuffer_resolution.y);

    if (global_game_texture_surface) {
        SDL_DestroyTexture(global_game_texture_surface);
    }

    global_game_texture_surface = SDL_CreateTexture(global_game_sdl_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, global_default_framebuffer.width, global_default_framebuffer.height);
}


void handle_sdl_events(void) {
    {
        SDL_Event current_event;

        f32 resolution_scale_x = (f32)REAL_SCREEN_WIDTH / SCREEN_WIDTH;
        f32 resolution_scale_y = (f32)REAL_SCREEN_HEIGHT / SCREEN_HEIGHT;

        while (SDL_PollEvent(&current_event)) {
            switch (current_event.type) {
                case SDL_WINDOWEVENT: {
                    switch (current_event.window.event) {
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED: {
                            s32 new_width  = current_event.window.data1;
                            s32 new_height = current_event.window.data2;

                            REAL_SCREEN_WIDTH  = new_width;
                            REAL_SCREEN_HEIGHT = new_height;
                            initialize_framebuffer();
                        } break;
                    }
                } break;

                case SDL_QUIT: {
                    global_game_running = false;
                } break;
                        
                case SDL_KEYUP:
                case SDL_KEYDOWN: {
                    bool is_keydown = (current_event.type == SDL_KEYDOWN);
                    if (is_keydown) {
                        register_key_down(translate_sdl_scancode(current_event.key.keysym.scancode));
                    } else {
                        register_key_up(translate_sdl_scancode(current_event.key.keysym.scancode));
                    }
                } break;

                case SDL_MOUSEWHEEL: {
                    register_mouse_wheel(current_event.wheel.x, current_event.wheel.y);
                } break;
                case SDL_MOUSEMOTION: {
                    register_mouse_position(current_event.motion.x / resolution_scale_x, current_event.motion.y / resolution_scale_y);
                } break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP: {
                    s32 button_id = 0;
                    switch (current_event.button.button) {
                        case SDL_BUTTON_LEFT: {
                            button_id = MOUSE_BUTTON_LEFT;
                        } break;
                        case SDL_BUTTON_MIDDLE: {
                            button_id = MOUSE_BUTTON_MIDDLE;
                        } break;
                        case SDL_BUTTON_RIGHT: {
                            button_id = MOUSE_BUTTON_RIGHT; 
                        } break;
                    }

                    register_mouse_position(current_event.button.x/resolution_scale_x, current_event.button.y/resolution_scale_y);
                    register_mouse_button(button_id, current_event.button.state == SDL_PRESSED);
                } break;

                case SDL_CONTROLLERDEVICEREMOVED:
                case SDL_CONTROLLERDEVICEADDED: {
                    poll_and_register_controllers();
                } break;

                case SDL_TEXTINPUT: {
                    char* input_text = current_event.text.text;
                    size_t input_length = strlen(input_text);
                    send_text_input(input_text, input_length);
                } break;

                default: {} break;
            }
        }
    }
}

local void initialize(void) {
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);

    /* global_game_window          = SDL_CreateWindow("RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN); */
    global_game_window          = SDL_CreateWindow("RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    global_game_sdl_renderer    = SDL_CreateRenderer(global_game_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    initialize_thread_pool();

    game_initialize();
    initialize_framebuffer();

    audio_initialize();
}

void deinitialize(void) {
    software_framebuffer_finish(&global_default_framebuffer);
    audio_deinitialize();
    synchronize_and_finish_thread_pool();
    game_deinitialize();

    close_all_controllers();

    SDL_Quit();
    _debugprintf("Peak allocations at: %d bytes", system_heap_peak_allocated_amount());
    if (system_heap_memory_leak_check())
        _debugprintf("no leaked memory");
    else
        _debugprintf("leaked memory somewhere: %d bytes", system_heap_currently_allocated_amount());
    /* assertion(system_heap_memory_leak_check()); */
}

#include "sandbox_tests.c"

f32 last_elapsed_delta_time = (1.0 / 60.0f);
void engine_main_loop(void* _) {
    char window_name_title_buffer[256] = {};
    u32 start_frame_time = SDL_GetTicks();

    begin_input_frame();
    {
        handle_sdl_events();
        update_all_controller_inputs();
        update_and_render_game(&global_default_framebuffer, last_elapsed_delta_time);
    }
    end_input_frame();

    swap_framebuffers_onto_screen();

    last_elapsed_delta_time = (SDL_GetTicks() - start_frame_time) / 1000.0f;
    global_elapsed_time    += last_elapsed_delta_time;
    add_frametime_sample(last_elapsed_delta_time);

#if 1
    {
        f32 average_frametime = get_average_frametime();
        snprintf(window_name_title_buffer, 256, "RPG(%s) - instant fps: %d, (%f ms)", _build_flags, (int)(1.0/(f32)average_frametime), average_frametime);
        SDL_SetWindowTitle(global_game_window, window_name_title_buffer);
    }
#endif

    memory_arena_clear_top(&scratch_arena);
    memory_arena_clear_bottom(&scratch_arena);
}

int engine_main(int argc, char** argv) {
    initialize();
    
    /* feature testing */
    {
        printf("%d byte sized cache line\n", SDL_GetCPUCacheLineSize());
        printf("%d MB of ram\n", SDL_GetSystemRAM());
        if (SDL_HasMMX()) {
            printf("MMX Detected\n");
        }

        if (SDL_HasSSE()) {
            printf("SSE Detected\n");
        }

        if (SDL_HasAVX()) {
            printf("AVX Detected\n");
        }

        if (SDL_HasSSE2()) {
            printf("SSE2 Detected\n");
        }

        if (SDL_HasSSE3()) {
            printf("SSE2 Detected\n");
        }

        if (SDL_HasSSE41() || SDL_HasSSE42()) {
            printf("SSE4 Detected\n");
        }
    }

    if (sandbox_testing())
        return 1;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(engine_main_loop, 0, true);
#else
    while (global_game_running) {
        engine_main_loop(NULL);
    }
#endif

    deinitialize();
    return 0;
}

int main(int argc, char** argv) {
    return engine_main(argc, argv);
}
