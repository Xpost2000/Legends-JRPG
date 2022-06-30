#ifndef CAMERA_DEF_C
#define CAMERA_DEF_C

/* NOTE/TODO For now our camera isn't really intelligent */
/* I'm tired of having a distinction between the view camera, and a game camera
   which is basically the same struct but *smarter*. I'm just going to shove all camera code in here... */
struct camera {
    v2f32 xy;
    u8    centered;
    f32   zoom;
};

v2f32 camera_project(struct camera* camera, v2f32 point, s32 screen_width, s32 screen_height) {
#if 0
    point.x *= camera->zoom;
    point.y *= camera->zoom;

    if (camera->centered) {
        point.x += screen_width / 2;
        point.y += screen_height / 2;
    }

    point.x -= camera->xy.x;
    point.y -= camera->xy.y;
#else
    point.x += camera->xy.x * camera->zoom;
    point.y += camera->xy.y * camera->zoom;

    if (camera->centered) {
        point.x -= screen_width / 2;
        point.y -= screen_height / 2;
        point.x /= camera->zoom;
        point.y /= camera->zoom;
    }
#endif
    return point;
}

#define camera(XY, ZOOM) (struct camera) { .xy=XY, .zoom=ZOOM }
#define camera_centered(XY, ZOOM) (struct camera) { .xy=XY, .centered=true, .zoom=ZOOM }
#endif
