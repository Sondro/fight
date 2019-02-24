
typedef struct AttackType
{
    f32 start_up;
    v2 size;
    v2 offset;
    v2 directional_offset;
    f32 duration;
    f32 recovery;
    f32 strength;
}
AttackType;

enum
{
    ATTACK_TYPE_jab,
    ATTACK_TYPE_ftilt,
    ATTACK_TYPE_crouch_jab,
    ATTACK_TYPE_crouch_ftilt,
};

global AttackType global_attack_types[] = {
    
    // NOTE(rjf): Jab
    {
        0.07f,
        { 24, 16 },
        { 0, 0 },
        { 28, 0 },
        0.034f,
        0.034f,
        0.4f,
    },
    
    // NOTE(rjf): Forward Tilt
    {
        0.2f,
        { 38, 16 },
        { 0, 0 },
        { 48, 0 },
        0.08f,
        0.2f,
        0.7f,
    },
    
    // NOTE(rjf): Crouch Jab
    {
        0.07f,
        { 48, 8 },
        { 0, 0 },
        { 20, 0 },
        0.034f,
        0.034f,
        0.4f,
    },
    
    // NOTE(rjf): Crouch F-Tilt
    {
        0.2f,
        { 32, 32 },
        { 0, -64 },
        { 8, 0 },
        0.08f,
        0.4f,
        0.7f,
    },
    
};

enum
{
    ATTACK_STAGE_ready,
    ATTACK_STAGE_start_up,
    ATTACK_STAGE_active,
    ATTACK_STAGE_recovery,
};

typedef struct Attack
{
    i32 stage;
    i32 type;
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
    b32 on_ground;
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
            if(resolve_vector.y < 0.f)
            {
                box.on_ground |= 1;
            }
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
    Attack attack;
    i32 direction;
    f32 stun_time;
    Animation animation;
}
Player;

typedef struct HitBox
{
    Box box;
    f32 strength;
    i32 origin_index;
}
HitBox;

typedef struct Camera
{
    v2 position;
    v2 center_position;
    v2 target;
    f32 shake_magnitude;
    f32 shake_sin_pos;
}
Camera;

internal void
CollideBoxWithHitBoxes(i32 origin_index, f32 *health, f32 *stun_time,
                       Box *ptr_to_box, HitBox *boxes, u32 box_count,
                       Sound *hit_sound, Camera *camera)
{
    Box check_box = *ptr_to_box;
    for(u32 i = 0; i < box_count; ++i)
    {
        Box hit_box = boxes[i].box;
        hit_box.x -= hit_box.size.x;
        hit_box.y -= hit_box.size.y;
        hit_box.size.x *= 2;
        hit_box.size.y *= 2;
        if(origin_index != boxes[i].origin_index &&
           BoxesIntersect(check_box, hit_box, 0))
        {
            v2 hit_vector = V2Subtract(v2(check_box.x + check_box.w/2, check_box.y + check_box.h/2),
                                       v2(hit_box.x + hit_box.w/2, hit_box.y + hit_box.h/2)
                                       );
            
            check_box.velocity.x += boxes[i].strength * (hit_vector.x * AbsoluteValueF(boxes[i].box.velocity.x) / 4.f);
            check_box.velocity.y += boxes[i].strength * (hit_vector.y * AbsoluteValueF(boxes[i].box.velocity.y) / 4.f);
            
            *health -= 0.1f;
            *stun_time = 0.3f;
            
            f32 velocity_magnitude = boxes[i].strength * V2LengthSquared(hit_vector) / 700.f;
            
            AudioPlaySoundAtPoint(&core->audio, hit_sound, AUDIO_master,
                                  1-velocity_magnitude,
                                  velocity_magnitude*1.f + RandomF32(-0.2f, 0.2f),
                                  v2(hit_box.x + hit_box.w/2,
                                     hit_box.y + hit_box.h/2));
            
            camera->shake_magnitude += velocity_magnitude;
        }
    }
    *ptr_to_box = check_box;
}

