#include <cstdio>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cassert>
#include <unistd.h>

using namespace std;

// print a number in variable length format
void print_variable_length (ostream &stream, uint32_t value) {

    // figure out how many bytes it is
    int length = 1;
    int copy = value;
    while (copy >>= 7)
        length++;

    // now put the bytes
    while (length--)
        stream << (char) (((value >> (7 * length)) & 0x7f) | (length ? 0x80 : 0));
}

void print_uint16 (ostream &stream, uint16_t value) {
    stream << (char) ((value >> (8 * 1)) & 0xff);
    stream << (char) ((value >> (8 * 0)) & 0xff);
}

void print_uint24 (ostream &stream, uint32_t value) {
    stream << (char) ((value >> (8 * 2)) & 0xff);
    stream << (char) ((value >> (8 * 1)) & 0xff);
    stream << (char) ((value >> (8 * 0)) & 0xff);
}

void print_uint32 (ostream &stream, uint32_t value) {
    stream << (char) ((value >> (8 * 3)) & 0xff);
    stream << (char) ((value >> (8 * 2)) & 0xff);
    stream << (char) ((value >> (8 * 1)) & 0xff);
    stream << (char) ((value >> (8 * 0)) & 0xff);
}

// rerpresents a midi event
class MidiEvent {
    protected:
        int tick;

    public:
        MidiEvent (int tick)
            : tick (tick) {}
        virtual ~MidiEvent () {};

        // write event to stream
        virtual void write (ostream &stream) = 0;

        // compare two midi events to sort them
        static bool compare (MidiEvent *a, MidiEvent *b) {
            return a->tick < b->tick;
        }

        // get delta time given current tick
        int get_delta_time (int tick) {
            return this->tick - tick;
        }
};

// midi note off event
class NoteOffEvent : public MidiEvent {
    private:
        uint8_t channel;
        uint8_t note;
        uint8_t velocity;

    public:
        NoteOffEvent (int tick, uint8_t channel, uint8_t note, uint8_t velocity)
            : MidiEvent (tick), channel (channel), note (note), velocity (velocity) {}
        ~NoteOffEvent () {}

        void write (ostream &stream) {
            stream << (char) (0x80 | (channel & 0x0f));
            stream << (char) (note & 0x7f);
            stream << (char) (velocity & 0x7f);
        }
};

// midi note on event
class NoteOnEvent : public MidiEvent {
    private:
        uint8_t channel;
        uint8_t note;
        uint8_t velocity;

    public:
        NoteOnEvent (int tick, uint8_t channel, uint8_t note, uint8_t velocity)
            : MidiEvent (tick), channel (channel), note (note), velocity (velocity) {}
        ~NoteOnEvent () {}

        void write (ostream &stream) {
            stream << (char) (0x90 | (channel & 0x0f));
            stream << (char) (note & 0x7f);
            stream << (char) (velocity & 0x7f);
        }
};

// midi controller event
class ControllerEvent : public MidiEvent {
    private:
        uint8_t channel;
        uint8_t controller;
        uint8_t value;

    public:
        ControllerEvent (int tick, uint8_t channel, uint8_t controller, uint8_t value)
            : MidiEvent (tick), channel (channel), controller (controller), value (value) {}
        ~ControllerEvent () {}

        void write (ostream &stream) {
            stream << (char) (0xb0 | (channel & 0x0f));
            stream << (char) (controller & 0x7f);
            stream << (char) (value & 0x7f);
        }
};

// midi program change event
class ProgramChangeEvent : public MidiEvent {
    private:
        uint8_t channel;
        uint8_t program_id;

    public:
        ProgramChangeEvent (int tick, uint8_t channel, uint8_t program_id)
            : MidiEvent (tick), channel (channel), program_id (program_id) {}
        ~ProgramChangeEvent () {}

        void write (ostream &stream) {
            stream << (char) (0xc0 | (channel & 0x0f));
            stream << (char) (program_id & 0x7f);
        }
};

// set tempo event
class TempoEvent : public MidiEvent {
    private:
        int bpm;

    public:
        TempoEvent (int tick, uint8_t bpm)
            : MidiEvent (tick), bpm (bpm) {};
        ~TempoEvent () {}

        void write (ostream &stream) {
            stream << (char) 0xff;
            stream << (char) 0x51;
            stream << (char) 0x03;
            print_uint24 (stream, 60000000 / bpm);
        }
};

