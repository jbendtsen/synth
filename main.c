#include "jams.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#define PLAYBACK_TEMP_SIZE (16*1024)

void talloc_init(Temp_Allocator *alloc, int max_size)
{
    alloc->buf = malloc(max_size);
    alloc->size = max_size;
    alloc->head = 0;
}

u8 *talloc_get(Temp_Allocator *alloc, int size)
{
    int next = alloc->head + size;
    if (size <= 0 || next > alloc->size)
        return NULL;

    u8 *ptr = &alloc->buf[alloc->head];
    alloc->head = next;
    return ptr;
}

void talloc_pad_to_next(Temp_Allocator *alloc, int align)
{
    int add = (align - (alloc->head % align)) % align;
    talloc_get(alloc, add);
}

void talloc_close(Temp_Allocator *alloc)
{
    if (alloc->buf)
        free(alloc->buf);

    alloc->buf = NULL;
}

static jack_port_t *input_port = NULL;
static jack_port_t *output_port = NULL;

static Playback state;

int audio_callback(jack_nframes_t n_frames, void *extra)
{
    state.alloc.head = 0;

    void *input   = jack_port_get_buffer(input_port, n_frames);
    float *output = jack_port_get_buffer(output_port, n_frames);

    state.n_events = jack_midi_get_event_count(input);
    state.events = (Event*)talloc_get(&state.alloc, state.n_events * sizeof(Event));

    for (int i = 0; i < state.n_events; i++) {
        jack_midi_event_t ev;
        jack_midi_event_get(&ev, input, i);

        int len = (int)ev.size;
        u8 *buf = talloc_get(&state.alloc, len);
        for (int j = 0; j < len; j++) {
            buf[j] = ev.buffer[j];
            printf("%02x ", buf[j]);
        }
        putchar('\n');

        state.events[i].time = ev.time;
        state.events[i].size = len;
        state.events[i].buf = buf;
    }

    talloc_pad_to_next(&state.alloc, sizeof(void*));
    synthesize(&state, output, n_frames);
    return 0;
}

int update_sample_rate(jack_nframes_t sample_rate, void *extra)
{
    state.sample_rate = (int)sample_rate;
    return 0;
}

static int exit_fd = -1;

void exit_program(char code)
{
    putchar('\n');
    write(exit_fd, &code, 1);
}
void shutdown_jack(void *extra)
{
    exit_program(1);
}
void handle_sigint(int signal)
{
    exit_program(2);
}

int make_exit_pipe()
{
    int fds[2];
    fds[0] = 0;
    fds[1] = 0;
    pipe(fds);
    exit_fd = fds[1];
    return fds[0];
}

int main(int argc, char **argv)
{
    talloc_init(&state.alloc, PLAYBACK_TEMP_SIZE);

    int read_exit_fd = make_exit_pipe();
    signal(SIGINT, handle_sigint);

    jack_client_t *client = jack_client_open("Jams", JackNullOption, NULL);
    if (!client) {
        printf("Failed to connect to JACK server\n");
        return 1;
    }

    jack_set_process_callback(client, audio_callback, 0);
    jack_set_sample_rate_callback (client, update_sample_rate, 0);
    jack_on_shutdown(client, shutdown_jack, 0);

    input_port = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    output_port = jack_port_register(client, "audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    int res = jack_activate(client);
    if (res != 0) {
        printf("Failed to activate JACK client (%d)\n", res);
        return 1;
    }

    char code = 0;
    read(read_exit_fd, &code, 1);

    jack_client_close(client);
    talloc_close(&state.alloc);
    return 0;
}