internal void
CameraUpdate(Camera *camera)
{
    camera->center_position.x += (camera->target.x - camera->center_position.x) * core->delta_t * 8.f;
    camera->center_position.y += (camera->target.y - camera->center_position.y) * core->delta_t * 8.f;
    camera->position.x = camera->center_position.x - core->render_w/2;
    camera->position.y = camera->center_position.y - core->render_h/2;
    
    camera->shake_sin_pos += core->delta_t * 16.f;
    camera->position.y += sinf(camera->shake_sin_pos) * 32.f * camera->shake_magnitude;
    camera->shake_magnitude -= camera->shake_magnitude * core->delta_t;
}

internal void
CameraTargetPlayerMidpoint(Camera *camera, Player *players, u32 player_count)
{
    f32 x_sum = 0.f;
    f32 y_sum = 0.f;
    
    for(u32 i = 0; i < player_count; ++i)
    {
        v2 player_center_pos = {
            players[i].box.x + players[i].box.w/2,
            players[i].box.y + players[i].box.h/2,
        };
        
        if(player_center_pos.x >= -1024 && player_center_pos.x <= 1024 &&
           player_center_pos.y >= -1024 && player_center_pos.y <= 1024)
        {
            x_sum += player_center_pos.x;
            y_sum += player_center_pos.y;
        }
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
    Sound jump_sound;
    Sound hit_sound;
    Sound death_hit_sound;
    Sound music_sound;
    AudioSourceID background_music_source;
    
    struct
    {
        Texture texture;
        AnimationType idle;
        AnimationType walk;
        AnimationType stand_block;
        AnimationType crouch;
        AnimationType jump;
        AnimationType hurt;
        AnimationType crouch_block;
        AnimationType jab;
        AnimationType heavy;
        AnimationType crouch_heavy;
        AnimationType crouch_jab;
    } player_texture_data[2];
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
        origin_index,
    };
    state->hit_boxes[state->hit_box_count++] = box;
}

internal void
ExecuteAttack(Attack *attack, i32 attack_type)
{
    attack->stage = ATTACK_STAGE_start_up;
    attack->type = attack_type;
    attack->start_up_time_left = global_attack_types[attack_type].start_up;
    attack->size = global_attack_types[attack_type].size;
    attack->duration_left = global_attack_types[attack_type].duration;
    attack->recovery_left = global_attack_types[attack_type].recovery;
    attack->strength = global_attack_types[attack_type].strength;
}

internal void
AttackUpdate(Attack *attack, i32 direction, i32 origin_index, v2 anchor_pos,
             v2 velocity, GameState *state)
{
    switch(attack->stage)
    {
        
        case ATTACK_STAGE_start_up:
        {
            attack->start_up_time_left -= core->delta_t;
            if(attack->start_up_time_left <= 0.f)
            {
                attack->stage = ATTACK_STAGE_active;
            }
            break;
        }
        
        case ATTACK_STAGE_active:
        {
            attack->duration_left -= core->delta_t;
            PushHitBox(state,
                       V2Add(anchor_pos,
                             v2(global_attack_types[attack->type].offset.x +
                                global_attack_types[attack->type].directional_offset.x * (direction == LEFT ? -1 : 1),
                                global_attack_types[attack->type].offset.y +
                                global_attack_types[attack->type].directional_offset.y)),
                       attack->size, velocity,
                       attack->strength, origin_index);
            if(attack->duration_left <= 0.f)
            {
                attack->stage = ATTACK_STAGE_recovery;
            }
            break;
        }
        
        case ATTACK_STAGE_recovery:
        {
            attack->recovery_left -= core->delta_t;
            if(attack->recovery_left <= 0.f)
            {
                attack->stage = ATTACK_STAGE_ready;
            }
            break;
        }
        
        default: break;
    }
}

