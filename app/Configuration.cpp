#include "Configuration.hpp"
#include "Platform.hpp"

#include "../engine/core/Text.hpp"
#include "../engine/core/Log.hpp"
#include "../engine/instrument/SynthMachine.hpp"
#include "../engine/instrument/DrumMachine.hpp"
#include "../engine/instrument/Dx7.hpp"
#include "../engine/instrument/TB303.hpp"

#include "json.hpp"
#include "App.hpp"

#include "../vendor/microtar/microtar.h"
#include "../vendor/miniz/miniz.hpp"


#include <filesystem>
#include <fstream>

constexpr auto ROOT_WINDOW = "window";
constexpr auto SETTINGS_FILENAME = "settings.json";
constexpr auto PRESETS_FOLDER_NAME = "presets";
constexpr auto SEQUENCES_FOLDER_NAME = "sequences";
constexpr auto CHAINS_FOLDER_NAME = "chains";
constexpr auto MIDI_FOLDER_NAME = "midi";
constexpr auto PROJECTS_FOLDER_NAME = "projects";
constexpr auto BACKUPS_FOLDER_NAME = "backups";
constexpr int CONFIGURATION_VERSION = 2;


constexpr auto TAG = "Configuration";

const size_t startup_data_size = 14597;
extern unsigned char startup_data[startup_data_size];

namespace sns {
	using json = nlohmann::json;

	std::string rootFolder() {
		return platformLocalFolder("senos");
	}

	void saveConfiguration(App* app, Configuration const& configuration) {

		// save configuration file
		{
			json object;

			object["project"]["name"] = configuration.project;

			// header
			object["header"]["version"] = CONFIGURATION_VERSION;
			object["header"]["runs"] = configuration.runs;

			//settings
			object["settings"]["audio_buffer_size"] = configuration.audio_buffer_size;
			object["settings"]["video_fps"] = configuration.video_fps;

			// midi
			object["settings"]["midi"]["filter_active_instrument"] = configuration.midi.filter_active_instrument;
			object["settings"]["midi"]["preset"] = configuration.midi_preset;
			for (int i = InstrumentStart; i != InstrumentCount; ++i) {
				std::string name = sanitizeName(instrumentToString(i));
				object["settings"]["midi"][name + "_port"] = configuration.midi.instruments[i].port;
				object["settings"]["midi"][name + "_channel"] = configuration.midi.instruments[i].channel;
			}

			// windows
			if (!app->isFullscreen()) {
				object["window_state"][ROOT_WINDOW]["width"] = int(app->width() / app->dpiScale());
				object["window_state"][ROOT_WINDOW]["height"] = int(app->height() / app->dpiScale());
			}
			object["window_state"][ROOT_WINDOW]["fullscreen"] = app->isFullscreen();
			for (auto& window : app->windows()) {
				json state;
				window->saveTo(state);
				object["window_state"][sanitizeName(window->name())] = state;
			}


			std::string path = mergePaths(rootFolder(), SETTINGS_FILENAME);
			makeDirectoryForFile(path);
			writeRawText(path, object.dump(4));
		}
	}

	static void loadMidi(App* app, Configuration& configuration, json settings) {

		if (!settings.contains("midi")) return;

		json midi = settings["midi"];

		if (midi.contains("filter_active_instrument"))
			configuration.midi.filter_active_instrument = midi["filter_active_instrument"].get<bool>();

		for (int i = InstrumentStart; i != InstrumentCount; ++i) {
			std::string name = sanitizeName(instrumentToString(i));

			if (midi.contains(name + "_port"))
				configuration.midi.instruments[i].port = midi[name + "_port"].get<std::string>();

			if (midi.contains(name + "_channel"))
				configuration.midi.instruments[i].channel = midi[name + "_channel"].get<int>();
		}

		if (midi.contains("preset")) {
			configuration.midi_preset = midi["preset"];

			if (!configuration.midi_preset.empty()) {
				loadMidiPreset(configuration, configuration.midi_preset);
			}
		}
	}

