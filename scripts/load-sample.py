"""Load a local PCM WAV into the shell sampler over USB MIDI."""
import argparse
import binascii
import math
import secrets
import time
import wave

RATE = 11025
PREFIX = (0x7D, 83, 77, 80)


def read_clip(path, start):
    with wave.open(str(path), "rb") as wav:
        rate, width, channels = wav.getframerate(), wav.getsampwidth(), wav.getnchannels()
        if width not in (1, 2, 3, 4) or not 0 <= start < wav.getnframes() / rate:
            raise ValueError("Choose a PCM WAV and a start time within the recording.")
        wav.setpos(int(start * rate))
        data = wav.readframes(rate * 2)
    stride = width * channels
    frames = []
    for pos in range(0, len(data), stride):
        value = 0
        for ch in range(channels):
            off = pos + ch * width
            value += ((data[off] - 128) if width == 1 else
                      int.from_bytes(data[off:off + width], "little", signed=True))
        frames.append(value / (channels * 2 ** (width * 8 - 1)))
    count = min(2 * RATE, len(frames) * RATE // rate)
    if count < 64:
        raise ValueError("Select at least six milliseconds of audio.")
    payload = bytearray()
    for i in range(count):
        lo, hi = i * rate / RATE, (i + 1) * rate / RATE
        # Average each source interval when downsampling. Interpolate when upsampling.
        a = int(lo)
        if rate >= RATE:
            val = sum(frames[j] * (min(j + 1, hi) - max(j, lo))
                      for j in range(a, min(len(frames), math.ceil(hi)))) / (hi - lo)
        else:
            val = frames[a] + (frames[min(a + 1, len(frames) - 1)] - frames[a]) * (lo - a)
        word = min(16383, max(0, round(val * 8192) + 8192))
        payload.extend((word & 127, word >> 7))
    return payload


def packed(n):
    return [n & 127, n >> 7 & 127, n >> 14 & 127]


def packets(payload, tag):
    size = len(payload) // 2
    yield [*PREFIX, 1, tag, *packed(size), *packed(binascii.crc_hqx(payload, 0xFFFF))], 0
    for off in range(0, size, 96):
        chunk = payload[2 * off:2 * (off + 96)]
        yield [*PREFIX, 2, tag, *packed(off), *chunk], off + len(chunk) // 2
    yield [*PREFIX, 3, tag], size


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wav", nargs="?")
    parser.add_argument("--start", type=float, default=0, help="start in seconds, up to two seconds are loaded")
    parser.add_argument("--list", action="store_true", help="list MIDI ports")
    parser.add_argument("--out-port", help="exact name of the FM-1 MIDI output on the computer")
    parser.add_argument("--in-port", help="exact name of its MIDI input for acknowledgements")
    args = parser.parse_args()
    try:
        import mido
    except ImportError:
        parser.exit(2, "Install the MIDI helper with: python3 -m pip install mido python-rtmidi\n")
    if args.list:
        print("Outputs:", *mido.get_output_names(), sep="\n  ")
        print("Inputs:", *mido.get_input_names(), sep="\n  ")
        return
    if not (args.wav and args.out_port and args.in_port):
        parser.error("Use --list, then supply a WAV, --out-port and --in-port.")
    payload = read_clip(args.wav, args.start)
    tag = secrets.randbelow(128)
    with mido.open_input(args.in_port) as source, mido.open_output(args.out_port) as dest:
        for frame, offset in packets(payload, tag):
            dest.send(mido.Message("sysex", data=frame))
            deadline = time.monotonic() + 3
            while time.monotonic() < deadline:
                for msg in source.iter_pending():
                    if msg.type != "sysex" or tuple(msg.data[:7]) != (*PREFIX, 4, tag, frame[4]):
                        continue
                    if len(msg.data) != 11 or not msg.data[7] or list(msg.data[8:]) != packed(offset):
                        parser.exit(1, "Transfer declined. Press SEQ for the sampler, finish recording and retry.\n")
                    break
                else:
                    time.sleep(.002)
                    continue
                break
            else:
                parser.exit(1, "No acknowledgement. Check both MIDI ports and the FM-1 sampler screen.\n")
    print(f"Loaded {len(payload) / (2 * RATE):.2f} seconds. Press PLAY or a key on the FM-1.")


if __name__ == "__main__":
    main()
