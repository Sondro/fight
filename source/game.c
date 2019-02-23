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
    DrawFilledCircle(v2(platform->mouse_x, platform->mouse_y), 128, v3(1, 0, 0));
}