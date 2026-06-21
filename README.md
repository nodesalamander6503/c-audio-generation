Generate a `.WAV` audio form using C. Can play "When The Saints Go Marching In" somewhat well! Execute it with this song pattern as an argument if you want: `"Gb3 Bb3 C4 D4:3 R:2  Gb3 Bb3 C4 D4:3 R:2  Gb3 Bb3 C4 D4:2 Bb3:2 Gb3:2 Bb3:2 A3:6"`.

# Features
- Note-by-note playback
- Polyphony
- Arbitrary note sounds (in Hz)
- Sine generation
    - Global timer based (this may be changed later)
    - Does not support non-sine waveforms
    - Does not support samples
- Pattern-to-music

# How To Use
It treats its first argument as a music pattern. Simply follow the following rules to generate music:
- Follow a `A4:2` pattern, with the note, then octave, then colon, then duration. This generates one note.
- All notes are played in order.
- Omit the colon and duration to get the default duration (quarter note).
- Replace the note and octave with simply `R` to create a rest (no sound) instead.

# Future updates
I might add the following later:
- Non-sine waveforms (triangle, sawtooth, maybe noise)
- A way to actually use the polyphony
- Multiple tracks/patterns
- Stereo audio

