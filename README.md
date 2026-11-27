# Seamonster Choir 2.4.0 beta

**[Play Seamonster Choir](https://joakimeriksson.github.io/fm1-emu/?package=https%3A%2F%2Fraw.githubusercontent.com%2Fcofab-apps%2Fabyss-1%2Fmain%2Fbuilds%2Fseamonster-choir-2.4.0.fwsc&name=Seamonster%20Choir&embed=1&mhz=60)** · [Download 2.4.0 beta for FM-1](builds/seamonster-choir-2.4.0.fwsc) · [Previous 2.3.1](builds/seamonster-choir-2.3.1.fwsc) · [Watch and hear the combined build](evidence/choir/2.4.0/ensemble.mp4) · [See and hear the four turns and sea effects](evidence/choir/rows-and-sea-2.2.0.mp4)

Play twenty sea monsters on the M-VAVE FM-1. Trade singing calls, shape a four-part choir or turn a sound from home into an instrument.

Tap **Start**, press **HOME** and tap **SEL** three times to fill a fresh choir. Press **ARP**. Play a short call as the top-left monster, then another when the marker reaches the top right. Lift your fingers and the two monsters below sing your phrases back. Each turn lasts two beats. Press **PLAY** to stop and press it again to loop your exchange.

In **2.4.0**, a monster can answer with your own sampled sound. Hold **EDIT** in the stopped choir to strum four singers with your fingers. Hold **GLO** to save three songs and play them in the order you choose. [2.4 player controls](docs/MANUAL.md#give-bloop-your-sound) · [Physical playtest and simulator evidence](docs/MONSTER-KEYS.md#combined-hardware-beta-240)

## Come playtest

Can you teach four sea monsters a tune? Try it in the browser, on your own or with a child. Give the two top-row singers different calls, then listen for which monster answers each one. Tell us where you got stuck, or what you wanted the monsters to do next.

[Suggest a feature](https://github.com/cofab-apps/abyss-1/issues/new?template=feature-idea.yml) · [Report a playtest or bug](https://github.com/cofab-apps/abyss-1/issues/new?template=playtest.yml)

![Four choir seats on the native FM-1 display](evidence/choir/four-singers.png)

The screen shows four of the 20 portraits made for Monster Keys from Gabor Varga’s descriptions. His visual reference was MetaVine Labs’ [Legendary Sea Monsters 2025](https://www.youtube.com/watch?v=VciSINFWAzw). [Portrait atlas, SCP creator credits and artwork licence](LICENSING.md).

| To play | On the FM-1 |
|---|---|
| Build a choir | HOME opens four portrait seats. Tap SEL to fill an empty seat. SELECT moves the yellow marker. Turn PRESETS or ALGORITHM to change the portrait in that seat. It stays when you move on. |
| Dive underwater | Press FX, hold a key and turn K1 to descend. K2 adds distorted bite, K3 sweeps a stereo flanger and K4 opens a cavern reverb. FX returns the knobs to the monster’s voice. |
| Sample a sound | Press SEQ for Shell Sampler. Play its starter chirp, or press REC and play a monster. Press REC again to finish. ARP cycles pitched keys, backwards and four slices. [Load your own audio file](docs/MANUAL.md#load-a-sound-from-a-computer). |
| Hear a chord | With transport stopped, hold one white key. The occupied seats add root, fifth, third and low octave. Each sounding monster lights up. |
| Make a repeating song | REC arms one singer. The first note starts two bars. Your phrase repeats after 32 steps. Arm another seat to join on the next downbeat. PLAY stops or restarts them together. EDIT undoes the most recent take. |
| Give one singer a sample | Select its seat, open SEQ, capture or import sound and press SAVE. The sampled seat follows live notes and recorded replies. |
| Strum the four seats | Hold EDIT in the stopped choir. Hold a low white key and tap the four rightmost white keys in any order. |
| Save or perform three songs | Hold GLO in the stopped choir. SELECT chooses a shell, SAVE stores, and the three lowest white keys preview. REC followed by three shell keys sets the order. PLAY performs it. Quick SAVE and GLO still use shell A. |
| Whisper and growl | Hold a white note and rock onto its paired raised key. The bend grows into a growl over 0.7 seconds. On the FM-1, press ENV with all keys up to enable continuous finger pressure on the lower 20 keys. |

**Hardware.** Monster Choir 2.1.0 booted on a physical M-VAVE FM-1 with Puya P25Q80 flash after installation from stock 015. Gabor played notes, browsed portraits and brought up Bloop’s Wobble control. Version 2.4.0 has passed the packaged simulator checks and awaits physical UAT on this firmware. The earlier 2.3.1 beta carries the Hub-compatible format for its firmware ID, `FM-1_85020301`. It passed on the simulated Puya P25Q80 and UC25HQ80. The real-device install from stock and another firmware, USB identity after boot, switch away and physical USB sample transfer are open. [Beta evidence and hardware test](docs/MONSTER-KEYS.md#firmware-hub-candidate) · [Previous hardware results](docs/MONSTER-KEYS.md#fm-1-hardware-compatibility).

[Player guide](docs/MANUAL.md) · [Verification and limitations](docs/MONSTER-KEYS.md) · [Rebuild](BUILDING.md) · [All 20 portraits](monster-keys/bestiary.png) · [Credits](LICENSING.md)

Created by Gabor Varga, Zurich. Built on fm1-base. Browser player by Joakim Eriksson.
