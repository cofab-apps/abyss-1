"""Render the real RGB565 display and unnormalized stereo DAC from --cast and --sound."""
import argparse
import subprocess
import tempfile
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("previous", type=Path)
parser.add_argument("candidate", type=Path)
parser.add_argument("destination", type=Path)
args = parser.parse_args()
args.destination.mkdir(parents=True, exist_ok=True)
cast = [(0, "Crab Monster"), (2, "Lernaean Hydra"), (9, "The Bloop"),
        (16, "Colossal Whale"), (17, "SCP-169")]
width = 480
size = (width, 540)
frame_bytes = 240 * 240 * 2

def lcd(pixels):
    return Image.frombytes("RGB", (240, 240), pixels, "raw", "BGR;16")

def label(draw, xy, text, size):
    draw.text(xy, text, font=ImageFont.load_default(size=size), fill="white")

with tempfile.TemporaryDirectory() as temp:
    directory = Path(temp)
    with (directory / "audio.s16").open("wb") as audio, (directory / "frames.rgb").open("wb") as frames:
        for index, name in cast:
            for folder, version in [(args.previous, "2.4.0"), (args.candidate, "2.5.0")]:
                source = folder / f"cast-{index}"
                raw = source.with_suffix(".rgb").read_bytes()
                pcm = source.with_suffix(".s16").read_bytes()
                count, remainder = divmod(len(raw), frame_bytes)
                assert not remainder and count > 0
                pad_bytes = count * 4410 * 4 - len(pcm)
                assert 0 <= pad_bytes < 4410 * 4
                audio.write(pcm + bytes(pad_bytes))
                for offset in range(0, len(raw), frame_bytes):
                    im = Image.new("RGB", size, "#102432")
                    label(ImageDraw.Draw(im), (12, 13), f"{name}   /   {version}", 25)
                    im.paste(lcd(raw[offset:offset+frame_bytes]).resize((480, 480), Image.Resampling.NEAREST), (0, 60))
                    frames.write(im.tobytes())
    subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
                    "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", "480x540", "-r", "10", "-i", str(directory / "frames.rgb"),
                    "-f", "s16le", "-ar", "44100", "-ac", "2", "-i", str(directory / "audio.s16"),
                    "-c:v", "libx264", "-crf", "19", "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "160k",
                    "-movflags", "+faststart", str(args.destination / "listen.mp4")], check=True)
guide = Image.new("RGB", (720, 272), "#102432")
for i, (name, title) in enumerate([("motion", "LFO"), ("contour", "ENV"), ("filters", "EDIT")]):
    guide.paste(lcd((args.candidate / (name + ".rgb")).read_bytes()), (240*i, 32))
    label(ImageDraw.Draw(guide), (240*i+8, 4), title, 20)
guide.save(args.destination / "sound-pages.png", optimize=True)
