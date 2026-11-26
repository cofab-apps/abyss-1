# Playing ABYSS-1

## Play the firmware in the web emulator

[Launch ABYSS-1 in the simulator](https://abyss-one-fm1-simulator.aware-field-4307.chatgpt.site/?fw=abyss-1&embed=1&mhz=60). Tap **Start** for audio. The link selects a 60 MHz emulated clock. On the S22, the 100 MHz setting was observed running at 0.5× real time. Turn the phone sideways and tap the keys. The firmware opens on four portraits. The white frame shows which creature the keys will play.

| Control | What it does |
|---|---|
| SELECT encoder | Move between the four tracks. In step view, move between the 16 steps. |
| PRESETS / ALGORITHM | Pick a variant within the family / move to another family. Listen for its short preview while stopped. |
| HOME / SCL (SEL) | See the whole orchestra / one full-screen portrait. |
| K1–K4 | Change size, mood, voice and room depth for the selected creature. Each family has its own mappings. |
| REC | Arm a 16-step loop. The first note starts the clock. Turn SELECT and play to overdub a different creature. Tap REC again to finish recording while the loop continues. |
| PLAY | Stop or restart the loop. |
| SEQ | Show the step grid. Turn SELECT to choose a step and tap a note to enter it. Tap SEQ again to return. |
| GLO | Open the four-track mixer. SELECT picks a track, SCL mutes it, K1 sets its level and K2 changes tempo. HOME returns to the orchestra. |
| SAVE | Store the current song when stopped and quiet. A white indicator confirms the write. The same package restores it on restart. |
| MASTER / OCT− / OCT+ | Set output volume / keyboard octave. |

On the phone, drag an encoder up or down to turn it.

### Load a different package manually

1. Download [abyss-1-creatures.fwsc](../builds/abyss-1-creatures.fwsc). On the GitHub file page, choose **Download raw file**. Keep the `.fwsc` extension.
2. Open the [FM-1 web emulator](https://joakimeriksson.github.io/fm1-emu/). Under **Now running**, choose **Open .fwsc…** and select the downloaded file.
3. Press **Start** and play the onscreen keys. On a phone, landscape mode gives the keyboard more room. The four portraits appear on the device LCD.

The creature package is tested in the chip emulator. This revision still needs a physical FM-1 and Samsung S22 playtest. The emulator runs the package without a connected instrument.

### Run the bundled emulator locally

From the repository root, start a local server, open <http://127.0.0.1:8765>, and follow the same file-selection steps. Press Ctrl-C in the terminal to stop the server.

```sh
python3 -m http.server 8765 --bind 127.0.0.1 --directory simulator
```

## Play the creature prototype

[Open the creature instrument](https://abyss-one-fm1-simulator.aware-field-4307.chatgpt.site/creatures/index.html) and tap **Demo groove** to hear all four tracks.

From the repository root, run the following command and open <http://127.0.0.1:8000/abyss1/>. GitHub’s HTML file viewer displays the source.

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory prototype
```

Tap **Demo groove** to load a pattern. Use the transport to play or stop it. Select a track, use the arrows to choose a creature, then play the keys. The grid button switches between one portrait and the four-track orchestra. Try muting one track to hear its contribution. Save and load operate in the browser.

The prototype synthesizes its voices with Web Audio. Recording writes one note per step. Sustained note-off, real key pressure and the four authored firmware voices are still in development. [Prototype details](../prototype/abyss1/README.md)
