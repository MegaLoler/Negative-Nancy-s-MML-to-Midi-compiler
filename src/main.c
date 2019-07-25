#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// print a number in variable length format
void print_variable_length (FILE *stream, uint32_t value) {
    do {
        fputc (value & 0x7f, stream);
        value >>= 7;
    } while (value & 0x80);
}

void print_chars (FILE *stream, int length, char *data) {
    for (int i = 0; i < length; i ++)
        fputc (data[i], stream);
}

void print_uint16 (FILE *stream, uint16_t value) {
    fputc ((value >> (8 * 1)) & 0xff, stream);
    fputc ((value >> (8 * 0)) & 0xff, stream);
}

void print_uint32 (FILE *stream, uint32_t value) {
    fputc ((value >> (8 * 3)) & 0xff, stream);
    fputc ((value >> (8 * 2)) & 0xff, stream);
    fputc ((value >> (8 * 1)) & 0xff, stream);
    fputc ((value >> (8 * 0)) & 0xff, stream);
}

void note_off (FILE *stream, int delta_time, uint8_t channel, uint8_t note, uint8_t velocity) {
    print_variable_length (stream, delta_time);
    fputc (0x80 | (channel & 0x0f), stream);
    fputc (note & 0x7f, stream);
    fputc (velocity & 0x7f, stream);
}

void note_on (FILE *stream, int delta_time, uint8_t channel, uint8_t note, uint8_t velocity) {
    print_variable_length (stream, delta_time);
    fputc (0x90 | (channel & 0x0f), stream);
    fputc (note & 0x7f, stream);
    fputc (velocity & 0x7f, stream);
}

void controller (FILE *stream, int delta_time, uint8_t channel, uint8_t controller, uint8_t value) {
    print_variable_length (stream, delta_time);
    fputc (0xe0 | (channel & 0x0f), stream);
    fputc (controller & 0x7f, stream);
    fputc (value & 0x7f, stream);
}

// print a midi chunk
void chunk (FILE *stream, const char *type, int length, char *data) {
    fputs (type, stream);
    print_uint32 (stream, length);
    print_chars (stream, length, data);
}

// print a midi header chunk
void header (FILE *stream, uint16_t format, uint16_t num_tracks, uint16_t division) {
    char *data;
    size_t length = 0;
    FILE *data_stream = open_memstream (&data, &length);
    print_uint16 (data_stream, format);
    print_uint16 (data_stream, num_tracks);
    print_uint16 (data_stream, division);
    fclose (data_stream);
    chunk (stream, "MThd", length, data);
    free (data);
}

// print a track chunk
void track (FILE *stream, int length, char *data) {
    chunk (stream, "MTrk", length, data);
}

// the top level compile function
void compile (FILE *in_stream, FILE *out_stream) {
    note_on (out_stream, 0, 0, 60, 127);
    note_off (out_stream, 100, 0, 60, 0);
}

int main (int argc, char **argv) {

    // print the header
    header (stdout, 0, 1, 100); // TODO: figure out what division to use

    // print the track with the midi events
    char *data;
    size_t length = 0;
    FILE *data_stream = open_memstream (&data, &length);
    compile (stdin, data_stream);
    fclose (data_stream);
    track (stdout, length, data);
    free (data);

    return 0;
}
