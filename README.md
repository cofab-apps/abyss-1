# ABYSS-1

ABYSS-1 is a four-track musical instrument in development for the M-VAVE FM-1. A child picks a sea creature, plays its voice, and layers a loop. Portraits and sound provide the cues for players who cannot yet read.

**Project creator and maintainer:** Gabor Varga · Zurich, Switzerland. Firmware base: Optimist on fm1-base. Browser emulator: Joakim Eriksson’s fm1-emu.

## Play Beta 1.5

**[Open ABYSS-1 in the FM-1 simulator](https://joakimeriksson.github.io/fm1-emu/?package=https%3A%2F%2Fraw.githubusercontent.com%2Fcofab-apps%2Fabyss-1%2Fmain%2Fbuilds%2Fabyss-1-beta-1.5.fwsc&name=ABYSS-1&embed=1&mhz=60)** · [Download the firmware](builds/abyss-1-beta-1.5.fwsc) · [Watch and hear a Reef Reply become a song](evidence/duet/reef-duet.mp4)

Tap **Start** and pause for a moment. Two fish and the bright frame take turns while the ARP button pulses, inviting you into the game without words. Tap **ARP** and play three keys. Lift your fingers. A second creature lights up and plays your phrase back in its own voice. Watch the keys you just played light up in the same rhythm under the answering creature. When the PLAY lamp pulses, press it to keep the exchange as a real loop: your call fills the first half and the other creature answers in the second.

![The new-player invitation moves from Bloop to Julia while the ARP button pulses](evidence/invitation/take-turns.png)

![Bloop answers with the gold frame. Two illuminated keys show the chord to play. Captured from the FM-1 simulator.](evidence/keylights/echoed-chord.png)

Press **PLAY** before playing a call to hear the band. On a fresh song Bloop, Kraken, Julia and Space Bloop play a bar together. The moving jaws and tentacles show who is sounding. Four pearls across the top count the beats.

To make your own music, start with **REC** and play a key. Select another creature to overdub it. On HOME, **EDIT** lifts the last take. Press **EDIT** again to bring it back. If you started with the quartet, you can lift all four factory parts in one press.

![Three native LCD frames of the orchestra at rest, Julia singing and the quartet in motion](evidence/reef/native-lcd.png)

Try the low and high keys on Kraken. Low strikes have a falling, tuned body. High keys produce a short shell rattle. All 16 creatures respond to notes from the keyboard, MIDI and the sequencer. The four family engines are FM bass, percussion, formant voice and detuned strings.

[Player guide](docs/MANUAL.md) · [Beta 1.5 verification](docs/BETA-1.5.md) · [Hear the quartet](evidence/reef/abyss-1-beta-1.3.mp4) · [Hardware](docs/HARDWARE.md) · [Original browser prototype](prototype/abyss1/)

## Build and contribute

- [Building and testing](BUILDING.md) covers the firmware build configurations, Mac setup and regression commands.
- [Architecture decision](docs/discovery.md) records the Optimist and fm1-base selection, flash and RAM budgets, display format and hardware recovery research.
- [Contributing](CONTRIBUTING.md) describes where changes belong and how to report a playback issue.
- [Credits and licensing](LICENSING.md) links the upstream notices and character provenance.

The firmware wrapper pins Optimist at `d1fbcf9`. Source revisions, local test results and build measurements are available in the [source lock](docs/sources.lock) and [evidence directory](evidence/).
