#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// ticks in a quarter note
#define DIVISION 128

// max number of tick markers
#define MAX_MARKERS 256

// return the delta time for a given rhythmic value
int get_delta_time (float length) {
    return DIVISION * 4 / length;
}

// print a number in variable length format
void print_variable_length (FILE *stream, uint32_t value) {
    // TODO: this should be big endian insteadd i think lol
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

void print_uint24 (FILE *stream, uint32_t value) {
    fputc ((value >> (8 * 2)) & 0xff, stream);
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

void tempo (FILE *stream, int delta_time, int bpm) {
    print_variable_length (stream, delta_time);
    fputc (0xff, stream);
    fputc (0x51, stream);
    fputc (0x03, stream);
    print_uint24 (stream, 60000000 / bpm);
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

// see what character is coming up next
char fpeek (FILE *stream) {
    char c = fgetc (stream);
    ungetc (c, stream);
    return c;
}

// read the rest of a note after the note name
void read_note (FILE *in_stream, FILE *out_stream, int channel, int note, int octave, int velocity, float length, int *tick) {
    int value = note + 12 * (octave + 1);
    int scan_result;
    int read;
    int done = 0;
    while (!done) {
        char next = fpeek (in_stream);
        switch (next) {
            case '+':
                value++;
                getc (in_stream);
                break;
            case '-':
                value++;
                getc (in_stream);
                break;
            case '/':
                getc (in_stream);
                scan_result = fscanf (in_stream, "%i", &read);
                if (scan_result > 0)
                    length *= read;
                done = 1;
                break;
            default:
                scan_result = fscanf (in_stream, "%i", &read);
                if (scan_result > 0)
                    length /= read;
                done = 1;
        }
    }

    // output the note start and stop
    note_on (out_stream, 0, channel, value, velocity);
    int delta_time = get_delta_time (length);
    *tick += delta_time;
    note_off (out_stream, delta_time, value, note, 0);
}

// read the rest of the line
void consume_line (FILE *stream) {
    char c;
    while ((c = getc (stream)) != '\n') {}
}

// the top level compile function
void compile (FILE *in_stream, FILE *out_stream) {

    int markers[MAX_MARKERS];   // tick markers

    int tick = 0;       // current time tick
    int channel = 0;    // current midi channel
    int octave = 4;     // current octave
    float length = 4;     // current rhythmic note value
    int velocity = 80;  // current note velocity
    
    // get the next char
    int temp1, temp2;
    char c;
    while ((c = getc (in_stream)) != EOF) {
        switch (c) {
            case ';':
                // comment
                consume_line (in_stream);
                break;
            case 'T':
                // set bpm
                fscanf (in_stream, "%i", &temp1);
                tempo (out_stream, 0, temp1);
                break;
            case 't':
                // set tick
                fscanf (in_stream, "%i", &tick);
                break;
            case 'C':
                // set channel
                fscanf (in_stream, "%i", &channel);
                break;
            case 'o':
                // set octave
                fscanf (in_stream, "%i", &octave);
                break;
            case '>':
                // increase octave
                octave++;
                break;
            case '<':
                // decrease octave
                octave--;
                break;
            case 'l':
                // set rhythmic value
                fscanf (in_stream, "%f", &length);
                break;
            case 'v':
                // set velocity
                fscanf (in_stream, "%i", &velocity);
                break;
            case '$':
                // send midi controller message
                fscanf (in_stream, "%i,%i", &temp1, &temp2);
                controller (out_stream, 0, channel, temp1, temp2);
                break;
            case 'm':
                // store current time tick
                fscanf (in_stream, "%i", &temp1);
                markers[temp1] = tick;
                break;
            case 'M':
                // restore time tick
                fscanf (in_stream, "%i", &temp1);
                tick = markers[temp1];
                break;
            case 'c':
                read_note (in_stream, out_stream, channel, 0, octave, velocity, length, &tick);
                break;
            case 'd':
                read_note (in_stream, out_stream, channel, 2, octave, velocity, length, &tick);
                break;
            case 'e':
                read_note (in_stream, out_stream, channel, 4, octave, velocity, length, &tick);
                break;
            case 'f':
                read_note (in_stream, out_stream, channel, 5, octave, velocity, length, &tick);
                break;
            case 'g':
                read_note (in_stream, out_stream, channel, 7, octave, velocity, length, &tick);
                break;
            case 'a':
                read_note (in_stream, out_stream, channel, 9, octave, velocity, length, &tick);
                break;
            case 'b':
                read_note (in_stream, out_stream, channel, 11, octave, velocity, length, &tick);
                break;
        }
    }
}

int main (int argc, char **argv) {

    // print the header
    header (stdout, 0, 1, DIVISION);

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