internal void
LoadPlayerInput(GameState *state, Player *player, i32 index, i32 texture_index)
{
    
    enum
    {
        CONTROL_move_right,
        CONTROL_move_left,
        CONTROL_jump,
        CONTROL_crouch,
        CONTROL_attack_light,
        CONTROL_attack_heavy,
        CONTROL_air_dash,
        CONTROL_MAX
    };
    
    b32 controls[CONTROL_MAX] = {0};
    
    // NOTE(rjf): Player index 1
    if(index == 0)
    {
        controls[CONTROL_move_right] = !!platform->key_down[KEY_d];
        controls[CONTROL_move_left] = !!platform->key_down[KEY_a];
        controls[CONTROL_jump] = !!platform->key_pressed[KEY_w];
        controls[CONTROL_crouch] = !!platform->key_down[KEY_s];
        controls[CONTROL_attack_light] = !!platform->key_pressed[KEY_e];
        controls[CONTROL_attack_heavy] = !!platform->key_pressed[KEY_r];
        controls[CONTROL_air_dash] = !!platform->key_pressed[KEY_shift];
    }
    // NOTE(rjf): Player index 2 (I guess?)
    else
    {
        controls[CONTROL_move_right] = !!platform->key_down[KEY_l];
        controls[CONTROL_move_left] = !!platform->key_down[KEY_j];
        controls[CONTROL_jump] = !!platform->key_pressed[KEY_i];
        controls[CONTROL_crouch] = !!platform->key_down[KEY_k];
        controls[CONTROL_attack_light] = !!platform->key_pressed[KEY_o];
        controls[CONTROL_attack_heavy] = !!platform->key_pressed[KEY_p];
    }
    
    for(int i = 0; i < ArrayCount(controls); ++i)
    {
        controls[i] *= (player->stun_time < 0.001f);
    }
    
    b32 on_ground = player->box.on_ground;
    b32 walking   = 0;
    b32 crouching = 0;
    b32 jabbing   = player->attack.stage > 0 && (player->attack.type == ATTACK_TYPE_jab || player->attack.type == ATTACK_TYPE_crouch_jab);
    b32 heavying  = player->attack.stage > 0 && (player->attack.type == ATTACK_TYPE_ftilt || player->attack.type == ATTACK_TYPE_crouch_ftilt);
    
    if(player->attack.stage == ATTACK_STAGE_ready && player->box.on_ground)
    {
        
        if(controls[CONTROL_move_right])
        {
            walking = 1;
            player->box.velocity.x += (750 - player->box.velocity.x) * core->delta_t * 16.f;
            player->direction = RIGHT;
        }
        else if(controls[CONTROL_move_left])
        {
            walking = 1;
            player->box.velocity.x += (-750 - player->box.velocity.x) * core->delta_t * 16.f;
            player->direction = LEFT;
        }
        
    }
    else if(player->attack.stage == ATTACK_STAGE_ready)
    {
        if(controls[CONTROL_move_right])
        {
            player->box.velocity.x += (250 - player->box.velocity.x) * core->delta_t * 2.f;
            player->direction = RIGHT;
        }
        
        else if(controls[CONTROL_move_left])
        {
            player->box.velocity.x += (-250 - player->box.velocity.x) * core->delta_t * 2.f;
            player->direction = LEFT;
        }
    }
    
    if(controls[CONTROL_jump])
    {
        player->box.velocity.y = -700;
    }
    
    if(controls[CONTROL_crouch])
    {
        crouching = 1;
        if((i32)player->box.size.y == 96)
        {
            player->box.y += 32;
            player->box.size.y = 64;
            player->animation.offset.y = -32;
        }
    }
    else
    {
        if((i32)player->box.size.y == 64)
        {
            player->box.y -= 32;
            player->box.size.y = 96;
            player->animation.offset.y = 0;
        }
    }
    
    if(player->attack.stage == ATTACK_STAGE_ready)
    {
        if(controls[CONTROL_crouch])
        {
            if(controls[CONTROL_attack_light])
            {
                ExecuteAttack(&player->attack, ATTACK_TYPE_crouch_jab);
                jabbing = 1;
            }
            else if(controls[CONTROL_attack_heavy])
            {
                ExecuteAttack(&player->attack, ATTACK_TYPE_crouch_ftilt);
                heavying = 1;
            }
        }
        else
        {
            if(controls[CONTROL_attack_light])
            {
                ExecuteAttack(&player->attack, ATTACK_TYPE_jab);
                jabbing = 1;
            }
            else if(controls[CONTROL_attack_heavy])
            {
                ExecuteAttack(&player->attack, ATTACK_TYPE_ftilt);
                heavying = 1;
            }
        }
    }
    
    if(!on_ground)
    {
        AnimationSet(&player->animation, &state->player_texture_data[texture_index].jump);
    }
    else
    {
        if(jabbing)
        {
            if(crouching)
            {
                AnimationSet(&player->animation, &state->player_texture_data[texture_index].crouch_jab);
            }
            else
            {
                AnimationSet(&player->animation, &state->player_texture_data[texture_index].jab);
            }
        }
        else if(heavying)
        {
            if(crouching)
            {
                AnimationSet(&player->animation, &state->player_texture_data[texture_index].crouch_heavy);
            }
            else
            {
                AnimationSet(&player->animation, &state->player_texture_data[texture_index].heavy);
            }
        }
        else if(crouching)
        {
            AnimationSet(&player->animation, &state->player_texture_data[texture_index].crouch);
        }
        else if(walking)
        {
            AnimationSet(&player->animation, &state->player_texture_data[texture_index].walk);
        }
        else
        {
            AnimationSet(&player->animation, &state->player_texture_data[texture_index].idle);
        }
    }
}

