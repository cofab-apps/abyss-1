# Monster Choir 2.2.0

[Play or download](../README.md) · [Player guide](MANUAL.md) · [Native LCD and stereo audio demo](../evidence/choir/rows-and-sea-2.2.0.mp4)

## Four turns and underwater effects

The original 2.1.0 package reproduced both reports. A browsed portrait vanished when SELECT moved on without a confirmation press. In a real four-lane simulator save, other recorded voices sounded during the call and reply rests. The two-bar game now assigns the upper row to the call and the lower row to the answer. A left/right pair splits a bar into two beats each. A lone singer gets a full bar. SEL invites a distinct singer and browsing commits immediately.

### Target verification

| Exercise | Observed with the 2.2.0 package |
|---|---|
| Four occupied seats | The native LCD marker visited top left, top right, bottom left and bottom right. Saved notes stayed in their own eight-step windows. Each reply reproduced its caller’s pitches, lengths and rests. At 60 MHz all four rest windows measured below 0.0007 RMS. Three-seat layouts gave the lone singer a full bar. |
| Add, recall and undo | Three SEL taps filled a fresh choir with distinct portraits. A browsed third portrait remained when moving to the fourth. A four-part favourite created by 2.1.0 loaded into 2.2.0. EDIT restored its original four parts after a new correspondence. |
| Depth and Anger | Full Depth shifted a steady Loch Ness whistle from 329 Hz to 166 Hz. Full Anger increased harmonics 4–10 from 0.3% to 5.4% of the energy in the first ten harmonics on that test note. |
| Current and Cavern | Current’s stereo phase swept on a held note. Cavern was audible after release and decayed to digital silence. A four-voice chord with all four sea effects at maximum had zero missed DAC refills at both test clocks. |
| Earlier play modes | All 20 solo portraits and voices, finger bends, solo replay, 32-step overdubbing and favourite recall passed their target regressions. |

The primary run used the Puya model at 60 MHz, the second the UC25HQ80 at 100 MHz. Browser touch playback and a clean cross-build were checked against the release bytes. Firmware ID `FM-1_709020200`, package size 858216 bytes, SHA-256 `429781e17d4ade8dc28a6ccfc5cd0577d242e17091db401261f8ad894a3f608f`. [Measurements](../evidence/choir/results-2.2.0.json) · [Build and test commands](../BUILDING.md)

Each singer has one note at a time. The game runs at 100 BPM. Sea settings apply to the full choir and are saved as one set per favourite. Physical results below apply to 2.1.0. A 2.2.0 hardware playtest, especially the row cues and stereo effects over headphones, remains open. [Previous 2.1.0 firmware](../builds/abyss-1-monster-choir-2.1.0.fwsc)

## Monster Choir 2.1.0 verification

[Play or download](../README.md) · [Player guide](MANUAL.md) · [Watch the LCD and hear the stereo DAC](../evidence/choir/duet.mp4)

This release adds a two-bar call-and-answer game, one persistent favourite and pressure-controlled vocal expression to standalone Monster Keys. HOME opens the four-seat instrument. ARP goes straight into a duet from the solo screen. The approved [2.0.1 package](../builds/abyss-1-monster-keys-2.0.1.fwsc) remains available.

## What the target firmware did

The packaged FM-1 image ran in the chip emulator at 60 and 100 MHz. These measurements use its DAC output. RMS values are fractions of full scale on the left channel. The film uses both channels.

