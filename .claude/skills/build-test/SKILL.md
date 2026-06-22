---
name: build-test
description: Build Sysex77 and run its JUCE unit tests, reporting pass/fail. Use whenever asked to build, compile, rebuild, or verify the Sysex77 app or its tests.
---

# Build & test Sysex77

Two build systems coexist (both target JUCE 8.0.7). Prefer CMake; fall back to Xcode if asked.

## Preferred: CMake (fast, cross-platform)

```bash
cd /Users/sebastienportrait/juce/Sysex77
./build.sh --debug --test
```

`build.sh` configures + builds in `./build`, then runs the unit tests via the `--test` flag.
JUCE is auto-resolved (`JUCE_DIR` → `/Applications/JUCE` → auto-download).

Equivalent manual steps if `build.sh` is unavailable:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
build/Sysex77_artefacts/Debug/Sysex77.app/Contents/MacOS/Sysex77 --test
```

## Alternative: Xcode (the historical Projucer build)

```bash
cd /Users/sebastienportrait/juce/Sysex77/Builds/MacOSX
xcodebuild -project Sysex77.xcodeproj -scheme "Sysex77 - App" -configuration Debug build
./build/Debug/Sysex77.app/Contents/MacOS/Sysex77 --test
```

## What to report

- Build result: surface any `error:` lines, or confirm `BUILD SUCCEEDED` / `Built target Sysex77`.
- Test result: the final line `=== UnitTests: N/N assertions passed, M failure(s) ===`. Any
  failure (M > 0) means the change is NOT verified — report it, don't claim success.

## Notes

- The codebase is a unity build (everything flows through `Source/Main.cpp`). The clang LSP shows
  bogus errors on individual `Source/*.h` files analysed in isolation — ignore LSP noise; trust the
  build result.
- Tests live in `Source/Tests.h` (pure sysex logic in `Source/SysexUtils.h`). Extend them when
  adding pure logic.
