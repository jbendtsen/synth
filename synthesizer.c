#include "jams.h"

#include <stdio.h>

static Instrument *instrument_table;
static int n_instruments = 0;

void init_synthesizer(Playback *play)
{
    instrument_table = load_instruments(play, &n_instruments);
}

void close_synthesizer(Playback *play)
{
    close_instruments();
}

void update_playback(Playback *play, u64 offset, int ev_idx)
{
    int size = play->events[ev_idx].size;
    u8 *buf  = play->events[ev_idx].buf;

    u8 cmd = 0;
    int note = -1;
    int ctrl = -1;
    for (int j = 0; j < size; j++) {
        if (!cmd) {
            cmd = buf[j] >> 4;
            continue;
        }
        
        if (cmd == 8) {
            if (note < 0) {
                note = buf[j] & 0x7f;
                continue;
            }
            play->notes[note].out_vel = buf[j];
            play->notes[note].end = play->position + offset;
            note = -1;
            cmd = 0;
        }
        else if (cmd == 9) {
            if (note < 0) {
                note = buf[j] & 0x7f;
                continue;
            }
            play->notes[note].in_vel = buf[j];
            play->notes[note].out_vel = 0;
            play->notes[note].start = play->position + offset;
            note = -1;
            cmd = 0;
        }
        else if (cmd == 0xb) {
            if (ctrl < 0) {
                ctrl = buf[j];
                continue;
            }
            play->ctrl_value = buf[j];
            ctrl = -1;
            cmd = 0;
        }
        else if (cmd == 0xe) {
            u16 bend = buf[j];
            if (j < size-1) {
                bend <<= 8;
                bend |= buf[j];
            }
            play->bend = bend;
            play->bend_pos = play->position + offset;
            cmd = 0;
        }
    }
}

void synthesize(Playback *play, float *out, int n_samples)
{
    int ev_idx = 0;
    for (int i = 0; i < n_samples; i++) {
        while (play->events && ev_idx >= 0 && i == play->events[ev_idx].time) {
            update_playback(play, (u64)i, ev_idx);
            if (ev_idx < 0 || ev_idx >= play->n_events-1)
                ev_idx = -1;
            else
                ev_idx += 1;
        }

        Sample_Info s;
        s.sample = 0.0;
        s.volume = 0.0;

        for (int j = 0; j < N_NOTES; j++) {
            int idx = play->notes[j].instrument;
            if (idx >= 0 && idx < n_instruments && instrument_table[idx])
                instrument_table[idx](play, (u64)i, j, &s);
        }
        
        out[i] = s.sample; // * s.volume;
    }
}
