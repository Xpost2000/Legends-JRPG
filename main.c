/* #define NO_POSTPROCESSING */
/* #define RELEASE */
/* #define NO_FANCY_FADEIN_INTRO */
#define EXPERIMENTAL_VFS

#if 1
#ifndef RELEASE
#define VFS_configuration_prefer_local (true)
#else
#define VFS_configuration_prefer_local (false)
#endif
#else
#define VFS_configuration_prefer_local (false)
#endif

/*
  NOTE: Software is always the primary target,

  the hardware render path is really a "hybrid" path. The main game world is hardware accelerated as well
  as the post processing(since that's the slowest part), however any UI will always be composited on top of the game world.
*/

#ifdef STRETCHYBUFFER_C
#error "Avoid the temptation of easy dynamically allocated arrays! I want to avoid this during runtime!"
#error "Unless I have a genuinely good reason, this will stay here forever!"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
/* EMCC does not allow me to specify -D flags??? WHAT THE FUCK? */
/* so I have to define this crap here... */
/* #define RELEASE */
/* #define NOAUDIO */

#else
#define MULTITHREADED_EXPERIMENTAL
#define USE_SIMD_OPTIMIZATIONS
#endif

#include <SDL2/SDL.h>

#include "common.c"

static bool                global_game_running      = true;
static f32                 global_elapsed_time      = 0.0f;
static f32                 global_elapsed_game_time = 0.0f;
static struct memory_arena scratch_arena            = {};
static struct memory_arena game_arena               = {};

string format_temp_s(const char* fmt, ...) {
    int written = 0;
    char* where;
    {
        va_list args;
        va_start(args, fmt);
        where = (char*)(scratch_arena.memory + scratch_arena.used);
        written = vsnprintf(where, scratch_arena.capacity-(scratch_arena.used+scratch_arena.used_top), fmt, args);
        scratch_arena.used += written+1;
        va_end(args);
    }

    return string_from_cstring_length_counted(where, written);
}

/* target engine res */
local u32 SCREEN_WIDTH  = 0;
local u32 SCREEN_HEIGHT = 0;

local bool SCREEN_IS_FULLSCREEN = false;

/* real res */
/* consider a better way to decouple this from the editor logic. */
/* game logic is okay because we don't use the mouse for UI. (I mean I probably should allow it, but whatever.) */
local u32 REAL_SCREEN_WIDTH  = 640;
local u32 REAL_SCREEN_HEIGHT = 480;
/* local u32 REAL_SCREEN_WIDTH  = 640; */
/* local u32 REAL_SCREEN_HEIGHT = 480; */
/* local u32 REAL_SCREEN_WIDTH  = 1280; */
/* local u32 REAL_SCREEN_HEIGHT = 720; */

local const f32 r16by9Ratio  = 16/9.0f;
local const f32 r16by10Ratio = 16/10.0f;
local const f32 r4by3Ratio   = 4/3.0f;

local bool close_enough_f32(f32 a, f32 b) {
    const f32 EPISILON = 0.001f;

    if ((a - b) <= EPISILON) {
        return true;
    }

    return false;
}

local bool aspect_ratio(void) {
    return SCREEN_WIDTH / SCREEN_HEIGHT;
}

local bool is_16by9(void) {
    return close_enough_f32(r16by9Ratio, aspect_ratio());
}
local bool is_16by10(void) {
    return close_enough_f32(r16by10Ratio, aspect_ratio());
}
local bool is_4by3(void) {
    return close_enough_f32(r4by3Ratio, aspect_ratio());
}

local bool is_widescreen_resolution(void) {
    return is_16by10() || is_16by9();
}

#include "thread_pool.c"
#include "serializer.c"
#include "lisp_read.c"
#include "input.c"
#include "sdl_scancode_table.c"
#include "graphics.c"

#ifdef NOAUDIO
#include "audio_none.c"
#else
#include "audio_sdl2_mixer.c"
#endif

