# Discovery and architecture decision · 26 November 2026

## ADR 001 · Optimist on fm1-base

Use the pinned release architecture at w0ts/optimist d1fbcf9. Its target app connects app_render, app_frame and input callbacks to fm1-base. Its DSP, voice allocator, pattern sequencer and effects compile from the same C sources used in the project’s host path. Inspect fm1base/optimist/core/opt_core.c, app.h, fm1.toml and firmware/src/core/core.h. Start the character layer beside those callbacks. Keep the UI on the main loop and audio processing in fixed-size render blocks. Use the existing transport, mixer and project store. Audit the existing storage map before adding a versioned character ID and expression events.

The release pulls fm1base and its nested BSP. The warning about a private submodule in BUILDING.md was stale for this checkout. An HTTPS rewrite scoped to the Git command resolved both exact commits. Classic with the default mots-config failed its own configuration validation on Wi-Fi and USB host. The unchanged release configuration compiled successfully. Full revision IDs and recursive gitlinks are in sources.lock and evidence/submodules.txt.

| Candidate examined | Relevant implementation and terms | Decision |
|---|---|---|
| Official fm1_open | app/project.c, midi_router.c, screen.c and instrument.h. Eight tracks, MIDI and sample/DX7 support. Documented package build uses Windows x64, JieLi 2.5.3 and a binary engine SDK. Application GPLv3 with SDK linking exception; separate SDK terms. | Reference for expected panel, MIDI and restore behaviour. |
| Optimist and Workbench | Release app on fm1-base; engines, per-track model, host render and configurable package. Workbench includes builder, simulator, installer and editor. GPL-3.0-only app; fm1-base/BSP have their own notices. | Cross-build locally. Use the chip emulator for tests of the exact .fwsc. Keep the Workbench for upstream comparison. |
| Supplied Felucca fork and canonical Hügelton project | Supplied fork separates engines and BSP into submodules. Engine interface, host/web targets and sequencing are useful references. App GPL-3.0-only, engine submodule MIT, BSP MPL-2.0 with component notices. Canonical README lists 1.8.3 and says development moved to Fune. | Upstream lineage and fallback. Avoid porting a second sequencer. |
| MONK-1 | firmware/src/dsp/monk.c uses shared 44.1 kHz formant/grain tables and a substantial delay buffer. app/sense.c maps measured key contact to expression. Port combines MIT MonkSynth with GPL-3.0-only firmware and separately identified MPL components. | Start with Optimist’s available four-resonator vocal engine. Benchmark MONK’s voice if it sounds better in a matched audition. Port sensing separately. Exclude singer artwork. |
| fm1-emu | Rust chip emulator, WebAssembly runtime, JS AudioWorklet and local package loader. Repository MIT with component-specific terms. | Integration test of packaged target code. Emulated timing and output are separate from hardware measurements. |
| Mocky | web/mocky.js converts native-size images to high-byte-first RGB565, with optional ordered dithering; RLE and display firmware are available. GPL-3.0-only. Hosted app retrieval failed in discovery; source was cloned and inspected. | Asset conversion reference. Device gallery testing requires an approved later hardware session. |
| Community firmware directory | Official stock/update reference and firmware index. Its descriptions sometimes simplify vendor licensing and compatibility. | Use to locate releases. Verify package, licence and device compatibility against the authors’ own materials. |

