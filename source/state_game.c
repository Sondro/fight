
typedef struct AttackType
{
    f32 start_up;
    v2 size;
    f32 duration;
    f32 recovery;
    f32 strength;
}
AttackType;

enum
{
    ATTACK_TYPE_jab,
    ATTACK_TYPE_ftilt,
};

global AttackType global_attack_types[] = {
    
    // NOTE(rjf): Jab
    {
        0.07f,
        { 16, 16 },
        0.034f,
        0.034f,
        0.2f,
    },
    
    // NOTE(rjf): Forward Tilt
    {
        0.2f,
        { 24, 24 },
        0.08f,
        0.2f,
        0.6f,
    },
    
};

enum
{
    ATTACK_STAGE_start_up,
    ATTACK_STAGE_active,
    ATTACK_STAGE_recovery,
};

typedef struct Attack
{
    i32 stage;
    f32 start_up_time_left;
    v2 size;
    f32 duration_left;
    f32 recovery_left;
    f32 strength;
}
Attack;

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
    for(f32 t = 0.1f; t <= 1.f; t += 0.1f)
    {
        Box box = *ptr_to_box;
        box.x += box.velocity.x * core->delta_t * t;
        box.y += box.velocity.y * core->delta_t * t;
        v2 resolve_vector;
        
        if(BoxesIntersect(box, static_box, &resolve_vector))
        {
            box.position.x += resolve_vector.x;
            box.position.y += resolve_vector.y;
            box.velocity.x += resolve_vector.x / (core->delta_t / t);
            box.velocity.y += resolve_vector.y / (core->delta_t / t);
            *ptr_to_box = box;
            break;
        }
    }
}

typedef struct Player
{
    f32 health;
    f32 health_target;
    Box box;
    v3 color;
    i32 direction;
    Attack attack;
}
Player;

typedef struct HitBox
{
    Box box;
    f32 strength;
    i32 origin_index;
}
HitBox;

internal void
CollideBoxWithHitBoxes(i32 origin_index, f32 *health, Box *ptr_to_box, HitBox *boxes, u32 box_count)
{
    Box check_box = *ptr_to_box;
    for(u32 i = 0; i < box_count; ++i)
    {
        Box hit_box = boxes[i].box;
        if(origin_index != boxes[i].origin_index &&
           BoxesIntersect(check_box, hit_box, 0))
        {
            v2 hit_vector = V2Subtract(v2(check_box.x + check_box.w/2, check_box.y + check_box.h/2),
                                       v2(hit_box.x + hit_box.w/2, hit_box.y + hit_box.h/2)
                                       );
            
            check_box.velocity.x += boxes[i].strength * (hit_vector.x * AbsoluteValueF(boxes[i].box.velocity.x) / 3.f);
            check_box.velocity.y += boxes[i].strength * (hit_vector.y * AbsoluteValueF(boxes[i].box.velocity.y) / 3.f);
            
            *health -= 0.1f;
        }
    }
    *ptr_to_box = check_box;
}

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

#define MAX_HIT_BOX 32

typedef struct GameState
{
    Camera camera;
    Player players[2];
    u32 hit_box_count;
    HitBox hit_boxes[MAX_HIT_BOX];
    Box ground;
}
GameState;

internal void
PushHitBox(GameState *state, v2 pos, v2 size, v2 velocity, f32 strength, i32 origin_index)
{
    Assert(state->hit_box_count < ArrayCount(state->hit_boxes));
    HitBox box = {
        {
            pos,
            size,
            velocity,
        },
        strength,
    };
    state->hit_boxes[state->hit_box_count++] = box;
}

internal void
ExecuteAttack(Attack *attack, i32 attack_type)
{
    attack->stage = ATTACK_STAGE_start_up;
    attack->start_up_time_left = global_attack_types[attack_type].start_up;
    attack->size = global_attack_types[attack_type].size;
    attack->duration_left = global_attack_types[attack_type].duration;
    attack->recovery_left = global_attack_types[attack_type].recovery;
    attack->strength = global_attack_types[attack_type].strength;
}

