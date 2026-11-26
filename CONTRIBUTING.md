# Contributing to ABYSS-1

| Change | Where it belongs |
|---|---|
| Creature prototype and artwork | [Prototype](prototype/), with character IDs and provenance in [characters.json](prototype/abyss1/characters.json) |
| Firmware configuration and builds | [four-part.config](four-part.config) and [build wrapper](scripts/build.sh). The wrapper requires a clean pinned upstream checkout. Keep firmware source changes in versioned patches or a dedicated fork. |
| Emulator and regression tests | [Bundled emulator](simulator/) and [test scripts](scripts/) |
| Hardware design and acceptance | [Architecture decision](docs/discovery.md) and [backlog](docs/backlog.md) |

For a playback issue, record the package filename or commit, browser and device, input method, steps, expected sound or screen, and observed result. Identify whether you used the creature prototype, compiled firmware emulator or physical instrument.

For a pull request, name the affected target and include the relevant test result. For sound changes, describe what to play and what should be audible. For portrait changes, show the single-creature and four-track views at native resolution. [Build and test commands](BUILDING.md)
