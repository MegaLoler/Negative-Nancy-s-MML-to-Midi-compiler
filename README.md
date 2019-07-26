# Negative Nancy's MML to MIDI compiler

Just another MML to MIDI compiler!  Just because I haven't found anything that's quite what I want, so I'm making my own, ya know? ;o

It's a work in progress, I just started!!! Most of this readme is just what the goal is, rather than what things are right now, lol.

## How to build

Just run `make` to produce `build/nnmmlc`!

If you have `timidity` installed, try `make test` to compile the `twinkle.mml` and play it back!

## How to run

Compile an MML file like this: `nnmmlc < input.mml > output.midi`

## Examples

Check out `examples` for some documented examples!

## Tutorial

It's really simple, really!

Firstly, anything between a `;` and the end of the line is a comment and is ignored.
Whitespace in general is also ignored.

Then there are *commands* and there are *notes*.

The notes are: `c`, `d`, `e`, `f`, `g`, `a`, and `b`. Add a `+` or `-` directly after them to sharpen or flatten respectively: `f+` means F sharp, and `d--` means D double flat!
Actually, there's one more, but it's not really a note, and more like the *lack* of a note, lol, it's the rest: `r`
Also, if you put a number directly after a note, you can multiply its length. If you precede that number with a `/` you can divide the length instead.

A command is a single character followed by a comma separated list of numbers as arguments. (If there is only one argument, there's no comma!) For example: `T80` is the command to set the tempo to 80 bpm, and `$1,127` is the command to send a midi controller message to controller number 1 with value 127!

Here's a table of all the commands:

| Command | Argument 1    | Argument 2 | Example | Explanation                                                                                            |
|:-------:|:-------------:|:----------:|:--------|:-------------------------------------------------------------------------------------------------------|
| `T`     | BPM           |            | `T120`  | Set the tempo.                                                                                         |
| `t`     | Tick          |            | `t0`    | Set the tick counter; all notes and commands entered hereafter will begin from the given tick in time. |
| `C`     | Channel ID    |            | `C10`   | Set the active MIDI channel; all entered hereafter will happen on the given MIDI channel.              |
| `o`     | Octave        |            | `o4`    | Set the octave for notes following.                                                                    |
| `>`     |               |            | `>`     | Increase the active octave by one.                                                                     |
| `<`     |               |            | `<`     | Decrease the active octave by one.                                                                     |
| `l`     | Note value    |            | `l8`    | Set the rhythmic note value for notes entered hereafter.                                               |
| `v`     | Velocity      |            | `v127`  | Set the note velocity for notes entered hereafter.                                                     |
| `$`     | Controller ID | Value      | `$1,80` | Send the given MIDI controller a given value.                                                          |
| `m`     | Label ID      |            | `m0`    | Mark the current time tick so you might return to it later.                                            |
| `M`     | Label ID      |            | `M0`    | Set the current time tick to a previously marked tick.                                                 |
| `@`     | Program ID    |            | `@0`    | Send a program change message.                                                                         |

Anything you put in between `[` and `]` gets played at the same time, like a chord!

Then anything you put in between `(` and `)` gets crammed into the time space of a single note. For example, `l4 cc(dddd)d` will play two quarter note Cs followed by 4 sixteenth note Ds and finally a quarter note D.

And anything between `{` and `}` gets repeated!

## Todo list

- Timing scopes
- Multi-repeats with variable endings
- Make [] and () and {} work 
