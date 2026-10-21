# Clock and live-recording timing candidate

This version builds on the separate 909 hats and snare/clap/reverb changes.
Its package is `build/x0x-1.1.1-timing.fwsc`, identity `FM-1_9010101`.

## Findings

TRS MIDI is decoded from a DMA ring. The firmware polls that ring alongside USB
outside the audio render, then consumes the messages at the next render. Audio
uses 512-frame DMA halves (11.61 ms). The old sequencer armed each external-clock
step directly from the received tick; variable delivery delays therefore became
step jitter. Hardware/transport timing and the source's own jitter also matter;
this investigation does not establish that all audible jitter comes from firmware.

Live drum/303 recording used the last-fired step throughout its whole interval.
A hit just before the next grid line consequently went into the preceding step.

## Changes

- Follow external MIDI with a sample-clock oscillator. Filter the measured period
  over incoming ticks and spread phase corrections over sixteen ticks, retaining
  the source tick count to avoid long-term drift. Batched arrivals are weighted
  by their tick count. USB and TRS use the same sequencer path.
- MIDI Start/Continue wait for the master's next clock instead of starting from
  the internal clock first. Clocks still measure tempo while stopped. Loss of
  clock for half a second returns to internal timing and clears the old estimate.
- Cued parts start on the same external-clock downbeat, rather than one MIDI
  tick later. Swing and the existing rates remain available.
- Publish a nearest-grid position for live drum/303 recording, with the boundary
  halfway between successive grid lines. Include swing, rates, reverse,
  ping-pong, random direction and pattern wrap. This does not change knob-motion
  recording, project formats or the immediate live-key sound.

## Validation

The sequencer test models an ideal clock delivered in batches at 512-frame
boundaries. After acquisition, across 256 steps at each of 30, 60, ... 300 BPM:

| Test | Previous step-phase variation | Candidate variation |
| --- | ---: | ---: |
| 90 BPM | 508 samples / 11.52 ms | 64 samples / 1.45 ms |
| 120 BPM | 500 samples / 11.34 ms | 65 samples / 1.47 ms |
| 180 BPM | 505 samples / 11.45 ms | 96 samples / 2.18 ms |
| Worst across tested tempos | 508 samples / 11.52 ms | 249 samples / 5.65 ms |

These are peak-to-peak simulated variations relative to the source grid, not
measurements at the physical audio output. They do not measure constant output
latency. A before/after comparison is in `build/host/timing-before.log` and
`timing-after.log`; the old source fails the new jitter/recording regressions.

Cold Start, Continue, Stop, missing clock, faster/slower tempo changes, external
swing and pattern cues pass. Nearest-grid tests cover all rates and directions,
including odd-length loops and random look-ahead without consuming its RNG.
The panel scenario `tests/scenarios/record_grid.x0x` checks early and late hits
around the same grid line. All 38 host scenarios pass. The sequencer also passes
AddressSanitizer and UndefinedBehaviorSanitizer.

The compiled target boots both cores in the emulator with 512 KiB of RAM and
runs for 2.5 seconds. Its emulated TRS UART receives all 99 scripted bytes
(85 clock pulses, Start, Stop and four drum notes), reports zero drops and
decodes all 91 messages. The run produces 102912 audio frames without a CPU halt.
The emulator still reports its known unmodeled P33 power-management registers;
its instruction timing is not a hardware latency measurement. Logs are in
`build/emulator/timing-trs.log`.

The target build passes flash, RAM, pool, HAL and single-precision checks:

| Region | Used | Capacity | Free |
| --- | ---: | ---: | ---: |
| App flash | 544032 B | 581564 B | 37532 B |
| RAM .data + .bss | 97736 B | 98304 B | 568 B |
| Buffer pool | 335808 B | 344064 B | 8256 B |

Compared with the voicing build, this adds 784 bytes of app flash and 32 bytes of
RAM; the pool and sample storage are unchanged. The audio buffer size is unchanged.

Physical TRS clock-to-audio timing and live-key latency have not been measured.
The follower cannot remove the source's jitter or the device's fixed audio
latency, and deliberate tempo changes require acquisition time. This package
was flashed to the user's FM-1 on 2026-10-21; the updater verified identity
`FM-1_9010101`, and the saved project matches the pre-update backup exactly.
