# ABYSS-1 Beta 1.2 · Play the demo

Open the FM-1 simulator. Choose **Open .fwsc…**, select **abyss-1-beta-1.2.fwsc**, set the clock to **60 MHz** and press **Start**. If the browser already has a song for this firmware, use a fresh browser profile to hear the factory demo and keep the saved song in its original profile.

## Start playing

On an empty song, press **SEQ → PLAY**. The display changes from **PLAY: DEMO** to **PLAY: LOOP**. Bloop plays syncopated bass, Kraken answers with accented percussion, Julia carries the vocal line and Space Bloop holds the harmony. The 16-step loop uses your current tempo and creature assignments. The preset adds notes when you start it. Pressing SEQ alone leaves the song empty.

Press **HOME** to watch all four creatures. Turn **SELECT** two clicks from the starting track to pick Julia in the lower-left tile. Press **LFO** while the groove runs. Turn **K3** two clicks from **SAW** to **S&H** for stepped random pitch, then turn **K2** to set the depth. Select **FX** and turn **K3** to hear echoes.

| View | K1 | K2 | K3 | K4 |
|---|---|---|---|---|
| LFO | Speed | Pitch swing | Wave shape | Amplitude chop |
| FX | Drive | Chorus | Echo | Room |
| SCL / SEL | The portrait's four labels show the selected creature’s own sound controls. | | | |

**LFO** and **FX** engage on the selected track. Press the same button again while its page is open to bypass. **HOME** returns to the orchestra and keeps the sound settings. **SCL / SEL** opens the portrait. Press it again there to switch the keyboard between free notes and minor pentatonic. In **GLO**, SCL / SEL mutes the selected track. Turning a sound knob on HOME shows its name and value.

**Editing and saving.** In SEQ, SELECT moves the cursor, a key enters a note and EDIT erases the selected step. Use HOME and SELECT to choose another part. PLAY stops the loop. Press SAVE after stopping it. When you have recorded or entered your own notes, SEQ → PLAY plays your song. Starting a take or editing a step also leaves you in control of the current session.

## Verify this build

The performance test boots the compiled firmware at 60 MHz. It checks four individually audible parts, tone changes from all four knobs on both effect pages, replay after saving, and protection for an existing song with a note on track three. The browser test checks the exact package hash, onscreen transport and audio output. The movie captures the emulator display and stereo output together.

From the source tree with Node and Python installed, run the following. The browser tests also need Pillow, Playwright and Chromium.

```sh
node scripts/test-performance.mjs simulator abyss-1-beta-1.2.fwsc out/performance
ABYSS_PLAY_CONTROLS=1 node scripts/test-creatures.mjs simulator abyss-1-beta-1.2.fwsc
ABYSS_DEMO_BROWSER=1 python3 scripts/test-simulator-launch.py simulator abyss-1-beta-1.2.fwsc --file
python3 scripts/test-simulator-launch.py simulator abyss-1-beta-1.2.fwsc --file
```

Select an installed Chromium executable with the CHROMIUM environment variable. Instructions for building the firmware from its pinned upstream sources are in BUILDING.md. Emulator evidence covers this firmware file. Physical FM-1 verification and Gabor’s listening review are open.
