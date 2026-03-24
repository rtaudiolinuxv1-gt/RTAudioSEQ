# RTAudioSeq Dependency License Audit

Date: 2026-03-24

## Scope

This audit is based on:

- direct build dependencies declared in [CMakeLists.txt](/home/jim/BUILD/CMakeLists.txt)
- runtime dependencies observed from `ldd build/RTAudioSeq` on this machine
- local filesystem inspection under `/usr`, `/usr/share/doc`, and `/opt`
- upstream or official project licensing pages where local package metadata was missing

This is a practical engineering audit, not formal legal advice.

## Executive Summary

The most important findings are:

1. `RTAudioSeq` directly links against `Qt5`, `JACK`, `libsndfile`, and `FluidSynth`.
2. `Qt` is the most significant licensing decision driver if you are not using a commercial Qt license. The Qt open-source path is mainly `LGPLv3` with obligations around notices, source availability for the LGPL-covered libraries, and user relinking rights. Some Qt modules may be GPL-only.
3. `libsndfile` and `FluidSynth` are both `LGPL`, which is generally workable for closed or open applications if dynamically linked and if you comply with the LGPL terms.
4. The current runtime on this machine pulls in `liblash.so.1` and `libreadline.so.8` transitively, most likely via the installed `libfluidsynth`. That is a serious issue for any non-GPL distribution plan:
   - `LASH` is GPL according to its own manual.
   - `GNU Readline` is GPL.
5. Because those GPL libraries are currently in the runtime link chain, the present binary as built on this machine is not a clean “LGPL/permissive-only” distribution story.

Practical recommendation:

- If you want a simple permissive or weak-copyleft release of `RTAudioSeq`, rebuild or replace the `FluidSynth` stack so it does not pull in `liblash` and `libreadline`.
- If you are comfortable licensing `RTAudioSeq` itself under `GPL-3.0`, the GPL risk is much less problematic.

## Direct Build Dependencies

These are declared in [CMakeLists.txt](/home/jim/BUILD/CMakeLists.txt):

- `Qt5::Widgets`
- `Qt5::Multimedia`
- `jack`
- `sndfile`
- `fluidsynth`
- `Threads::Threads`

## Direct Dependency Findings

### 1. Qt 5.15 (`Widgets`, `Multimedia`, plus `Core`, `Gui`, `Network`)

Observed local runtime path:

- `/opt/qt5/lib/libQt5Widgets.so.5`
- `/opt/qt5/lib/libQt5Multimedia.so.5`
- resolved to `/opt/qt-5.15.0/...`

Licensing status:

- Qt uses a dual licensing model: commercial or open-source.
- Qt’s own licensing pages state that the primary open-source option is `LGPLv3`, with some modules available only under `GPL`.

Implications:

- If you use Qt under its open-source terms and distribute binaries, you need to comply with the LGPL obligations for the Qt libraries you use.
- That usually means:
  - provide license notices
  - provide the LGPL text
  - provide or offer the corresponding Qt source for the LGPL-covered libraries you distribute
  - allow reverse engineering/debugging of user modifications to the LGPL-covered libraries
  - avoid locking users out of replacing the LGPL-covered Qt libraries
- Because this installation is a custom `/opt/qt-5.15.0` tree and I did not find bundled local license files, the exact module-by-module licensing should be verified against the exact Qt source package used to build that tree.

Risk level:

- Medium

Sources:

- Qt licensing overview: https://www.qt.io/qt-licensing
- Qt LGPL obligations: https://www.qt.io/development/open-source-lgpl-obligations
- Qt 5 licensing reference: https://qthub.com/static/doc/qt5/qtdoc/licensing.html

### 2. JACK client library (`libjack.so.0`)

Observed local runtime path:

- `/usr/lib/libjack.so.0`

Licensing status:

- The JACK project states that the JACK server is GPL, but the JACK library is LGPL, specifically to allow proprietary programs to link against it.

Implications:

- Linking to the JACK client library itself is normally compatible with non-GPL applications, subject to LGPL compliance.

Risk level:

- Low

Source:

- JACK project API/license note: https://jackaudio.org/api/index.html

### 3. libsndfile (`libsndfile.so.1`)

Observed local runtime path:

- `/usr/lib/libsndfile.so.1`

Licensing status:

- `libsndfile` says it is available under `LGPL-2.1` or `LGPL-3.0`, at the user’s choice.

