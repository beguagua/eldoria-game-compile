"""Generate small original procedural SFX for Eldoria using only Python's standard library."""
from __future__ import annotations
import math
import os
import random
import wave
from array import array

OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "audio")
RATE = 44100
random.seed(1337)


def write_wav(name: str, samples: list[float]) -> None:
    os.makedirs(OUT, exist_ok=True)
    pcm = array("h", [max(-32767, min(32767, int(v * 32767))) for v in samples])
    with wave.open(os.path.join(OUT, name), "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(RATE)
        f.writeframes(pcm.tobytes())


def tone(seconds: float, freqs: list[float], volume: float = 0.35, sweep: float = 0.0, noise: float = 0.0) -> list[float]:
    n = int(seconds * RATE)
    out: list[float] = []
    for i in range(n):
        t = i / RATE
        phase = 0.0
        for j, base in enumerate(freqs):
            freq = base + sweep * t * (j + 1)
            phase += math.sin(2.0 * math.pi * freq * t) / (j + 1)
        attack = min(1.0, t / 0.012)
        release = min(1.0, max(0.0, (seconds - t) / 0.055))
        env = attack * release
        out.append((phase * 0.62 + random.uniform(-noise, noise)) * volume * env)
    return out


write_wav("sfx_attack.wav", tone(0.16, [180, 360], 0.38, 260.0, 0.015))
write_wav("sfx_hit.wav", tone(0.20, [92, 145, 230], 0.42, -180.0, 0.025))
write_wav("sfx_pickup.wav", tone(0.28, [520, 780, 1040], 0.26, 160.0, 0.0))
write_wav("sfx_craft.wav", tone(0.42, [260, 390, 620, 920], 0.24, 240.0, 0.0))
write_wav("sfx_portal.wav", tone(0.80, [110, 220, 440, 880], 0.18, 420.0, 0.008))
write_wav("sfx_hurt.wav", tone(0.24, [130, 90], 0.32, -140.0, 0.02))
write_wav("sfx_victory.wav", tone(1.10, [392, 494, 587, 784], 0.22, 90.0, 0.0))
print(f"Generated audio assets in {OUT}")
