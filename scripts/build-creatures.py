"""Cross-build the ABYSS control surface, portraits and patches on the pinned base."""
import json
import re
import subprocess
import sys
from contextlib import ExitStack
from itertools import groupby
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "prototype/abyss1"
FAMILIES = json.loads((ROOT / "firmware/voices.json").read_text())


def portrait(path):
    image = Image.open(path).convert("RGB").quantize(colors=16, dither=Image.Dither.NONE)
    assert image.size == (240, 240)
    palette = image.getpalette()[:48]
    colours = [(r >> 3) << 11 | (g >> 2) << 5 | b >> 3 for r,g,b in zip(*[iter(palette)]*3)]
    pixels = image.tobytes()
    offsets, runs = [], []
    for row in range(240):
        offsets.append(len(runs))
        for colour, group in groupby(pixels[row*240:(row+1)*240]):
            count = sum(1 for _ in group)
            while count:
                n = min(16, count)
                runs.append((n-1) << 4 | colour)
                count -= n
    return colours, offsets, runs


def generate():
    out = []
    def array(ctype, name, values):
        out.append(f"static const {ctype} {name}[] = {{")
        for i in range(0, len(values), 32):
            out.append(",".join(map(str, values[i:i+32]))+",")
        out.append("};")
    for i, item in enumerate(json.loads((ART/"characters.json").read_text())):
        assert item["id"] == i
        pal, offsets, runs = portrait(ART/item["file"])
        array("uint16_t", f"pal{i}", [(c>>8) | (c&255)<<8 for c in pal])
        array("uint16_t", f"row{i}", offsets)
        array("uint8_t", f"run{i}", runs)
    out.append("static const struct { const uint16_t *pal, *row; const uint8_t *run; } abyss_art[] = {")
    out.extend(f"{{pal{i},row{i},run{i}}}," for i in range(16))
    out.append("};")
    out.append("static const struct { uint8_t engine, preview, macro[4]; } families[] = {")
    for f in FAMILIES:
        out.append("{ENG_SLOT_%s,%d,{%s}}," % (f["engine"], f["preview"], ",".join(f["macros"])))
    return "\n".join(out+["};",""])


def patches(f):
    lines = []
    for name, ops, adsr, transpose, pitch_drop, room in f["patches"]:
        ops, adsr = ",".join(map(str,ops)), ",".join(map(str,adsr))
        lines.append(f'    {{"{name}", {{{ops}}}, {{{adsr}}}, 0, 0, FX(0,0,0,{room}), '
                     f'XP(P_TRANS+1,{transpose},P_E1PIT+1,{pitch_drop})}},')
    return "\n".join(lines)


def build(checkout):
    ui = checkout / "fm1base/optimist/ui"
    with ExitStack() as undo:
        def replace(path, data):
            if path.exists():
                undo.callback(path.write_bytes, path.read_bytes())
            else:
                undo.callback(path.unlink, missing_ok=True)
            path.write_text(data)
        for f in FAMILIES:
            path = checkout/"firmware/src/engines"/f["source"]
            pattern = r"(static const preset_t "+f["table"]+r"\[\] = \{\n).*?(\n\};)"
            data, n = re.subn(pattern, lambda m:m[1]+patches(f)+m[2], path.read_text(), flags=re.S)
            assert n == 1, path
            replace(path, data)
        replace(ui/"abyss_art.h", generate())
        replace(ui/"abyss_panel.h", (ROOT/"firmware/panel.h").read_text())
        p = ui/"opt_ui.c"
        data = p.read_text()
        for old,new,n in [
            ('#include "ui/splash.c"','#include "ui/splash.c"\n#include "abyss_panel.h"',1),
            ('    boot_splash();','    abyss_picture(0, 0, 0, 1);',1),
            ('            set_engine_of(t, trk_def_engine(i));','            abyss_assign(i, i * 4);',1),
            ('            trk_def_sound(t, i);','            /* ABYSS patch already applied. */',1),
            ('            t->p[P_REV] = 0;','            /* Keep the creature room send. */',1),
            ('    ui_input();','    abyss_input();',2),
            ('        ui_draw();','        abyss_draw();',1),
            ('    layers_init();','    layers_init();\n    memset(ly_bit, 0, sizeof ly_bit);\n    abyss_restore();',1),
        ]:
            assert data.count(old)==n, old
            data=data.replace(old,new)
        replace(p,data)
        subprocess.run([sys.executable,"tools/optimist.py","build","--config",str(ROOT/"four-part.config"),
                        "--summary",str(ROOT/"out/creatures.json")], cwd=checkout,check=True)


if __name__ == "__main__":
    build(Path(sys.argv[1]).resolve())
