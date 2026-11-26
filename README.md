# ABYSS-1

ABYSS-1 is a four-track musical instrument in development for the M-VAVE FM-1. A child picks a sea creature, plays its voice, and layers a loop. Portraits and sound provide the cues for players who cannot yet read.

![Four-creature orchestra design at the FM-1 screen resolution](prototype/abyss1/orchestra-240.png)

**Project creator and maintainer:** Gabor Varga · Zurich, Switzerland. Firmware base: Optimist on fm1-base. Browser emulator: Joakim Eriksson’s fm1-emu.

## Try the current build

**[Play the current ABYSS-1 firmware in the simulator](https://abyss-one-fm1-simulator.aware-field-4307.chatgpt.site/?fw=abyss-1&embed=1&mhz=60)** — tap **Start**, then play the keys. This simulator opens the four-part build directly. On a phone, turn the screen sideways.

| What to try | What works today |
|---|---|
| [Play the creature prototype](https://abyss-one-fm1-simulator.aware-field-4307.chatgpt.site/creatures/index.html) | 16 portraits, four assignable tracks, a 16-step sequencer, live step recording, mute and browser save/load. |
| [Compiled FM-1 firmware](docs/MANUAL.md#play-the-firmware-in-the-web-emulator) | The four-part Optimist configuration boots in desktop Chromium and produces audio on MIDI channels 1–4. It uses the upstream panel and sounds. |

The current firmware uses wired MIDI and audio. Bluetooth and Wi-Fi are disabled. [FM-1 hardware used by ABYSS-1](docs/HARDWARE.md)

The next firmware milestone brings four creatures, their portraits and four authored voices into the same build. Physical FM-1 playtesting is pending. [Milestones and acceptance criteria](docs/backlog.md)

## Build and contribute

- [Building and testing](BUILDING.md) covers the two build configurations, Mac setup and regression commands.
- [Architecture decision](docs/discovery.md) records the Optimist and fm1-base selection, flash and RAM budgets, display format and hardware recovery research.
- [Contributing](CONTRIBUTING.md) describes where changes belong and how to report a playback issue.
- [Credits and licensing](LICENSING.md) links the upstream notices and character provenance.

The firmware wrapper pins Optimist at `d1fbcf9`. Source revisions, local test results and build measurements are available in the [source lock](docs/sources.lock) and [evidence directory](evidence/).
