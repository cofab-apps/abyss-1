# Requirements, acceptance and project backlog

## Phase 0 evidence

Passed: recursive public checkout, verified compiler and SDK setup, upstream default target release build, four-part target build, second build from saved configuration, package boot and display in desktop Chromium, emulator audio from four MIDI channels, both original prototype entry points, prototype record/playback/track mute and save/load smoke tests. See evidence/. The original archive SHA-256 is b49889a22e33d7e35ce960afcf6b9df5064bd8adbb46d950377b659811931c14.

The hardware, user’s Mac, creator permissions and hosted CI are outstanding. A local Git repository and a manual CI workflow exist. Approval to publish is outstanding. Preserve the reference unit. No physical test results exist.

## Phase 1 implementation order

| Priority | Work | Acceptance on the next build |
|---|---|---|
| 1 | Character definition and assignment | Each of four stable character IDs references family, preset, four macro maps, preview note and portrait. Any of 16 track/character pairings loads with the original track notes and volume preserved. |
| 2 | Four authored sound patches | Bass: downward FM growl over a tuned fundamental. Percussion: pitched impact plus filtered noise and a short resonant tail. Vocal: pressure-driven vowel glide. Atmosphere: slow detuned, evolving partials with bounded shared ambience. Audition dry, octave-shifted and together; reject patches that lose pitch or obscure attacks. These are design targets. |
| 3 | Recording and transport | Record a bass line, overdub percussion on a second part, layer a held melody and pad, replay two complete loops. Assert note on/off ordering, wraparound duration, mute and independent track gain. Confirm tempo changes preserve beat position. Scale lock rounds to allowed notes and can be disabled. |
| 4 | Expression and mixing | Separate key-down velocity, held pressure and note-off events; reserve per-voice state and smooth macro updates. Respond to MIDI velocity in the emulator. Validate real soft/firm strikes and pressure on supported keys after approval; use voice knob on every key. |
| 5 | Native-resolution portraits and orchestra | Compare original and palette-converted images at 240 × 240 and 120 × 120. Show four distinct silhouettes. Repaint the active tile on a note; measure worst redraw while four tracks sound. Integrate decoder and remeasure flash and CPU. |
| 6 | Storage | Round-trip character IDs, macro settings, tracks, note lengths, velocities, tempo, volume and mute. Test load after restart, invalid versions, interrupted write and recovery of the previous valid save. Use the upstream storage map. |
| 7 | Performance | Capture idle and peak audio-block time, underruns, voice shedding, stack/arena high-water and redraw time over ten minutes with four active tracks; require no audible dropout or stuck note. Test DIN and USB MIDI with Ableton Live 12 Suite on the Mac. |

## Proposed panel interaction to test

| Control | Normal play | Feedback |
|---|---|---|
| SELECT encoder | Move among four tracks | Thick outline around the selected portrait; preserve note state |
| PRESETS encoder | Select a creature within the current family | Portrait and short preview |
| ALGORITHM encoder | Change family | Distinct family icon and preview |
| HOME / SEL | HOME shows the orchestra; SEL shows the selected portrait | Stable spatial position and consistent outline |
| K1–K4 | Size, mood, voice, depth | Audible change during a held note; restrained visual response |
| PLAY / REC / SAVE | Toggle transport; arm/disarm overdub; save current song | Transport LED and icon, red recording pulse, brief save acknowledgement |
| SEQ | Open step view; SELECT chooses step, keyboard enters its note; SEQ returns | Playhead and occupied steps use shape as well as colour |
| GLO | Enter mix view. SELECT chooses track, K1 volume, K2 tempo, SEL mute. HOME exits. | Four level bars, beat pulse and crossed-out portrait for mute |
| MASTER / OCT− / OCT+ | Master output and keyboard octave | Direct physical control |

Use tactile/colour stickers for first child sessions if needed. Verify that a child can discover the mapping before committing overlays. Loading, scale choice, save slots and overwrite confirmation still need a simple parent/child interaction design.

## Phase 2–4 gates

- Expand from four to 16 distinct, attributable characters with four per family. Track each asset’s origin and permission separately from firmware licensing. Test recognition at both sizes and blind listening for different timbres.
- On the development unit only, after approval: verify backup and recovery route, instrument latency and tuning, top-row exception, display colours, charge/power cycles and saved compositions. Run the same short phrase on the reference unit to compare response.
- Child session: present a silent orchestra, let the child find a sound, ask for a second creature to join, then let them record and replay a phrase without reading. Note independent actions, mistaken controls, time to first intentional note, and voluntary return. No child testing has yet taken place.
- Distribution: verify corresponding sources and asset permissions, produce a versioned package, tested install/restore instructions per supported hardware revision, website demo and disclosure of supported key expression.

## Open defects and risks

- Prototype laptop keyboard shortcuts ignore focused buttons. After clicking REC, typing a note can appear to do nothing; the onscreen keys work. Regression test clicks the canvas before typing. Save/load currently accepts lightly validated JSON with a single localStorage slot.
- The prototype stores one pitch per track/step, with fixed-duration sound and no stored pressure, gate or velocity. Timing uses browser timers; the master volume is global. These limitations are inappropriate for expressive firmware.
- Four-part profile uses the upstream memory layout with reduced space for stored music. Measure useful song capacity on stock hardware before fixing the production flash layout. No SD modification is assumed.
- Package rebuilds from identical configuration had identical target sizes and different SHA-256 values. Save the tested package and its own checksum. Byte-for-byte determinism remains open.
- Sound quality, final art legibility, palette fidelity, device headroom, pressure sensing and child engagement cannot be accepted from a bootstrap test. Research character-specific visual and sonic references before final artwork.
