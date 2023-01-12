# Senos
Senos is sound exploration tool from a developer point of view.   
It is very lightweight, can be used as a toy or a minimal song composer.

Features:
- 4 playable instruments
  - SynthMachine, a minimal synthesizer
  - Dx7 FM synthesizer
  - 303
  - DrumMachine, 808 909
- Step Sequencer
- Chainer (sequencer chaining)
- Software keyboard 
- Midi Support

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
./Senos

# To build a Release version on Windows with the VisualStudio toolchain:
cmake ..
cmake --build . --config MinSizeRel
MinSizeRel\Senos.exe
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