| Exercise | Result |
|---|---|
| Trade calls | A held Hydra note followed by silence produced RMS 0.0622, then zero. With all keys released, Bloop answered in bar two at 0.0756. Both occupied the same steps in their respective bars. |
| Recall a favourite | SAVE wrote the favourite to the first storage sector at 0xD8000. After a new boot with the retained flash, HOME and PLAY reproduced the two voices and rests. The call and answer measured 0.0635 and 0.0736. |
| Four independent singers | Hydra, Bloop, SCP-682 and Jacob Leviathan sounded together from one key with zero missed DAC refills during the measured chord. Two additional lines were recorded on the same transport. The saved payload contained four independent lines with rests. |
| Record expression and undo | The fourth part retained the raised-key bend and multiple Mouth values. Undo restored that lane to silence and preserved the first three parts byte-for-byte for the tested note, bend and mouth fields. |
| Whisper to growl | With ENV enabled, the same simulated held key landed at velocity 32 in both runs. Increasing its held force from 32 to 126 changed RMS from 0.0654 to 0.0924. Capture uses the same key and elapsed time in each run. |
| Browser and solo regression | In touch Chromium the exact packaged firmware booted, displayed Hydra, responded audibly to a key, released to silence and auditioned the next portrait. The solo regression separately exercised all 20 creatures, four-note chords, finger rocking and one-shot replay. |

[60 MHz results](../evidence/choir/results-60.json) · [100 MHz results](../evidence/choir/results-100.json) · [Native four-seat display](../evidence/choir/four-singers.png)

## FM-1 hardware compatibility

The same 2.1.0 package was tested at 60 MHz with each flash chip selected explicitly before every simulated boot.

| M-VAVE FM-1 flash | JEDEC ID | Result in the chip emulator |
|---|---|---|
| Puya P25Q80 | `0x856014` | Boot, call and answer, four-part save, and favourite replay after restarting passed. |
| UC25HQ80 | `0xB36014` | Boot, call and answer, four-part save, and favourite replay after restarting passed. |

On each model the first save changed sector `0xD8000`. After restoring the saved flash on a fresh boot, the call and answer measured 0.0635 and 0.0736 left-channel RMS. The four-singer chord had zero missed DAC refills. The package reports `FM-1_709020100` and carries the 13,396-byte update loader. To identify a unit, read **Device Info → Flash Chip** in M-Flasher. [M-VAVE compatibility notice](https://forum.m-vave.com/topic/89) · [Firmware Hub requirements](https://thefirmwarehub.com/submit/).

### Physical FM-1 with Puya P25Q80 flash

On 27 November 2026 Gabor installed the released package on a black M-VAVE FM-1. M-Flasher 0.10.0 reported stock 015, chip ID `0x856014` (1 MiB) and m-bootloader 2.3-6bc8f45 before the write. It identified the package as `FM-1_709020100`, saved a full backup and verified all 203 sectors in the write plan. The settings and Bluetooth erase option was selected for this installation.

After boot, the physical LCD showed Lernaean Hydra, Jacob Leviathan and The Bloop, including the Wobble control. Gabor’s phone video shows live key presses and captures pitched tones during play. Physical four-part looping, favourite recall after power-off, USB identity, installation from another firmware, switching away and a UC25HQ80 unit remain unverified.

### Repeat the emulator checks

Run these from the repository root with Node 24.

```sh
for chip in p25q80 uc25hq80; do
    ABYSS_FLASH="$chip" ABYSS_MHZ=60 node scripts/test-performance.mjs simulator \
        builds/abyss-1-monster-choir-2.1.0.fwsc "out/compat-$chip" --choir
done
```

## Reproduce

Use the [build and test commands](../BUILDING.md). Firmware identity is `FM-1_709020100`. The package is 858216 bytes with SHA-256 `9d6d60c411e1621956c29a6bfe1f53ed522862a368cab61f04ad8800f51367c6`. The [choir test](../scripts/test-choir.mjs) plays through the panel, restores a saved flash snapshot, and injects velocity and aftertouch at the emulated key contacts.

## Limits

The choir supports one note per singer, a fixed 100 BPM and one favourite. Each cell captures one note and its expression. Multiple attacks within one sixteenth collapse into that cell. A browser favourite is specific to its storage and firmware package. Continuous hardware pressure is opt-in and supported on the lower 20 keys. The paired-key growl also works through ordinary key switches. Physical key feel, physical power-loss recovery, speaker balance and unaided child use remain open.
