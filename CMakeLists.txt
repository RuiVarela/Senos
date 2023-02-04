cmake_minimum_required(VERSION 3.0)
project(Senos)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 2)
set(VERSION_BUILD 25)

add_compile_definitions(VERSION_MAJOR=${VERSION_MAJOR})
add_compile_definitions(VERSION_MINOR=${VERSION_MINOR})
add_compile_definitions(VERSION_BUILD=${VERSION_BUILD})

#set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

#
#=== LIBRARY: engine
#
set(ENGINE_FILES
    engine/core/Lang.cpp
    engine/core/Lang.hpp	
    engine/core/Worker.cpp
    engine/core/Worker.hpp
    engine/core/Text.cpp
    engine/core/Text.hpp	
    engine/core/Log.cpp
    engine/core/Log.hpp

    engine/audio/Audio.cpp
    engine/audio/Audio.hpp	
    engine/audio/Easing.cpp
    engine/audio/Easing.hpp	
    engine/audio/CircularBuffer.hpp	
    engine/audio/RunningAverage.hpp	
    engine/audio/Wav.cpp
	engine/audio/Wav.hpp
	engine/audio/Recorder.cpp
	engine/audio/Recorder.hpp
    engine/audio/Midi.cpp
	engine/audio/Midi.hpp

	engine/Engine.cpp
	engine/Engine.hpp

    engine/Sequencer.cpp
	engine/Sequencer.hpp
    engine/Chainer.cpp
	engine/Chainer.hpp
	
	engine/instrument/Instrument.hpp
    engine/instrument/Instrument.cpp
    engine/instrument/SynthMachine.hpp	
	engine/instrument/SynthMachine.cpp	
    engine/instrument/synthmachine/Value.hpp
	engine/instrument/synthmachine/Value.cpp
    engine/instrument/synthmachine/Envelope.hpp
	engine/instrument/synthmachine/Envelope.cpp
    engine/instrument/synthmachine/Oscillator.hpp	
	engine/instrument/synthmachine/Oscillator.cpp
    engine/instrument/synthmachine/Filter.hpp	
	engine/instrument/synthmachine/Filter.cpp

    engine/instrument/DrumMachine.hpp	
	engine/instrument/DrumMachine.cpp
	
	engine/instrument/Dx7.hpp	
	engine/instrument/Dx7.cpp	
    engine/instrument/dx7/controllers.h
	engine/instrument/dx7/dx7note.h
	engine/instrument/dx7/dx7note.cc
	engine/instrument/dx7/env.h
	engine/instrument/dx7/env.cc	
	engine/instrument/dx7/exp2.h
	engine/instrument/dx7/exp2.cc	
	engine/instrument/dx7/fm_core.h
	engine/instrument/dx7/fm_core.cc	
	engine/instrument/dx7/fm_op_kernel.h
	engine/instrument/dx7/fm_op_kernel.cc	
	engine/instrument/dx7/freqlut.h
	engine/instrument/dx7/freqlut.cc	
	engine/instrument/dx7/lfo.h
	engine/instrument/dx7/lfo.cc	
	engine/instrument/dx7/module.h
	engine/instrument/dx7/patch.h
	engine/instrument/dx7/patch.cc	
	engine/instrument/dx7/pitchenv.h
	engine/instrument/dx7/pitchenv.cc	
	engine/instrument/dx7/sin.h
	engine/instrument/dx7/sin.cc	
	engine/instrument/dx7/synth.h
	engine/instrument/dx7/tuning.h
	engine/instrument/dx7/tuning.cc
    engine/instrument/dx7/banks/Banks.hpp
	engine/instrument/dx7/banks/Banks.cpp
    engine/instrument/dx7/banks/Factory.cpp
    engine/instrument/dx7/banks/VRC.cpp
    engine/instrument/dx7/banks/EGreyMatter.cpp
    engine/instrument/dx7/banks/Finetales.cpp 

    engine/instrument/TB303.hpp
	engine/instrument/TB303.cpp
    engine/instrument/tb303/GlobalDefinitions.h
    engine/instrument/tb303/GlobalFunctions.h
    engine/instrument/tb303/rosic_AcidPattern.cpp
    engine/instrument/tb303/rosic_AcidPattern.h
    engine/instrument/tb303/rosic_AcidSequencer.cpp
    engine/instrument/tb303/rosic_AcidSequencer.h
    engine/instrument/tb303/rosic_AnalogEnvelope.cpp
    engine/instrument/tb303/rosic_AnalogEnvelope.h
    engine/instrument/tb303/rosic_BiquadFilter.cpp
    engine/instrument/tb303/rosic_BiquadFilter.h
    engine/instrument/tb303/rosic_BlendOscillator.cpp
    engine/instrument/tb303/rosic_BlendOscillator.h
    engine/instrument/tb303/rosic_Complex.cpp
    engine/instrument/tb303/rosic_Complex.h
    engine/instrument/tb303/rosic_DecayEnvelope.cpp
    engine/instrument/tb303/rosic_DecayEnvelope.h
    engine/instrument/tb303/rosic_EllipticQuarterBandFilter.cpp
    engine/instrument/tb303/rosic_EllipticQuarterBandFilter.h
    engine/instrument/tb303/rosic_FourierTransformerRadix2.cpp
    engine/instrument/tb303/rosic_FourierTransformerRadix2.h
    engine/instrument/tb303/rosic_FunctionTemplates.h
    engine/instrument/tb303/rosic_LeakyIntegrator.cpp
    engine/instrument/tb303/rosic_LeakyIntegrator.h
    engine/instrument/tb303/rosic_MidiNoteEvent.cpp
    engine/instrument/tb303/rosic_MidiNoteEvent.h
    engine/instrument/tb303/rosic_MipMappedWaveTable.cpp
    engine/instrument/tb303/rosic_MipMappedWaveTable.h
    engine/instrument/tb303/rosic_OnePoleFilter.cpp
    engine/instrument/tb303/rosic_OnePoleFilter.h
    engine/instrument/tb303/rosic_Open303.cpp
    engine/instrument/tb303/rosic_Open303.h
    engine/instrument/tb303/rosic_RealFunctions.h
    engine/instrument/tb303/rosic_TeeBeeFilter.cpp
    engine/instrument/tb303/rosic_TeeBeeFilter.h

	vendor/json.hpp
	vendor/tinyformat.h

    vendor/tsf/tml.h
    vendor/tsf/tsf.h
)

