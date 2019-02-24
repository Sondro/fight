typedef struct AnimationType
{
    iv2 pos;
    iv2 size;
    i32 frame_count;
    f32 rate;
    Texture *texture;
    b32 only_play_once;
}
AnimationType;

internal AnimationType
AnimationTypeInit(iv2 pos, iv2 size, i32 frame_count, f32 rate, Texture *texture,
                  b32 only_play_once)
{
    AnimationType type = {
        pos,
        size,
        frame_count,
        rate,
        texture,
        only_play_once
    };
    return type;
}

typedef struct Animation
{
    AnimationType *type;
    i32 current_frame;
    f32 time_left_in_frame;
    b32 done;
    v2 offset;
}
Animation;

internal Animation
AnimationInit(AnimationType *type)
{
    Animation animation = {
        type,
        0,
        type->rate,
        0,
        {0},
    };
    return animation;
}

internal void
AnimationSet(Animation *animation, AnimationType *type)
{
    if(animation->type != type)
    {
        v2 old_offset = animation->offset;
        *animation = AnimationInit(type);
        animation->offset = old_offset;
    }
}

internal void
AnimationUpdate(Animation *animation)
{
    if(!animation->done)
    {
        animation->time_left_in_frame -= core->delta_t;
        if(animation->time_left_in_frame <= 0.f)
        {
            animation->time_left_in_frame = animation->type->rate;
            if(++animation->current_frame >= animation->type->frame_count)
            {
                if(animation->type->only_play_once)
                {
                    --animation->current_frame;
                    animation->done = 1;
                }
                else
                {
                    animation->current_frame = 0;
                }
            }
        }
    }
}