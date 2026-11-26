# FM-1 hardware used by ABYSS-1

The current four-part package has target build ID `C450E559`. The earlier four-part build included Bluetooth. The upstream baseline in `builds/` is an unmodified reference package. Use `four-part-formant.fwsc` to test ABYSS-1.

| FM-1 feature | Use in this build | Verification |
|---|---|---|
| 27 note keys | Trigger synth notes through the inherited key scanner. Velocity and continuous key pressure sensing are excluded. | Source and target feature report. Key feel requires a physical unit. |
| Seven rotary encoders, panel buttons and master volume | Optimist parameter editing, track navigation, transport and output level. Creature-specific control mappings are planned. | Inherited panel and audio code. Physical controls untested. |
| 240 × 240 RGB565 LCD | Renders the Optimist panel. Creature portraits and the four-tile orchestra currently run in the browser prototype. | Display path exercised in the emulator during Phase 0. |
| Audio codec, speaker and headphone output | Render the stereo mix, scaled by master volume. | The new package produced audio on all four MIDI channels in emulation. Physical outputs untested. |
| Wired MIDI | USB device MIDI and TRS MIDI use the inherited input path. The synth exposes four assignable parts. USB host mode is excluded. | Individual channel responses measured. Neither physical port has been tested. |
| USB audio | Device-side playback and capture, including four configurable capture channels. | Target report and base configuration. DAW routing and round-trip latency untested. |
| Processor and SRAM | Second-core support is compiled in. Static SRAM including pool and RAM code is 174,596 bytes against the 486,144-byte budget. | Linker measurement. Live load, peak stack and dropout testing remain open. |
| Internal flash | Stores firmware and supports upstream settings and project storage. Application size is 556,140 bytes of the 823,228-byte slot. | Linker measurement. Physical save and power-cycle testing remain open. |
| Optional SD hardware | The platform build advertises SD capability. The Optimist application profile has SDCARD=0. The MVP has no card dependency. | Platform manifest and generated application configuration. |
| Bluetooth and Wi-Fi | Bluetooth MIDI, BLE central, pairing, Bluetooth audio, Wi-Fi, access point and wireless Ableton Link are disabled. | Generated configuration and linked target feature report. Physical RF emissions have not been measured. |

The next sound-design milestone adds authored bass, percussion, vowel and atmosphere voices. The current firmware uses Optimist sounds. The musical acceptance test is to distinguish each creature by ear and hear all four in one loop.

[Build report](../evidence/wired-target.log) · [Target manifest](../evidence/wired-target.json) · [Four-channel audio test](../evidence/wired-channels.log) · [Package checksums](../evidence/checksums.txt)

The landscape touch-browser regression loads the documented package, taps Start and holds and releases an onscreen note. [Touch-test result](../evidence/wired-touch.log) · [Regression script](../scripts/test-simulator-launch.py)