#== organize in folders for VS
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ENGINE_FILES}) 

add_library(engine STATIC ${ENGINE_FILES})
target_include_directories(engine PUBLIC vendor)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
	set_target_properties(engine PROPERTIES COMPILE_FLAGS "/EHsc")
endif()


#
#=== App
#
set(APP_FILES
    main.cpp
    app/assets/icon.cpp
    app/assets/startup.cpp
    
    app/Configuration.cpp
    app/ConfigurationMigrations.cpp
    app/ConfigurationPresets.cpp
	app/Configuration.hpp
    app/App.cpp
	app/App.hpp
    app/Window.cpp
    app/Window.hpp
	app/SettingsWindow.cpp
    app/SettingsWindow.hpp
    app/AboutWindow.cpp
    app/AboutWindow.hpp
    app/AboutWindow.cpp
    app/Menu.hpp
    app/Platform.hpp
    
    app/tools/KeyboardWindow.cpp
    app/tools/KeyboardWindow.hpp
	app/tools/DebugWindow.cpp
    app/tools/DebugWindow.hpp
    app/tools/SequencerWindow.cpp
    app/tools/SequencerWindow.hpp
    app/tools/ChainerWindow.cpp
    app/tools/ChainerWindow.hpp

    app/instrument/SynthMachineWindow.hpp
	app/instrument/SynthMachineWindow.cpp
    app/instrument/DrumMachineWindow.hpp
	app/instrument/DrumMachineWindow.cpp
    app/instrument/Dx7Window.hpp	
	app/instrument/Dx7Window.cpp
    app/instrument/TB303Window.hpp	
	app/instrument/TB303Window.cpp

    vendor/imgui/imgui.cpp
    vendor/imgui/imgui.h
    vendor/imgui/imgui_widgets.cpp 
    vendor/imgui/imgui_draw.cpp 
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_demo.cpp

    vendor/imgui-knobs/imgui-knobs.h
    vendor/imgui-knobs/imgui-knobs.cpp

    vendor/imgui-fonts/fonts.h
    vendor/imgui-fonts/Inconsolata-Bold.cpp
    vendor/imgui-fonts/Inconsolata-Regular.cpp
    vendor/imgui-fonts/material_design_icons.cpp
    vendor/imgui-fonts/material_design_icons.h
    vendor/imgui-fonts/Roboto-Black.cpp
    vendor/imgui-fonts/Roboto-BlackItalic.cpp
    vendor/imgui-fonts/Roboto-Bold.cpp
    vendor/imgui-fonts/Roboto-BoldItalic.cpp
    vendor/imgui-fonts/Roboto-Italic.cpp
    vendor/imgui-fonts/Roboto-Light.cpp
    vendor/imgui-fonts/Roboto-LightItalic.cpp
    vendor/imgui-fonts/Roboto-Medium.cpp
    vendor/imgui-fonts/Roboto-MediumItalic.cpp
    vendor/imgui-fonts/Roboto-Regular.cpp
    vendor/imgui-fonts/Roboto-Thin.cpp
    vendor/imgui-fonts/Roboto-ThinItalic.cpp

    vendor/sokol/sokol_gfx.h
    vendor/sokol/sokol_app.h
    vendor/sokol/sokol_glue.h
    vendor/sokol/sokol.cpp
    vendor/sokol/util/sokol_imgui.h

    vendor/microtar/microtar.h
    vendor/microtar/microtar.cpp

    vendor/miniz/miniz.hpp
    vendor/miniz/miniz.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(APP_FILES ${APP_FILES} app/Platform_win.cpp)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/app/assets/app.rc.in ${CMAKE_CURRENT_BINARY_DIR}/app.rc @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/app/assets/images/senos.ico ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

    add_executable(Senos WIN32 ${APP_FILES} ${CMAKE_CURRENT_BINARY_DIR}/app.rc)

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Senos)
	set_target_properties(Senos PROPERTIES COMPILE_FLAGS "/EHsc")
         
    target_link_libraries(Senos engine Winmm.lib)

elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(APP_FILES ${APP_FILES} app/Platform_osx.mm)

    set(APP_BUNDLE_IDENTIFIER "pt.demanda.senos")
 
    set(MACOSX_BUNDLE_ICON_FILE senos.icns)
    set(MACOSX_BUNDLE_COPYRIGHT "Rui Varela")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_BUNDLE_IDENTIFIER})
    set(MACOSX_BUNDLE_BUNDLE_NAME "Senos")
    
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${VERSION})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING ${VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION})

    
    set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/app/assets/images/senos.icns")
    set_source_files_properties(${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    add_executable(Senos MACOSX_BUNDLE ${APP_FILES} ${APP_ICON})
    target_compile_options(Senos PRIVATE -x objective-c++)
    target_link_libraries(Senos engine
        "-framework UniformTypeIdentifiers"
        
        "-framework QuartzCore"
        "-framework Cocoa"
        "-framework MetalKit"
        "-framework Metal"
        "-framework AudioToolbox"

        "-framework CoreAudio"
        "-framework CoreMIDI"
        )
else()

    add_executable(Senos ${APP_FILES})
    target_link_libraries(Senos engine X11 Xi Xcursor GL dl m)

    #target_link_libraries(Senos INTERFACE X11 Xi Xcursor GL dl m)
    #target_link_libraries(Senos PUBLIC Threads::Threads)
    # 
endif()

#== organize in folders for VS
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${APP_FILES})



# explicitly strip dead code
if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    target_link_options(Senos PRIVATE LINKER:-dead_strip)
endif()

# this hack removes the xxx-CMakeForceLinker.cxx dummy file
#set_target_properties(Senos PROPERTIES LINKER_LANGUAGE C)