internal void
GameStateInit(GameState *state)
{
    // NOTE(rjf): Initialize player texture data
    {
        {
            Texture *texture = &state->player_texture_data[0].texture;
            state->player_texture_data[0].texture      = TextureLoad("data/player1.png");
            state->player_texture_data[0].idle         = AnimationTypeInit(iv2(0, 0), iv2(24, 48), 1, 0, texture, 0);
            state->player_texture_data[0].walk         = AnimationTypeInit(iv2(24, 0), iv2(24, 48), 2, 0.1f, texture, 0);
            state->player_texture_data[0].stand_block  = AnimationTypeInit(iv2(72, 0), iv2(24, 48), 1, 0, texture, 0);
            state->player_texture_data[0].crouch       = AnimationTypeInit(iv2(0, 48), iv2(24, 48), 1, 0, texture, 0);
            state->player_texture_data[0].jump         = AnimationTypeInit(iv2(24, 48), iv2(24, 48), 1, 0, texture, 0);
            state->player_texture_data[0].hurt         = AnimationTypeInit(iv2(48, 48), iv2(24, 48), 1, 0, texture, 1);
            state->player_texture_data[0].crouch_block = AnimationTypeInit(iv2(72, 48), iv2(24, 48), 1, 0, texture, 0);
            state->player_texture_data[0].jab          = AnimationTypeInit(iv2(0, 96), iv2(24, 48), 2, 0.16f, texture, 1);
            state->player_texture_data[0].heavy        = AnimationTypeInit(iv2(48, 96), iv2(24, 48), 2, 0.16f, texture, 1);
            state->player_texture_data[0].crouch_heavy = AnimationTypeInit(iv2(0, 144), iv2(24, 48), 2, 0.16f, texture, 1);
            state->player_texture_data[0].crouch_jab   = AnimationTypeInit(iv2(48, 144), iv2(24, 48), 2, 0.16f, texture, 1);
        }
        
        {
            Texture *texture = &state->player_texture_data[1].texture;
            state->player_texture_data[1].texture      = TextureLoad("data/player2.png");
            state->player_texture_data[1].idle         = AnimationTypeInit(iv2(0, 0), iv2(24, 48), 1, 0, texture, 0);
            state->player_texture_data[1].walk         = AnimationTypeInit(iv2(24, 0), iv2(24, 48), 2, 0.1f, texture, 0);
            state->player_texture_data[1].stand_block  = AnimationTypeInit(iv2(72, 0), iv2(24, 48), 1, 0, texture, 0);
            state->player_texture_data[1].crouch       = AnimationTypeInit(iv2(0, 48), iv2(24, 48), 1, 0, texture, 0);
            state->player_texture_data[1].jump         = AnimationTypeInit(iv2(24, 48), iv2(24, 48), 1, 0, texture, 0);
            state->player_texture_data[1].hurt         = AnimationTypeInit(iv2(48, 48), iv2(24, 48), 1, 0, texture, 1);
            state->player_texture_data[1].crouch_block = AnimationTypeInit(iv2(72, 48), iv2(24, 48), 1, 0, texture, 0);
            state->player_texture_data[1].jab          = AnimationTypeInit(iv2(0, 96), iv2(24, 48), 2, 0.16f, texture, 1);
            state->player_texture_data[1].heavy        = AnimationTypeInit(iv2(48, 96), iv2(24, 48), 2, 0.16f, texture, 1);
            state->player_texture_data[1].crouch_heavy = AnimationTypeInit(iv2(0, 144), iv2(24, 48), 2, 0.16f, texture, 1);
            state->player_texture_data[1].crouch_jab   = AnimationTypeInit(iv2(48, 144), iv2(24, 48), 2, 0.16f, texture, 1);
        }
    }
    
    state->players[0].box = Box(-256, -256, 48, 96);
    state->players[0].box.velocity = v2(0, 0);
    state->players[0].color = v3(0.7f, 0.3f, 0.3f);
    state->players[0].animation = AnimationInit(&state->player_texture_data[0].idle);
    
    state->players[1].box = Box(256-48, -256, 48, 96);
    state->players[1].box.velocity = v2(0, 0);
    state->players[1].color = v3(0.6f, 0.2f, 0.7f);
    state->players[1].animation = AnimationInit(&state->player_texture_data[1].idle);
    
    state->hit_box_count = 0;
    
    CameraTargetPlayerMidpoint(&state->camera, state->players, ArrayCount(state->players));
    state->camera.target.y -= 1000;
    state->camera.center_position = state->camera.target;
    
    state->ground = Box(-512, 96, 1024, 1024);
    
    state->jump_sound = SoundLoad("data/jump.ogg");
    state->hit_sound = SoundLoad("data/hit.ogg");
    state->death_hit_sound = SoundLoad("data/finish.ogg");
    state->music_sound = SoundLoad("data/music.ogg");
    
    state->background_music_source = AudioReserveSource(&core->audio);
    AudioPlaySource(&core->audio, state->background_music_source, &state->music_sound,
                    AUDIO_master, 0.8f, 1.f, 1);
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
    
    if(platform->key_pressed[KEY_g])
    {
        
        state->players[0].box = Box(-256, -256, 48, 96);
        state->players[0].box.velocity = v2(0, 0);
        state->players[0].color = v3(0.7f, 0.3f, 0.3f);
        state->players[0].animation = AnimationInit(&state->player_texture_data[0].idle);
        
        state->players[1].box = Box(256-48, -256, 48, 96);
        state->players[1].box.velocity = v2(0, 0);
        state->players[1].color = v3(0.6f, 0.2f, 0.7f);
        state->players[1].animation = AnimationInit(&state->player_texture_data[1].idle);
        
        state->hit_box_count = 0;
        
        CameraTargetPlayerMidpoint(&state->camera, state->players, ArrayCount(state->players));
        state->camera.target.y -= 1000;
        state->camera.center_position = state->camera.target;
        
    }
    
    state->hit_box_count = 0;
    
    for(int i = 0; i < ArrayCount(state->players); ++i)
    {
        LoadPlayerInput(state, &state->players[i], i, i == 0 ? 0 : 1);
        AttackUpdate(&state->players[i].attack, state->players[i].direction, i,
                     v2(state->players[i].box.position.x + state->players[i].box.size.x/2,
                        state->players[i].box.position.y + state->players[i].box.size.y/2),
                     state->players[i].box.velocity,
                     state);
    }
    
    for(int i = 0; i < ArrayCount(state->players); ++i)
    {
        Player *player = state->players+i;
        
        if(player->attack.stage == ATTACK_STAGE_ready && player->box.on_ground)
        {
            state->players[i].box.velocity.x -= state->players[i].box.velocity.x * core->delta_t * 8.f;
        }
        
        state->players[i].box.velocity.y += 1500 * core->delta_t;
        
        player->box.on_ground = 0;
        
        CollideBoxWithStaticBox(&state->players[i].box, state->ground);
        CollideBoxWithHitBoxes((i32)i, &state->players[i].health, &state->players[i].stun_time,
                               &state->players[i].box, state->hit_boxes, state->hit_box_count,
                               &state->hit_sound, &state->camera);
        
        state->players[i].box.x += state->players[i].box.velocity.x * core->delta_t;
        state->players[i].box.y += state->players[i].box.velocity.y * core->delta_t;
        
        player->stun_time -= core->delta_t;
        
        AnimationUpdate(&player->animation);
    }
    
    
    CameraTargetPlayerMidpoint(&state->camera, state->players, ArrayCount(state->players));
    CameraUpdate(&state->camera);
    
    // NOTE(rjf): Render everything
    {
        RendererPushFilledRectS(&core->renderer,
                                v2(0, 0),
                                v2(core->render_w, core->render_h),
                                v4(0.5f, 0.42f, 0.5f, 1.f));
        
        local_persist b32 first = 1;
        local_persist struct
        {
            v4 rect;
            f32 depth;
        }
        buildings[64];
        if(first)
        {
            first = 0;
            for(int i = 0; i < ArrayCount(buildings); ++i)
            {
                buildings[i].rect = v4(
                    RandomF32(0, core->render_w),
                    RandomF32(core->render_h/8, core->render_h),
                    RandomF32(64, 256),
                    1024
                    );
                buildings[i].depth = 0.5f + 0.5f*(i / (f32)ArrayCount(buildings));
            }
        }
        
        for(int i = 0; i < ArrayCount(buildings); ++i)
        {
            f32 depth = buildings[i].depth;
            
            v4 top_color = {
                depth,
                depth,
                depth,
                1.f
            };
            
            v4 bottom_color = {
                depth * (7/8),
                depth * (7/8),
                depth * (7/8),
                1.f
            };
            
            RendererPushFilledRect(&core->renderer,
                                   V2Subtract(v2(buildings[i].rect.x, buildings[i].rect.y),
                                              v2(state->camera.position.x * (1-depth) / 10.f, state->camera.position.y * (1-depth) / 10.f)),
                                   v2(buildings[i].rect.z, buildings[i].rect.w),
                                   top_color,
                                   bottom_color,
                                   top_color,
                                   bottom_color);
            
        }
        
        RendererPushFilledRect(&core->renderer,
                               V2Subtract(state->ground.position, state->camera.position),
                               state->ground.size,
                               v4(0.3f, 0.3f, 0.3f, 1.f),
                               v4(0.1f, 0.1f, 0.1f, 1.f),
                               v4(0.3f, 0.3f, 0.3f, 1.f),
                               v4(0.1f, 0.1f, 0.1f, 1.f));
        
        for(u32 i = 0; i < ArrayCount(state->players); ++i)
        {
            Player *player = state->players + i; 
#if 0
            RendererPushFilledRectS(&core->renderer,
                                    V2Subtract(state->players[i].box.position, state->camera.position),
                                    state->players[i].box.size,
                                    v4(state->players[i].color.r, state->players[i].color.g, state->players[i].color.b, 1.f));
#endif
            RendererPushTexture(&core->renderer, player->animation.type->texture,
                                RENDERER_FLIP_HORIZONTAL * (player->direction == LEFT),
                                v4(player->animation.type->pos.x +
                                   player->animation.current_frame * player->animation.type->size.x,
                                   player->animation.type->pos.y,
                                   player->animation.type->size.x,
                                   player->animation.type->size.y),
                                v4(player->box.position.x - state->camera.position.x + player->animation.offset.x,
                                   player->box.position.y - state->camera.position.y + player->animation.offset.y, 
                                   player->animation.type->size.x*2,
                                   player->animation.type->size.y*2),
                                1.f);
        }
        
        // for(u32 i = 0; i < state->hit_box_count; ++i)
#if 0
        {
            RendererPushFilledRectS(&core->renderer,
                                    V2Subtract(v2(state->hit_boxes[i].box.position.x - state->hit_boxes[i].box.size.x,
                                                  state->hit_boxes[i].box.position.y - state->hit_boxes[i].box.size.y),
                                               state->camera.position),
                                    v2(state->hit_boxes[i].box.size.x*2,
                                       state->hit_boxes[i].box.size.y*2),
                                    v4(1, 0, 0, 1));
        }
#endif
    }
}