#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check the embedded asset format, lossless hat tables and decoder exports."""
import importlib.util
import re
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("drum_samples", ROOT / "tools/gen_drum_samples.py")
gen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gen)


def wav(path, channels=1, rate=44100, width=2):
    with wave.open(str(path), "wb") as w:
        w.setparams((channels, width, rate, 0, "NONE", "not compressed"))
        w.writeframes(bytes(24))


with tempfile.TemporaryDirectory() as td:
    tmp = Path(td)
    assets, argv = gen.ASSETS, sys.argv
    try:
        gen.ASSETS = tmp
        sys.argv = [str(ROOT / "tools/gen_drum_samples.py"), str(tmp / "missing.h")]
        try:
            gen.main()
        except SystemExit as e:
            assert "local-only" in str(e) and "ch.wav" in str(e) and "oh.wav" in str(e)
        else:
            raise AssertionError("missing local recordings accepted")
        assert not (tmp / "missing.h").exists()
    finally:
        gen.ASSETS, sys.argv = assets, argv
    for channels, rate, width in [(2, 44100, 2), (1, 48000, 2), (1, 44100, 3)]:
        wav(tmp / "bad.wav", channels, rate, width)
        try:
            gen.read_wav(tmp / "bad.wav")
        except ValueError:
            pass
        else:
            raise AssertionError("invalid embedded asset accepted")
    wav(tmp / "bad.wav")
    data = (tmp / "bad.wav").read_bytes()
    (tmp / "bad.wav").write_bytes(data[:-2])
    try:
        gen.read_wav(tmp / "bad.wav")
    except ValueError:
        pass
    else:
        raise AssertionError("truncated embedded asset accepted")
    with wave.open(str(tmp / "bad.wav"), "wb") as w:
        w.setparams((1, 2, 44100, 0, "NONE", "not compressed"))
    try:
        gen.read_wav(tmp / "bad.wav")
    except ValueError:
        pass
    else:
        raise AssertionError("empty embedded asset accepted")

    out = ROOT / "build/gen/x0x_drum_samples.h"
    subprocess.run([sys.executable, str(ROOT / "tools/gen_drum_samples.py"), str(out),
                    "--ref-wavs", str(tmp)], check=True)
    header = out.read_text()
    total = 512
    for name, fn, pcm16 in gen.SAMPLES:
        pcm, _ = gen.read_wav(gen.ASSETS / fn)
        values = [int(v) for v in re.search(rf"{name}\[\d+\] = \{{(.*?)\}};", header, re.S)[1].split(",") if v.strip()]
        expected = pcm if pcm16 else [gen.mulaw_enc(v) for v in pcm]
        assert values == expected, fn
        decoded, _ = gen.read_wav(tmp / ("hh.wav" if fn == "oh.wav" else fn))
        assert decoded == (pcm if pcm16 else [gen.mulaw_dec(v) for v in expected]), fn
        total += len(values) * (2 if pcm16 else 1)
    print(f"drum samples: mono 44.1 kHz/16-bit; PCM hats exact; {total} flash bytes including decode table")