#include "bigfilemaker/bigfile_unpacker.c"

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
/* an exact duplicate of the existing framebuffer, which is used for cross-fading. This is extremely expensive! */
static struct software_framebuffer global_copy_framebuffer      = {};
static struct lightmask_buffer     global_lightmask_buffer      = {};
static struct graphics_assets      graphics_assets              = {};

local void set_window_transparency(f32 transparency) {
    SDL_SetWindowOpacity(global_game_window, transparency);
}

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

void register_controller_down(s32 which, s32 button) {
    SDL_GameController* controller = global_controller_devices[which];

    if (controller) {
        for (s32 controller_index = 0; controller_index < 4; ++controller_index) {
            if (global_controllers[controller_index]._internal_controller_handle == controller) {
                global_controllers[controller_index].buttons_that_received_events[button] = true;
                return;
            }
        }
    }
}

void controller_rumble(struct game_controller* controller, f32 x_magnitude, f32 y_magnitude, u32 ms) {
    SDL_GameController* sdl_controller = controller->_internal_controller_handle;
    x_magnitude                        = clamp_f32(x_magnitude, 0, 1);
    y_magnitude                        = clamp_f32(y_magnitude, 0, 1);
    SDL_GameControllerRumble(sdl_controller, (0xFFFF * x_magnitude), (0xFFFF * y_magnitude), ms);
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

                const f32 DEADZONE_X = 0.03;
                const f32 DEADZONE_Y = 0.03;
                if (fabs(axis_x) < DEADZONE_X) axis_x = 0;
                if (fabs(axis_y) < DEADZONE_Y) axis_y = 0;

                gamepad->left_stick.axes[0] = axis_x;
                gamepad->left_stick.axes[1] = axis_y;
            }

            {
                f32 axis_x = (f32)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) / (32767.0f);
                f32 axis_y = (f32)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) / (32767.0f);

                const f32 DEADZONE_X = 0.03;
                const f32 DEADZONE_Y = 0.03;
                if (fabs(axis_x) < DEADZONE_X) axis_x = 0;
                if (fabs(axis_y) < DEADZONE_Y) axis_y = 0;
                
                gamepad->right_stick.axes[0] = axis_x;
                gamepad->right_stick.axes[1] = axis_y;
            }
        }
    }
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
    software_framebuffer_finish(&global_copy_framebuffer);
    software_framebuffer_finish(&global_default_framebuffer);
    lightmask_buffer_finish(&global_lightmask_buffer);

    v2f32 framebuffer_resolution = get_scaled_screen_resolution(v2f32(REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT));

    /* I know these sound like constants. They really aren't... */
    SCREEN_WIDTH  = framebuffer_resolution.x;
    SCREEN_HEIGHT = 480;


    /* TODO: Seems to be a variable crash sometimes. Check this later. */
    _debugprintf("framebuffer resolution is: (%d, %d) vs (%d, %d) real resolution", SCREEN_WIDTH, SCREEN_HEIGHT, REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT);
    global_default_framebuffer = software_framebuffer_create(framebuffer_resolution.x, framebuffer_resolution.y);
    global_copy_framebuffer    = software_framebuffer_create(framebuffer_resolution.x, framebuffer_resolution.y);
    global_lightmask_buffer    = lightmask_buffer_create(framebuffer_resolution.x, framebuffer_resolution.y);

    if (global_game_texture_surface) {
        SDL_DestroyTexture(global_game_texture_surface);
        global_game_texture_surface = NULL;
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
                /* why is emscripten so painful to work with?? */
                local string window_event_string_table[] = {
                    [SDL_WINDOWEVENT_NONE] = string_literal("NONE"),
                    [SDL_WINDOWEVENT_SHOWN] = string_literal("SHOWN"),
                    [SDL_WINDOWEVENT_HIDDEN] = string_literal("HIDDEN"),
                    [SDL_WINDOWEVENT_EXPOSED] = string_literal("EXPOSED"),
                    [SDL_WINDOWEVENT_MOVED] = string_literal("MOVED"),
                    [SDL_WINDOWEVENT_RESIZED] = string_literal("RESIZED"),
                    [SDL_WINDOWEVENT_SIZE_CHANGED] = string_literal("SIZE_CHANGED"),
                    [SDL_WINDOWEVENT_MINIMIZED] = string_literal("MINIMIZED"),
                    [SDL_WINDOWEVENT_MAXIMIZED] = string_literal("MAXIMIZED"),
                    [SDL_WINDOWEVENT_ENTER] = string_literal("ENTER"),
                    [SDL_WINDOWEVENT_LEAVE] = string_literal("LEAVE"),
                    [SDL_WINDOWEVENT_FOCUS_GAINED] = string_literal("FOCUS_GAINED"),
                    [SDL_WINDOWEVENT_FOCUS_LOST] = string_literal("FOCUS_LOST"),
                    [SDL_WINDOWEVENT_CLOSE] = string_literal("CLOSE"),
                    [SDL_WINDOWEVENT_TAKE_FOCUS] = string_literal("TAKE_FOCUS"),
                    [SDL_WINDOWEVENT_HIT_TEST] = string_literal("HIT_TEST"),
                    [SDL_WINDOWEVENT_ICCPROF_CHANGED] = string_literal("ICCPROF_CHANGED"),
                    [SDL_WINDOWEVENT_DISPLAY_CHANGED] = string_literal("DISPLAY_CHANGED"),
                };
                case SDL_WINDOWEVENT: {
                    _debugprintf("SDL window event? (%.*s)", window_event_string_table[current_event.window.event].length, window_event_string_table[current_event.window.event].data);
                    switch (current_event.window.event) {
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED: {
                            _debugprintf("Size change event... Reevaluating framebuffers");
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

                case SDL_CONTROLLERBUTTONUP:
                case SDL_CONTROLLERBUTTONDOWN: {
                    s32 controller_id  = current_event.cbutton.which;
                    u8  button_pressed = current_event.cbutton.button;
                    u8  button_state   = current_event.cbutton.state;

                    if (button_state == SDL_PRESSED) {
                        register_controller_down(controller_id, button_pressed);
                    }
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

    audio_initialize();

    u32 flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI;
#ifdef __EMSCRIPTEN__
    flags &= ~(SDL_WINDOW_HIDDEN);
    flags |= SDL_WINDOW_RESIZABLE;
#endif

    global_game_window          = SDL_CreateWindow("Legends RPG",
                                                   SDL_WINDOWPOS_CENTERED,
                                                   SDL_WINDOWPOS_CENTERED,
                                                   REAL_SCREEN_WIDTH,
                                                   REAL_SCREEN_HEIGHT,
                                                   flags);

#ifdef __EMSCRIPTEN__
    /* toggle_fullscreen(); */
#endif

    global_game_sdl_renderer    = SDL_CreateRenderer(global_game_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    /* global_game_sdl_renderer    = SDL_CreateRenderer(global_game_window, -1, SDL_RENDERER_ACCELERATED); */

    initialize_thread_pool();

#ifndef NO_FANCY_FADEIN_INTRO
    set_window_transparency(0);
#endif
    SDL_ShowWindow(global_game_window);

    game_initialize();
    initialize_framebuffer();
}

void deinitialize(void) {
    synchronize_and_finish_thread_pool();
    lightmask_buffer_finish(&global_lightmask_buffer);
    software_framebuffer_finish(&global_copy_framebuffer);
    software_framebuffer_finish(&global_default_framebuffer);
    audio_deinitialize();
    game_deinitialize();

    close_all_controllers();

    SDL_Quit();
    _memory_arena_peak_usages(&game_arena);
    _memory_arena_peak_usages(&scratch_arena);
    _memory_arena_peak_usages(&save_arena);
#ifdef USE_EDITOR
    _memory_arena_peak_usages(&editor_arena);
#endif
    _debugprintf("Peak allocations at: %s", memory_strings(system_heap_peak_allocated_amount()));
    if (system_heap_memory_leak_check())
        _debugprintf("no leaked memory");
    else
        _debugprintf("leaked memory somewhere: %zu bytes", system_heap_currently_allocated_amount());
    /* assertion(system_heap_memory_leak_check()); */
}

#include "sandbox_tests.c"

f32 last_elapsed_delta_time = (1.0 / 60.0f);

void engine_main_loop() {
    char window_name_title_buffer[256] = {};
    u32 start_frame_time = SDL_GetTicks();

    begin_input_frame();
    {
        handle_sdl_events();
        update_all_controller_inputs();

        local bool _did_window_intro_fade_in   = false;
        local f32  _window_intro_fade_in_timer = 0;

#ifdef NO_FANCY_FADEIN_INTRO
        _did_window_intro_fade_in = true;
#endif

        if (!_did_window_intro_fade_in) {
            const f32 MAX_INTRO_FADE_IN_TIMER = 0.4;
            f32 effective_frametime = last_elapsed_delta_time;
            if (effective_frametime >= 1.0/45.0f) {
                effective_frametime = 1.0/45.0f;
            }
            _window_intro_fade_in_timer += effective_frametime;

            f32 alpha = _window_intro_fade_in_timer / MAX_INTRO_FADE_IN_TIMER;
            if (alpha > 1) alpha = 1;

            set_window_transparency(alpha);

            if (_window_intro_fade_in_timer >= MAX_INTRO_FADE_IN_TIMER) {
                _did_window_intro_fade_in = true;
            }
        } else {
            lightmask_buffer_clear(&global_lightmask_buffer);
            update_and_render_game(&global_default_framebuffer, last_elapsed_delta_time);
        }
    }

    end_input_frame();

    swap_framebuffers_onto_screen();

    last_elapsed_delta_time = (SDL_GetTicks() - start_frame_time) / 1000.0f;
#ifdef __EMSCRIPTEN__
    _debugprintf("dt %f", last_elapsed_delta_time);
#endif
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
#ifdef _WIN32
    SetProcessDPIAware();
#endif
    initialize();
    
    /* feature testing */
    _debugprintf("endian: %.*s", endian_strings[system_get_endian()].length, endian_strings[system_get_endian()].data);
    {
        _debugprintf("%d byte sized cache line\n", SDL_GetCPUCacheLineSize());
        _debugprintf("%d MB of ram\n", SDL_GetSystemRAM());
        if (SDL_HasMMX()) {
            _debugprintf("MMX Detected\n");
        }

        if (SDL_HasSSE()) {
           _debugprintf("SSE Detected\n");
        }

        if (SDL_HasAVX()) {
            _debugprintf("AVX Detected\n");
        }

        if (SDL_HasSSE2()) {
           _debugprintf("SSE2 Detected\n");
        }

        if (SDL_HasSSE3()) {
           _debugprintf("SSE2 Detected\n");
        }

        if (SDL_HasSSE41() || SDL_HasSSE42()) {
           _debugprintf("SSE4 Detected\n");
        }
    }

    if (sandbox_testing())
        return 1;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(engine_main_loop, 0, true);
#else
    bool finished_fade_out_intro = false;
    f32 fade_out_intro_timer     = 0;

#ifdef NO_FANCY_FADEIN_INTRO
    finished_fade_out_intro = true;
#endif

    while (global_game_running || !finished_fade_out_intro) {
        engine_main_loop();

        if (!global_game_running) {
            const f32 MAX_INTRO_FADE_OUT_TIMER = 0.2756;
            fade_out_intro_timer += last_elapsed_delta_time;

            f32 alpha = 1 - (fade_out_intro_timer/MAX_INTRO_FADE_OUT_TIMER);
            if (alpha < 0) alpha = 0;

            set_window_transparency(alpha);

            if (fade_out_intro_timer >= MAX_INTRO_FADE_OUT_TIMER) {
                finished_fade_out_intro = true;
            }
        }
    }

#endif

    deinitialize();
    return 0;
}

int main(int argc, char** argv) {
    return engine_main(argc, argv);
}
