# ABYSS-1 · Phase 0 engineering kit

26 November 2026 • Private development snapshot

Two compiled FM-1 packages, a local browser simulator, the build configuration, regression tests and raw evidence are included. The first package is stock **upstream Optimist** built from source. The second selects four assignable melodic parts and enables the formant engine. Both still present Optimist’s controls and sounds. The discovery prototype remains in the original handover.

## Play the compiled firmware

From this directory, run the following using Python 3. Open http://127.0.0.1:8765 in a desktop browser. Select **Open .fwsc…**, choose **builds/four-part-formant.fwsc**, press **Start**, then use the onscreen keys or the computer keyboard. The FM-1 can stay unplugged. Ctrl-C stops the server.

```sh
python3 -m http.server 8765 --bind 127.0.0.1 --directory simulator
```

The hosted emulator also accepts the same file at https://joakimeriksson.github.io/fm1-emu/. The bundled simulator contains the playback page, JS and WASM runtime. External catalogue links and related site pages are upstream resources.

## Mac handoff

In Terminal, type `sh ` including the space, drag `scripts/mac-inventory.sh` from Finder to Terminal, then press Return. Send its output to the project chat. This checks the shell architecture, Apple silicon/Rosetta flags, OS, command-line tools and tool versions. No credentials or device identifiers are collected. It does not install software.

The build and emulator already work in Work on Linux x86-64. Nothing on the Mac is required to continue implementation here. An Apple silicon Mac running a native shell with Python 3.11+ can use the pinned native compiler. If the Mac is Intel, retain Linux for cross-compilation and use the Mac for browser playback, editing and device acceptance. The checked upstream installer has no Intel Mac compiler archive. No need to install Docker, Emscripten or Homebrew to preview the compiled instrument.

## Build and verify

```sh
sh scripts/build.sh baseline
sh scripts/build.sh four-part
node scripts/four-channels.mjs simulator out/four-part.fwsc
```

The wrapper checks out the lock revision and recursive submodules, uses upstream setup, and writes distinct outputs under `out/`. Run builds serially because upstream shares a build directory. Setup uses the network, verifies the compiler and three SDK inputs, and may also install optional flashing dependencies into its own build environment. The wrapper never invokes a hardware command. The first checkout is under `work/`. Keep product work in this repository and record upstream changes as versioned patches or in a dedicated fork. The upstream driver’s generated files contain absolute paths to the workspace that produced the evidence.

Browser regression tests use Python Playwright and a compatible Chromium. Set `CHROMIUM` to the executable if using system Chromium. Run `python3 scripts/emulator-smoke.py simulator builds/four-part-formant.fwsc`. Run `python3 scripts/prototype-smoke.py /path/to/extracted/handover` to test both original browser versions. The portrait estimate uses Pillow and accepts the extracted `abyss1` subfolder. The simulator WASM is the downloaded upstream Pages binary. Its hash is in evidence/checksums.txt. Building the emulator itself from Rust is independent of building firmware.

## Operating model

Work/Codex owns source changes, cross-compilation, host and emulated regression tests, memory budgets and revisioned handoffs. On the Mac, local Codex can work in the same checkout, execute installed build tools under its permissions and inspect local logs. A local IDE and GitHub are optional for the current workflow. Local Codex availability and account access await inventory. A private remote, issue tracker and manual GitHub Actions run can follow your approval. The included workflow has not run on GitHub.

See docs/discovery.md for the platform decision, constraints and recovery paths. See docs/backlog.md for controls, acceptance tests and the next implementation tasks. The ZIP contains a working tree and a portable Git bundle. To resume with its history, clone the included bundle with `git clone ABYSS-1-history.bundle abyss-dev` from the extracted folder.