The first native creature build confirms that a custom panel and 16 character patches can run on this base. Optimist supplies sustained notes, independent recording and track storage. MONK-1 is the expression reference because its singers respond to velocity and held pressure on supported keys. Fune has a 16-step sequencer and an instrument plugin SDK; its authors currently mark the FM-1 port beta, and Felucca-family songs and patches do not carry over. [MONK-1](https://github.com/charlesvestal/fm1-monk) · [Fune](https://github.com/hugelton/Fune)

The original measurements below describe the Phase 0 builds. See [current wired firmware](HARDWARE.md) for the replacement package and its measurements.

## Resource and integration findings

The board support describes 1 MiB of flash, two processor cores, a 240 × 240 ST7789-family panel, seven rotary encoders, a master pot, 14 buttons and 27 note keys. The chosen release’s actual linker report takes precedence over older classic-builder budget tables. Physical CPU frequency depends on the boot environment. Pin the measured device clock, audio deadline misses and display redraw time in the hardware test record. The simulator’s 100 MHz setting is a test parameter.

| Measured target result | Unmodified baseline | Four-part configuration |
|---|---:|---:|
| Bytes in application slot | 817,368 | 638,292 |
| Slot capacity | 823,228 | 823,228 |
| Flash available within slot | 5,860 | 184,936 |
| Static SRAM including RAM code and pool | 231,764 | 201,440 |
| Builder SRAM budget after reserve | 486,144 | 486,144 |

The two packages are the same padded length. Compare application bytes and configuration hashes. Dynamic voices, delay lines and stack peaks still need profiling. Physical display throughput, ghosting and MIDI roundtrip latency remain unmeasured.

Four equivalent tracks come from DRUMS=0, which turns the fourth part into a normal assignable synth. A percussion character must therefore use a synth voice with a transient/noise or resonator patch. A dedicated drum lane would break free assignment. The configuration enables ENG_FORMANT and disables Wi-Fi Link, Wi-Fi AP and USB host. Device-side USB MIDI is distinct from USB host. Node/WASM probes observed nonzero output on input channels 1–4 individually. Recording four simultaneous voices with pressure, or USB MIDI in Ableton, still requires separate testing.

The graphics driver configures RGB565 and transmits the high byte first. The handover generator writes little-endian uint16 files. Convert once in the asset pipeline or deliberately swap at the blit boundary. A direct copy into a transfer buffer would swap red/blue bit fields and corrupt other colours. Render through horizontal strips and update the reacting mouth, outline or selection region. One full RGB565 frame takes 115,200 B. Sixteen take 1,843,200 B. The measured 16-colour per-portrait trial took 87,479 B including palettes and individual compressed strip lengths; the decoder, indexes, final illustrations and overhead still need budgeting. Test palette loss against the original at 240 and 120 pixels before committing. Reuse full-size art for the orchestra tiles if downsampling meets the redraw budget.

At native resolution the supplied Bloop face has texture, depth and a dominant dark mouth. The orchestra plate repeats a near-identical round head and tentacles across two tiles. Colour and placement currently do more recognition work than silhouette. Give each slice portrait a distinct outline and face direction and test it without captions or colour cues.

For expression, fm1base/include/plat.h exposes opt-in analog sampling (APP_KEY_SENSE). MONK-1 documents pressure on the two ADC rows and velocity-only probing on the seven highest keys. Allow the voice knob to reach the same expressive range on every key. Physical sensing, LED interaction and calibration on both of the user’s units are unverified.

## Recovery and distribution

Before Phase 3, identify the development unit, its running firmware, flash ID and bootloader. Export its songs, voices and samples, record the recovery package version and checksum, keep a full backup where the verified device path supports it, and leave the reference unit available for comparison. For a working stock/open application, the official Workbench documents compatible returns to original V15. For a running Optimist image, use the pinned docs/INSTALL-PATHS.md decision table: USB-MIDI while the application responds; documented download-mode entry depends on the installed application or bootloader. Optimist documents OCT− and OCT+ at power-on for three seconds; m-bootloader documents REC. A nonresponsive stock device has no verified universal key sequence in this project. A USB_KEY recovery dongle is a distinct hardware path. Confirm a viable path for the identified unit before an approved write. No recovery was exercised on physical hardware.

The package is a private engineering snapshot. Keep upstream licence texts and component notices, arrange complete matching sources for a future public GPL firmware distribution, and audit enabled assets and SDK components separately. Character permissions are independent. The original archive marks Bloop art “rights unverified” and 15 other entries as procedural placeholders. NOAA identifies the real Bloop as an icequake. That establishes the sound’s origin; the fictional creature portrait needs its own provenance. Borisao Blois’s own El Gran Majá sound video identifies a creator to approach. No permission was found or requested. Source tracking for Sea Eater, Space Bloop, Julia depictions and the other video designs remains open. Record original reference, creator, visual cues, recording ownership and permitted uses per character before public release. Design original sounds and illustrations for early playtests.

Primary links: https://oceanservice.noaa.gov/facts/bloop.html · https://www.youtube.com/watch?v=Y9EzjXisIlw · https://developers.openai.com/codex/cli/ · https://w0ts.github.io/optimist/ · https://open.m-vave.com/fm1-workbench · https://joakimeriksson.github.io/fm1-emu/ . Repository sources and revisions are recorded in sources.lock.