	Configuration loadConfiguration(App* app) {
		Configuration configuration;
		configuration.project_folder = mergePaths(rootFolder(), PROJECTS_FOLDER_NAME, configuration.project);

		std::string path = mergePaths(rootFolder(), SETTINGS_FILENAME);
		int version = 0;

		std::ifstream in(path);
		if (!in) 
			return migrateApp(app, configuration, version, CONFIGURATION_VERSION);
		
		json data = json::parse(in);
		if (data.empty())
			return migrateApp(app, configuration, version, CONFIGURATION_VERSION);

		if (data.contains("header")) {
			json header = data["header"];

			if (header.contains("version"))
				version = header["version"].get<int>();

			if (header.contains("runs"))
				configuration.runs = header["runs"].get<int>();
		}

		if (data.contains("project")) {
			json project = data["project"];

			if (project.contains("name")) {
				configuration.project = project["name"].get<std::string>();
				configuration.project_folder = mergePaths(rootFolder(), PROJECTS_FOLDER_NAME, configuration.project);
			}
		}

		if (data.contains("settings")) {
			json settings = data["settings"];

			if (settings.contains("audio_buffer_size"))
				configuration.audio_buffer_size = settings["audio_buffer_size"].get<int>();

			if (settings.contains("video_fps"))
				configuration.video_fps = settings["video_fps"].get<int>();


			loadMidi(app, configuration, settings);
		}

		if (data.contains("window_state")) {
			json window_state = data["window_state"];

			if (window_state.contains(ROOT_WINDOW)) {
				json window = window_state[ROOT_WINDOW];

				if (window.contains("width"))
					configuration.window_width = window["width"].get<int>();

				if (window.contains("height"))
					configuration.window_height = window["height"].get<int>();

				if (window.contains("fullscreen"))
					configuration.window_fullscreen = window["fullscreen"].get<bool>();
			}

			// windows
			if (app) {
				for (auto& window : app->windows()) {
					std::string name = sanitizeName(window->name());
					if (window_state.contains(name)) {
						json state = window_state[name];
						window->loadFrom(state);
					}
				}
			}
		}

		if (app) {
			app->engine().midi().setMapping(configuration.midi);
		}

		if (version != CONFIGURATION_VERSION)
			return migrateApp(app, configuration, version, CONFIGURATION_VERSION);
		else 
			return configuration;
	}


	std::vector<std::string> projectNames() {
		std::vector<std::string> names;

		names.push_back(DefaultSessionName);

		std::string path = mergePaths(rootFolder(), PROJECTS_FOLDER_NAME);
		auto contents = getDirectoryContents(path);
		for (auto current : contents) {
			if (fileType(mergePaths(path, current)) != FileType::FileDirectory)
				continue;

			if (contains(names, current))
				continue;

			if (current.empty())
				continue;

			names.push_back(current);
		}

		return names;
	}

	void deleteProject(std::string const& name) {
		backupProjects();
		std::string path = mergePaths(rootFolder(), PROJECTS_FOLDER_NAME, name);
		sns::deleteFile(path);
	}

	void cloneProject(Configuration const& configuration, std::string const& name) {
		std::string source = configuration.project_folder;
		std::string target = mergePaths(rootFolder(), PROJECTS_FOLDER_NAME, name);
		if (source != target) {
			if (fileType(target) == FileType::FileDirectory)
				backupProjects();

			copyFolder(source, target);
		}
	}

	std::vector<std::string> importableProjects(std::string const& filename) {
		std::vector<std::string> output;

		bool is_zip = endsWith(filename, ".zip");

		auto handler = [&output](std::string const& current) {
			std::string project = getFirstFolder(current);
			if (!contains(output, project))
				output.push_back(project);
		};

		if (is_zip) {
			zip_file zip(filename);
			for (auto& member : zip.infolist())
				handler(member.filename);
		}
		else {
			mtar_t tar;
			mtar_header_t h;
			if (mtar_open(&tar, filename.c_str(), "r") != MTAR_ESUCCESS)
				return output;

			while ((mtar_read_header(&tar, &h)) != MTAR_ENULLRECORD) {
				handler(h.name);
				mtar_next(&tar);
			}

			mtar_close(&tar);
		}

		return output;
	}

