#include "Configuration.hpp"
#include "Platform.hpp"

#include "../engine/core/Text.hpp"
#include "../engine/core/Log.hpp"

#include "json.hpp"
#include "App.hpp"

#include "../vendor/microtar/microtar.h"
#include "../vendor/miniz/miniz.hpp"

#include <filesystem>
#include <fstream>

constexpr auto ROOT_WINDOW = "window";
constexpr auto SETTINGS_FILENAME = "settings.json";
constexpr auto PROJECTS_FOLDER_NAME = "projects";
constexpr auto BACKUPS_FOLDER_NAME = "backups";

constexpr auto TAG = "Configuration";

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
			object["header"]["version"] = ConfigurationVersion;
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
			return migrateApp(app, configuration, version, ConfigurationVersion);
		
		json data = json::parse(in);
		if (data.empty())
			return migrateApp(app, configuration, version, ConfigurationVersion);

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

		if (version != ConfigurationVersion)
			return migrateApp(app, configuration, version, ConfigurationVersion);
		else 
			return configuration;
	}


	std::vector<std::string> projectNames() {
		std::vector<std::string> names;

		names.push_back(DefaultSessionName);

		std::string path = mergePaths(rootFolder(), PROJECTS_FOLDER_NAME);
		auto contents = getDirectoryContents(path, DirectorySorting::DirectorySortingAlphabetical);
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

}