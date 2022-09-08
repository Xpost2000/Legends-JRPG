#ifndef WEATHER_DEF_C
#define WEATHER_DEF_C

#define WEATHER_FIXED_UPDATE_RATE (60) /* in FPS */
/* mostly for screen displays, actual other weather effects would be a bit more complicated
   and require more renderer outreach. Like a skew matrix. I can hard-code special rendering procedures
   for this, but... Oh you know what whatever, I'm unlikely to do hardware acceleration since that's just
   the games' engine special gimmick. */
enum weather_features {
    WEATHER_NONE = 0,
    WEATHER_RAIN = BIT(0),
    WEATHER_FOGGY = BIT(1),
    WEATHER_SNOW = BIT(2),
    WEATHER_STORMY = BIT(3),
    WEATHER_COUNT = 4,
};
static string weather_strings[] = {
    string_literal("(none)"),
    string_literal("(rain)"),
    string_literal("(fog)"),
    string_literal("(snow)"),
    string_literal("(stormy)"),
};

#define MAX_WEATHER_PARTICLES (512)
struct weather {
    u32 features;

    v2f32 snow_positions[MAX_WEATHER_PARTICLES];
    v2f32 rain_positions[MAX_WEATHER_PARTICLES];
    
    f32 timer;
};

#endif
