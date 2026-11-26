# Building and testing ABYSS-1

Run these commands from the root of a clone of this repository. For playback of an existing package, see the [player guide](docs/MANUAL.md).

| Build target | Input | Local output | Checked-in package |
|---|---|---|---|
| ABYSS creature firmware | [Voice definitions](firmware/voices.json), panel, portraits and four-part config | `out/creatures.fwsc` | [abyss-1-creatures.fwsc](builds/abyss-1-creatures.fwsc) |
| Baseline | Upstream `mots-config` release profile | `out/baseline.fwsc` | [upstream-baseline.fwsc](builds/upstream-baseline.fwsc) |
| Four-part | [four-part.config](four-part.config) | `out/four-part.fwsc` | [four-part-formant.fwsc](builds/four-part-formant.fwsc) |

## Mac handoff

In Terminal, type `sh ` including the space, drag `scripts/mac-inventory.sh` from Finder to Terminal, then press Return. Send its output to the project chat. This checks the shell architecture, Apple silicon/Rosetta flags, OS, command-line tools and tool versions. No credentials or device identifiers are collected. It does not install software.

The build and emulator already work in Work on Linux x86-64. Nothing on the Mac is required to continue implementation here. An Apple silicon Mac running a native shell with Python 3.11+ can use the pinned native compiler. If the Mac is Intel, retain Linux for cross-compilation and use the Mac for browser playback, editing and device acceptance. The checked upstream installer has no Intel Mac compiler archive. No need to install Docker, Emscripten or Homebrew to preview the compiled instrument.

## Build and verify

```sh
python3 -m pip install Pillow
sh scripts/build.sh creatures
python3 scripts/test-art.py
node scripts/test-creatures.mjs simulator out/creatures.fwsc
```

The wrapper checks out the lock revision and recursive submodules, uses upstream setup, and writes distinct outputs under `out/`. Run builds serially because upstream shares a build directory. Setup uses the network, verifies the compiler and three SDK inputs, and may also install optional flashing dependencies into its own build environment. The wrapper never invokes a hardware command. The first checkout is under `work/`. Keep product work in this repository and record upstream changes as versioned patches or in a dedicated fork. The upstream driver’s generated files contain absolute paths to the workspace that produced the evidence.

Browser regression tests use Python Playwright and a compatible Chromium. Set `CHROMIUM` to the executable if using system Chromium. Run `python3 scripts/emulator-smoke.py simulator builds/four-part-formant.fwsc`. Run `python3 scripts/prototype-smoke.py prototype` to test both original browser versions. The portrait estimate uses Pillow and accepts `prototype/abyss1`. The simulator WASM is the downloaded upstream Pages binary. Its hash is in evidence/checksums.txt. Building the emulator itself from Rust is independent of building firmware.


## Verify the checked-in packages

On Linux, run the following from the repository root. On macOS, use `shasum -a 256 -c evidence/checksums.txt` for the checksum step.

```sh
sha256sum -c evidence/checksums.txt
node scripts/four-channels.mjs simulator builds/four-part-formant.fwsc
```

The audio probe tests each MIDI channel separately and reports idle and note RMS. It does not measure four simultaneous voices, device latency or pressure. For a remote cross-build, open **Actions → Build and exercise firmware → Run workflow**. The [workflow](.github/workflows/firmware.yml) builds the creature package and runs the portrait, MIDI and panel tests against it.
