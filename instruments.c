#include "jams.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static float frequencies[N_NOTES] = {0};

static const int N_BUFFERS = 64;
static Track tracks[64] = {0};
static int drum_map[N_NOTES];

static void square(Playback *play, u64 offset, int note_idx, Sample_Info *s)
{
    Note *note = &play->notes[note_idx];
    if (note->in_vel == 0 || note->out_vel != 0)
        return;

    float pos = (float)(play->position + offset - note->start);
    double temp;
    pos = modf(pos / ((float)play->sample_rate / frequencies[note_idx]), &temp);

    float sample = pos >= 0.5 ? -0.2 : 0.2;

    float vel = (float)note->in_vel / 127.0;
    vel = 0.33 + (vel * 0.67);

    s->sample += sample * vel;
    s->volume += vel;
}

static void sawtooth(Playback *play, u64 offset, int note_idx, Sample_Info *s)
{
    Note *note = &play->notes[note_idx];
    if (note->in_vel == 0 || note->out_vel != 0)
        return;

    float pos = (float)(play->position + offset - note->start);
    float temp = pos / ((float)play->sample_rate / frequencies[note_idx]);
    pos = temp - (int)temp;

    float sample = (pos*0.4) - 0.2;

    float vel = (float)note->in_vel / 127.0;
    vel = 0.33 + (vel * 0.67);

    s->sample += sample * vel;
    s->volume += vel;
}

static void sinewave(Playback *play, u64 offset, int note_idx, Sample_Info *s)
{
    Note *note = &play->notes[note_idx];
    if (note->in_vel == 0 || note->out_vel != 0)
        return;

    float pos = (float)(play->position + offset - note->start);
    pos = sin(2.0 * M_PI * pos / ((float)play->sample_rate / frequencies[note_idx]));

    float sample = (pos*0.125) - 0.0625;

    float vel = (float)note->in_vel / 127.0;
    vel = 0.33 + (vel * 0.67);

    s->sample += sample * vel;
    s->volume += vel;
}

static int counter = 0;
static void drums(Playback *play, u64 offset, int note_idx, Sample_Info *s)
{
    const float vel = 0.875;

    Note *note = &play->notes[note_idx];
    if (note->in_vel != 0 && note->out_vel == 0) {
        Track *track = &tracks[drum_map[note_idx]];
        float pos = (float)(play->position + offset - note->start);
        pos *= track->rate / play->sample_rate;

        //float vel = (float)note->in_vel / 127.0;
        //vel = 0.33 + (vel * 0.67);

        int idx = (int)pos;
        if (idx >= 0 && idx < track->len) {
            note->prev_sample = track->buf[idx] * vel;
            s->sample += note->prev_sample;
            s->volume += vel;
            return;
        }
    }

    if (note->prev_sample < -0.01 || note->prev_sample > 0.01) {
        note->prev_sample *= 0.95;
        s->sample += note->prev_sample;
        s->volume += vel;
    }
}

static Instrument instrument_table[] = {
    drums,
    sawtooth,
    sinewave,
    square
};

Instrument *load_instruments(Playback *play, int *n_instruments) {
    // Middle A is 440Hz and note 57
    for (int i = 0; i < N_NOTES; i++) {
        frequencies[i] = 440.0 * pow(2.0, (float)(i - 57) / 12.0);
    }

    int map_sz;
    char *map = read_whole_file("Samples/map.txt", &map_sz);
    if (map) {
        char path[256];
        strcpy(path, "Samples/");
        char *path_mid = &path[strlen(path)];

        char *p = path_mid;
        int n = 0;
        for (int i = 0; i < map_sz; i++) {
            if (map[i] == '\n' && p > path_mid) {
                *p = 0;
                puts(path);
                tracks[n++] = read_wav_file_as_mono(path);
                p = path_mid;
            }
            else if (p < &path[255] && map[i] >= ' ' && map[i] <= '~') {
                *p++ = map[i];
            }
        }
        if (p > path_mid && n < N_BUFFERS)
            tracks[n++] = read_wav_file_as_mono(path);

        /*
        FILE *f = fopen("debug.track", "wb");
        fwrite(tracks[2].buf, 1, tracks[2].len * sizeof(float), f);
        fclose(f);
        */

        for (int i = 0; i < N_NOTES; i++) {
            drum_map[i] = (i * N_BUFFERS) / N_NOTES;
        }

        free(map);
    }

    *n_instruments = 1;
    return instrument_table;
}

void close_instruments()
{
    for (int i = 0; i < N_BUFFERS; i++) {
        close_track(&tracks[i]);
    }
}