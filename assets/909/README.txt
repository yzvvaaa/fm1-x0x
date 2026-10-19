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
The hats remain separate voices with their existing Tune/Decay/Level controls
and reciprocal 3 ms choke fade (rendering stops by 6 ms). The existing decay
envelopes still shorten the recordings; they are not bypassed.

Sample data including the mu-law decode table: 173997 bytes, up from 111061
(+62936 bytes, before code/alignment changes). The sampler reuses its existing
buffer pointer; the voice determines PCM16 vs mu-law. No additional RAM or pool
space is used, and no sample buffers are copied into RAM.

Target builds with the same compiler, fonts and default built-in break bank:
                    original       separate hats      capacity / free
app image           480032 B       543024 B           581564 B / 38540 B
RAM .data + .bss     97704 B        97704 B            98304 B / 600 B
buffer pool         335808 B       335808 B           344064 B / 8256 B
The app grew 62992 bytes including code and alignment. Both RAM regions are
tight already; the pool has 64 bytes beyond the build's required 8192-byte
reserve. tools/build.py's flash, RAM, pool, HAL and single-precision checks pass.

Validation on this Mac:
- Asset validation and exact PCM export; playback, interpolation, sample ends,
  reciprocal choke, tuning extremes and retriggers pass, also with AddressSanitizer.
- All 395 comparisons in tests/host/run_drum909.sh pass using 9W9 10cbe5c
  (NINEW9_DIR=/tmp/fm1-hats-9w9-ref); the reference wrapper uses the new hat
  recordings with its original DSP. The earlier unmodified firmware also passed.
- Drum pan/stereo checks and groove/plock/keysound/stereo simulator scenarios pass.
- UndefinedBehaviorSanitizer reports the existing signed shift in fastmath.h:57;
  reproduced on the unmodified firmware, left outside this change.
- The target package builds using Apple Container 1.5.0, Debian bookworm amd64,
  JieLi clang 4.0.1 (jieli-linux-toolchains-20260730.1), and AC79 SDK d179b44.
  The three SDK assets match tools/build.py's expected SHA-256 checksums.
- Native fm1-emu / fm1-live at 19177bd builds; all 49 ISA/core/SoC tests pass.
  The final package boots both cores and renders closed, open and choked hats.
  Captures: build/emulator/hats-final.wav and hats-original.wav (same MIDI
  schedule, 240 MHz emulated clock, 44.1 kHz stereo PCM16, no clipped samples).
  The emulator's FM1_BENCH_WAV output is raw PCM; the .wav files add a WAV header
  without changing the samples. build/emulator/panel.png shows the booted UI.
- The final app's RAM dump leaves the region above 512 KiB untouched. The
  batch runner also plays closed/open/choked hats with --ram 524288: 1.8 s,
  71680 audio frames, no halt/clipping, eng_guard_count 0. Results are in
  build/emulator/hats-final-512k.log. On-device timing/listening is unverified.
- The interpreter logs unmodeled P33 accesses (0x13E08/0x13E0C), the power
  management register interface in firmware/hal/fm1_sys.h. This emulator check
  is not a check of every peripheral or of physical flash/cache timing.

Local build environment:
The reusable container is fm1-jieli-hats-k612. Start it with container start,
then run: container exec fm1-jieli-hats-k612 python3 tools/build.py
It mounts this checkout at /work, ~/.jieli read-only at /opt/jieli, and the SDK
read-only at /opt/sdk, with JIELI_DOCKER=0. It has Python/Pillow and a host C
compiler for asset generation; no Docker compatibility wrapper is used.
macOS 15's Rosetta failed with the default 6.18 kernel. This container uses
the Kata 3.24.0 Linux 6.12.47-173 kernel via its own --kernel option:
https://github.com/apple/container/issues/1142
Kernel archive SHA-256:
da934ff8206f5780c101e1a2c087bc32975ab2370385cf4bdad71102b3fb00dc
