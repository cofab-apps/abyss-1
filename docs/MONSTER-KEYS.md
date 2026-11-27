# Monster Choir 2.1.0

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

## Reproduce

Use the [build and test commands](../BUILDING.md). Firmware identity is `FM-1_709020100`. The package is 858216 bytes with SHA-256 `9d6d60c411e1621956c29a6bfe1f53ed522862a368cab61f04ad8800f51367c6`. The [choir test](../scripts/test-choir.mjs) plays through the panel, restores a saved flash snapshot, and injects velocity and aftertouch at the emulated key contacts.

## Limits

The choir supports one note per singer, a fixed 100 BPM and one favourite. Each cell captures one note and its expression. Multiple attacks within one sixteenth collapse into that cell. A browser favourite is specific to its storage and firmware package. Continuous hardware pressure is opt-in and supported on the lower 20 keys. The paired-key growl also works through ordinary key switches. Physical key feel, physical power-loss recovery, speaker balance and unaided child use remain open.
