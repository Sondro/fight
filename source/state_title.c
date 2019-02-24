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
    
    if(platform->key_pressed[KEY_enter])
    {
        core->next_state_type = STATE_game;
    }
    
}
