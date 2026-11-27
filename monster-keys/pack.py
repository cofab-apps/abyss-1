"""Pack the portrait atlas and build the four vowel tables for each creature."""
from math import cos, exp, log2, pi, sin
from pathlib import Path
import sys
from PIL import Image

ROOT = Path(__file__).parent
OUT = Path(sys.argv[1])
SIZE = 168
# Spectral resonances (in harmonics), glottal brightness, odd/even balance.
# Each model has a separate envelope and articulation in voices.h.
spectra = [
    (4, 13, .85, .2), (3, 7, .76, .9), (3, 11, .88, .6), (2, 6, .65, .5),
    (4, 9, .84, 1), (5, 14, .90, .2), (3, 16, .82, .35), (2, 9, .73, .2),
    (4, 7, .78, .3), (2, 5, .62, .92), (2, 6, .88, .16), (4, 15, .92, .45),
    (6, 11, .90, .6), (2, 4, .73, .7), (3, 8, .84, .27), (5, 16, .87, .17),
    (2, 7, .68, 1), (3, 12, .84, .14), (4, 10, .89, .8), (3, 14, .85, .55),
]

def wave(model, shape):
    f1, f2, brightness, even = model
    f1 *= (.57, .9, 1.35, 1.72)[shape]
    f2 *= (.72, .91, 1.14, .92)[shape]
    partials = []
    for h in range(1, 25):
        def peak(center, width):
            return exp(-.5 * (log2(h / center) / width) ** 2)
        weight = (.7 * (h == 1) + .19/h + 1.05*peak(f1,.30) + .78*peak(f2,.19))
        partials.append(weight * brightness**(h/3) * (1 if h%2 else even))
    samples = [sum(w * sin(2*pi*(k+1)*x/256 + .4*cos(k)) for k,w in enumerate(partials))
               for x in range(256)]
    peak = max(abs(s) for s in samples)
    return [round(120*s/peak) for s in samples]

def emit(out, typ, name, values):
    out.write(f"static const {typ} {name}[] = {{\n")
    for i in range(0,len(values),48):
        out.write(",".join(map(str,values[i:i+48]))+",\n")
    out.write("};\n")

with (OUT/"monster-data.h").open("w") as out:
    atlas = Image.open(ROOT/"bestiary.png").convert("RGB")
    assert atlas.size == (4*SIZE, 5*SIZE)
    palettes, packed = [], bytearray()
    for i in range(20):
        x, y = i%4*SIZE, i//4*SIZE
        tile = atlas.crop((x,y,x+SIZE,y+SIZE)).quantize(64, dither=Image.Dither.NONE)
        pal = tile.getpalette()[:192]
        pal.extend([0]*(192-len(pal)))
        for r,g,b in zip(pal[::3],pal[1::3],pal[2::3]):
            rgb = ((r>>3)<<11)|((g>>2)<<5)|(b>>3)
            palettes.append((rgb>>8)|((rgb&255)<<8))
        px = tile.tobytes()
        for a,b,c,d in zip(px[::4],px[1::4],px[2::4],px[3::4]):
            packed.extend((a<<2|b>>4, (b&15)<<4|c>>2, (c&3)<<6|d))
    emit(out,"uint16_t","monster_palette",palettes)
    emit(out,"uint8_t","monster_pixels",packed)
    emit(out,"int8_t","mouth_waves",[v for m in spectra for s in range(4) for v in wave(m,s)])
    emit(out,"int8_t","sine_wave",[round(126*sin(2*pi*i/256)) for i in range(256)])