	bool importProject(std::string const& project, std::string const& source) {
		deleteProject(project);

		bool is_zip = endsWith(source, ".zip");


		auto valid = [&project](std::string const& filename) -> bool {
			std::string current_project = getFirstFolder(filename);
			if (project != current_project)
				return false;

			return true;
		};

		auto saveBytes = [](std::string const& filename, std::vector<uint8_t> const& data) {
			std::string destination = mergePaths(rootFolder(), PROJECTS_FOLDER_NAME, filename);
			makeDirectoryForFile(destination);
			writeRawBinary(destination, data);
			Log::d(TAG, sfmt("Wrote File [%s] -> [%s]", filename, destination));
		};

		if (is_zip) {

			std::vector<uint8_t> data;
			zip_file zip(source);
			for (auto& member : zip.infolist()) {
				std::string filename(member.filename);
				if (!valid(member.filename))
					continue;

				zip.read(member, data);

				saveBytes(filename, data);
			}

		}
		else {

			std::vector<uint8_t> data;
			mtar_t tar;
			mtar_header_t h;
			if (mtar_open(&tar, source.c_str(), "r") != MTAR_ESUCCESS)
				return false;


			while ((mtar_read_header(&tar, &h)) != MTAR_ENULLRECORD) {
				std::string filename(h.name);
				if (!valid(filename))
					continue;

				mtar_find(&tar, filename.c_str(), &h);
				data.resize(h.size);
				mtar_read_data(&tar, data.data(), h.size);

				saveBytes(filename, data);

				mtar_next(&tar);
			}

			mtar_close(&tar);
		}



		return true;
	}

	void exportProject(Configuration const& configuration, std::string const& filename) {
		backupFolder(configuration.project_folder, filename);
	}

	void backupProjects() {
		std::string output = mergePaths(rootFolder(), BACKUPS_FOLDER_NAME, datetimeMarker() + "." + PackExtension);
		std::string source = mergePaths(rootFolder(), PROJECTS_FOLDER_NAME);

		makeDirectoryForFile(output);
		backupFolder(source, output);
	}

	bool backupFolder(std::string const& source, std::string const& filename) {
		std::string const parent = getFilePath(source);

		Log::d(TAG, sfmt("Packing [%s] -> [%s]", source, filename));

		bool is_zip = endsWith(filename, ".zip");

		std::unique_ptr<zip_file> zip;
		mtar_t tar;

		if (is_zip) {
			zip = std::make_unique<zip_file>();
			if (zip.get() == nullptr)
				return false;
		}
		else {
			// Open archive for writing
			if (mtar_open(&tar, filename.c_str(), "w") != MTAR_ESUCCESS)
				return false;
		}


		std::vector<uint8_t> data;

		for (auto const& entry : std::filesystem::recursive_directory_iterator(source)) {
			if (!entry.is_regular_file()) continue;

			auto const& src_path = entry.path().string();
			auto dst_path = src_path.substr(parent.size());
			trim(dst_path, TRIM_DEFAULT_WHITESPACE "\\/");

			dst_path = convertFileNameToUnixStyle(dst_path);

			//Log::d(TAG, sfmt("packing [%s]", dst_path));
			if (readRawBinary(src_path, data) && !data.empty()) {
				unsigned int size = (unsigned int)data.size();

				if (is_zip) {
					zip->write(dst_path, data);
				}
				else {
					mtar_write_file_header(&tar, dst_path.c_str(), size);
					mtar_write_data(&tar, data.data(), size);
				}
			}
			else {
				Log::e(TAG, sfmt("Failed to pack [%s]", dst_path));
			}
		}

		if (is_zip) {

			zip->save(filename);

			zip.reset();

		}
		else {
			// Finalize -- this needs to be the last thing done before closing
			mtar_finalize(&tar);

			// Close archive
			mtar_close(&tar);
		}


		Log::d(TAG, "Packing done");
		return true;
	}


