#include "assets.h"
#include "renderer.h"
#include "audio.h"

typedef struct Core
{
    Platform *platform;
    f32 delta_t;
    
    Renderer renderer;
    Audio audio;
    f32 render_w;
    f32 render_h;
    
    f32 state_change_transition;
    i32 state_type;
    i32 next_state_type;
    void *state_memory;
}
Core;

global Core *core = 0;
global Platform *platform = 0;

#include "assets.c"
#include "debug.c"
#include "renderer.c"
#include "audio.c"
#include "state.c"

internal void
GameInit(Platform *platform_)
{
    Assert(sizeof(Core) <= platform_->permanent_storage_size);
    
    core = platform_->permanent_storage;
    core->platform = platform_;
    platform = platform_;
    core->delta_t = 1.f / FRAMES_PER_SECOND;
    core->state_type = STATE_title;
    core->next_state_type = STATE_null;
    core->state_memory = (u8 *)platform->permanent_storage + sizeof(Core);
    core->state_change_transition = 1.f;
    AudioInit(&core->audio);
    RendererInit(&core->renderer);
    StateInit(core->state_type, core->state_memory);
}

internal void
GameUpdate(void)
{
    core->render_w = 1280;
    core->render_h = 720;
    
    RendererBeginFrame(&core->renderer, core->render_w, core->render_h);
    StateUpdate(core->state_type, core->state_memory);
    RendererEndFrame(&core->renderer);
    
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
        // TODO(rjf): Draw transition thing
    }
    
    AudioUpdate(&core->audio);
    
    // NOTE(rjf): Reset input stuff that should be reset.
    {
        for(i32 i = 0; i < KEY_MAX; ++i)
        {
            platform->key_pressed[i] = 0;
        }
    }
}