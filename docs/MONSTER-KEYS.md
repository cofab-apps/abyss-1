# Seamonster Choir

For the current instrument, see the [feature definition](FEATURES.md) and [illustrated player manual](MANUAL.md). This page records versioned tests, hardware evidence and open checks.

## Sound design beta 2.5.0

[Compare five voices by ear](../evidence/choir/2.5.0/listen.mp4) · [Native editor screens](../evidence/choir/2.5.0/sound-pages.png) · [Target measurements](../evidence/choir/2.5.0/results.json) · [Firmware](../builds/seamonster-choir-2.5.0.fwsc) · [Reproduce](../BUILDING.md)

The FM-1 package identifies as `FM-1_85020500`. A clean rebuild reproduced all 858216 bytes, SHA-256 `d5434501fd780f63f660cf45255d3345bddb6a7cd9525c9cda98572824bff7b1`. The 14-second comparison plays 2.4.0, then 2.5.0, for each portrait. Both receive the same held key at the same master setting. The video uses the actual 240 × 240 LCD and stereo DAC, with no loudness normalization.

| Check | Observed on the packaged simulator |
|---|---|
| Notes inside a monster | One dry Hydra key produced components near 165, 196, 247 and 330 Hz. Four held Hydra keys with LFO, filters and all Sea FX active missed zero DAC refills at 60 MHz on Puya P25Q80 and 100 MHz on UC25HQ80. |
| Heard as a pattern | Ness repeated root, upper octave and root. Two held notes played low–high, then the same pair an octave up. |
| Synth controls | A held pitch moved between approximately 290 and 370 Hz in the vibrato test. Low-pass reduced the upper partial relative to the root. High-pass reduced the root. A long attack rose gradually and release faded to silence. The held sustain could close and reopen. |
| Song compatibility | 2.4.0 itself saved a nonempty phrase. 2.5.0 recalled and played it, saved a filter edit, and recalled that edited arrangement after another boot. The earlier 2.1.0 four-lane fixture also played. |
| Existing modes and player | All seven target suites passed on both simulated chips. They exercised all 20 portraits and voices, choir rows, looping, guest, strum, shell storage and sampler. The bundled player also passed a touch Chromium launch with audio. Browser sample transfer and the host receiver passed. |

Two pairs that had sounded alike separated in the sustained-note spectral check. On a scale from zero for identical octave-band proportions to one for disjoint proportions, Bloop versus Lyngbakr went from 0.067 to 0.308, and Hydra versus Ningen from 0.065 to 0.463. Across all 190 pairs, the average went from 0.472 to 0.455. That measure leaves out note attacks, pulses and the order of pitches. Overall identification by ear remains a listening test.

**Physical UAT.** At a comfortable fixed volume, hide the screen and alternate Hydra, Crab, Bloop, Colossal Whale and SCP-169. Record which pairs the listener confuses on the FM-1 speaker, then repeat on headphones. Check whether the low impact reads at ordinary volume and whether a child can play the octave pattern and return to Call & Answer. Keep Firmware Hub submission on hold while the sound design is being tested.

## Combined hardware beta 2.4.0

[Play or download the firmware](../README.md) · [22-second native LCD and stereo DAC capture](../evidence/choir/2.4.0/ensemble.mp4) · [Three native screens](../evidence/choir/2.4.0/screens.png) · [Measured results on both simulated flash chips](../evidence/choir/2.4.0/results.json) · [Reproduce the runs](../BUILDING.md)

The firmware identifies as `FM-1_85020400`. The 858216-byte package has SHA-256 `2603bc05d6bfd72c499ac454068d9ef054d559a2af1697e19760467555f6c474`. A clean rebuild produced identical bytes. The tested target is the M-VAVE FM-1. Hardware acceptance of this version is open. The photographs of a successful 2.1.0 installation establish the earlier Puya FM-1 result.

| Feature | How it was checked |
|---|---|
| Guest Singer | Captured Crab in Shell Sampler, assigned it to bottom-left Bloop and measured its reply against the actual captured waveform. Checked pitched duration at two roots, isolated each of the other three voices, and rebooted a saved song. The caller survived and the missing PCM seat was silent. |
| Strum the Choir | Held a low root, swept four rightmost white keys forwards and backwards, read each active portrait from the native LCD and measured DAC output. Checked silence on root alone, empty seat and root release. |
| Songs in Shells | Recorded different first pitches in A, B and C, rebooted, then observed the saved C–A–B order on the LCD and measured an audible first note at each boundary. Preview and playback left the flash image intact. Re-saving the working song after preview recovered A’s pitch. |
| Previous workflows | All 20 voices, call and response, four recorded lanes, pressure, four underwater effects, sampler, and the 2.1.0 favourite fixture ran through the packaged target. Chromium also exercised the default player with touch and the file loader with a compiled receiver. |

