# FM-1 hardware used by ABYSS-1

The current package is [abyss-1-creatures.fwsc](../builds/abyss-1-creatures.fwsc), build ID `E460D200`. The baseline and four-part packages are retained as engineering references.

| FM-1 feature | Use in this build | Verification |
|---|---|---|
| 27 note keys | Trigger synth notes through the inherited key scanner. Velocity and continuous key pressure sensing are excluded. | Source and target feature report. Key feel requires a physical unit. |
| Seven rotary encoders, panel buttons and master volume | Select a track, family or creature, change four voice macros, enter steps, record and mix. | Inherited panel and audio code. Physical controls untested. |
| 240 × 240 RGB565 LCD | Draws 16 native 240 × 240 portraits and subsamples them to 120 × 120 for the four-tile view. | Emulated LCD pixels compared with the quantized source portraits. |
| Audio codec, speaker and headphone output | Render the stereo mix, scaled by master volume. | The new package produced audio on all four MIDI channels in emulation. Physical outputs untested. |
| Wired MIDI | USB device MIDI and TRS MIDI use the inherited input path. The synth exposes four assignable parts. USB host mode is excluded. | Individual channel responses measured. Neither physical port has been tested. |
| USB audio | Device-side playback and capture, including four configurable capture channels. | Target report and base configuration. DAW routing and round-trip latency untested. |
| Processor and SRAM | Second-core support is compiled in. Static SRAM including pool and RAM code is 170,596 bytes against the 486,144-byte budget. | Linker measurement. Live load, peak stack and dropout testing remain open. |
| Internal flash | Stores firmware and supports upstream settings and project storage. Application size is 589,228 bytes of the 823,228-byte slot. | Linker measurement. Physical save and power-cycle testing remain open. |
| Optional SD hardware | The platform build advertises SD capability. The Optimist application profile has SDCARD=0. The MVP has no card dependency. | Platform manifest and generated application configuration. |
| Bluetooth and Wi-Fi | Bluetooth MIDI, BLE central, pairing, Bluetooth audio, Wi-Fi, access point and wireless Ableton Link are disabled. | Generated configuration and linked target feature report. Physical RF emissions have not been measured. |

Initial family patches run on the device DSP. Physical key velocity, pressure, loudspeaker balance and headroom still require work. The emulator tests capture DAC output and individual track mutes.

[Control guide](MANUAL.md) · [Native build report](../evidence/creature-target.log) · [Native acceptance tests](../evidence/creature-tests.log) · [Package checksums](../evidence/checksums.txt)
