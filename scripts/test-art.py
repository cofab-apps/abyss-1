"""Unit-test every compressed row and palette against the original portrait pixels."""
import importlib.util
import unittest
from pathlib import Path
from PIL import Image

path = Path(__file__).with_name("build-creatures.py")
spec = importlib.util.spec_from_file_location("creatures", path)
creatures = importlib.util.module_from_spec(spec)
spec.loader.exec_module(creatures)


class PortraitTests(unittest.TestCase):
    def test_lcd_encoding(self):
        for i in range(16):
            with self.subTest(creature=i):
                source = creatures.ART / "assets" / f"{i:02d}.png"
                palette, offsets, data = creatures.portrait(source)
                image = Image.open(source).convert("RGB").quantize(colors=16, dither=Image.Dither.NONE).convert("RGB")
                encoded = bytearray()
                for start, end in zip(offsets, offsets[1:] + [len(data)]):
                    row = bytearray()
                    for code in data[start:end]:
                        row.extend(palette[code & 15].to_bytes(2,"big") * ((code >> 4) + 1))
                    self.assertEqual(len(row),480)
                    encoded += row
                expected = b"".join((((r>>3)<<11 | (g>>2)<<5 | b>>3).to_bytes(2,"big"))
                                    for r,g,b in zip(*[iter(image.tobytes())]*3))
                self.assertEqual(encoded, expected)


if __name__ == "__main__":
    unittest.main()
