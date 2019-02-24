#define AUDIO_SOURCE_MAX 256

#define AUDIO_TYPE_LIST \
AudioType(master, 32) \
AudioType(background_music, 33) \
AudioType(entities, 34) \
AudioType(environment, 35) \
AudioType(user_interface, 36) \

enum
{
#define AudioType(code_name, str_name) AUDIO_##code_name,
    AUDIO_TYPE_LIST
#undef AudioType
        AUDIO_MAX
};

internal i32
AudioTypeName(i32 type)
{
    local_persist i32 names[AUDIO_MAX] = {
#define AudioType(code_name, str_name) str_name,
        AUDIO_TYPE_LIST
#undef AudioType
    };
    assert(type >= 0 && type < AUDIO_MAX);
    return names[type];
}

typedef struct AudioSource
{
    Sound *sound;
    i32 volume_type;
    f32 volume;
    f32 pitch;
    b32 playing;
    b32 distanced;
    b32 loop;
    v2 position;
    f64 play_position;
}
AudioSource;

typedef struct Audio
{
    AudioSource sources[AUDIO_SOURCE_MAX];
    b32 source_reserved[AUDIO_SOURCE_MAX];
    f32 volumes[AUDIO_MAX];
    f32 modifiers[AUDIO_MAX];
    v2 listener_position;
}
Audio;

typedef i32 AudioSourceID;