# Building and testing ABYSS-1

The current live instrument builds with `sh scripts/build-monsters.sh`. The [Monster Keys handoff](docs/MONSTER-KEYS.md) gives the pinned inputs, output checksum and simulator tests.

## Legacy sequencer builds

Run these commands from the root of a clone of this repository. For playback of a Beta 1.5 song, see the [player guide](docs/MANUAL.md).

| Build target | Input | Local output | Checked-in package |
|---|---|---|---|
| ABYSS creature firmware | [Voice definitions](firmware/voices.json), panel, portraits and four-part config | `out/creatures.fwsc` | [abyss-1-beta-1.5.fwsc](builds/abyss-1-beta-1.5.fwsc) |
| Baseline | Upstream `mots-config` release profile | `out/baseline.fwsc` | [upstream-baseline.fwsc](builds/upstream-baseline.fwsc) |
| Four-part | [four-part.config](four-part.config) | `out/four-part.fwsc` | [four-part-formant.fwsc](builds/four-part-formant.fwsc) |

## Mac handoff

In Terminal, type `sh ` including the space, drag `scripts/mac-inventory.sh` from Finder to Terminal, then press Return. Send its output to the project chat. This checks the shell architecture, Apple silicon/Rosetta flags, OS, command-line tools and tool versions. No credentials or device identifiers are collected. It does not install software.

The build and emulator already work in Work on Linux x86-64. Nothing on the Mac is required to continue implementation here. An Apple silicon Mac running a native shell with Python 3.11+ can use the pinned native compiler. If the Mac is Intel, retain Linux for cross-compilation and use the Mac for browser playback, editing and device acceptance. The checked upstream installer has no Intel Mac compiler archive. No need to install Docker, Emscripten or Homebrew to preview the compiled instrument.

## Build and verify

```sh
python3 -m pip install Pillow==13.0.0
sh scripts/build.sh creatures
python3 scripts/test-art.py
node scripts/test-creatures.mjs simulator out/creatures.fwsc
```

The wrapper checks out the lock revision and recursive submodules, uses upstream setup, and writes distinct outputs under `out/`. Run builds serially because upstream shares a build directory. Setup uses the network, verifies the compiler and three SDK inputs, and may also install optional flashing dependencies into its own build environment. The wrapper never invokes a hardware command. The first checkout is under `work/`. Keep product work in this repository and record upstream changes as versioned patches or in a dedicated fork. The upstream driver’s generated files contain absolute paths to the workspace that produced the evidence.

Browser regression tests use Python Playwright and a compatible Chromium. Set `CHROMIUM` to the executable if using system Chromium. Run `python3 scripts/emulator-smoke.py simulator builds/four-part-formant.fwsc`. Run `python3 scripts/prototype-smoke.py prototype` to test both original browser versions. The portrait estimate uses Pillow and accepts `prototype/abyss1`. The simulator WASM is the downloaded upstream Pages binary. Its hash is in evidence/checksums.txt. Building the emulator itself from Rust is independent of building firmware.


