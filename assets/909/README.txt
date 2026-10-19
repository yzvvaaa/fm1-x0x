909 samples in this local version:

ch.wav and oh.wav: Hihat Closed 909.aif and Hihat Open 909.aif supplied by the
user from Ableton Live 12 Suite's Core Library/Samples/One Shots/Drums/Hihat/.
Converted from mono 96 kHz/24-bit to mono 44.1 kHz/16-bit PCM using FFmpeg's
64-tap swresample filter and triangular dither. No normalization or trimming.
These recordings retain their own rights; they are not GPL assets.

ride.wav and crash.wav: ER-99 by Matthew Cieplak
(https://github.com/matthewcieplak/er-99), shipped unchanged by 9W9, which this
firmware's drum engine is ported from. Licence: GPL-3.0.

tools/gen_drum_samples.py embeds the hats as int16 PCM arrays and the ride/crash
as 8-bit mu-law in build/gen/x0x_drum_samples.h. All arrays stay in flash.
The hats remain separate voices with Tune/Decay/Level controls and reciprocal
3 ms choke fades (rendering stops by 6 ms). The decay knob adds a progressively
lighter envelope as it approaches 127; at 127 it plays the recording's own tail.
Playback ends at the recording length adjusted for pitch. At unity pitch the
closed recording is 165.3 ms and the open recording is 777.8 ms long.

Embedded sample data including the mu-law decode table: 173997 bytes, up from
111061 (+62936 bytes). The sampler reuses its existing buffer pointer; the voice
determines PCM16 vs mu-law. No sample buffers are copied into RAM and no sample
state was added. The snare/clap/reverb variant uses the existing synthesizers
and buffers; see docs/DRUM-VOICING.md for changes, builds and validation.
