"""Run the FM-1 platform's analog-key model through ABYSS and measure its output."""
from array import array
from math import sqrt
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess
import sys

host = Path(sys.argv[1]).resolve()


def probe(folder, key, velocity, pressure):
    pcm = folder / "voice.pcm"
    script = folder / "play.txt"
    script.write_text(f"""wait 450
tap ENV
turn SELECT 2
key {key} down {velocity} 0
wait 390
key {key} down {velocity} {pressure}
wait 130
pcm {pcm}
wait 180
pcm stop
key {key} up
wait 400
""")
    result = subprocess.run([str(host), str(script), str(folder)], cwd=folder, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    samples = array('i')
    samples.frombytes(pcm.read_bytes())
    assert samples, "host generated no PCM"
    return [s / (1 << 23) for s in samples[::2]]


def rms(signal):
    return sqrt(sum(s*s for s in signal)/len(signal))


with TemporaryDirectory() as temp:
    folder = Path(temp)
    for key in [7,24]:
        a = probe(folder,key,80,0)
        reference = probe(folder,key,80,0)
        b = probe(folder,key,80,85)
        variation = rms([x-y for x,y in zip(a,b)])
        repeat = rms([x-y for x,y in zip(a,reference)])
        soft, firm = (rms(probe(folder,key,v,0)) for v in (35,115))
        print(f"key {key}: repeated waveform delta {repeat:.6f}, pressure delta {variation:.6f}, soft/firm RMS {soft:.6f}/{firm:.6f}")
        assert firm > soft * 1.4 and soft > 0.0001, "strike velocity"
        if key < 20:
            assert variation > repeat * 3 + 0.002, "analog-row pressure"
        else:
            assert variation <= repeat + 0.0001, "top row should have velocity only"
print("PASS simulated contacts: analog-row note velocity and sustained vowel pressure, top-row velocity only")
