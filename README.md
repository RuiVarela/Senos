# Senos
Senos is sound exploration tool from a developer point of view.   
It is very lightweight, can be used as a toy or a minimal song composer.

Play Setup | Instruments | Sequencer | Chainer   | Analyser  
:---------:|:-----------:|:---------:|:---------:|:---------:
![Play](https://raw.githubusercontent.com/RuiVarela/Senos/main/docs/00.png) | ![Instruments](https://raw.githubusercontent.com/RuiVarela/Senos/main/docs/01.png) | ![Sequencer](https://raw.githubusercontent.com/RuiVarela/Senos/main/docs/02.png) | ![Chainer](https://raw.githubusercontent.com/RuiVarela/Senos/main/docs/03.png) | ![Analyser](https://raw.githubusercontent.com/RuiVarela/Senos/main/docs/04.png)


## Features
- 4 playable instruments
  - SynthMachine, a minimal synthesizer
  - Dx7 FM synthesizer
  - 303
  - DrumMachine, 808 909
- Step Sequencer
- Chainer (sequencer chaining)
- Software keyboard 
- Midi Support
- Wav Recording
- Projects minimal management
- A couple of [demo songs](https://github.com/RuiVarela/Senos/tree/main/songs)

## Development
```bash
git git@github.com:RuiVarela/Senos.git
cd Senos

mkdir build
cd build

# General
cmake ..
cmake --build .

# To build a Release version on Linux and Mac:
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..
cmake --build . -- -j 8

# Build a release version for mac with arm support
cmake -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_BUILD_TYPE=Release .. 
cmake --build . -- -j 8
lipo -archs Senos.app/Contents/MacOS/Senos

# To build a Release version on Windows with the VisualStudio toolchain:
cmake ..
cmake --build . --config Release
cmake --build . --config MinSizeRel
```

# Credits
- [sokol](https://github.com/floooh/sokol)
- [imgui](https://github.com/ocornut/imgui)
- [imgui-knobs](https://github.com/altschuler/imgui-knobs)
- [tinyformat](https://github.com/c42f/tinyformat)
- [nlohmann json](https://github.com/nlohmann/json)
- [libremidi](https://github.com/jcelerier/libremidi)
- [TinySoundFont](https://github.com/schellingb/TinySoundFont)
- [martin-finke.de](http://martin-finke.de/)
- [musicdsp](https://www.musicdsp.org/)
- [msfa dx7](https://github.com/google/music-synthesizer-for-android)
- [open303](https://github.com/maddanio/open303)
- [microtar](https://github.com/rxi/microtar)
- [miniz-cpp](https://github.com/tfussell/miniz-cpp)