[Beta capture, soak and key-sensing tests](docs/BETA-1.md#reproduce-the-captures) exercise the packaged artifact and the host contact model.

When publishing a tested build, copy its versioned package to `builds/abyss-1.fwsc` in the same commit. The bundled simulator loads the stable path. The README links the versioned package so its public player can be checked against an exact build.

## Verify the checked-in packages

On Linux, run the following from the repository root. On macOS, use `shasum -a 256 -c evidence/checksums.txt` for the checksum step.

```sh
sha256sum -c evidence/checksums.txt
node scripts/four-channels.mjs simulator builds/four-part-formant.fwsc
```

The audio probe tests each MIDI channel separately and reports idle and note RMS. It does not measure four simultaneous voices, device latency or pressure. For a remote cross-build, open **Actions → Build and exercise firmware → Run workflow**. The [workflow](.github/workflows/firmware.yml) builds the creature package and runs the portrait, MIDI and panel tests against it.


## Beta 1.3 release checks

Use the Beta 1.3 release commit to reproduce these historical checks.

Use Python 3.12, Pillow 13.0.0 and the pinned Linux compiler. The wrapper sets the source timestamp from the pinned upstream commit. The manual workflow builds the current source and exercises that result.

```sh
sh scripts/build.sh creatures
cmp out/creatures.fwsc builds/abyss-1-beta-1.3.fwsc
ABYSS_PRIOR=builds/abyss-1-beta-1.2.fwsc ABYSS_QUARTET_SECONDS=600 node scripts/test-performance.mjs simulator out/creatures.fwsc out/reef --reef
node scripts/test-performance.mjs simulator out/creatures.fwsc out/performance
ABYSS_VOICE_AUDIT=1 ABYSS_ALL_ASSIGNMENTS=1 ABYSS_PLAY_CONTROLS=1 node scripts/test-creatures.mjs simulator out/creatures.fwsc
```

The voice audit requires NumPy. To repeat the touch tests, install Playwright and its Chromium browser, or set `CHROMIUM` to a compatible executable. Run `ABYSS_DEMO_BROWSER=home python3 scripts/test-simulator-launch.py simulator builds/abyss-1.fwsc` and repeat without the environment variable for record, save and reload. For the performance movie, `node scripts/test-performance.mjs simulator out/creatures.fwsc out/film --showcase` produces RGB565LE LCD at 240 × 240, 10 fps, stereo signed 16-bit little-endian PCM at 44.1 kHz, and caption timestamps.

## Beta 1.4 release checks

Build the current firmware, compare it with the published package, and exercise the call-and-response game at the emulated 60 MHz clock.

```sh
sh scripts/build.sh creatures
cmp out/creatures.fwsc builds/abyss-1-beta-1.4.fwsc
node scripts/test-performance.mjs simulator out/creatures.fwsc out/reply --reply
ABYSS_PRIOR=builds/abyss-1-beta-1.3.fwsc ABYSS_QUARTET_SECONDS=600 node scripts/test-performance.mjs simulator out/creatures.fwsc out/reef --reef
ABYSS_REPLY_BROWSER=1 ABYSS_DEMO_BROWSER=home python3 scripts/test-simulator-launch.py simulator builds/abyss-1.fwsc
```

The reply test writes 240 × 240 RGB565LE frames at 10 fps and matching 44.1 kHz stereo signed 16-bit PCM to `out/reply/reply.rgb` and `out/reply/reply.s16`. It checks returned pitches using isolated DAC output. Repeat the browser probe without the two environment variables to exercise recording, persistence and replay.

## Beta 1.4.1 keyboard regression

Run the Beta 1.4 release checks above using `builds/abyss-1-beta-1.4.1.fwsc` for the package comparison and `builds/abyss-1-beta-1.4.fwsc` for `ABYSS_PRIOR`. The reply probe now exercises staggered chord strikes and measures the scanned keyboard LEDs during the call, answer and rest.

## Beta 1.4.2 invitation regression

Run the reply probe against `builds/abyss-1-beta-1.4.2.fwsc`. It waits for both turns of the wordless invitation, measures the ARP lamp, dismisses the invitation by entering the game, and verifies that a restored song never receives it. The reef check dismisses the invitation before exact-pixel animation restoration checks.

```sh
node scripts/test-performance.mjs simulator builds/abyss-1-beta-1.4.2.fwsc out/invitation --reply
ABYSS_PRIOR=builds/abyss-1-beta-1.4.1.fwsc ABYSS_QUARTET_SECONDS=600 node scripts/test-performance.mjs simulator builds/abyss-1-beta-1.4.2.fwsc out/reef --reef
ABYSS_REPLY_BROWSER=1 ABYSS_DEMO_BROWSER=home CHROMIUM=/path/to/chromium python3 scripts/test-simulator-launch.py simulator builds/abyss-1.fwsc
```

## Beta 1.5 duet regression

The reply probe now completes a phrase, checks the pulsing PLAY lamp, converts the exchange into a two-track loop and measures its DAC output. It verifies first-half caller steps, second-half answer steps, two untouched tracks, atomic undo/redo, save/reboot and protection of a pre-existing song. Repeat at both clock rates; then migrate a Beta 1.4.2 save and soak the quartet at 60 MHz.

```sh
sh scripts/build.sh creatures
cmp out/creatures.fwsc builds/abyss-1-beta-1.5.fwsc
ABYSS_PANEL_FILM=1 node scripts/test-performance.mjs simulator out/creatures.fwsc out/duet --reply
ABYSS_MHZ=100 node scripts/test-performance.mjs simulator out/creatures.fwsc out/duet-100 --reply
ABYSS_PRIOR=builds/abyss-1-beta-1.4.2.fwsc ABYSS_QUARTET_SECONDS=600 node scripts/test-performance.mjs simulator out/creatures.fwsc out/reef --reef
```
