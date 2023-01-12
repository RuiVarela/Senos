# Senos
Audio exploration 


## Clone:

```bash
> git git@github.com:RuiVarela/Senos.git
> cd Senos
```

## Build:

```bash
> mkdir build
> cd build

> cmake ..
> cmake --build .
```

To build a Release version on Linux and Mac:

```bash
> cmake -DCMAKE_BUILD_TYPE=Release ..
> cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..
> cmake --build . -- -j 8
```

To build a Release version on Windows with the VisualStudio toolchain:

```bash
> cmake ..
> cmake --build . --config MinSizeRel
```

NOTE: on Linux you'll also need to install the 'usual' dev-packages needed for X11+GL development.

## Run:

On Linux and macOS:
```bash
> ./Senos
```

On Windows with the Visual Studio toolchain the exe is in a subdirectory:
```bash
> Debug\Senos.exe
> MinSizeRel\Senos.exe
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
