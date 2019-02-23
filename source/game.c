typedef struct Core
{
    Platform *platform;
    f32 delta_t;
}
Core;

global Core *core = 0;
global Platform *platform = 0;

#include "render.c"

internal void
GameInit(Platform *platform_)
{
    core = platform_->permanent_storage;
    core->platform = platform_;
    platform = platform_;
    core->delta_t = 1.f / FRAMES_PER_SECOND;
}

internal void
GameUpdate(void)
{
    ClearBackbuffer();
    DrawFilledRectangle(v2(32, 32), v2(128, 128), v3(1, 0, 0));
    
    if(platform->key_down[KEY_a])
    {
        DrawFilledCircle(v2(platform->mouse_x, platform->mouse_y), 128, v3(1, 0, 0));
    }
    
    // NOTE(rjf): Reset input stuff that should be reset.
    {
        for(i32 i = 0; i < KEY_MAX; ++i)
        {
            platform->key_pressed[i] = 0;
        }
    }
}