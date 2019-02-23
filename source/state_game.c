typedef struct Player
{
    v2 position;
    v2 velocity;
    v2 box_size;
    v3 color;
}
Player;

typedef struct Camera
{
    v2 position;
    v2 center_position;
    v2 target;
}
Camera;

internal void
CameraUpdate(Camera *camera)
{
    camera->center_position.x += (camera->target.x - camera->center_position.x) * core->delta_t * 16.f;
    camera->center_position.y += (camera->target.y - camera->center_position.y) * core->delta_t * 16.f;
    camera->position.x = camera->center_position.x - platform->backbuffer_width/2;
    camera->position.y = camera->center_position.y - platform->backbuffer_height/2;
}

internal void
CameraTargetPlayerMidpoint(Camera *camera, Player *players, u32 player_count)
{
    f32 x_sum = 0.f;
    f32 y_sum = 0.f;
    
    for(u32 i = 0; i < player_count; ++i)
    {
        x_sum += players[i].position.x + players[i].box_size.x/2;
        y_sum += players[i].position.y + players[i].box_size.y/2;
    }
    
    camera->target.x = x_sum / player_count;
    camera->target.y = y_sum / player_count;
}

typedef struct GameState
{
    Camera camera;
    Player players[2];
}
GameState;

internal void
GameStateInit(GameState *state)
{
    state->players[0].position = v2(32, 32);
    state->players[0].velocity = v2(0, 0);
    state->players[0].box_size = v2(64, 80);
    state->players[0].color = v3(1, 0, 0);
    
    state->players[1].position = v2(512, 32);
    state->players[1].velocity = v2(0, 0);
    state->players[1].box_size = v2(64, 80);
    state->players[1].color = v3(1, 0, 1);
    
    CameraTargetPlayerMidpoint(&state->camera, state->players, ArrayCount(state->players));
    state->camera.target.y -= 1000;
    state->camera.center_position = state->camera.target;
}

internal void
GameStateCleanUp(GameState *state)
{
    
}

internal void
GameStateUpdate(GameState *state)
{
    if(platform->key_pressed[KEY_a])
    {
        core->next_state_type = STATE_title;
    }
    
    for(int i = 0; i < ArrayCount(state->players); ++i)
    {
        state->players[i].position.x += state->players[i].velocity.x * core->delta_t;
        state->players[i].position.y += state->players[i].velocity.y * core->delta_t;
    }
    
    CameraTargetPlayerMidpoint(&state->camera, state->players, ArrayCount(state->players));
    CameraUpdate(&state->camera);
    
    // NOTE(rjf): Render everything
    {
        // NOTE(rjf): Draw ground
        DrawFilledRectangle(v2(-512-state->camera.position.x, 96-state->camera.position.y),
                            v2(platform->backbuffer_width+1024, 256),
                            v3(0.3f, 0.3f, 0.3f));
        
        // NOTE(rjf): Draw players
        for(int i = 0; i < ArrayCount(state->players); ++i)
        {
            DrawFilledRectangle(v2(state->players[i].position.x - state->camera.position.x,
                                   state->players[i].position.y - state->camera.position.y),
                                state->players[i].box_size,
                                state->players[i].color);
        }
    }
}