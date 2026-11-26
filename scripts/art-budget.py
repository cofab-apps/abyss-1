"""Estimate strip-compressed storage on the supplied portraits; pass extracted abyss1/."""
import csv
from pathlib import Path
import sys
import zlib
from PIL import Image
out = csv.writer(sys.stdout)
out.writerow(['id','raw_rgb565','palette16_strip_deflate_bytes'])
for img in sorted((Path(sys.argv[1])/'assets').glob('[0-9][0-9].png')):
    with Image.open(img) as im:
        assert im.size == (240,240)
        indexed = im.convert('RGB').quantize(colors=16, dither=Image.Dither.NONE)
    pixels = list(indexed.getdata())
    packed = bytes(a << 4 | b for a,b in zip(pixels[::2],pixels[1::2]))
    # Palette: sixteen RGB565 words. 15 strips, each 16 rows, with 16-bit lengths.
    strips = [packed[i:i+1920] for i in range(0,len(packed),1920)]
    encoded = [zlib.compress(strip,9) for strip in strips]
    assert b''.join(map(zlib.decompress, encoded)) == packed
    out.writerow([img.stem,115200,32+sum(2+len(s) for s in encoded)])
