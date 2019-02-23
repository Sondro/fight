typedef struct GameState
{
    int hi;
}
GameState;

internal void
GameStateInit(GameState *state)
{
    
}

internal void
GameStateCleanUp(GameState *state)
{
    
}

internal void
GameStateUpdate(GameState *state)
{
    DrawFilledRectangle(v2(32, 32), v2(128, 128), v3(1, 0, 1));
    
    if(platform->key_pressed[KEY_a])
    {
        core->next_state_type = STATE_title;
    }
    
}
