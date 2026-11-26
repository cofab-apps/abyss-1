"""Turn actual little-endian RGB565 LCD dumps into lossless PNG evidence."""
import hashlib
import json
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

raw, output, beta, base = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)

def digest(data):
    return hashlib.sha256(data).hexdigest()

def decode(path):
    data = path.read_bytes()
    if len(data) != 240*240*2:
        raise ValueError(f"Invalid LCD frame size for {path}")
    return Image.frombytes("RGB",(240,240),data,"raw","BGR;16")

def caption(draw, xy, label, size=16):
    try:
        font=ImageFont.truetype("DejaVuSans.ttf",size)
    except OSError:
        font=ImageFont.load_default(size=size)
    draw.text(xy,label,fill="#e6eff3",font=font)

screens = {}
for name in ["optimist-previous","orchestra","portrait-00","mix-playing","steps"]:
    frame=decode(raw/(name+".rgb"))
    frame.save(output/(name+".png"))
    screens[name]=frame
sheet=Image.new("RGB",(780,300),"#101820")
draw=ImageDraw.Draw(sheet)
for i,(k,title) in enumerate([("optimist-previous","Previous Optimist build"),("orchestra","ABYSS-1 orchestra"),("portrait-00","ABYSS-1 Bloop")]):
    x=12+i*258
    sheet.paste(screens[k],(x,14))
    caption(draw,(x,265),title)
sheet.save(output/"firmware-comparison.png")
contact=Image.new("RGB",(520,592),"#101820")
draw=ImageDraw.Draw(contact)
names=json.loads((Path(__file__).resolve().parents[1]/"prototype/abyss1/characters.json").read_text())
for i,creature in enumerate(names):
    x,y=(i%4)*130+5,(i//4)*148+4
    frame=decode(raw/f"portrait-{i:02}.rgb")
    contact.paste(frame.resize((120,120),Image.Resampling.NEAREST),(x,y))
    caption(draw,(x,y+121),creature["name"],13)
contact.save(output/"native-portraits.png")
# Compare the artwork above the portrait control legend and inside the live frame.
art_root = Path(__file__).resolve().parents[1]/"prototype/abyss1"
def oracle(i):
    image = Image.open(art_root/names[i]["file"]).convert("RGB").quantize(colors=16, dither=Image.Dither.NONE).convert("RGB")
    pixels = zip(*[iter(image.tobytes())]*3)
    rgb565 = b"".join(((r>>3)<<11 | (g>>2)<<5 | b>>3).to_bytes(2,"little") for r,g,b in pixels)
    return Image.frombytes("RGB",(240,240),rgb565,"raw","BGR;16")

def matches(frame, reference, size):
    box = (3,3,size-3,184 if size == 240 else size-3)
    actual, expected = frame.crop(box).tobytes(), reference.crop(box).tobytes()
    same = sum(actual[i:i+3] == expected[i:i+3] for i in range(0,len(actual),3))
    return {"matching":same,"compared":len(expected)//3}

comparisons = {}
for i in range(16):
    name = f"portrait-{i:02}"
    comparisons[name] = matches(decode(raw/(name+".rgb")),oracle(i),240)
for trk in range(4):
    x,y = trk%2*120,trk//2*120
    tile = screens["orchestra"].crop((x,y,x+120,y+120))
    # Firmware selects columns 0,2,... and rows 0,2,... of the 240px image.
    src=oracle(trk*4); wanted=Image.new("RGB",(120,120))
    wanted.putdata([src.getpixel((x*2,y*2)) for y in range(120) for x in range(120)])
    comparisons[f"orchestra-tile-{trk+1}"] = matches(tile,wanted,120)
for name,counts in comparisons.items():
    assert counts["matching"] == counts["compared"], (name,counts)
manifest={
    "capture":"fm1_web.wasm fm1_lcd(), 240x240, little-endian RGB565, 60 MHz",
    "firmware_sha256":digest(beta.read_bytes()),"previous_sha256":digest(base.read_bytes()),
    "emulator_sha256":digest((Path(__file__).resolve().parents[1]/"simulator/fm1_web.wasm").read_bytes()),
    "picture_interior_pixel_comparisons":comparisons,
    "portrait_legend_starts_at_y":184,
    "raw_lcd_sha256":{p.stem:digest(p.read_bytes()) for p in sorted(raw.glob("*.rgb"))},
    "png_sha256":{p.name:digest(p.read_bytes()) for p in sorted(output.glob("*.png"))},
}
(output/"capture-manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
