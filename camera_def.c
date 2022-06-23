#ifndef CAMERA_DEF_C
#define CAMERA_DEF_C

/* NOTE/TODO For now our camera isn't really intelligent */
struct camera {
    v2f32 xy;
    u8    centered;
#if 1
    f32   zoom;
#endif
};

#define camera(XY, ZOOM) (struct camera) { .xy=XY, .zoom=ZOOM }
#define camera_centered(XY, ZOOM) (struct camera) { .xy=XY, .centered=true, .zoom=ZOOM }
#endif
