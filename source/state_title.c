typedef struct TitleState
{
    int hi;
}
TitleState;

internal void
TitleStateInit(TitleState *state)
{
    
}

internal void
TitleStateCleanUp(TitleState *state)
{
    
}

internal void
TitleStateUpdate(TitleState *state)
{
    DrawFilledRectangle(v2(32, 32), v2(128, 128), v3(1, 0, 0));
    
    if(platform->key_pressed[KEY_a])
    {
        core->next_state_type = STATE_game;
    }
    
}