	enum class StoreKind {
		InstrumentPreset,
		Midi,
		Sequence,
		Chain
	};

	static void storeGatherJsonFolder(std::string const& path, std::vector<std::string>& names) {
		auto contents = getDirectoryContents(path);
		for (auto current : contents) {
			if (getFileExtension(current) != "json")
				continue;

			current = getNameLessExtension(current);

			if (contains(names, current))
				continue;

			if (current.empty())
				continue;

			names.push_back(current);
		}
	}

	static std::string storeFilename(Configuration const& configuration, StoreKind kind, std::string const& name) {
		std::string folder = "not_set";

		if (kind == StoreKind::Midi)
			return mergePaths(rootFolder(), MIDI_FOLDER_NAME, sfmt("%s.json", name));

		if (kind == StoreKind::InstrumentPreset) {
			folder = PRESETS_FOLDER_NAME;
		}
		else if (kind == StoreKind::Chain) {
			folder = CHAINS_FOLDER_NAME;
		}
		else if (kind == StoreKind::Sequence) {
			folder = SEQUENCES_FOLDER_NAME;
		}
		return mergePaths(configuration.project_folder, folder, sfmt("%s.json", name));
	}

	static std::vector<std::string> storeNames(Configuration const& configuration, StoreKind kind, std::string id = "") {
		std::string folder = "not_set";
		std::vector<std::string> names;

		if (kind == StoreKind::Midi) {
			folder = mergePaths(rootFolder(), MIDI_FOLDER_NAME);
		}
		else {
			if (kind == StoreKind::InstrumentPreset)
				folder = mergePaths(configuration.project_folder, PRESETS_FOLDER_NAME, id);
			else if (kind == StoreKind::Chain)
				folder = mergePaths(configuration.project_folder, CHAINS_FOLDER_NAME);
			else if (kind == StoreKind::Sequence)
				folder = mergePaths(configuration.project_folder, SEQUENCES_FOLDER_NAME);

			names.push_back(LastSessionName);
			names.push_back(DefaultSessionName);
		}

		storeGatherJsonFolder(folder, names);
		return names;
	}


	static void storeDelete(Configuration const& configuration, StoreKind kind, std::string name) {
		if (name == DefaultSessionName || name == LastSessionName)
			return;

		std::string path = storeFilename(configuration, kind, name);
		sns::deleteFile(path);
	}

	static void storeSave(Configuration const& configuration, StoreKind kind, std::string name,
		std::function<json()> processor,
		bool sanitize_name = true) {
		if (sanitize_name)
			name = sanitizeName(name, true);

		if (name == DefaultSessionName)
			return;
		std::string path = storeFilename(configuration, kind, name);

		makeDirectoryForFile(path);

		json root = processor();

		writeRawText(path, root.dump(4));
	}





	//
	// Preset Manager
	//

	static std::string presetName(InstrumentId id, std::string name) { return mergePaths(instrumentToString(id), name); }

	std::vector<std::string> instrumentPresetsNames(Configuration const& configuration, InstrumentId id) {
		return storeNames(configuration, StoreKind::InstrumentPreset, instrumentToString(id));
	}
	void deleteInstrumentPreset(Configuration const& configuration, InstrumentId id, std::string name) {
		storeDelete(configuration, StoreKind::InstrumentPreset, presetName(id, name));
	}

	ParametersValues instrumentPreset(Configuration const& configuration, InstrumentId id, std::string const& name) {
		ParametersValues values;

		if (id == InstrumentIdSynthMachine) {
			values = SynthMachine::defaultParameters();
		}
		else if (id == InstrumentIdDrumMachine) {
			values = DrumMachine::defaultParameters();
		}
		else if (id == InstrumentIdDx7) {
			values = Dx7::defaultParameters();
		}
		else if (id == InstrumentIdTB303) {
			values = TB303::defaultParameters();
		}

		if (name == DefaultSessionName) return values;

		std::string path = storeFilename(configuration, StoreKind::InstrumentPreset, presetName(id, name));
		if (!fileExists(path)) return values;

		std::ifstream in(path);
		if (!in) return values;

		json data = json::parse(in);
		if (data.contains("parameters")) {
			json parameters = data["parameters"];

			for (auto const& [key, value] : parameters.items())
			{
				Parameter parameter = parameterFromString(key);
				if (parameter != ParameterNone)
				{
					values[parameter] = value.get<float>();
				}
			}
		}

		return values;
	}

