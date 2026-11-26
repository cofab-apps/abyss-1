# ABYSS-1 — v0.1 browser proof of concept / Work handover

## Product intent
Turn passive sea-monster video consumption into active musical creation. Target pre-reading children, with parents as installers. Not a game: no scores, levels, battles or passive video. Use identifiable portraits and immediate musical feedback. Artwork and musical sound quality take priority over sophisticated animation.

## Run
Open index.html in a modern browser, or serve the folder via `python3 -m http.server 8000`. The separately supplied ABYSS-1-prototype.html embeds all images and needs no companion assets. Tap Demo groove, then select a track, select a creature, play keys, edit steps, or record while playing. The canvas is exactly 240×240 logical pixels, not a simulation of physical screen brightness/size. Use the grid icon to switch full portrait / four-track view. The interface surrounding the canvas is a browser test harness, not a literal FM-1 control panel.

## Current scope
16 selectable creatures, four default sound families, four freely assignable tracks, 16 steps per track, one note per step, live step overwrite, mute, tempo, four per-track synthesis parameters, local browser save/load. Web Audio synthesis is approximate and not a port of FM-1 DSP. No real pressure sensing, sustained note-off behavior, MIDI, 64-step patterns, hardware deployment or true overdub yet. Flash-on-note visual feedback rather than complex sprites.

## Assets
`characters.json` contains character IDs, family, file and provenance. 16 full portraits (240×240), 16 recognition thumbnails (120×120), 16 raw little-endian RGB565 files (115200 bytes each, row-major). Bloop is the supplied image, original ownership unknown; remaining images are procedural placeholder illustrations, not verified character likenesses. Do not publish or distribute outside this private evaluation without clearance. Replace placeholders with recognizable, licensed artwork before testing character identification. `build_assets.py` regenerates the assets (Pillow + NumPy). `orchestra-240.png` and `selection-240.png` are static reference screens. Real device color, byte order, memory and rendering performance remain to be validated.

## Proposed hardware mapping (not verified)
MASTER volume; SELECT creature; PRESETS track/family navigation; ALGORITHM sound variant or reserved; K1 size/resonance, K2 mood/distortion, K3 voice/timbre, K4 depth/filter/ambience; keyboard note/velocity/pressure; use existing transport controls for record/play/stop and a button for track selection or orchestra view. Audit actual control IDs and event capabilities before committing this map. Use physical icons and feedback to avoid reading requirements.

## Implementation sequence for Work
1. Inspect upstream source and licenses: https://github.com/m-vave/fm1_open, https://github.com/w0ts/optimist, https://github.com/iamweswebb/FM1-Firmware1, https://github.com/charlesvestal/fm1-monk. Verify the exact hardware revision, toolchain, supported host simulator and recovery procedure. Select one multitrack base, recording architecture decisions and license compatibility; do not combine GPL code with incompatible dependencies.
2. Build unmodified upstream reproducibly on macOS or supported host; run tests and simulator, measure audio CPU and flash/RAM budget, document rollback. Do not flash without approval.
3. Implement native 240×240 portrait and four-tile screens using the supplied RGB565 assets; check byte order and streaming/storage strategy. A 16-image uncompressed library alone requires 1,843,200 bytes, before thumbnails or firmware. Measure rather than assume feasibility. Keep visuals subordinate to real-time audio.
4. Wire hardware controls and four-track sequencer; first target 16 steps then extend toward 64 only after benchmarking. Implement note-on/off, velocity and pressure where supported; preserve four independent track patches and playback.
5. Design genuinely differentiated bass, percussion, vocal and atmosphere sounds, at least four per family. Use original synthesis rather than unlicensed recordings. Reference the creator's sound examples for design direction only: https://www.youtube.com/watch?v=Y9EzjXIsIlw. Confirm creator permissions and artwork sources for recognizable characters before public release.
6. Test with children: identify portraits without text; independently choose a creature, make sound, record a rhythm, add a second track and play all four. Record observations, not invented findings. Evaluate whether the instrument inspires voluntary musical play.
7. Create GitHub repository and CI, documentation, simulator release and install/recovery guide; no publishing or flashing without approval.

## Acceptance criteria for first hardware milestone
- Reproducible upstream build and simulator execution evidenced by logs.
- Display a full portrait and four 120×120 tiles on actual hardware without clipping or wrong colors.
- Four independently selectable tracks, each with at least four available patches in its default family, freely assignable.
- Play/record/stop and visual feedback work without reading; note events trigger sounds and visual feedback.
- Stable audio during simultaneous four-track playback; measure CPU, memory, latency and glitches.
- Explicit license/provenance inventory and recovery procedure.

## Validation in this package
Node syntax check passed. All 16 sets of PNG dimensions and raw file lengths passed. Browser interaction testing attempted with Playwright; default Chromium was missing, system Chromium launched but administrator policy blocked both file and localhost navigation. Audio quality and runtime interactions not verified. No firmware has been built or flashed. 
