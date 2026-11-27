# Monster Choir · proposed design

**For review.** Monster Keys 2.0.1 plays one creature with up to four notes. This proposal uses those four voices in a second way: four different creatures, each with a recognisable place, sound and melody.

## A child's first choir

1. Start by playing Hydra as usual. Press **HOME** to open four portrait seats in a square. Hydra occupies the first seat. Three empty seats show a large **+**.
2. Turn **SELECT** to a **+**. Turn **PRESETS** to see and hear candidates. Press **SEL** (labelled **SCL** on the emulator) to seat one. On an occupied seat, browsing is an audition. Only confirming changes the singer. Its recorded notes stay in place.
3. Hold a white key. Every seated monster sings. One singer plays the root, the second adds a fifth, the third adds the scale’s third and the fourth doubles the root an octave below. On C, hear C–G–E with a low C underneath. Give each singer its assigned concert pitch even when its solo preset normally transposes the keyboard. Each sounding portrait lights up. K1–K4 shape only the outlined singer, which remembers its four settings. Press HOME again to return to the large solo portrait.
4. Press **REC** on Bloop's seat and play two low notes. On the next pass, choose Crab and tap a rhythm, then sing an answer with Hydra. PLAY starts or stops all recorded parts together.

## What REC and PLAY should do

Use **32 sixteenth-note steps**, two bars of 4/4. At a proposed starting tempo of 100 BPM one pass lasts 4.8 seconds. Show two rows of 16 lights, with the beats grouped in fours. A moving light gives the child a place to enter. Filled steps show attacks, attached segments show held notes and empty steps stay silent.

| Gesture | Proposed result |
|---|---|
| REC on a fresh choir | Arm the outlined seat. The first key starts step 1. Record one two-bar pass, snap attacks to the nearest step, preserve gates and rests, and automatically repeat what was played. REC a second time finishes early and leaves the unused steps silent. |
| REC on the next seat while the loop runs | The other parts keep playing. The seat flashes until the next downbeat. Capture its own line on the same clock. An empty pass preserves the existing line. |
| Play while a loop is running | The keyboard plays the outlined seat so the child can practise against the others. At most one note per seat in choir mode. A live note takes precedence on that seat. |
| REC again on a filled seat | Replace only that seat's line after a nonempty pass. EDIT undoes the last completed pass, including a replacement. |
| PLAY | Start all recorded seats at step 1, or stop them together. When the loop is stopped the keyboard again plays the assembled choir. |
| Change a singer or its sound | Keep that seat's rhythm and pitch data. Switching seats restores its four knobs. Capture bends and knob gestures with the recorded seat so an expressive take repeats. |

**Acceptance before release.** Hear four different creatures from one key. Record two bars on one seat, add three independent lines, then solo each in the test harness to verify pitch, rhythm and rests. Check step 32 returning to step 1, tied notes, a short tap, empty recording, undo and transport stop. Measure stereo DAC output and refills at 60 MHz with all four parts sounding. With a child, see whether they can add a second singer and hear their first phrase come round without reading.

Saving the choir across power cycles is a separate acceptance item in the original plan. For the first playable prototype, test the whole performance in RAM and label its lifetime on the guide.

[Play the current instrument](../README.md) · [Current controls](MANUAL.md)
