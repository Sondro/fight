internal Sound
SoundInitFromOGGData(void *data, u64 len)
{
    Sound s = {0};
    s.sample_count = 
        stb_vorbis_decode_memory(
        (u8 *)data, (i32)len, &s.channels, &s.sample_rate, 
        &s.samples
        );
    s.sample_count *= s.channels;
    return s;
}

internal Sound
SoundLoad(const char *filename)
{
    Sound sound = {0};
    void *ogg_data = 0;
    u64 ogg_data_len = 0;
    platform->LoadEntireFile(filename, &ogg_data, &ogg_data_len, 0);
    if(ogg_data && ogg_data_len)
    {
        sound = SoundInitFromOGGData(ogg_data, ogg_data_len);
        platform->FreeFileData(ogg_data);
    }
    return sound;
}

internal void
SoundCleanUp(Sound *s)
{
    if(s->samples)
    {
        free(s->samples);
        s->samples = 0;
        s->sample_count = 0;
    }
}
