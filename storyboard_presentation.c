/*
 * A basic type of cutscene

 for text exposition, and some moving image or something.

 Or maybe just a black screen (which is what it will be for now or until I stop
 working on this)
 */


struct storyboard_page {
    string string;
    f32 linger_time;
};

struct {
    s32 page_count; /* strings are expected to pages. */
    struct storyboard_page pages[256];

    s32 current_page;
    s32 character_index;
    f32 character_timer;

    f32 timer;
} storyboard;

local void storyboard_next_page(void) {
    storyboard.current_page    += 1;
    storyboard.character_timer  = 0;
    storyboard.character_index  = 0;
    storyboard.timer            = 0;

    if (storyboard.current_page >= storyboard.page_count) {
        storyboard_active = false;
    }
}
local void start_storyboard(void) {
    storyboard_active          = true;
    storyboard.timer           = 0;
    storyboard.current_page    = 0;
    storyboard.page_count      = 0;
    storyboard.character_timer = 0;
    storyboard.character_index = 0;
}

void open_TEST_storyboard(void) {
#if 0
    start_storyboard();

    {
        struct storyboard_page* n = &storyboard.pages[storyboard.page_count++];
        n->string      = string_literal("Long ago, there was a great war over the home of the Elves.");
        n->linger_time = 1.5;
    }
    {
        struct storyboard_page* n = &storyboard.pages[storyboard.page_count++];
        n->string      = string_literal("The humans came from across the sea. Lead by a warmongering ruler, all that was left in his stead was plumes of smoke and ashes.\nFor many long months it was said that the rings of Elven screams could be heard from coast to coast.");
        n->linger_time = 1.5;
    }
    {
        struct storyboard_page* n = &storyboard.pages[storyboard.page_count++];
        n->string      = string_literal("The Elves prayed to their gods, and were blessed with the magics to defend themselves. Forming a small army and pushing the humans back. Barely managing to do so, due to such small numbers. The damage was irrevocable.\nThough that ruler had long since died, a great animosity between elves and humans would hold for many generations.");
        n->linger_time = 1.5;
    }
    {
        struct storyboard_page* n = &storyboard.pages[storyboard.page_count++];
        n->string      = string_literal("An uneasy peace rested over the land.\n\nIt was not until fate forced a discord to mend the broken trust between kin.\n\nLet us unravel what happened.");
        n->linger_time = 2.5;
    }
#endif
}

/* I'm a bit too lazy to fully animate this right now so I'm just going to blurt some text. We'll animate it more later */
s32 game_display_and_update_storyboard(struct software_framebuffer* framebuffer, f32 dt) {
    if (storyboard_active) {
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,framebuffer->width,framebuffer->height), color32u8(0,0,0,255), BLEND_MODE_ALPHA);

        struct storyboard_page* current_page = &storyboard.pages[storyboard.current_page];

        const f32          NORMAL_FONT_SCALE = 2;
        struct font_cache* font1             = game_get_font(MENU_FONT_COLOR_STEEL);
        {
            if (storyboard.character_index == current_page->string.length-1) {
                if (storyboard.timer >= current_page->linger_time) {
                    storyboard_next_page();
                }

                storyboard.timer += dt;
            } else {
                if (storyboard.character_timer >= STORYBOARD_READ_SPEED) {
                    storyboard.character_timer     = 0;
                    storyboard.character_index += 1;
                }

                f32 modifier = 1;

                if (any_key_down()) {
                    modifier = 2.5f;
                }

                storyboard.character_timer += dt * modifier;
            }
        }

        software_framebuffer_draw_text_bounds(framebuffer, font1, NORMAL_FONT_SCALE, v2f32(10,50), framebuffer->width-10,
                                              string_slice(current_page->string, 0, storyboard.character_index), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        /* draw_ui_breathing_text_centered(framebuffer, */
        /*                                 rectangle_f32(50,50,framebuffer->width-50, framebuffer->height-50), */
        /*                                 font1, */
        /*                                 NORMAL_FONT_SCALE, */
        /*                                 string_slice(current_page->string, 0, storyboard.character_index), 0); */

        return 1;
    } else {
        return 0;
    }
}
