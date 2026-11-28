# Seamonster Choir 2.5.0 beta — feature definition

Seamonster Choir turns the M-VAVE FM-1 into an instrument for playing by ear. A child can give each creature a voice, decide who enters a chord, record a musical exchange, and arrange three short songs into a performance. This page defines the behavior of the downloadable beta. The player manual gives the button sequences. [Illustrated manual](MANUAL.md) · [Play or download](../README.md) · [Release measurements](MONSTER-KEYS.md#sound-design-beta-250)

## The twenty voices

Play the same key and hold it for one second. The first sound, the pitches that join it and the sustained rhythm are part of each preset. These are original musical interpretations of the portraits. [Hear five before-and-after examples](../evidence/choir/2.5.0/listen.mp4) · [Portrait provenance](../LICENSING.md)

| Monster | Listen for | Monster | Listen for |
|---|---|---|---|
| Crab Monster | Two quick claw strikes and a gap | SCP-682 | Low jaw grind with a slow pulse |
| Lernaean Hydra | Root, minor third, fifth and octave enter in succession | Bake-Kujira | A struck, slightly inharmonic ring gives way to a hollow whale tone |
| Ningen | A slow breath with an octave halo | Loch Ness Monster | A rounded high whistle with a scooped attack |
| Jacob Leviathan | Modulated, pulsing metallic plates | Scylla | A fifth and octave answer a pulsing attack |
| Terrible Dogfish | Repeated short, raspy bites | The Bloop | A rounded pitch drop above a low fundamental layer |
| The Dark Bloop | A slower, deeper throbbing growl | El Gran Maja | A buzzy jaw with faster pulsing |
| Julia Beast | Bright split cry with a delayed octave | Lyngbakr | A slow horn swell with a high overtone above the low drone |
| Sea Eater | Rapid rough bites with an audible sub | SCP-3000 | A winding reed with a slow pulse |
| Colossal Whale | A low swell with a fifth arriving later | SCP-169 | A hard descending impact that settles into a sub drone |
| World Serpent | Fifth and octave accumulate slowly around the root | Space Bloop | A sustained key cycles root–fifth–octave–fifth beneath the upper layers |

The solo keyboard can play four notes, including four independent Hydra chords. Harmony inside a voice leaves each choir seat available. The solo OCTAVE ARP walks a held chord across two octaves. The shared LFO modulates pitch, amplitude and low-pass cutoff independently. Each synth note has a four-stage amplitude envelope. Low-pass, high-pass, third-harmonic and sub-octave controls operate before the existing sea effects. The four sea effects retain their fixed routing. [Control mapping, reset positions and exercises](MANUAL.md#design-a-monsters-sound)

## The musical building blocks

| What the player chooses | What the instrument does | How to recognize it |
|---|---|---|
| One of 20 solo monsters | A portrait selects a pitched synth voice with its own timbre and four starting knob settings. Up to four notes sound at once. Raised keys can articulate their paired white note. | The name and portrait change together. The four colored bars reflect the current settings. [Play and shape a voice](MANUAL.md#shape-the-voice) |
| Four choir seats | Each occupied seat owns one singer, four voice settings and one 32-step line. Browsing commits the new singer immediately and loads its voice settings. Recorded pitches and rhythm stay with the seat. | Moving the selection away leaves the chosen portrait in place. A plus marks an empty seat. [Build a choir](MANUAL.md#pick-four-singers) |
| One-key harmony | At rest, one note sounds the occupied seats as root, fifth, third and lower octave. The third changes with the root. During ordinary loop playback, a live key plays the selected seat over the other recorded parts. | A single low C can sound C, G, E and low C in seat order. [Hear the four parts](MANUAL.md#strum-the-choir) |
| A singer’s voice or the shared sea | Mouth, Teeth, Wobble and Cave belong to a synth voice. Depth, Anger, Current and Cavern act on the whole mix. Depth darkens the mix and lowers synth pitch. Current and Cavern produce stereo movement and a tail. | Switching knob pages preserves the settings. Release a note to hear its echo. [Sea effects](MANUAL.md#dive-underwater) |
| Two calls and two replies | At 100 BPM, a full top row divides bar one into two calls. The bottom row answers in the same order in bar two, repeating the corresponding pitches, durations and rests with its own voices. A lone singer in a row takes the whole bar. | The gold marker visits the four portraits in reading order. PLAY can loop the completed exchange. [Four-turn example](MANUAL.md#trade-calls) |
| A solo take or a choir loop | Solo captures one performance of keys and control gestures. Choir recording puts one part per seat on a common 32-step clock. A nonempty pass replaces the armed part. Undo restores the most recent recorded change. | Solo playback ends. In the choir the moving step returns to the beginning and the other seats keep their parts. [Recorder controls](MANUAL.md#loop-a-phrase) |
| A sample | Shell Sampler holds a mono clip of up to two seconds at 11,025 Hz. Capture a solo monster or incoming USB audio, or receive a clip from the computer loader. Audition the whole clip pitched forwards, backwards, or play one of four slices. | A waveform shows the available sound. A silent take preserves the previous clip. [Catch a sound](MANUAL.md#catch-a-sound) |

## Guest Singer

**Player goal.** Hear Bloop answer Hydra with a personally chosen sound, while the other seats keep their own voices. The destination is the choir seat selected before opening SEQ. The monster named beneath the waveform is the recording source. The name beside SAVE is the destination. Starting in solo targets the first choir seat.

**Behavior.** SAVE in the sampler assigns a copy of the current clip and returns to the choir. One seat per arrangement can use the guest. The seat keeps its recorded notes, so a sampled reply can follow the same melody and rests as the original singer. Choir notes play the guest forwards at a speed set by pitch. A higher pitch shortens its duration. A sustained gate cannot extend a finite clip. The sampler’s reverse and slice modes apply to its own keyboard. The guest still feeds the seat’s echo and the shared sea effects.

**Acceptance.** Assign Crab’s captured click to bottom-left Bloop. Compare the reply against the captured waveform and audition all four seats individually. The other three voices should retain their sound. After saving and rebooting, the song should play its other seats and identify the missing guest. The clip itself lives in RAM. Reattach a clip with sampler SAVE, or use sampler EDIT on that guest’s seat to restore its synth voice. [Illustrated walkthrough](MANUAL.md#give-bloop-your-sound) · [Regression test](../scripts/test-ensemble.mjs)

## Strum the Choir

**Player goal.** Decide which singer enters next and how long to wait between entries. In the stopped choir, holding EDIT for 520 ms or more enables strum. Hold one of the first eight white keys as the root. Fresh presses on the last four white keys trigger seats in reading order. A root on its own is silent. An empty seat contributes silence.

**Behavior.** Synth seats sustain after the strum finger lifts while the same root is held. The same pad can be struck again. Changing or releasing the root clears the entries, ready for a new chord. A guest seat plays its clip through once per attack. The child may sweep backwards, skip a singer or tap Bloop last. REC, ARP and PLAY leave strum for their usual choir functions. Strum is a live gesture in this version.

**Acceptance.** Sweep 1–2–3–4 and 4–3–2–1. Each occupied portrait should light only when its pad is struck. The root should release all voices, allowing effect tails to decay. On hardware, try making the same monster enter last twice using the lights alone. [Keyboard map](MANUAL.md#strum-the-choir) · [Native LCD and stereo recording](../evidence/choir/2.4.0/ensemble.mp4)

## Songs in Shells

**Player goal.** Recognize three songs by ear and choose a beginning, middle and end. Hold GLO in the stopped choir to open A (scallop), B (conch) and C (spiral). Turning SELECT highlights a destination. SAVE writes there. SEL explicitly loads it. The three lowest white keys preview saved slots for one pass. A preview restores the working arrangement when it finishes or is stopped.

**Behavior.** Shelf REC starts a three-position order. Three taps on occupied slots set and store the order, including repeats such as A–A–C. Shelf PLAY performs those three 32-step arrangements once and stops. Each phrase is 4.8 seconds, giving 14.4 seconds for the three phrases. An empty song slot is different from an occupied slot with silent lanes. The former shows EMPTY SHELL when requested. The latter can play a full phrase of rests.

**Persistence.** Each slot holds the seats, voice settings, recorded lines, four sea settings, twelve synth edits and guest association. The order is stored separately. Shell A reuses the earlier favourite. Quick choir SAVE and GLO continue to write and recall A, including after B or C was loaded. Saving on the shelf replaces the highlighted slot. An interrupted or abandoned three-tap order leaves the last completed order available.

**Acceptance.** Save distinctive first notes in A, B and C, enter C–A–B, power-cycle and play. Check the active shell at each boundary and listen for its first note. Previewing and performing should leave both the stored songs and the original working arrangement available. [Shell controls and pictures](MANUAL.md#put-three-songs-in-shells) · [Flash and DAC results](../evidence/choir/2.4.0/results.json)

## Verification and hardware scope

The 2.5.0 package is `FM-1_85020500`. The release report records the emulator runs and DAC comparisons. Gabor’s documented physical boot, portraits and played notes apply to 2.1.0 on a Puya FM-1. The remaining listening test is whether a player can identify the new voices without the screen and hear the low layers on the built-in speaker. [Current and earlier hardware evidence](MONSTER-KEYS.md) · [Run the checks](../BUILDING.md)