The video starts with two strums, followed by the captured reply and the three-shell performance. Its three songs hold a short note near the start of each 32-step phrase. Listen for the rests and watch the progress line move to the next shell.

A flash pair is reserved for each arrangement and for the play order. Shell A uses the existing favourite sectors at `0xD8000`. B starts at `0xDA000`, C at `0xDC000`, and order at `0xDE000`. Stored songs are validated before playback. The guest’s mono PCM lives in RAM. Its identifying signature lives with the song.

**On the physical FM-1**, verify boot and the running USB ID, speaker and headphone sound, pad response while strumming, three songs after power-off and computer audio over USB. Document the flash chip and starting firmware with each result. Keep the stock full-flash backup for that unit. The Firmware Hub submission remains on hold for this iteration.

## Firmware Hub candidate

[Download 2.3.1 beta](../builds/seamonster-choir-2.3.1.fwsc) · [Play in the browser](../README.md) · [Simulator measurements](../evidence/choir/results-2.3.1.json)

2.3.1 beta packages the 2.3.0 instrument with the eight-digit ID `FM-1_85020301`. The Hub's identity checker reported it free on 27 November 2026. The candidate is 858216 bytes. SHA-256 is `ad289835fce7d9bd79caaad79f908b3978f1bf7952d99227b5cdd5d6667f7683`. A second clean cross-build produced identical bytes. The package marker and firmware application both contain the new ID. Both package entries passed their CRC checks. The included 13396-byte update loader matched the pinned build output.

The packaged firmware was exercised at 60 MHz on simulated Puya P25Q80 flash and at 100 MHz on UC25HQ80. The tests covered all 20 voices, seat retention, two calls and two replies, four-part playback, favourite recall, the sea effects and the sampler. In touch Chromium the package booted, matched the Hydra LCD capture and sounded a note on touch. [Build and reproduction](../BUILDING.md)

### Physical test for the Hub

Use the exact candidate above on a backed-up M-VAVE FM-1. If the instrument currently runs Monster Choir 2.1.0, this sequence covers the Hub's requirements in three installs. Use the normal USB firmware updater for the switches so the test exercises the firmware's own update path.

1. From 2.1.0 install 2.3.1 beta. After boot, read the running USB firmware ID and record whether it is `FM-1_85020301`. Play a note and check the LCD.
2. From the beta install stock V15. Confirm that stock boots.
3. From stock V15 install the same beta again and confirm the USB firmware ID after boot.

Record the model, flash chip, starting and ending IDs and result of each switch. The only physical playtest on record is for 2.1.0. After the candidate passes, submit the repository and hardware results through the [Firmware Hub maker form](https://thefirmwarehub.com/submit/).

## Seamonster Choir 2.3.0

[Play or download](../README.md) · [Play the sampler or load a sound](MANUAL.md#catch-a-sound) · [Native LCD and stereo sampler recording](../evidence/choir/shell-2.3.0.mp4)

## Shell Sampler

SEQ opens a one-voice sampler with pitched, reverse and four-slice playback. It captures a live solo monster or routed USB audio, or receives a clip over USB MIDI. One two-second mono clip at 11,025 Hz fits in RAM. A second bank holds a recording or incoming file until ready. An interrupted upload or silent take preserves the current clip. The shipped chirp returns on power-up.

| Check | Result |
|---|---|
| Target at 60 MHz (Puya) and 100 MHz (UC25HQ80) | A captured monster sounded after all keys were released. Upper keys shortened the clip, reverse produced audio and different slices produced different outputs. A silent USB take preserved the clip. SEQ returned to solo and choir. |
| File transfer | The browser loaded a stereo WAV and received acknowledgements from the firmware receiver compiled on the host. The Python WAV loader and receiver agreed on all 22,050 sample values. The receiver rejected missing, corrupt and out-of-order data. A known stereo PCM input was captured through the sampler’s USB render path in the host harness. |
| Regressions | Four-row turns, seat retention, sea controls, previous favourites, 32-step overdubbing, all 20 solo voices, LCD cues and touch browser playback passed. The four-singer chord with maximum sea effects missed zero DAC refills in both target runs. |

Package ID `FM-1_709020300`. Size 858216 bytes. SHA-256 `c9926b9716d600f3eb38a611c1de4b9824a2789431ebe5698179daa27964876f`. The release rebuilt byte-for-byte. Physical upload over USB MIDI, computer-audio capture over the cable and speaker/headphone sound remain to be tested on 2.3.0. The manufacturer documents USB, a built-in speaker, a headphone output and MIDI in. I found no documented onboard microphone. [FM-1 product specifications](https://www.sincoaudio.com/product?id=fm-1) · [Measurements](../evidence/choir/results-2.3.0.json) · [Reproduction](../BUILDING.md)

[Previous 2.2.0 firmware](../builds/abyss-1-monster-choir-2.2.0.fwsc)

## Monster Choir 2.2.0

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
