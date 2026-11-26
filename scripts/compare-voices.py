"""Compare two keyboard takes per creature from the FM-1 DAC. Read float32 PCM on stdin."""
import itertools
import sys

import numpy as np

voices, takes, window = 16, 2, 2048
pcm = np.frombuffer(sys.stdin.buffer.read(), dtype="<f4").reshape(voices, takes, -1)
# Retain the attack, sustain and release separately, using broad spectral bands
# to tolerate oscillator phase and individual noise samples. Remove overall gain.
frames = pcm.reshape(voices, takes, -1, window)
magnitudes = np.abs(np.fft.rfft(frames * np.hanning(window), axis=-1)) ** 2
edges = [0, 2, 4, 8, 16, 32, 64, 128, 256, 512, magnitudes.shape[-1]]
bands = np.stack([magnitudes[..., lo:hi].sum(axis=-1) for lo, hi in zip(edges, edges[1:])], axis=-1)
profiles = np.sqrt(bands).reshape(voices, takes, -1)
norm = np.linalg.norm(profiles, axis=-1, keepdims=True)
assert np.all(norm > 0), "silent voice"
profiles /= norm

dist = lambda a, b: float(np.linalg.norm(a - b))
repeat = [dist(*takes_) for takes_ in profiles]
pairs = []
for a, b in itertools.combinations(range(voices), 2):
    separation = min(dist(x, y) for x in profiles[a] for y in profiles[b])
    pairs.append((separation, a, b))
    assert separation > max(repeat[a], repeat[b]) + 0.02, (a, b, separation, repeat[a], repeat[b])

nearest, a, b = min(pairs)
print(f"PASS all {len(pairs)} pairs separated on gain-normalized time/frequency profiles.")
print(f"Closest pair: IDs {a} and {b}, distance {nearest:.4f}. Largest same-voice repeat distance {max(repeat):.4f}.")