internal void
LoadPlayerInput(GameState *state, Player *player, i32 index)
{
    
    enum
    {
        CONTROL_move_right,
        CONTROL_move_left,
        CONTROL_jump,
        CONTROL_crouch,
        CONTROL_attack,
        CONTROL_MAX
    };
    
    b32 controls[CONTROL_MAX] = {0};
    
    // NOTE(rjf): Player index 1
    if(index == 0)
    {
        controls[CONTROL_move_right] = !!platform->key_down[KEY_d];
        controls[CONTROL_move_left] = !!platform->key_down[KEY_a];
        controls[CONTROL_jump] = !!platform->key_down[KEY_w];
        controls[CONTROL_crouch] = !!platform->key_down[KEY_s];
        controls[CONTROL_attack] = !!platform->key_down[KEY_e];
    }
    // NOTE(rjf): Player index 2 (I guess?)
    else
    {
        controls[CONTROL_move_right] = !!platform->key_down[KEY_right];
        controls[CONTROL_move_left] = !!platform->key_down[KEY_left];
        controls[CONTROL_jump] = !!platform->key_down[KEY_up];
        controls[CONTROL_crouch] = !!platform->key_down[KEY_down];
        controls[CONTROL_attack] = !!platform->key_down[KEY_l];
    }
    
    if(controls[CONTROL_move_right])
    {
        player->box.velocity.x += (1000 - player->box.velocity.x) * core->delta_t * 16.f;
    }
    
    else if(controls[CONTROL_move_left])
    {
        player->box.velocity.x += (-1000 - player->box.velocity.x) * core->delta_t * 16.f;
    }
    
    if(controls[CONTROL_jump])
    {
        player->box.velocity.y = -700;
    }
    
    if(controls[CONTROL_crouch])
    {
        if((i32)player->box.size.y == 64)
        {
            player->box.y += 32;
            player->box.size.y = 32;
        }
    }
    else
    {
        if((i32)player->box.size.y == 32)
        {
            player->box.y -= 32;
            player->box.size.y = 64;
        }
    }
    
    if(controls[CONTROL_attack])
    {
        ExecuteAttack(&player->attack, ATTACK_TYPE_jab);
    }
}

internal void
GameStateInit(GameState *state)
{
    state->players[0].box = Box(-256, -256, 48, 64);
    state->players[0].box.velocity = v2(0, 0);
    state->players[0].color = v3(0.7f, 0.3f, 0.3f);
    
    state->players[1].box = Box(256-48, -256, 48, 64);
    state->players[1].box.velocity = v2(0, 0);
    state->players[1].color = v3(0.6f, 0.2f, 0.7f);
    
    state->hit_box_count = 0;
    
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
    
    state->hit_box_count = 0;
    
    for(int i = 0; i < ArrayCount(state->players); ++i)
    {
        LoadPlayerInput(state, &state->players[i], i);
        
        state->players[i].box.velocity.x -= state->players[i].box.velocity.x * core->delta_t * 16.f;
        state->players[i].box.velocity.y += 2000 * core->delta_t;
        
        CollideBoxWithStaticBox(&state->players[i].box, state->ground);
        CollideBoxWithHitBoxes((i32)i, &state->players[i].health,
                               &state->players[i].box, state->hit_boxes, state->hit_box_count);
        
        state->players[i].box.x += state->players[i].box.velocity.x * core->delta_t;
        state->players[i].box.y += state->players[i].box.velocity.y * core->delta_t;
    }
    
    CameraTargetPlayerMidpoint(&state->camera, state->players, ArrayCount(state->players));
    CameraUpdate(&state->camera);
    
    // NOTE(rjf): Render everything
    {
        RendererPushFilledRect(&core->renderer,
                               V2Subtract(state->ground.position, state->camera.position),
                               state->ground.size,
                               v4(0.3f, 0.3f, 0.3f, 1.f),
                               v4(0.1f, 0.1f, 0.1f, 1.f),
                               v4(0.3f, 0.3f, 0.3f, 1.f),
                               v4(0.1f, 0.1f, 0.1f, 1.f));
        
        for(u32 i = 0; i < ArrayCount(state->players); ++i)
        {
            RendererPushFilledRectS(&core->renderer,
                                    V2Subtract(state->players[i].box.position, state->camera.position),
                                    state->players[i].box.size,
                                    v4(state->players[i].color.r, state->players[i].color.g, state->players[i].color.b, 1.f));
        }
        
        for(u32 i = 0; i < state->hit_box_count; ++i)
        {
            RendererPushFilledRectS(&core->renderer,
                                    V2Subtract(state->hit_boxes[i].box.position, state->camera.position),
                                    state->hit_boxes[i].box.size,
                                    v4(1, 0, 0, 1));
        }
    }
}