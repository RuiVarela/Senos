#include "Instrument.hpp"
#include "../../engine/core/Text.hpp"

namespace sns {

	bool isParameterOfClass(Parameter const& p, int base, int class_params, int class_count) {
		return (p > base) && (p < (base + class_count * class_params));
	}

	Parameter classBase(int index, int base, int class_params, int class_count) {
		Parameter output = base + index * class_params;
		assert(output >= base && output < (base + class_count * class_params));
		return output;
	}

	void classParameter(Parameter const& p, Parameter& class_parameter, int& class_index, int base, int class_params, int class_count) {
		class_index = -1;

		if (isParameterOfClass(p, base, class_params, class_count)) {
			int distance = p - base;
			class_index = distance / class_params;

			class_parameter = distance - (class_index * class_params);
		}
	}

	std::string instrumentToString(InstrumentId id) {

		switch (id) {
		case InstrumentIdSynthMachine: return "SynthMachine";
		case InstrumentIdDrumMachine: return "DrumMachine";
		case InstrumentIdDx7: return "Dx7";
		case InstrumentIdTB303: return "TB-303";
		}

		return sfmt("[Unknown Instrument %d]", id);
	}

	InstrumentId instrumentFromString(std::string const& name) {

		if (name == "SynthMachine") {
			return InstrumentIdSynthMachine;
		}
		else if (name == "DrumMachine") {
			return InstrumentIdDrumMachine;
		}
		else if (name == "Dx7") {
			return InstrumentIdDx7;
		}
		else if (name == "TB-303") {
			return InstrumentIdTB303;
		}

		return InstrumentIdNone;
	}

	struct Mapper {
		std::map<Parameter, std::string> parameterToName;
		std::map<std::string, Parameter> nameToParameter;

		Mapper() {
			setupParameters();
		}

	private:
		void setupParameters() {
			for (int parameter = 0; parameter != ParameterCount; ++parameter) {
				std::string name = parameterName(parameter);
				if (!name.empty()) {
					parameterToName[parameter] = name;
					nameToParameter[name] = parameter;
				}
			}
		}

		std::string parameterName(Parameter const& parameter) {
			Parameter class_parameter;
			int class_index = 0;

			if (isOscParameter(parameter)) {

				getOscParameter(parameter, class_parameter, class_index);
				switch (class_parameter) {
				case ParameterOscKind: return sfmt("OscKind_%d", class_index);
				case ParameterOscDetune: return sfmt("OscDetune_%d", class_index);
				case ParameterOscVolume: return sfmt("OscVolume_%d", class_index);
				}

			}
			else if (isEnvParameter(parameter)) {

				getEnvParameter(parameter, class_parameter, class_index);
				switch (class_parameter) {
				case ParameterEnvAttack: return sfmt("EnvAttack_%d", class_index);
				case ParameterEnvDecay: return sfmt("EnvDecay_%d", class_index);
				case ParameterEnvSustain: return sfmt("EnvSustain_%d", class_index);
				case ParameterEnvRelease: return sfmt("EnvRelease_%d", class_index);
				case ParameterEnvMod: return sfmt("EnvMod_%d", class_index);

				}

			}
			else if (isFilterParameter(parameter)) {

				getFilterParameter(parameter, class_parameter, class_index);
				switch (class_parameter) {
				case ParameterFilterKind: return sfmt("FilterKind_%d", class_index);
				case ParameterFilterCutoff: return sfmt("FilterCutoff_%d", class_index);
				case ParameterFilterResonance: return sfmt("FilterResonance_%d", class_index);
				}

			}
			else if (isLfoParameter(parameter)) {

				getLfoParameter(parameter, class_parameter, class_index);
				switch (class_parameter) {
				case ParameterLfoKind: return sfmt("LfoKind_%d", class_index);
				case ParameterLfoFrequency: return sfmt("LfoFrequency_%d", class_index);
				case ParameterLfoAmplitude: return sfmt("LfoAmplitude_%d", class_index);
				case ParameterLfoPitch: return sfmt("LfoPitch_%d", class_index);
				}
			}
			else {
				switch (parameter) {
				case ParameterGroup: return "Group";
				case ParameterBank: return "Bank";
				case ParameterPatch: return "Patch";

				case ParameterModulationWheel: return "ModulationWheel";
				case ParameterModulationWheelRange: return "ModulationWheelRange";
				case ParameterModulationWheelPitch: return "ModulationWheelPitch";
				case ParameterModulationWheelAmp: return "ModulationWheelAmp";
				case ParameterModulationWheelEnv: return "ModulationWheelEnv";

				case ParameterAftertouchRange: return "AftertouchRange";
				case ParameterAftertouchPitch: return "AftertouchPitch";
				case ParameterAftertouchAmp: return "AftertouchAmp";
				case ParameterAftertouchEnv: return "AftertouchEnv";

				case ParameterPitchBend: return "PitchBend";
				case ParameterPitchBendStep: return "PitchBendStep";
				case ParameterPitchBendUp: return "PitchBendUp";
				case ParameterPitchBendDown: return "PitchBendDown";

				case ParameterMono: return "Mono";
				case ParameterPortamento: return "Portamento";

				case ParameterTuning: return "Tuning";
				case ParameterAccent: return "Accent";
				case ParameterVolume: return "Volume";


				default: break;
				}
			}

			return "";
		}
	};

