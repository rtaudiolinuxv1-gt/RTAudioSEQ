# RTAudioSeq Instructions

## What RTAudioSeq Does

`RTAudioSeq` is a Linux-native step sequencer for building patterns with:

- SoundFont instruments (`.sf2`)
- Sample playback (`.wav`, `.flac`)
- MIDI output to external devices
- Pattern generation and mutation
- JACK audio output
- WAV recording and offline WAV rendering

## Build

From the project root:

```bash
mkdir -p build
cd build
cmake ..
make -j4
```

Run:

```bash
./RTAudioSeq
```

## Audio Setup

`RTAudioSeq` outputs audio through `JACK`.

Before starting the app:

- Start `JACK` or a `PipeWire` JACK-compatible session
- Make sure your system audio graph is running normally

Inside the app:

- Press `Connect Outputs` on the `Performance` tab to connect the app to the first physical JACK playback ports automatically

If there is no JACK server, the UI will still open but live audio features will not work.

## Main Workflow

Typical workflow:

1. Load a soundfont on the `SoundFont` tab, or load samples per instrument on the `Instruments` tab
2. Go to the `Performance` tab
3. Set tempo, bars, steps per bar, key, scale, note variation, and mutation depth
4. Press `Regenerate`
5. Press `Play`
6. Edit steps directly in the grid
7. Record or render from the `Export` tab

## Performance Tab

The `Performance` tab contains:

- `Play`: starts or stops transport
- `Connect Outputs`: connects JACK outputs automatically
- `Regenerate`: creates a fresh pattern from the current settings
- `Mutate Now`: mutates the current pattern immediately
- `Enable Mutation`: enables or disables automatic mutation
- `BPM`: tempo
- `Pattern Bars`: total number of bars in the pattern
- `Steps / Bar`: step resolution for each bar
- `Edit Bar`: chooses which bar is shown in the step grid
- `Repeat Before Mutate`: number of full pattern loops before automatic mutation
- `Swing`: timing swing amount
- `Note Variation`: increases melodic pitch spread and note-change intensity
- `Mutation Amount`: overall mutation strength
- `Key`: root note for melodic generation
- `Scale`: pitch set for melodic generation; choose `Chromatic` for no scale restriction

## Step Grid

The step grid shows the currently selected bar.

- Left click a step to toggle it on or off
- Right click a step to open detailed step settings
- The currently selected step can be edited from the `Step Note Keyboard` at the bottom of the `Performance` tab

Each row has:

- instrument name
- `Defaults` button for instrument-wide step defaults

Row labels may also show:

- sample filename in brackets
- selected soundfont preset in brackets

## Step Note Keyboard

When you click a visible step, the note keyboard at the bottom of the `Performance` tab becomes active.

Use it to:

- assign a note quickly to the selected step
- move octave up or down

This is faster than opening the full step editor when you only want to change pitch.

## Step Settings

Right click a step to edit:

- `Velocity`
- `Volume`
- `Note`
- `Attack`
- `Decay`
- `Sustain`
- `Release`
- `Gate`
- `Lock Step`

If `Lock Step` is enabled, later instrument-wide default changes will not overwrite that step.

## Instrument Defaults

Each instrument has a `Step Defaults` button.

This applies default values across all unlocked steps on that row:

- `Volume`
- `Attack`
- `Decay`
- `Sustain`
- `Release`
- `Gate`

Use this when you want one instrument lane to share the same envelope and level behavior.

## Instruments Tab

The `Instruments` tab lets you:

- add a new instrument with a custom name and role
- rename instruments
- change instrument role
- set instrument density
- move instruments up or down
- remove instruments

Per instrument you can also enable:

- `Sample`
- `SF2`
- `MIDI`

Other per-instrument controls include:

- MIDI channel
- SoundFont channel
- SoundFont bank
- SoundFont program
- preset selector populated from the currently loaded soundfont
- sample load/clear controls

## SoundFont Tab

Use the `SoundFont` tab to:

- `Load SF2`
- `Clear SF2`

If `/usr/share/sounds/default.sf2` exists, the app attempts to load it on startup and enables `SF2` for all instruments.

## Export Tab

The `Export` tab contains:

- `Record WAV`
- `Record FLAC`
- `Stop Recording`
- `Render WAV By Bars`
- `Render WAV By Seconds`
- preview file loader for `.wav` or `.flac`
- preview waveform display
- preview transport: `RW`, `Play`, `Stop`, `FF`
- `Preview Gain`

Preview playback uses the app's JACK outputs, not a separate desktop media backend.

`Preview Gain` is measured in `dB`:

- `0 dB` means unity gain
- negative values make preview quieter
- positive values make preview louder

## Projects

Use the `Instruments` tab to:

- `Save Project`
- `Load Project`

Project files store the current scene, including:

- instruments
- steps
- soundfont path
- sample paths
- pattern settings
- key and scale settings
- note variation and mutation settings

## Notes

- Drum-oriented rows keep their GM drum note behavior
- Bass, lead, custom, and pitched percussion rows follow the selected key and scale unless `Chromatic` is selected
- Changing `Key` or `Scale` quantizes existing active melodic notes into the chosen scale
- Offline rendering writes WAV files; live preview and playback depend on JACK routing
