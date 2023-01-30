#pragma once

#include "../engine/core/Lang.hpp"
#include "../engine/instrument/Instrument.hpp"
#include "../engine/audio/Midi.hpp"
#include "../engine/Sequencer.hpp"
#include "../engine/Chainer.hpp"

namespace sns {

	constexpr char LastSessionName[] = "last session";
	constexpr char DefaultSessionName[] = "default";

	//constexpr auto PackExtension = "tar";
	constexpr auto PackExtension = "zip";

	class App;

	struct Configuration {
		int runs = 0;

		int window_width = 1024;
		int window_height = 768;
		bool window_fullscreen = false;

		int audio_buffer_size = 2048 / 4;
		int video_fps = 0;

		Midi::Mapping midi;
		std::string midi_preset;

		std::string project = DefaultSessionName;
		std::string project_folder;
	};

	std::string rootFolder();
	void saveConfiguration(App* app, Configuration const& configuration);
	Configuration loadConfiguration(App* app);
	Configuration migrateApp(App* app, Configuration cfg, int from, int to);

	std::vector<std::string> projectNames();
	void deleteProject(std::string const& name);
	void cloneProject(Configuration const& configuration, std::string const& name);
	void exportProject(Configuration const& configuration, std::string const& filename);

	std::vector<std::string> importableProjects(std::string const& filename);
	bool importProject(std::string const& project, std::string const& filename);

	void backupProjects();
	bool backupFolder(std::string const& source, std::string const& filename);

	//
	// Midi devices presets
	//
	std::vector<std::string> midiPresetNames(Configuration& configuration);
	void loadMidiPreset(Configuration& configuration, std::string const& preset);

	//
	// Instrument presets
	//
	std::vector<std::string> instrumentPresetsNames(Configuration const& configuration, InstrumentId id);
	ParametersValues instrumentPreset(Configuration const& configuration, InstrumentId id, std::string const& name);
	void saveInstrumentPreset(Configuration const& configuration, InstrumentId id, std::string name, ParametersValues const& values);
	void deleteInstrumentPreset(Configuration const& configuration, InstrumentId id, std::string name);

	//
	// Sequences
	//
	std::vector<std::string> sequenceNames(Configuration const& configuration);
	Sequencer::Configuration loadSequence(Configuration const& configuration, std::string name);
	void saveSequence(Configuration const& configuration, std::string const& name, Sequencer::Configuration const& values);
	void deleteSequence(Configuration const& configuration, std::string name);

	//
	// chains
	//
	std::vector<std::string> chainNames(Configuration const& configuration);
	Chainer::Configuration loadChain(Configuration const& configuration, std::string name);
	void saveChain(Configuration const& configuration, std::string const& name, Chainer::Configuration const& values);
	void deleteChain(Configuration const& configuration, std::string name);
}