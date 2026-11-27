# Contributing to Seamonster Choir

The standalone FM-1 application and portrait atlas live in [monster-keys/](monster-keys/). The [build guide](BUILDING.md) gives the target and touch-panel checks. For sound or control changes, include a short phrase that demonstrates the change, the firmware checksum and the measured result. Use the native LCD and stereo DAC capture for changes that affect what a player sees or hears.

For a playback issue, include the firmware version, device or browser, exact gesture and what happened. The [choir design](docs/backlog.md) records the original acceptance criteria.

## Keep the player documentation with the release

When a change reaches players, update the [feature definition](docs/FEATURES.md) with its behavior, limits and observable acceptance criteria. Update the [player manual](docs/MANUAL.md) in the same release with the starting screen, exact gestures, expected sound or screen response, and what survives a power cycle. Keep the README links and the simulator’s chosen package in step. Development ideas belong in the backlog until a playable package ships.

If a screen, control or visual cue changes, take a fresh image from the packaged simulator. Place it beside the instructions it explains and give it descriptive alt text. The 2.4.0 guide pictures came from the Puya 60 MHz target. Four-turn frames came from the rows suite. The guest sequence shows the sampler immediately after capture, the assigned seat and the missing seat after reboot. The strum illustration uses the simulator panel renderer with the active LCD and LEDs. The shell sequence shows C, A and B partway through their phrases. Keep simulator and physical evidence labeled separately.

Run `python3 scripts/check-player-docs.py` from the checkout. The firmware workflow runs the same release-heading and local-link checks. Check changed gestures against the actual package as described in [BUILDING.md](BUILDING.md).