// represents a midi file, duh
class MidiFile {
    private:
        int division;               // midi time ticks per quarter
        vector<MidiEvent *> events;   // midi events

        // print a midi chunk
        static void chunk (ostream &stream, string type, string data) {
            stream << type;
            print_uint32 (stream, data.size ());
            stream << data;
        }

        // print a midi header chunk
        static void header (ostream &stream, uint16_t format, uint16_t num_tracks, uint16_t division) {
            stringstream data;
            print_uint16 (data, format);
            print_uint16 (data, num_tracks);
            print_uint16 (data, division);
            chunk (stream, "MThd", data.str ());
        }

        // print a track chunk
        static void track (ostream &stream, vector<MidiEvent *> events) {

            // first sort all the events by time before writing them
            sort (events.begin (), events.end (), MidiEvent::compare);

            // then write them out sequentially to a buffer
            stringstream data;
            int tick = 0;
            for (auto *event : events) {

                // write the delta time
                int delta_time = event->get_delta_time (tick);
                print_variable_length (data, delta_time);
                tick += delta_time;

                // then write the event data
                event->write (data);
            }

            // then write a track chunk with that buffer
            chunk (stream, "MTrk", data.str ());
        }

    public:
        MidiFile (int division = 128) : division (division) {}
        ~MidiFile () {
            for (auto *event : events) {
                delete event;
            }
        }

        // return how many events are currently collected
        int size () {
            return events.size ();
        }

        // write the midi file out to a stream
        void write (ostream &stream) {

            // write the header first
            header (stream, 0, 1, division);

            // then write the track chunk with all the event data
            track (stream, events);

            // write end of track message
            stream << (char) 0x00;
            stream << (char) 0xff;
            stream << (char) 0x2f;
            stream << (char) 0x00;
        }

        // add a midi event
        void push (MidiEvent *event) {
            events.push_back (event);
        }

        // return the delta time for a given rhythmic value
        int get_delta_time (float length) {
            return division * 4 / length;
        }
};

// read the rest of a note after the note name
void read_note (istream &stream, MidiFile &midi_file, int channel, int note, int octave, int velocity, float length, int *tick, bool chord) {
    int value = note + 12 * (octave + 1);
    int read;
    int done = 0;
    
    while (!done) {
        char next = stream.peek ();
        switch (next) {
            case '+':
                value++;
                stream.get ();
                break;
            case '-':
                value--;
                stream.get ();
                break;
            case '/':
                stream.get ();
                stream >> read;
                if (stream.good ())
                    length *= read;
                done = 1;
                break;
            default:
                stream >> read;
                if (stream.good ())
                    length /= read;
                done = 1;
        }
        stream.clear ();
    }

    // push the note on event
    midi_file.push (new NoteOnEvent (*tick, channel, value, velocity));

    // increment the tick by delta time
    int delta_time = midi_file.get_delta_time (length);
    *tick += delta_time;

    // and store the note off event there
    //midi_file.push (new NoteOffEvent (*tick, channel, value, velocity));
    midi_file.push (new NoteOffEvent (*tick, channel, value, 0));

    // if its a chord put the tick back to the beginning
    if (chord)
        *tick -= delta_time;
}

// read the rest of the line
void consume_until (istream &stream, const char until) {
    char c;
    do {
        c = stream.get ();
    } while (c != until);
}

