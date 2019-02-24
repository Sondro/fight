typedef struct Box
{
    union
    {
        v2 position;
        struct
        {
            f32 x;
            f32 y;
        };
    };
    union
    {
        v2 size;
        struct
        {
            f32 w;
            f32 h;
        };
    };
    v2 velocity;
}
Box;

#define Box(x, y, w, h) BoxInit(x, y, w, h)
internal Box
BoxInit(f32 x, f32 y, f32 w, f32 h)
{
    Box box = { x, y, w, h};
    return box;
}

internal b32
BoxesIntersect(Box a, Box b, v2 *resolve_vector_ptr)
{
    b32 result = 0;
    if(a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y)
    {
        result = 1;
        
        if(resolve_vector_ptr)
        {
            b32 resolve_vector_set = 0;
            v2 resolve_vector = {0};
            
            if(a.x + a.w/2 < b.x + b.w/2)
            {
                resolve_vector.x = (b.x) - (a.x + a.w);
            }
            else
            {
                resolve_vector.x = (b.x + b.w) - (a.x);
            }
            
            if(a.y + a.h/2 < b.y + b.h/2)
            {
                resolve_vector.y = (b.y) - (a.y + a.h);
            }
            else
            {
                resolve_vector.y = (b.y + b.h) - (a.y);
            }
            
            if(AbsoluteValueF(resolve_vector.x) < AbsoluteValueF(resolve_vector.y))
            {
                resolve_vector.y = 0;
            }
            else
            {
                resolve_vector.x = 0;
            }
            
            *resolve_vector_ptr = resolve_vector;
        }
    }
    return result;
}

internal void
CollideBoxWithStaticBox(Box *ptr_to_box, Box static_box)
{
    Box box = *ptr_to_box;
    {
        v2 resolve_vector;
        if(BoxesIntersect(box, static_box, &resolve_vector))
        {
            box.position.x += resolve_vector.x;
            box.position.y += resolve_vector.y;
            box.velocity.x += resolve_vector.x / core->delta_t;
            box.velocity.y += resolve_vector.y / core->delta_t;
        }
    }
    *ptr_to_box = box;
}

typedef struct Player
{
    Box box;
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
    camera->position.x = camera->center_position.x - core->render_w/2;
    camera->position.y = camera->center_position.y - core->render_h/2;
}

internal void
CameraTargetPlayerMidpoint(Camera *camera, Player *players, u32 player_count)
{
    f32 x_sum = 0.f;
    f32 y_sum = 0.f;
    
    for(u32 i = 0; i < player_count; ++i)
    {
        x_sum += players[i].box.x + players[i].box.w/2;
        y_sum += players[i].box.y + players[i].box.h/2;
    }
    
    camera->target.x = x_sum / player_count;
    camera->target.y = y_sum / player_count;
}

typedef struct GameState
{
    Camera camera;
    Player players[2];
    Box ground;
}
GameState;

internal void
GameStateInit(GameState *state)
{
    state->players[0].box = Box(-256, -256, 48, 64);
    state->players[0].box.velocity = v2(0, 0);
    state->players[0].color = v3(1, 0, 0);
    
    state->players[1].box = Box(256-48, -256, 48, 64);
    state->players[1].box.velocity = v2(0, 0);
    state->players[1].color = v3(1, 0, 1);
    
    CameraTargetPlayerMidpoint(&state->camera, state->players, ArrayCount(state->players));
    state->camera.target.y -= 1000;
    state->camera.center_position = state->camera.target;
    
    state->ground = Box(-512, 96, 1024, 1024);
}

internal void
GameStateCleanUp(GameState *state)
{
    
}

internal void
GameStateUpdate(GameState *state)
{
    if(platform->key_pressed[KEY_escape])
    {
        core->next_state_type = STATE_title;
    }
    
    for(int i = 0; i < ArrayCount(state->players); ++i)
    {
        
        enum
        {
            PLAYER_CONTROL_move_right,
            PLAYER_CONTROL_move_left,
            PLAYER_CONTROL_MAX
        };
        
        // NOTE(rjf): Player 1 input
        if(i == 0) {
            b32 controls[] = {
                !!platform->key_down[KEY_d],
                !!platform->key_down[KEY_a],
            };
            
            if(controls[PLAYER_CONTROL_move_right])
            {
                state->players[i].box.velocity.x += (750 - state->players[i].box.velocity.x) * core->delta_t * 16.f;
            }
            
            else if(controls[PLAYER_CONTROL_move_left])
            {
                state->players[i].box.velocity.x += (-750 - state->players[i].box.velocity.x) * core->delta_t * 16.f;
            }
            
            if(platform->key_pressed[KEY_w])
            {
                state->players[i].box.velocity.y = -500;
            }
            
            if(platform->key_down[KEY_s])
            {
                if((i32)state->players[i].box.size.y == 64)
                {
                    state->players[i].box.y += 32;
                    state->players[i].box.size.y = 32;
                }
            }
            else
            {
                if((i32)state->players[i].box.size.y == 32)
                {
                    state->players[i].box.y -= 32;
                    state->players[i].box.size.y = 64;
                }
            }
            
        }
        
        state->players[i].box.velocity.x -= state->players[i].box.velocity.x * core->delta_t * 16.f;
        state->players[i].box.velocity.y += 1000 * core->delta_t;
        
        CollideBoxWithStaticBox(&state->players[i].box, state->ground);
        
        state->players[i].box.x += state->players[i].box.velocity.x * core->delta_t;
        state->players[i].box.y += state->players[i].box.velocity.y * core->delta_t;
    }
    
    CameraTargetPlayerMidpoint(&state->camera, state->players, ArrayCount(state->players));
    CameraUpdate(&state->camera);
    
    // TODO(rjf): Render everything
    {
        
    }
}