Implications:

- Normally compatible with closed or open applications when dynamically linked and LGPL terms are followed.

Risk level:

- Low

Source:

- libsndfile licensing page: https://libsndfile.github.io/libsndfile/

### 4. FluidSynth (`libfluidsynth.so.2`)

Observed local runtime path:

- `/usr/lib/libfluidsynth.so.2`

Licensing status:

- FluidSynth states that its library is distributed under `LGPL-2.1`.

Implications:

- On paper, the library itself is generally usable from non-GPL applications if dynamically linked and LGPL terms are followed.
- However, on this machine the installed `libfluidsynth` brings in GPL runtime dependencies, which changes the practical distribution risk materially.

Risk level:

- Medium by itself
- High in this exact runtime configuration

Sources:

- FluidSynth repository/license note: https://github.com/FluidSynth/fluidsynth
- FluidSynth API docs: https://www.fluidsynth.org/api/

## Critical Runtime Findings

These were observed in `ldd build/RTAudioSeq`.

### 1. `liblash.so.1`

Observed:

- `liblash.so.1 => /usr/lib/liblash.so.1`

Licensing status:

- The LASH manual says LASH is distributed under the GNU GPL and explicitly says software linked against the LASH library must be released under the GPL.

Why this matters:

- Your app does not link to `liblash` directly in CMake, but the built binary does load it transitively at runtime.
- If you distribute this exact binary/runtime combination, this is a serious GPL compatibility concern.

Risk level:

- High

Source:

- LASH manual: https://www.nongnu.org/lash/lash-manual.html

### 2. `libreadline.so.8`

Observed:

- `libreadline.so.8 => /lib/libreadline.so.8`

Licensing status:

- GNU Readline is GPL-licensed.

Why this matters:

- Like `liblash`, this appears transitively in the runtime link chain.
- A GPL library in the runtime chain is a significant issue for any intended non-GPL distribution.

Risk level:

- High

Source:

- GNU Readline entry: https://directory.fsf.org/readline.html

## Important Transitive Runtime Dependencies

These are present in the current binary’s runtime link chain and are worth tracking, but they are not the primary blockers.

### Audio/codec stack

- `libFLAC.so.8`
  - Xiph states the reference libraries are under the New BSD license.
  - Source: https://xiph.org/flac/license.html
- `libogg.so.0`, `libvorbis.so.0`, `libvorbisenc.so.2`
  - Xiph states the libraries/SDKs are under their BSD-like license.
  - Sources:
    - https://xiph.org/vorbis/
    - https://xiph.org/vorbis/faq/
- `libopus.so.0`
  - Opus reference implementation is BSD-licensed, with royalty-free patent grants.
  - Source: https://opus-codec.org/license
- `libinstpatch-1.0.so.2`
  - Not fully verified in this audit from a local or official source page.
  - Treat as verify-before-release.
- `libasound.so.2`
  - Commonly LGPL in practice, but not fully verified from a local or official source page in this audit.
  - Treat as verify-before-release.
- `libpulse.so.0`, `libpulse-simple.so.0`, `libpulse-mainloop-glib.so.0`
  - PulseAudio is commonly LGPL in practice, but I did not confirm the exact library license text from a primary source in this audit.
  - Treat as verify-before-release.

### Core GNOME/freedesktop stack

- `libglib-2.0.so.0`, `libgobject-2.0.so.0`, `libgmodule-2.0.so.0`, `libgthread-2.0.so.0`
  - GLib docs list GLib as `LGPL-2.1-or-later`.
  - Source: https://docs.gtk.org/glib/
- `libdbus-1.so.3`
  - D-Bus FAQ says the reference implementation offers a choice of GPL or AFL.
  - In practice this is usually used under the permissive AFL side.
  - Source: https://dbus.freedesktop.org/doc/dbus-faq.html

### Text, font, image, and graphics stack

- `libxml2.so.2`
  - Official docs say MIT.
  - Source: https://gnome.pages.gitlab.gnome.org/libxml2/devhelp/index.html
- `libharfbuzz.so.0`
  - HarfBuzz states it is under the “Old MIT” license, with some subparts under different licenses.
  - Sources:
    - https://github.com/harfbuzz/harfbuzz
    - https://chromium.googlesource.com/external/github.com/harfbuzz/harfbuzz/%2B/HEAD/COPYING
