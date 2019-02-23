typedef struct Core
{
    Platform *platform;
    f32 delta_t;
    
    f32 state_change_transition;
    i32 state_type;
    i32 next_state_type;
    void *state_memory;
}
Core;

global Core *core = 0;
global Platform *platform = 0;

#include "render.c"
#include "state.c"

internal void
GameInit(Platform *platform_)
{
    core = platform_->permanent_storage;
    core->platform = platform_;
    platform = platform_;
    core->delta_t = 1.f / FRAMES_PER_SECOND;
    core->state_type = STATE_title;
    core->next_state_type = STATE_null;
    core->state_memory = (u8 *)platform->permanent_storage + sizeof(Core);
    core->state_change_transition = 1.f;
    StateInit(core->state_type, core->state_memory);
}

internal void
GameUpdate(void)
{
    ClearBackbuffer();
    
    StateUpdate(core->state_type, core->state_memory);
    
    if(core->next_state_type == STATE_null)
    {
        core->state_change_transition += (0 - core->state_change_transition) * core->delta_t * 32.f;
    }
    else
    {
        core->state_change_transition += (1 - core->state_change_transition) * core->delta_t * 32.f;
        if(core->state_change_transition > 0.9f)
        {
            StateCleanUp(core->state_type, core->state_memory);
            core->state_type = core->next_state_type;
            core->next_state_type = STATE_null;
            StateInit(core->state_type, core->state_memory);
        }
    }
    
    if(core->state_change_transition > 0.05f)
    {
        DrawFilledRectangle(v2(0, 0), v2(platform->backbuffer_width, platform->backbuffer_height*core->state_change_transition),
                            v3(0, 0, 0));
    }
    
    // NOTE(rjf): Reset input stuff that should be reset.
    {
        for(i32 i = 0; i < KEY_MAX; ++i)
        {
            platform->key_pressed[i] = 0;
        }
    }
}