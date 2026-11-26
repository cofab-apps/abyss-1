# Playing ABYSS-1

## Play the firmware in the web emulator

1. Download [four-part-formant.fwsc](../builds/four-part-formant.fwsc). On the GitHub file page, choose **Download raw file**. Keep the `.fwsc` extension.
2. Open the [FM-1 web emulator](https://joakimeriksson.github.io/fm1-emu/). Under **Now running**, choose **Open .fwsc…** and select the downloaded file.
3. Press **Start** and play the onscreen keys. On a phone, landscape mode gives the keyboard more room. Expect the Optimist panel. The creature screen is available in the separate prototype below.

The four-part package has passed desktop emulator tests. Samsung S22 playback and physical FM-1 operation still need testing. The emulator runs the package without a connected instrument.

### Run the bundled emulator locally

From the repository root, start a local server, open <http://127.0.0.1:8765>, and follow the same file-selection steps. Press Ctrl-C in the terminal to stop the server.

```sh
python3 -m http.server 8765 --bind 127.0.0.1 --directory simulator
```

## Play the creature prototype

From the repository root, run the following command and open <http://127.0.0.1:8000/abyss1/>. GitHub’s HTML file viewer displays the source.

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory prototype
```

Tap **Demo groove** to load a pattern. Use the transport to play or stop it. Select a track, use the arrows to choose a creature, then play the keys. The grid button switches between one portrait and the four-track orchestra. Try muting one track to hear its contribution. Save and load operate in the browser.

The prototype synthesizes its voices with Web Audio. Recording writes one note per step. Sustained note-off, real key pressure and the four authored firmware voices are still in development. [Prototype details](../prototype/abyss1/README.md)
