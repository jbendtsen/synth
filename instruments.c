#include "jams.h"

#include <stdio.h>
#include <math.h>

static float frequencies[N_NOTES] = {0};

static void square(Playback *play, u64 offset, int note_idx, Sample_Info *s) {
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

static void sawtooth(Playback *play, u64 offset, int note_idx, Sample_Info *s) {
    Note *note = &play->notes[note_idx];
    if (note->in_vel == 0 || note->out_vel != 0)
        return;

    float pos = (float)(play->position + offset - note->start);
    double temp;
    pos = modf(pos / ((float)play->sample_rate / frequencies[note_idx]), &temp);

    float sample = (pos*0.4) - 0.2;

    float vel = (float)note->in_vel / 127.0;
    vel = 0.33 + (vel * 0.67);

    s->sample += sample * vel;
    s->volume += vel;
}

static Instrument instrument_table[] = {
    sawtooth,
    square
};

Instrument *load_instruments(Playback *play, int *n_instruments) {
    // Middle A is 440Hz and note 57
    for (int i = 0; i < N_NOTES; i++) {
        frequencies[i] = 440.0 * pow(2.0, (float)(i - 57) / 12.0);
    }

    *n_instruments = 1;
    return instrument_table;
}