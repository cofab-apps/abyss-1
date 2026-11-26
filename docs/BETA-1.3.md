# ABYSS-1 Beta 1.3 · The playing reef

[Play the instrument](https://joakimeriksson.github.io/fm1-emu/?package=https%3A%2F%2Fraw.githubusercontent.com%2Fcofab-apps%2Fabyss-1%2Fmain%2Fbuilds%2Fabyss-1-beta-1.3.fwsc&name=ABYSS-1&embed=1&mhz=60) · [Firmware](../builds/abyss-1-beta-1.3.fwsc) · [Watch and hear the session](../evidence/reef/abyss-1-beta-1.3.mp4)

On the first visit, press **Start → PLAY**. A four-creature band starts. Four pearls count the beats. Play over the band, or press **HOME → EDIT** to lift its phrase. A second EDIT puts it back. For a fresh recording, stop with PLAY, press REC and play the first note.

## What changed

- **Kraken is a playable percussion instrument across the keyboard.** A falling membrane and its octave partial give the lower keys a short drum tone. Higher pitches bring forward the resonant shell noise. Velocity brightens the shell, and the knobs still set decay, resonance, cutoff and room. Space Bloop answers on the offbeats with two detuned string registers, leaving the downbeat for bass and percussion.
- **The pictures perform.** Mouths open, tentacles flex, Hydra’s heads bob and Upsweep’s fins lift. Each motion follows a synth voice on its actual track, including incoming MIDI and sequenced notes. The frames return to the original portrait when the note finishes. A small deformation grid and palette-index cache keep the motion within the target’s memory budget.
- **A mistake costs one button press.** On HOME, EDIT toggles the last take or step edit. The operation includes every changed track in a multi-track take. A cancelled arm preserves the preceding edit. The factory quartet is one removable phrase. Selecting other creatures, changing the mix, and trying notes do not replace that history. History lasts for the current session. SAVE stores the composition.
- **The bundled simulator opens directly on ABYSS.** Serve the repository root and press Start on the simulator page. The public player and the bundled simulator both run the compiled firmware. Browser saves remain associated with the selected package.

## What the target produced

The tests booted the packaged instrument in the FM-1 chip emulator. The performance movie uses its native 240 × 240 LCD, enlarged for viewing, and its stereo DAC output. The clip contains 955,904 stereo frames at 44.1 kHz. Its largest absolute sample was 0.272 of full scale.

| Check | Result |
|---|---|
| Factory quartet at 60 MHz | 600.135 emulated seconds, 206,848 complete 128-frame blocks, zero missed refills and zero second-core faults. Left-channel peak 0.276 and RMS 0.035. |
| Sound identities | All 16 produced audio on every track. All 120 pairs separated on gain-normalized time/frequency profiles across two takes per creature. This measures signal differences. Blind recognition by listeners is still open. |
| Kraken by register | For the first 8,192 samples of a velocity-110 strike, low C (MIDI 36) RMS rose from 0.00716 to 0.01094. On high C (72), energy above 1 kHz rose from about 2% to 34% of the windowed spectrum. |
| Native response | All 16 moved on a key and restored their original pixels. Across the 16 sampled onsets, first visible movement took at most 105 ms at 60 MHz and 24 ms at 100 MHz. These are screen measurements. |
| Recording | Verified undo/redo of a whole quartet, step entry, a two-track take after cancelling a fresh arm, and an overdub with another track preserved. Two legacy songs created in the Beta 1 and Beta 1.2 packages played after restoring their saved flash sectors in the new target. |
| Browser | Touch Chromium booted the local package, played and stopped the quartet, recorded two parts, saved, reloaded the page and replayed the song. All four portrait assignments survived the reload. |
| Build | Three Linux builds produced identical package bytes, including a rebuild at a different path after removing the target build directory. Firmware ID C6236505. Application flash 608,832 of 823,228 bytes. Static SRAM 209,076 of 486,144 bytes. |

[Verification log](../evidence/reef/verification.txt) · [Reproduce the build and tests](../BUILDING.md#beta-13-release-checks) · [Archived Beta 1.2 package](../builds/abyss-1-beta-1.2.fwsc)

## Decisions from playtesting in emulation

The first full-motion build missed audio refills in the quartet. Changing the shared reverb algorithm did not resolve them. Muting individual tracks exposed the interactions between percussion, the strings and Julia. The released version keeps the original reverb, uses a dedicated percussion renderer and leaves more space between the pad attacks. The sustained four-part test above exercises that combination. Separately, a portrait border painted across the sound-control legend during a note. The released renderer clips the frame above the legend.

The next musical experiment is a call-and-response mode in which a second creature repeats a child’s short phrase. First test whether the player can hear who is answering and whether a new key reliably interrupts the reply. A physical FM-1, its onboard speaker, mobile wall-clock timing, and discovery by children remain unverified.
