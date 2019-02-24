
internal void
AudioInit(Audio *audio)
{
    MemorySet(audio, 0, sizeof(Audio));
    for(int i = 0; i < AUDIO_MAX; ++i)
    {
        audio->volumes[i] = 1;
        audio->modifiers[i] = 1;
    }
}

internal void
AudioPlaySource(Audio *audio,
                AudioSourceID source_id, Sound *sound,
                i32 volume_type, f32 volume, f32 pitch, b32 loop)
{
    
    AudioSource *source = audio->sources+source_id;
    
    source->playing = 1;
    source->play_position = 0;
    source->sound = sound;
    source->volume_type = volume_type;
    source->volume = volume;
    source->pitch = pitch;
    source->loop = loop;
}

internal void
AudioPlaySound(Audio *audio, Sound *sound, i32 volume_type, f32 volume, f32 pitch)
{
    
    for(int i = 0; i < AUDIO_SOURCE_MAX; ++i)
    {
        if(!audio->sources[i].playing && !audio->source_reserved[i])
        {
            audio->sources[i].distanced = 0;
            AudioPlaySource(audio,(i32)i, sound, volume_type, volume, pitch, 0);
            break;
        }
    }
}

internal void
AudioPlaySoundAtPoint(Audio *audio, Sound *sound, i32 volume_type, f32 volume, f32 pitch, 
                      v2 position)
{
    
    for(int i = 0; i < AUDIO_SOURCE_MAX; ++i)
    {
        if(!audio->sources[i].playing && !audio->source_reserved[i])
        {
            audio->sources[i].position = position;
            audio->sources[i].distanced = 1;
            
            AudioPlaySource(audio, 
                            (i32)i, sound, volume_type, volume, pitch, 0);
            break;
        }
    }
}

internal AudioSourceID
AudioReserveSource(Audio *audio)
{
    AudioSourceID id = 0;
    
    for(int i = 0; i < AUDIO_SOURCE_MAX; ++i)
    {
        if(!audio->source_reserved[i])
        {
            id = (AudioSourceID)i;
            audio->source_reserved[i] = 1;
            break;
        }
    }
    
    return id;
}


internal void
AudioUnreserveSource(Audio *audio, AudioSourceID id)
{
    AudioSource *source = audio->sources+id;
    audio->source_reserved[id] = 0;
    source->playing = 0;
    source->play_position = 0;
}

internal void
AudioStopSource(Audio *audio, AudioSourceID id)
{
    AudioSource *source = audio->sources+id;
    source->playing = 0;
    source->play_position = 0;
}

internal b32
AudioSourcePlaying(Audio *audio, AudioSourceID id)
{
    AudioSource *source = audio->sources+id;
    return source->playing;
}

internal void
AudioSetSourceVolume(Audio *audio, AudioSourceID id, f32 volume)
{
    AudioSource *source = audio->sources+id;
    source->volume = volume;
}

internal f32
AudioGetSourceVolume(Audio *audio, AudioSourceID id)
{
    AudioSource *source = audio->sources+id;
    return source->volume;
}

internal void
AudioSetSourcePosition(Audio *audio, AudioSourceID id, v2 position)
{
    AudioSource *source = audio->sources+id;
    source->position = position;
}

internal void
AudioSetSourcePitch(Audio *audio, AudioSourceID id, f32 pitch)
{
    AudioSource *source = audio->sources+id;
    source->pitch = pitch;
}

internal void
AudioUpdate(Audio *audio)
{
    // NOTE(rjf): Play the necessary samples of all playing sounds
    for(int i = 0; i < AUDIO_SOURCE_MAX; ++i)
    {
        if(audio->sources[i].playing &&
           audio->sources[i].sound &&
           audio->sources[i].sound->samples)
        {
            
            Sound *sound = audio->sources[i].sound;
            
            i16 *sample_out = platform->sample_out;
            i16 *samples = sound->samples;
            
            u64 samples_to_write_of_sound = platform->sample_count_to_output;
            
            f32 distance_factor = audio->sources[i].distanced ?
                1-(V2LengthSquared(V2Subtract(audio->sources[i].position, audio->listener_position)) / (1024*1024)) 
                : 1;
            
            if(distance_factor < 0) {
                distance_factor = 0.f;
            }
            else if(distance_factor > 1) {
                distance_factor = 1.f;
            }
            
            f32 sample_pitch = audio->sources[i].pitch;
            
            f32 sample_volume = 
                audio->volumes[AUDIO_master] *
                audio->volumes[audio->sources[i].volume_type] *
                audio->modifiers[audio->sources[i].volume_type] *
                audio->sources[i].volume *
                distance_factor;
            
            for(u64 write_sample = 0;
                write_sample < samples_to_write_of_sound; 
                ++write_sample)
            {
                
                int channels = sound->channels;
                
                f64 start_play_position = audio->sources[i].play_position;
                i16 start_left_sample;
                i16 start_right_sample;
                
                f64 target_play_position = start_play_position + (f64)channels*(f64)sample_pitch;
                if(target_play_position >= sound->sample_count)
                {
                    target_play_position -= 
                        sound->sample_count;
                }
                i16 target_left_sample;
                i16 target_right_sample;
                
                {
                    u64 left_index = (u64)start_play_position;
                    if(channels > 1)
                    {
                        left_index &= ~((u64)(0x01));
                    }
                    u64 right_index = left_index + (channels-1);
                    
                    i16 first_left_sample = samples[left_index];
                    i16 first_right_sample = samples[right_index];
                    i16 second_left_sample = samples[left_index+channels];
                    i16 second_right_sample = samples[right_index+channels];
                    
                    start_left_sample = (i16)(first_left_sample + (second_left_sample - first_left_sample) * (start_play_position/channels - (u64)(start_play_position/channels)));
                    start_right_sample = (i16)(first_right_sample + (second_right_sample - first_right_sample) * (start_play_position/channels - (u64)(start_play_position/channels)));
                }
                
                {
                    u64 left_index = (u64)target_play_position;
                    if(channels > 1)
                    {
                        left_index &= ~((u64)(0x01));
                    }
                    u64 right_index = left_index + (channels-1);
                    
                    i16 first_left_sample = samples[left_index];
                    i16 first_right_sample = samples[right_index];
                    i16 second_left_sample = samples[left_index+channels];
                    i16 second_right_sample = samples[right_index+channels];
                    
                    target_left_sample = (i16)(first_left_sample + (second_left_sample - first_left_sample) * (target_play_position/channels - (u64)(target_play_position/channels)));
                    target_right_sample = (i16)(first_right_sample + (second_right_sample - first_right_sample) * (target_play_position/channels - (u64)(target_play_position/channels)));
                }
                
                i16 left_sample = (i16)((((i64)start_left_sample + (i64)target_left_sample)/2) * sample_volume);
                i16 right_sample = (i16)((((i64)start_right_sample + (i64)target_right_sample)/2) * sample_volume);
                
                *sample_out++ += left_sample;  // Left
                *sample_out++ += right_sample; // Right
                
                audio->sources[i].play_position = target_play_position;
                
                // NOTE(rjf): This might not be necessary, but it will work
                
                if(audio->sources[i].play_position >= sound->sample_count - channels - 1)
                {
                    if(audio->sources[i].loop)
                    {
                        audio->sources[i].play_position -= sound->sample_count;
                    }
                    else
                    {
                        audio->sources[i].playing = 0;
                        audio->sources[i].play_position = 0;
                        break;
                    }
                }
            }
        }
    }
}