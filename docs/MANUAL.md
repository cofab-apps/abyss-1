# Playing ABYSS-1

## Play the firmware in the web emulator

[Launch ABYSS-1 in the simulator](https://joakimeriksson.github.io/fm1-emu/?package=https%3A%2F%2Fraw.githubusercontent.com%2Fcofab-apps%2Fabyss-1%2Fmain%2Fbuilds%2Fabyss-1-beta-1.3.fwsc&name=ABYSS-1&embed=1&mhz=60). Tap **Start** for audio. The link selects a 60 MHz emulated clock. On the S22, the 100 MHz setting was observed running at 0.5× real time. Turn the phone sideways and tap the keys. The firmware opens on four portraits. At rest the white frame shows which creature the keys will play. A sounding creature moves on its own tile. Four pearls mark the beats while the loop plays.

| Control | What it does |
|---|---|
| SELECT encoder | Move between the four tracks. In step view, move between the 16 steps. |
| PRESETS / ALGORITHM | Pick a variant within the family / move to another family. Listen for its short preview while stopped. |
| HOME / SCL (SEL) | See the whole orchestra / one full-screen portrait. |
| K1–K4 | Change the four sound parameters shown in the selected family’s portrait. On the FX page they control drive, chorus, echo and room. |
| REC | Arm a 16-step loop. The first note starts the clock. Turn SELECT and play to overdub a different creature. Tap REC again to finish recording while the loop continues. |
| PLAY | From HOME or SEQ, start the four-creature groove on a fresh song. With your own notes, stop or restart your song. |
| EDIT on HOME | Lift the last completed take, step edit or factory groove. Press again to bring it back. A new musical edit replaces that undo. Undo is available during playback and when stopped. |
| SEQ | Show the step grid. Turn SELECT to choose a step and tap a note to enter it. EDIT clears the selected step. Tap SEQ again to return. |
| GLO | Open the four-track mixer. SELECT picks a track, SCL mutes it, K1 sets its level and K2 changes tempo. HOME returns to the orchestra. |
| SAVE | Store the current song when stopped and quiet. A white indicator confirms the write. The same package restores it on restart. |
| LFO | Open the selected track’s speed, pitch swing, waveform and amplitude controls. Press LFO again on this page to bypass. |
| FX / ENV | Open the four effect controls / toggle physical key sensing with all keys released. Press SCL again in the portrait to switch minor pentatonic on or off. |
| MASTER / OCT− / OCT+ | Set output volume / keyboard octave. |

On the phone, drag an encoder up or down to turn it. To start a blank take after trying the factory groove, press PLAY to stop, HOME, EDIT, then REC. The first key starts your new bar. Browser saves belong to the exact firmware file. Opening an older package retrieves that package’s saved song.

### Load a different package manually

1. Download [abyss-1-beta-1.3.fwsc](../builds/abyss-1-beta-1.3.fwsc). On the GitHub file page, choose **Download raw file**. Keep the `.fwsc` extension.
2. Open the [FM-1 web emulator](https://joakimeriksson.github.io/fm1-emu/). Under **Now running**, choose **Open .fwsc…** and select the downloaded file.
3. Press **Start** and play the onscreen keys. On a phone, landscape mode gives the keyboard more room. The four portraits appear on the device LCD.

The creature package is tested in the chip emulator. This revision still needs a physical FM-1 and Samsung S22 playtest. The emulator runs the package without a connected instrument.

### Run the bundled emulator locally

From the repository root, start a local server and open <http://127.0.0.1:8765/simulator/?fw=abyss-1&embed=1&mhz=60>. Press **Start**. The bundled player loads the local firmware directly. Press Ctrl-C in the terminal to stop the server.

```sh
python3 -m http.server 8765 --bind 127.0.0.1
```

## Play the creature prototype

The [original prototype](../prototype/abyss1/) is included in this repository.

From the repository root, run the following command and open <http://127.0.0.1:8000/abyss1/>. GitHub’s HTML file viewer displays the source.

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory prototype
```

Tap **Demo groove** to load a pattern. Use the transport to play or stop it. Select a track, use the arrows to choose a creature, then play the keys. The grid button switches between one portrait and the four-track orchestra. Try muting one track to hear its contribution. Save and load operate in the browser.

The prototype synthesizes its voices with Web Audio. Recording writes one note per step. The separate firmware beta supports sustained note-off, per-family patches and optional key sensing. [Beta 1 evidence](BETA-1.md) [Prototype details](../prototype/abyss1/README.md)