	void saveInstrumentPreset(Configuration const& configuration, InstrumentId id, std::string name, ParametersValues const& values) {
		auto processor = [&values]() -> json {
			json root;
			root["version"] = CONFIGURATION_VERSION;
			for (auto const& current : values) {
				root["parameters"][parameterToString(current.first)] = current.second;
			}
			return root;
		};

		name = sanitizeName(name, true);
		storeSave(configuration, StoreKind::InstrumentPreset, presetName(id, name), processor, false);
	}



	//
	// midi
	//
	std::vector<std::string> midiPresetNames(Configuration& configuration) { return storeNames(configuration, StoreKind::Midi); }

	void loadMidiPreset(Configuration& configuration, std::string const& preset) {

		// cleanup
		for (size_t i = 0; i != configuration.midi.instruments.size(); ++i)
			configuration.midi.instruments[i].cc.clear();

		if (preset.empty())
			return;

		std::string path = storeFilename(configuration, StoreKind::Midi, preset);

		if (!fileExists(path))
			return;

		std::ifstream in(path);
		if (!in)
			return;

		json data = json::parse(in);

		for (size_t id = InstrumentStart; id != configuration.midi.instruments.size(); ++id) {
			std::string instrument_name = instrumentToString(InstrumentId(id));

			if (data.contains(instrument_name)) {
				json instrument = data[instrument_name];
				for (auto const& [key, value] : instrument.items()) {
					Parameter parameter = parameterFromString(key);
					if (parameter != ParameterNone)
						configuration.midi.instruments[id].cc[value.get<int>()] = parameter;
				}
			}
		}
	}


	//
	// sequences
	//
	std::vector<std::string> sequenceNames(Configuration const& configuration) { return storeNames(configuration, StoreKind::Sequence); }
	void deleteSequence(Configuration const& configuration, std::string name) { storeDelete(configuration, StoreKind::Sequence, name); }

	Sequencer::Configuration loadSequence(Configuration const& configuration, std::string name) {
		Sequencer::Configuration cfg;
		if (name == DefaultSessionName) return cfg;

		std::string path = storeFilename(configuration, StoreKind::Sequence, name);
		if (!fileExists(path)) return cfg;

		std::ifstream in(path);
		if (!in) return cfg;


		json data = json::parse(in);
		if (data.contains("duty")) cfg.duty = data["duty"].get<float>();
		if (data.contains("tempo")) cfg.tempo = data["tempo"].get<int>();
		if (data.contains("step_count")) cfg.step_count = data["step_count"].get<int>();

		if (data.contains("selected_instrument")) cfg.ui_selected_instrument = InstrumentId(data["selected_instrument"].get<int>());


		for (int instrument_id = InstrumentStart; instrument_id != cfg.instruments.size(); ++instrument_id) {
			std::string instrument_name = instrumentToString(InstrumentId(instrument_id));

			if (data.contains(instrument_name)) {
				json instument_data = data[instrument_name];

				if (instument_data.contains("muted"))
					cfg.instruments[instrument_id].muted = instument_data["muted"].get<bool>();

				if (instument_data.contains("first_col"))
					cfg.instruments[instrument_id].ui_first_col = instument_data["first_col"].get<int>();

				if (instument_data.contains("first_col"))
					cfg.instruments[instrument_id].ui_first_row = instument_data["first_row"].get<int>();

				if (instument_data.contains("steps")) {
					json steps = instument_data["steps"];
					for (auto const& step_data : steps) {
						int step = step_data.contains("step") ? step_data["step"].get<int>() : 0;
						int note = step_data.contains("note") ? step_data["note"].get<int>() : 0;
						int value = step_data.contains("value") ? step_data["value"].get<int>() : 0;

						cfg.setStepState(instrument_id, step, note, Sequencer::NoteMode(value));
					}
				}
			}
		}

		return cfg;
	}

