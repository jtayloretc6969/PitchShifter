# FluteShift

A first playable VST3 and standalone pitch shifter for live flute beatboxing,
built with C++17 and JUCE 8.0.6. Targets Ableton Live on macOS, including Apple
Silicon. This is an experimental instrument, not a finished commercial effect.

## Download and install in Ableton

Open this repository's **Actions** tab, select a successful **Build and test**
run for `main`, then download **FluteShift-macOS-universal** under **Artifacts**
(sign in to GitHub if needed). Extract the downloaded artifact ZIP, then extract
`FluteShift-macOS-universal.zip` inside it to obtain `FluteShift.vst3`.
An artifact is available only after the Mac build and tests succeed.

In Finder choose **Go → Go to Folder**, enter
`~/Library/Audio/Plug-Ins/VST3/`, and copy `FluteShift.vst3` there. Create that
folder if it does not exist. In Ableton Live enable **VST3 system folders** in
**Settings → Plug-Ins**, click **Rescan**, and add FluteShift to an audio track.
The completed bundle is ad-hoc signed and verified after manifest generation.
It is not Developer ID signed or notarized; macOS may require additional approval
before Live can load it. Successful CI does not establish live performance readiness.

For source code, use **Code → Download ZIP**. To build and install locally,
install Apple's command line tools and CMake, then run
`bash scripts/install-mac.sh` from the extracted repository folder.

## Performance controls

- **Transpose:** −24 to +24 semitones; octave-down, reset and octave-up buttons.
- **Tone:** longer grains for sustained flute.
- **Beat:** shorter grains for a tighter, more granular percussion sound.
- **Hybrid:** medium grains with latency-aligned dry attack blending.
- **Mix** and **Output:** automated by the host and saved with the Live Set.

These are three voicings of an original time-domain granular engine, not
Ableton's proprietary algorithms. Two complementary grains share a phase across
channels. Tonal zero-crossing period tracking aligns their spacing to reduce
cancellation on sustained notes. Ratio, window, mix and gain changes are smoothed.
The callback allocates no memory and takes no locks. Mono and stereo are supported.

The engine reports a fixed **16 ms nominal latency** to Live (768 samples at
48 kHz). Interface buffers, monitoring and host routing add latency; changing
grains also spreads transients around that nominal delay. Measure the complete
microphone-to-output path on the performance Mac before a show.

## Build on a Mac

Install Xcode command line tools (`xcode-select --install`) and CMake 3.22 or
later. Configure and build from this repository:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
```

CMake fetches the pinned JUCE release from GitHub. To use an existing checkout,
add `-DFETCHCONTENT_SOURCE_DIR_JUCE=/absolute/path/to/JUCE` at configuration.

Copy `build/FluteShift_artefacts/Release/VST3/FluteShift.vst3` to
`~/Library/Audio/Plug-Ins/VST3/`, creating the directory if necessary. In Live,
enable VST3 system folders in Settings → Plug-Ins, rescan, and insert FluteShift
on an audio track. Use headphones for the first microphone test. Select the
interface's native sample rate and begin with a 64- or 128-sample buffer.

For foot control, expose Transpose, Engine and Mix through Live's Configure
panel, then MIDI-map them or map Audio Effect Rack macros. Octave buttons change
the same Transpose parameter; they are not separate MIDI targets.

The GitHub Actions workflow builds a universal macOS VST3 ZIP when this repository
is pushed or its workflow is manually triggered. That workflow has not been run
from this workspace. Builds are ad-hoc signed and are not notarized distribution
packages. The standalone application is also built for audio-device testing.

## Validate without JUCE

```sh
cmake -S . -B build-dsp -DFLUTESHIFT_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp --parallel 2
ctest --test-dir build-dsp --output-on-failure
```

The suite checks exact delayed unity, reset, stereo coherence, block partitioning,
54 pitch/mode/sample-rate combinations, mono/stereo noise, and rapid automation.
It also reports DSP processing time, which is not an end-to-end latency measurement.
When the plugin is enabled, a second test exercises the JUCE processor's impulse
response, reported latency, and transpose/mode/mix session-state recall.

Validated in this workspace: Release VST3 and standalone compilation on Linux;
both CTest suites passed. DSP address/undefined-behaviour sanitizer checks also
passed, with leak detection disabled because the runner uses ptrace. macOS CI
and Ableton host validation have not been executed.

## Current sound limits

The period tracker assumes a mostly monophonic source. Breath noise, chords and
mixed flute/percussion can confuse it; Hybrid blends some attacks unshifted.
This version has no formant preservation, polyphonic spectral engine or dedicated
anti-alias oversampling. Wide upward shifts can alias, and short grains can colour
or flutter sustained sounds. The next quality milestone is comparison using real
flute-beatbox recordings and live monitoring, followed by stronger pitch tracking,
anti-alias filtering and an alternative spectral engine if listening supports it.
There is no claim of parity with RC-505, G2b or Ableton's engines yet.

macOS compilation, Ableton scanning, session recall, MIDI mapping and live audio
round-trip latency require validation on a Mac. Cloud DSP tests establish only
the properties they exercise.

JUCE has its own AGPL/commercial licensing terms. Choose a compatible licensing
route before distributing this plugin; this repository does not grant a JUCE
commercial licence.