	static Mapper& getMapper() {
		static Mapper singleton;
		return singleton;
	}


	std::string parameterToString(Parameter const& parameter) {
		auto found = getMapper().parameterToName.find(parameter);

		if (found == getMapper().parameterToName.end())
			return sfmt("[Unknown Parameter %d]", parameter);

		return found->second;
	}

	Parameter parameterFromString(std::string const& name) {
		auto found = getMapper().nameToParameter.find(name);

		if (found == getMapper().nameToParameter.end())
			return ParameterNone;

		return found->second;
	}

	BaseInstrument::BaseInstrument()
		:TAG("BaseInstrument"),
		m_midi_updated_values(false),
		m_midi_updated_note_pressed(false)
	{
	}

	float BaseInstrument::next() {
		return 0.0f;
	}

	void BaseInstrument::onMidi(MidiMessage const& message) {
		if (message.parameter != ParameterNone) {
			auto values = m_values;
			values[message.parameter] = message.parameter_value;
			setValues(values);
			m_midi_updated_values = true;
			return;
		}

		size_t data_size = message.bytes.size();
		unsigned char const* data = message.bytes.data();
		uint8_t cmd = data[0];
		uint8_t cmd_type = cmd & 0xf0;

		if (cmd_type == 0x80 || (cmd_type == 0x90 && data[2] == 0)) {
			// note off
			if (data_size < 3) return;
			setNote(data[1], 0.0f);
			internalTrackMidiNotePressed(data[1], false);
		}
		else if (cmd_type == 0x90) {
			// note on
			if (data_size < 3) return;

			float velocity = float(data[2]) / 127.0f;
			setNote(data[1], velocity);
			internalTrackMidiNotePressed(data[1], true);
		}
	}

	std::set<int> BaseInstrument::getNotesPressedOnMidiController() const {
		return m_midi_note_pressed;
	}

	void BaseInstrument::internalTrackMidiNotesReset() {
		m_midi_note_pressed.clear();
		m_midi_updated_note_pressed = true;
	}

	void BaseInstrument::internalTrackMidiNotePressed(int note, bool on) {
		if (on) {
			m_midi_note_pressed.insert(note);
		}
		else {
			m_midi_note_pressed.erase(note);
		}
		m_midi_updated_note_pressed = true;
	}

	void BaseInstrument::takeMidiControllerUpdates(bool& values, bool& notes) {
		values = m_midi_updated_values;
		notes = m_midi_updated_note_pressed;

		m_midi_updated_values = false;
		m_midi_updated_note_pressed = false;
	}

	void BaseInstrument::setNote(int note, float velocity) {

	}

	void BaseInstrument::setValues(ParametersValues const& values) {
		m_values = values;
	}

	ParametersValues BaseInstrument::getValues() const {
		return m_values;
	}

	void BaseInstrument::panic() {

	}
}