	void saveSequence(Configuration const& configuration, std::string const& name, Sequencer::Configuration const& cfg) {

		auto processor = [&cfg]() -> json {
			json root;
			root["version"] = CONFIGURATION_VERSION;
			root["duty"] = cfg.duty;
			root["tempo"] = cfg.tempo;
			root["step_count"] = cfg.step_count;

			root["selected_instrument"] = int(cfg.ui_selected_instrument);


			for (int instrument_id = InstrumentStart; instrument_id != cfg.instruments.size(); ++instrument_id) {
				std::string instrument_name = instrumentToString(InstrumentId(instrument_id));

				json array = json::array();

				for (auto const& [key, value] : cfg.instruments[instrument_id].steps) {
					auto [step, note] = key;

					json data;
					data["step"] = step;
					data["note"] = note;
					data["value"] = value;

					array.push_back(data);
				}

				root[instrument_name]["steps"] = array;
				root[instrument_name]["muted"] = cfg.instruments[instrument_id].muted;

				root[instrument_name]["first_col"] = cfg.instruments[instrument_id].ui_first_col;
				root[instrument_name]["first_row"] = cfg.instruments[instrument_id].ui_first_row;
			}

			return root;
		};

		storeSave(configuration, StoreKind::Sequence, name, processor);
	}



	//
	// Chainer
	//
	std::vector<std::string> chainNames(Configuration const& configuration) { return storeNames(configuration, StoreKind::Chain); }
	void deleteChain(Configuration const& configuration, std::string name) { storeDelete(configuration, StoreKind::Chain, name); }

	Chainer::Configuration loadChain(Configuration const& configuration, std::string name) {
		Chainer::Configuration cfg;
		if (name == DefaultSessionName) {
			Chainer::Link link;
			link.runs = 1;
			cfg.chain.resize(5, link);
			return cfg;
		}

		std::string path = storeFilename(configuration, StoreKind::Chain, name);
		if (!fileExists(path)) return cfg;

		std::ifstream in(path);
		if (!in) return cfg;

		json data = json::parse(in);

		if (data.contains("chain")) {
			json chain = data["chain"];
			for (auto const& current : chain) {
				Chainer::Link link;
				link.runs = current["runs"].get<int>();
				link.name = current["name"].get<std::string>();
				cfg.chain.push_back(link);
			}
		}

		return cfg;
	}

	void saveChain(Configuration const& configuration, std::string const& name, Chainer::Configuration const& values) {

		auto processor = [&values]() -> json {
			json root;
			root["version"] = CONFIGURATION_VERSION;

			json array = json::array();
			for (auto const& current : values.chain) {
				json item;
				item["runs"] = current.runs;
				item["name"] = current.name;
				array.push_back(item);
			}
			root["chain"] = array;

			return root;
		};

		storeSave(configuration, StoreKind::Chain, name, processor);
	}

	//
	// data migrations
	//
	void unpackAssetsData(Configuration cfg) {
		std::vector<uint8_t> source(startup_data_size);
		memcpy(source.data(), startup_data, startup_data_size);

		zip_file zip(source);
		std::vector<uint8_t> data;
		for (auto &member : zip.infolist()) {
			std::string filename(member.filename);
			if (member.file_size == 0)
				continue;

			zip.read(member, data);

			std::string destination = mergePaths(rootFolder(), filename);
			makeDirectoryForFile(destination);
			writeRawBinary(destination, data);
			Log::d(TAG, sfmt("Wrote File [%s] -> [%s]", filename, destination));
		}
	}

	Configuration migrateApp(App* app, Configuration cfg, int from, int to) {
		if (app == nullptr)
			return cfg;

		if (from == 0) {
			unpackAssetsData(cfg);
			app->windowLayoutPlay();
		}
			

		return cfg;
	}
}