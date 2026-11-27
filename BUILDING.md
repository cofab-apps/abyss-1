# Build Monster Keys

From the repository root, use Linux x86-64, Git, curl, Python 3.12 with Pillow 13.0.0, and Node 24. The build script fetches the pinned fm1-base, BSP, compiler and SDK inputs. To reuse an existing toolchain, set `JIELI_TOOLCHAIN` and `AC79_SDK`.

```sh
sh scripts/build-monsters.sh
cmp out/monster-keys.fwsc builds/abyss-1-monster-choir-2.1.0.fwsc
sha256sum -c evidence/checksums.txt
node scripts/test-performance.mjs simulator out/monster-keys.fwsc out/test-60 --monsters
ABYSS_MHZ=100 node scripts/test-performance.mjs simulator out/monster-keys.fwsc out/test-100 --monsters
node scripts/test-performance.mjs simulator out/monster-keys.fwsc out/choir60 --choir
ABYSS_MHZ=100 node scripts/test-performance.mjs simulator out/monster-keys.fwsc out/choir100 --choir
```

For the touch-panel check, install Python Playwright and a compatible Chromium. Set `CHROMIUM` if using a system executable.

```sh
python3 scripts/test-simulator-launch.py simulator builds/abyss-1-monster-choir-2.1.0.fwsc
```

[Target and measured results](docs/MONSTER-KEYS.md) · [Source pins](docs/sources.lock)
