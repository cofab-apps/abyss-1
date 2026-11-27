# Monster Keys 2.0.1

[Play the firmware](https://joakimeriksson.github.io/fm1-emu/?package=https%3A%2F%2Fraw.githubusercontent.com%2Fcofab-apps%2Fabyss-1%2Fmain%2Fbuilds%2Fabyss-1-monster-keys-2.0.1.fwsc&name=ABYSS-1%20Monster%20Keys&embed=1&mhz=60) · [Download the package](../builds/abyss-1-monster-keys-2.0.1.fwsc) · [Watch the native LCD with stereo DAC audio](../evidence/monsters/monster-calls-2.0.1.mp4)

## Playing with two hands or one

Start with Hydra. Hold a white key and turn K1 slowly from closed to open. Two resonances move through the voice while the sung pitch stays on the key. Add another finger. There are four voices. The white keys play a pentatonic scale. Rock a finger onto the raised key beside a white note to bend and brighten that same voice; it no longer retriggers or steals a voice from a four-note chord.

| Control | What the child sees and hears |
|---|---|
| SELECT or PRESETS | Browse all 20 creatures in either direction. Each introduces itself with a short call. SEL advances by one. |
| K1 · Mouth | The lip symbol opens. The mix of resonances moves between four vowel shapes. |
| K2 · Teeth | The teeth lengthen. The voice gains a rougher edge or metallic modulation. |
| K3 · Wobble | The wave grows. The pitch and articulation move more deeply or quickly. |
| K4 · Cave | The echo symbol widens. The delay repeats at different times in the left and right ears. |
| PLAY | Hear a short call, or play the current take. A fresh key press interrupts replay. |
| REC | Arm a take. The first played note or audition starts the clock. REC finishes it. PLAY can finish and immediately replay it. |
| HOME, OCT−, OCT+ | Restore this creature’s four knobs, or move the keyboard down or up one octave. |

Turn a knob with the keys released to hear what the knob does. Holding a note gives continuous control. Keyboard lights match the suggested or sounding keys. Changing creature loads that creature’s own four settings.

The recorder keeps one take in RAM, for up to 30 seconds or 512 gesture changes. A take includes key releases, chords, creature selections and knob turns. Arming and cancelling an empty take preserves the previous performance. Power cycling clears the take. The versioned Beta 1.5 player is still the place to open songs saved with the four-track instrument.

## Sound and artwork

Monster Keys is a standalone fm1-base application. Its audio interrupt generates all sound. Each monster combines a pitched oscillator, a per-species bank of four spectral shapes and a call model. A hard-synchronised pair of damped resonances supplies the vocal tract for the choir, growl and whale voices. The other models use whistle bends, pulsed metallic clacks or ring-modulated transmissions. A soft output curve leaves room for four voices and stereo echoes.

The new atlas holds twenty original generated interpretations of the supplied monster descriptions. Each portrait occupies 168 × 168 pixels on the 240 × 240 display and has its own 64-colour palette. Fixed side symbols leave the portrait visible while either hand plays. [Atlas](../monster-keys/bestiary.png) · [Credits](../LICENSING.md)

## Reproduce the release

The tested build environment used Linux x86-64, Python 3.12 and Pillow 13.0.0. Target checks ran on Node 24. The wrapper uses Git and curl to obtain the pinned fm1-base, BSP, drop-in compiler and SDK packaging files. Set `JIELI_TOOLCHAIN` and `AC79_SDK` to reuse the project compiler and SDK.

```sh
sh scripts/build-monsters.sh
cmp out/monster-keys.fwsc builds/abyss-1-monster-keys-2.0.1.fwsc
node scripts/test-performance.mjs simulator out/monster-keys.fwsc out/monsters --monsters
node scripts/test-performance.mjs simulator out/monster-keys.fwsc out/legacy --monster-legacy builds/abyss-1-beta-1.5.fwsc
ABYSS_MONSTER_BROWSER=1 python3 scripts/test-simulator-launch.py simulator out/monster-keys.fwsc
```

The last command requires Python Playwright and Chromium. Set `CHROMIUM` when using a system executable. The capture writes native RGB565 frames and interleaved signed 16-bit stereo samples taken from the simulated DAC at 44.1 kHz.

| Input or result | Verified value |
|---|---|
| fm1-base / BSP | `bc68fcd635a918e16019767acad67cc1d773dd6a` / `fdd709fd0066b878b03c144776404d63811db13e` |
| Compiler / SDK | pi32v2 LLVM `4f631b31b-e575321` / AC79NN SDK V1.2.1 packaging inputs |
| Package | 858,216 bytes, identity `FM-1_709020001` |
| SHA-256, also matched in a fresh build directory | `db9ceddaab95290201db3cde63d951a52d9f349bba763784d69f2b4483db6371` |

## What the target tests found

- At 60 MHz all 20 portraits changed on navigation and on a held note. All 20 creatures produced sound on the same key and accepted a four-note chord. The emulator reported zero missed DAC refills for those tests. The largest measured chord peak was 0.606 of digital full scale after smoothing the louder radio voices.
- A four-note chord remained audible after all four fingers rocked onto and off their paired raised keys. Plain, bent and released-bend chord RMS measured 0.127, 0.124 and 0.125; the same result passed at 60 and 100 MHz.
- The Bloop put 89% of its measured sustained-note energy below 250 Hz. The Crab put 23% above 8 kHz. All four knobs altered the DAC output. The [measurement file](../evidence/monsters/measurements-2.0.1.json) includes the windowed frequency-band energy fractions, chord peaks and finger-bend levels.
- A recorded two-note take retained the intervening rest after an empty REC cycle. Replay RMS measured 0.060 on the first note, 0.000047 in the gap and 0.063 on the second note. A separate test saved a Beta 1.5 user song, played and recorded in Monster Keys with that flash state, returned to Beta 1.5 and verified the original step, empty tracks and audible playback.
- The preserved 2.0 package passed the touch-Chromium panel test: it matched Hydra’s native LCD pixels, produced worklet audio from a panel key and restored Hydra after a SELECT round trip. The 2.0.1 target package booted and passed the native LCD/DAC tests, but this runner’s Chromium wrapper was unavailable for a second touch-browser pass.

The frequency measurements describe the emulator’s electrical output. Listening on the physical speaker, pressure response, and whether a child discovers the side symbols unaided remain untested. The firmware currently uses on/off contacts; the raised-key bend is discrete rather than pressure-sensitive. A next physical experiment is to measure whether that rocking gesture feels natural on the FM-1 keybed.

[Download the preserved 2.0 package](../builds/abyss-1-monster-keys-2.0.fwsc) · [Open the legacy Beta 1.5 sequencer](BETA-1.5.md)