// the top level compile function
void compile (istream &stream, MidiFile &midi_file, int &tick, int offset = 0, bool final_repeat = false, bool chord = false, int channel = 0, int octave = 4, float length = 4, int velocity = 80, int max_markers = 256) {

    int markers[max_markers];   // tick markers

    // temp stuff for awful coding
    MidiFile awful (0);

    // get the next char
    int temp1, temp2, temp3, temp4;
    char c;
    while ((c = stream.get ()) != EOF) {
        switch (c) {
            case '(':
                // TODO: rewrite this garbage
                // start of a cram sequence
                // save start position
                temp2 = stream.tellg ();
                // seek to end
                consume_until (stream, ')');
                // attempt to read rhymic multiplier 
                stream >> temp1;
                if (!stream.good ())
                    temp1 = 1;  // no multiplier by default
                stream.clear ();
                // seek back to beginning
                stream.seekg (temp2);
                // now... figure out how many dang notes there are!!
                // using this ABSOLUTEL AWFUL METHOD
                temp3 = 0;
                compile (cin, awful, temp3, 0);
                temp3 = awful.size () / 2; // bc note offs........
                // seek back to beginning
                stream.seekg (temp2);
                // and nowe.... get em for real
                compile (stream, midi_file, tick, tick, final_repeat, chord,
                         channel, octave, temp1 * length * temp3, velocity, max_markers);
                break;
            case ')':
                // end of cram sequence
                return;
            case '[':
                // start of chord
                // save start position
                temp2 = stream.tellg ();
                // seek to end
                consume_until (stream, ']');
                // now attempt to read a rhythmic value at teh end of this chord
                stream >> temp1;
                if (!stream.good ())
                    temp1 = 1;  // no multiplier by default
                stream.clear ();
                // seek back to beginning
                stream.seekg (temp2);
                compile (stream, midi_file, tick, tick, final_repeat, true,
                         channel, octave, length / temp1, velocity, max_markers);
                // and move the tick back to the end
                tick += midi_file.get_delta_time (length / temp1);
                break;
            case ']':
                // end of chord
                return;
            case '{':
                // TODO: rewrite this garbage
                // start of repeat
                // see how many times to repeat
                // try to read a repeat count if possible
                stream >> temp1;
                if (!stream.good ())
                    temp1 = 2;  // repeat twice by default
                stream.clear ();
                // save the read point
                temp2 = stream.tellg ();
                for (int i = 0; i < temp1; i++) {
                    temp3 = i == (temp1 - 1);
                    stream.seekg (temp2);
                    compile (stream, midi_file, tick, tick, temp3, chord,
                             channel, octave, length, velocity, max_markers);
                    if (temp3)      // on last repeat
                        stream.seekg (temp4);
                    else
                        temp4 = stream.tellg ();
                }
                break;
            case '}':
                // end of repeat
                return;
            case '|':
                // end of repeat if its the last repeat
                if (final_repeat)
                    return;
                break;
            case ';':
                // comment
                consume_until (stream, '\n');
                break;
            case 'T':
                // set bpm
                stream >> temp1;
                midi_file.push (new TempoEvent (tick, temp1));
                break;
            case 't':
                // set tick
                stream >> tick;
                tick += offset;
                break;
            case 'C':
                // set channel
                stream >> channel;
                break;
            case 'o':
                // set octave
                stream >> octave;
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
                stream >> length;
                break;
            case 'v':
                // set velocity
                stream >> velocity;
                break;
            case '@':
                // send program change command
                stream >> temp1;
                midi_file.push (new ProgramChangeEvent (tick, channel, temp1));
                break;
            case '$':
                // send midi controller message
                stream >> temp1;
                assert (stream.get () == ',');
                stream >> temp2;
                midi_file.push (new ControllerEvent (tick, channel, temp1, temp2));
                break;
            case 'm':
                // store current time tick
                stream >> temp1;
                markers[temp1] = tick;
                break;
            case 'M':
                // restore time tick
                stream >> temp1;
                tick = markers[temp1];
                break;
            case 'c':
                read_note (stream, midi_file, channel, 0, octave, velocity, length, &tick, chord);
                break;
            case 'd':
                read_note (stream, midi_file, channel, 2, octave, velocity, length, &tick, chord);
                break;
            case 'e':
                read_note (stream, midi_file, channel, 4, octave, velocity, length, &tick, chord);
                break;
            case 'f':
                read_note (stream, midi_file, channel, 5, octave, velocity, length, &tick, chord);
                break;
            case 'g':
                read_note (stream, midi_file, channel, 7, octave, velocity, length, &tick, chord);
                break;
            case 'a':
                read_note (stream, midi_file, channel, 9, octave, velocity, length, &tick, chord);
                break;
            case 'b':
                read_note (stream, midi_file, channel, 11, octave, velocity, length, &tick, chord);
                break;
            case 'r':
                // rest!!!
                // attempt to read rhymic multiplier 
                stream >> temp1;
                if (!stream.good ())
                    temp1 = 1;  // no multiplier by default
                stream.clear ();
                tick += midi_file.get_delta_time (length / temp1);
                break;
        }
    }
}

int main (int argc, char **argv) {

    int division = 128;

    // parse cli args
    int c;
    while ((c = getopt (argc, argv, "d:")) != -1) {
        switch (c) {
            case 'd':
                division = atoi (optarg);
                break;
            default:
                exit (EXIT_FAILURE);
        }
    }

    // create the midi file instance
    MidiFile midi_file (division);

    // compile the source
    int tick = 0;
    compile (cin, midi_file, tick, tick);

    // write the file out
    midi_file.write (cout);

    return 0;
}
