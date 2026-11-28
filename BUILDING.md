# Build Seamonster Choir

From the repository root, use Linux x86-64, Git, curl, Python 3.12 with Pillow 13.0.0, and Node 24. The build script fetches the pinned fm1-base, BSP, compiler and SDK inputs. To reuse an existing toolchain, set `JIELI_TOOLCHAIN` and `AC79_SDK`.

```sh
sh scripts/build-monsters.sh
cmp out/monster-keys.fwsc builds/seamonster-choir-2.5.0.fwsc
sha256sum -c evidence/checksums.txt
node scripts/test-performance.mjs simulator out/monster-keys.fwsc out/test-60 --monsters
ABYSS_MHZ=100 node scripts/test-performance.mjs simulator out/monster-keys.fwsc out/test-100 --monsters
node scripts/test-performance.mjs simulator out/monster-keys.fwsc out/choir60 --choir
ABYSS_MHZ=100 node scripts/test-performance.mjs simulator out/monster-keys.fwsc out/choir100 --choir
for mode in rows sea sample ensemble sound; do
    node scripts/test-performance.mjs simulator out/monster-keys.fwsc "out/$mode-60" "--$mode"
    ABYSS_FLASH=uc25hq80 ABYSS_MHZ=100 node scripts/test-performance.mjs simulator out/monster-keys.fwsc "out/$mode-100" "--$mode"
done
python3 scripts/test-sample-io.py
```

The ensemble suite writes native 240 × 240 RGB565 LCD frames at 10 fps and interleaved stereo signed 16-bit DAC samples at 44.1 kHz. The three capture names are `strum`, `guest-reply` and `shell-order`. Each suite also writes its measurements as JSON. [Combined results and review video](evidence/choir/2.4.0/)

To reproduce the 2.4.0 and 2.5.0 comparison, use the default 60 MHz Puya target for both recordings. The script pairs the same key and gate duration for five creatures. Rendering requires Pillow and FFmpeg with libx264 and AAC.

```sh
node scripts/test-performance.mjs simulator builds/seamonster-choir-2.4.0.fwsc out/cast-before --cast
node scripts/test-performance.mjs simulator builds/seamonster-choir-2.5.0.fwsc out/cast-after --sound
python3 scripts/render-sound-evidence.py out/cast-before out/cast-after out/sound-review
```

The sound suite also runs the older package to write a favourite, restores those sectors into the current package and checks the re-saved filter setting after reboot. [2.5.0 measurements](evidence/choir/2.5.0/results.json)

For the touch-panel check, install Python Playwright and a compatible Chromium. Set `CHROMIUM` if using a system executable.

```sh
python3 scripts/test-sample-io.py --browser
python3 scripts/test-simulator-launch.py simulator builds/seamonster-choir-2.5.0.fwsc
```

[Target and measured results](docs/MONSTER-KEYS.md) · [Source pins](docs/sources.lock)