- `libfreetype.so.6`
  - FreeType is dual licensed: FreeType License or GPLv2.
  - The FreeType License is the normal practical choice.
  - Source: https://freetype.sourceforge.net/license.html
- `libpng16.so.16`
  - libpng uses the libpng license, which is permissive.
  - Source:
    - https://libpng.org/pub/png/libpng.html
    - Qt attribution page showing libpng license text reference: https://doc.qt.io/qt-6/qtgui-attribution-libpng.html
- `libGL.so.1`, `libGLX.so.0`, `libGLdispatch.so.0`
  - Usually permissive/MIT-style Mesa or Khronos ecosystem licensing, but not fully verified from a primary source in this audit.
  - Treat as verify-before-release.
- `libX11.so.6`, `libXext.so.6`, `libxcb.so.1`, `libXau.so.6`, `libXdmcp.so.6`
  - Usually MIT/X11-style licensing, but not fully verified from a primary source in this audit.
  - Treat as verify-before-release.

## Toolchain Runtime Dependencies

### `libstdc++.so.6` and `libgcc_s.so.1`

Observed local runtime paths:

- `/opt/gcc-rtaudio-linux/lib/libstdc++.so.6`
- `/opt/gcc-rtaudio-linux/lib/libgcc_s.so.1`

Licensing status:

- These are usually GPL-licensed with the GCC Runtime Library Exception, which is specifically intended to permit use by non-GPL applications.

Important note:

- I did not find local license files in `/opt/gcc-rtaudio-linux`, so the exact packaging should still be verified if you plan to redistribute that runtime itself.

Source:

- GCC Runtime Library Exception announcement and references:
  - https://gcc.gnu.org/ml/gcc/2009-01/msg00417.html
  - http://www.gnu.org/licenses/gcc-exception.html

### `/opt/btver2/lib/libSDL2-2.0.so.0`

Observed local runtime path:

- `/opt/btver2/lib/libSDL2-2.0.so.0`

Local license evidence:

- `/opt/btver2/share/licenses/sdl2-compat/LICENSE.txt` contains the zlib-style SDL license text.

Important note:

- The exact library loaded is named `libSDL2-2.0.so.0`, but the local license file found was for `sdl2-compat`, not a clearly labeled `SDL2` package. That is good evidence of a permissive SDL-family license in this local tree, but I would still verify the exact origin of the shipped SDL runtime if you redistribute it.

Risk level:

- Medium-low, but verify provenance

## Standard System Libraries

Also observed:

- `libc.so.6`
- `libm.so.6`
- `libpthread.so.0`
- `libdl.so.2`
- `librt.so.1`
- `libresolv.so.2`
- `libz.so.1`
- `libzstd.so.1`
- `libuuid.so.1`
- and other low-level system components

These are standard runtime system libraries. They still have licenses, but they are not usually the main blocker when deciding your project’s outbound license.

## Release Risk Assessment

### Cleanest current story

If you license `RTAudioSeq` itself as `GPL-3.0`, the currently observed transitive GPL runtime libraries are much less of a licensing mismatch.

### Risky current story

If you want:

- `MIT`
- `Apache-2.0`
- `MPL-2.0`
- `LGPL`
- or closed/proprietary distribution

then the current runtime graph is problematic because of:

- `liblash.so.1`
- `libreadline.so.8`

### Best next technical fix

Rebuild or replace `FluidSynth` so your runtime no longer depends on:

- `liblash`
- `libreadline`

Once those disappear from `ldd build/RTAudioSeq`, the remaining licensing picture becomes much easier to manage.

## Recommended Follow-Up Before Distribution

1. Re-run `ldd build/RTAudioSeq` after rebuilding `FluidSynth` without GPL extras.
2. Capture exact license texts for the custom `/opt/qt-5.15.0` and `/opt/gcc-rtaudio-linux` runtimes you intend to ship.
3. Decide your outbound license for `RTAudioSeq`.
4. Add:
   - a top-level `LICENSE`
   - a `THIRD_PARTY_NOTICES.md`
   - copies or references to required LGPL texts and Qt notices if distributing binaries

## Bottom Line

The current codebase can certainly be released as open source.

The key legal question is not whether you can release it. You can.

The key question is whether you want:

- a `GPL` release, which fits the current runtime more comfortably
- or a non-GPL release, which likely requires cleaning up the `FluidSynth`-related runtime dependencies first
