#include "Configuration.hpp"

#include "../engine/core/Log.hpp"
#include "../engine/instrument/SynthMachine.hpp"
#include "../engine/instrument/DrumMachine.hpp"
#include "../engine/instrument/Dx7.hpp"
#include "../engine/instrument/TB303.hpp"

#include "json.hpp"

#include <filesystem>
#include <fstream>

//#include "App.hpp"

constexpr auto PRESETS_FOLDER_NAME = "presets";
constexpr auto SEQUENCES_FOLDER_NAME = "sequences";
constexpr auto CHAINS_FOLDER_NAME = "chains";
constexpr auto MIDI_FOLDER_NAME = "midi";

constexpr auto TAG = "Configuration";

namespace sns {
    using json = nlohmann::json;

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
			root["version"] = ConfigurationVersion;
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
			root["version"] = ConfigurationVersion;
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
			root["version"] = ConfigurationVersion;

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
}

