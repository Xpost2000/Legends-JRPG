#ifndef CAMERA_DEF_C
#define CAMERA_DEF_C

/* NOTE/TODO For now our camera isn't really intelligent */
/* I'm tired of having a distinction between the view camera, and a game camera
   which is basically the same struct but *smarter*. I'm just going to shove all camera code in here... */

#define MAX_CAMERA_TRAUMA (2)

#define MAX_TRAUMA_DISPLACEMENT_X (40)
#define MAX_TRAUMA_DISPLACEMENT_Y (40)

struct camera {
    union {
        struct {
            v2f32 xy; /* rename to position */
            f32   zoom;
        };
        f32 components[3];
    };

    u8    centered;

    /*
      NOTE:
      The region size where the camera may start to readjust to the player.
      
      Only used for the gameplay world camera, and specified in screen coordinates.
      
      Combat camera is free travel.
      
      All this data here is for camera updatings, only used by the game.
      Not the editor code.
      
      Would technically be a struct game_camera or something.
    */
    struct rectangle_f32 travel_bounds;
    union {
        struct {
            v2f32                tracking_xy;
            f32 tracking_zoom;
        };
        f32 tracking_components[3];
    };

    /* NOTE x/y/zoom */
    f32                  interpolation_t[3];
    f32                  start_interpolation_values[3];
    bool                 try_interpolation[3];

    f32 trauma;
};

void camera_traumatize(struct camera* camera, f32 trauma) {
    camera->trauma += trauma;

    if (camera->trauma >= MAX_CAMERA_TRAUMA) {
        camera->trauma = MAX_CAMERA_TRAUMA;
    }
}

void camera_set_point_to_interpolate(struct camera* camera, v2f32 point) {
    camera->interpolation_t[0] = 0;
    camera->interpolation_t[1] = 0;
    camera->try_interpolation[0] = true;
    camera->try_interpolation[1] = true;
    camera->start_interpolation_values[0] = camera->xy.x;
    camera->start_interpolation_values[1] = camera->xy.y;

    camera->tracking_xy = point;
}

v2f32 camera_transform(struct camera* camera, v2f32 point, s32 screen_width, s32 screen_height) {
    point.x *= camera->zoom;
    point.y *= camera->zoom;

    if (camera->centered) {
        point.x += screen_width/2;
        point.y += screen_height/2;
    }

    point.x -= camera->xy.x;
    point.y -= camera->xy.y;

    return point;
}
struct rectangle_f32 camera_transform_rectangle(struct camera* camera, struct rectangle_f32 rectangle, s32 screen_width, s32 screen_height) {
    v2f32 rectangle_position = v2f32(rectangle.x, rectangle.y);
    rectangle_position       = camera_transform(camera, rectangle_position, screen_width, screen_height);
    rectangle.x              = rectangle_position.x;
    rectangle.y              = rectangle_position.y;
    rectangle.w             *= camera->zoom;
    rectangle.h             *= camera->zoom;

    return rectangle;
}

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
    point.x += camera->xy.x;
    point.y += camera->xy.y;

    if (camera->centered) {
        point.x -= (screen_width / 2);
        point.y -= (screen_height / 2);
    }

    point.x /= camera->zoom;
    point.y /= camera->zoom;
#endif
    return point;
}

struct rectangle_f32 camera_project_rectangle(struct camera* camera, struct rectangle_f32 rectangle, s32 screen_width, s32 screen_height) {
    v2f32 rectangle_position = v2f32(rectangle.x, rectangle.y);
    rectangle_position       = camera_project(camera, rectangle_position, screen_width, screen_height);
    rectangle.x              = rectangle_position.x;
    rectangle.y              = rectangle_position.y;
    rectangle.w             /= camera->zoom;
    rectangle.h             /= camera->zoom;

    return rectangle;
}

#define camera(XY, ZOOM) (struct camera) { .xy=XY, .zoom=ZOOM }
#define camera_centered(XY, ZOOM) (struct camera) { .xy=XY, .centered=true, .zoom=ZOOM }
#endif
