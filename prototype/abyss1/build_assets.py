from PIL import Image, ImageDraw, ImageFilter, ImageOps
import numpy as np, random, pathlib, json, base64, math
P=pathlib.Path(__file__).parent; A=P/'assets'; A.mkdir(exist_ok=True)
items=[('Bloop','bass','whale'),('Mega Bloop','bass','whale'),('Sea Eater','bass','angler'),('El Gran Majá','bass','maja'),('Kraken','drums','octopus'),('Megalodon','drums','shark'),('Mosasaurus','drums','reptile'),('Hydra','drums','hydra'),('Julia','voice','jelly'),('Ningen','voice','humanoid'),('Upsweep','voice','ray'),('Whistle','voice','eel'),('Space Bloop','atmos','whale'),('Space Julia','atmos','jelly'),('World Eater','atmos','planet'),('Moon Woke Up','atmos','moon')]
palette=['#83b4a5','#496a91','#4c9c7d','#387ba8','#b85e6e','#8aabbc','#6b9a74','#bda16a','#bc8bdb','#d8e5e2','#3e9cbd','#dbb65f','#8775d5','#e79fc9','#a57968','#b9c8d8']
for i,(name,role,kind) in enumerate(items):
 if i==0: im=Image.open(A/'bloop.png').convert('RGB').resize((240,240))
 else:
  rng=random.Random(120+i); S=480; im=Image.new('RGB',(S,S)); a=np.zeros((S,S,3),dtype=np.uint8)
  for y in range(S):
   for x in range(S):
    light=max(0,1-math.hypot((x-180)/400,(y-80)/400)); a[y,x]=[int(4+12*light),int(12+48*light),int(24+66*light)] if i<12 else [int(8+22*light),int(5+10*light),int(24+67*light)]
  im=Image.fromarray(a); d=ImageDraw.Draw(im); c=palette[i]; edge='#112635'; white='#d6ede5'; dark='#07121d'
  for k in range(90):
   x=rng.randrange(S);y=rng.randrange(S);r=rng.choice([1,2,3]);d.ellipse((x,y,x+r,y+r),fill='#718ca1')
  def ellipse(b,fill=c,outline=edge,w=5):d.ellipse(b,fill,outline,w)
  def poly(p,fill=c):d.polygon(p,fill);d.line(p+[p[0]],fill=edge,width=4,joint='curve')
  def eyes(n,x,y,spacing=27):
   for j in range(n):
    xx=x+j*spacing;ellipse((xx,y,xx+18,y+18),white,edge,2);ellipse((xx+6,y+5,xx+13,y+13),dark,dark,1)
  def teeth(x,y,w,h):
   ellipse((x,y,x+w,y+h),dark,edge,5)
   for j in range(9):
    xx=x+12+j*(w-24)/9; d.polygon([(xx,y+8),(xx+14,y+8),(xx+7,y+27)],white); d.polygon([(xx,y+h-8),(xx+14,y+h-8),(xx+7,y+h-24)],white)
  if kind in ('whale','shark','reptile','maja','angler','eel'):
   poly([(300,210),(447,123),(421,220),(458,310),(300,267)])
   if kind=='shark':poly([(175,142),(236,49),(274,155)])
   if kind=='reptile':poly([(130,275),(80,390),(230,291)]);poly([(275,260),(340,360),(350,263)])
   if kind=='eel':d.line([(190,252),(306,320),(395,279),(420,170)],fill=c,width=70,joint='curve')
   ellipse((42,92,362,355));
   if kind=='maja': eyes(6,80,145,34);teeth(63,183,270,123)
   elif kind=='whale':eyes(1,115,130);teeth(54,155,205,185)
   else:eyes(2,93,143,80);teeth(55,205,220,91)
   if kind=='angler':d.arc((90,25,240,170),180,300,fill=c,width=9);ellipse((95,30,130,65),'#f7df8b',edge,2)
  elif kind in ('octopus','jelly','hydra'):
   for j in range(8 if kind=='octopus' else 6):
    x=55+j*53; pts=[(240+(j-3)*22,220),(x,300),(x+(-1)**j*35,390),(x,440)]
    d.line(pts,fill=edge,width=37,joint='curve');d.line(pts,fill=c,width=29,joint='curve')
   if kind=='hydra':
    for j in range(3):
     x=90+j*120;d.line([(240,300),(x,170),(x,95)],fill=c,width=55,joint='curve');ellipse((x-50,45,x+50,135));eyes(2,x-30,72,37)
   else:ellipse((83,65,397,295));eyes(2,175,147,105);teeth(168,195,143,65)
  elif kind=='humanoid':
   ellipse((140,52,335,248));poly([(190,218),(290,218),(350,420),(240,370),(130,420)]);d.line([(190,243),(60,340),(35,390)],fill=c,width=45);d.line([(285,243),(425,340),(450,390)],fill=c,width=45);eyes(2,177,125,95);d.arc((190,154,290,220),10,170,fill=edge,width=7)
  elif kind=='ray':poly([(18,140),(230,85),(462,140),(330,310),(240,280),(120,310)]);d.line([(240,260),(264,350),(230,450)],fill=c,width=20);eyes(2,165,155,120);teeth(192,190,96,49)
  else:
   ellipse((65,65,415,415));
   for j in range(12):
    x=rng.randrange(110,345);y=rng.randrange(100,350);r=rng.randrange(12,42);d.ellipse((x-r,y-r,x+r,y+r),outline=edge,width=4)
   eyes(2,153,166,150);teeth(110,224,260,108)
  im=im.resize((240,240),Image.Resampling.LANCZOS)
 arr=np.array(im).astype('uint16'); r=arr[:,:,0]>>3;g=arr[:,:,1]>>2;b=arr[:,:,2]>>3; raw=((r<<11)|(g<<5)|b).astype('<u2'); raw.tofile(A/f'{i:02}.rgb565'); preview=Image.fromarray(np.stack([r*255//31,g*255//63,b*255//31],2).astype('uint8'));preview.save(A/f'{i:02}.png'); preview.resize((120,120),Image.Resampling.LANCZOS).save(A/f'{i:02}-120.png')
manifest=[dict(id=i,name=n,role=r,art='user-upload; rights unverified' if i==0 else 'procedural placeholder; likeness unverified',file=f'assets/{i:02}.png') for i,(n,r,k) in enumerate(items)]
(P/'characters.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False))
# four tile view and selection, native 240x240
canvas=Image.new('RGB',(240,240));
for j,i in enumerate([0,4,8,12]):canvas.paste(Image.open(A/f'{i:02}-120.png'),((j%2)*120,(j//2)*120))
canvas.save(P/'orchestra-240.png'); Image.open(A/'00.png').save(P/'selection-240.png')
