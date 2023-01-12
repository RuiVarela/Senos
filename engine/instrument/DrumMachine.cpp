#include "DrumMachine.hpp"
#include "../core/Log.hpp"

#define TML_IMPLEMENTATION
#include "tsf/tml.h"

#define TSF_IMPLEMENTATION
#include "tsf/tsf.h"

namespace drummachine {
	#include "DrumMachine.sf2.cpp"
}

namespace sns {

	constexpr int SAMPLE_PACKET = 64;

	struct DrumMachine::PrivateImplementation {
		bool do_log;
		tsf* tsf;

		std::array<float, SAMPLE_PACKET> produced;
		size_t produced_index;
	};

	std::vector<DrumMachine::DMKey> const& DrumMachine::tr808() {
		static std::vector<DrumMachine::DMKey> data = {
			{ 48, "BaseDrum",	"DB"},
			{ 49, "BaseDrum",	"BD"},
			{ 50, "RimShot",	"RS"},
			{ 51, "SnareDrum",	"SD"},
			{ 53, "SnareDrum",	"SD"},
			{ 52, "handClaP",	"CP"},
			{ 54, "ClsdHihat",	"CH"},
			{ 56, "OpenHihat",	"OH"},
			{ 55, "LowTom",		"LT"},
			{ 58, "HiTom",		"HT"},
			{ 57, "CYmball",	"CY"},
			{ 59, "CowBell",	"CB"},
			{ 60, "MidConga",	"MC"},
			{ 61, "HiConga",	"HC"},
			{ 62, "LowConga",	"LC"},
			{ 63, "MAracas",	"MA"},
			{ 64, "CLaves",		"CL"}
		};

		return data;
	}
	std::vector<DrumMachine::DMKey> const& DrumMachine::tr909() {
		static std::vector<DrumMachine::DMKey> data = {
			{ 72, "BaseDrum",		"BD"},
			{ 74, "BaseDrum",		"BD"},
			{ 73, "RimShot",		"RS"},
			{ 76, "SnareDrum",		"SD"},
			{ 77, "SnareDrum",		"SD"},
			{ 75, "HandClap",		"HC"},
			{ 78, "ClosedHihat",	"CH"},
			{ 80, "HiHat",			"HH"},
			{ 82, "OpenHihat",		"OH"},
			{ 79, "LowTom",			"LT"},
			{ 81, "LowTom",			"LT"},
			{ 83, "MidTom",			"MT"},
			{ 84, "MidTom",			"MT"},
			{ 86, "HiTom",			"HT"},
			{ 85, "Crash",			"CS"},
			{ 87, "Ride",			"RD"},
			{ 88, "CowBell",		"CB"},
			{ 89, "Claves",			"CL"}
		};
		return data;
	}

	static std::map<int, std::string> buildAliasMap() {
		std::map<int, std::string> map;

		for (auto const& [key, name, alias] : DrumMachine::tr808())
			map[key] = alias;

		for (auto const& [key, name, alias] : DrumMachine::tr909())
			map[key] = alias;

		return map;
	}

	std::string DrumMachine::alias(int note) {
		static std::map<int, std::string> map = buildAliasMap();

		auto found = map.find(note);
		if (found != map.end()) {
			return found->second;
		}

		return "";
	}


	DrumMachine::DrumMachine()
		:m(std::make_unique<PrivateImplementation>())
	{
		TAG = "DrumMachine";
		m->do_log = false;
		m->tsf = tsf_load_memory(drummachine::sf2_data, sizeof(drummachine::sf2_data));
		if (m->tsf) {
			tsf_set_output(m->tsf, TSFOutputMode::TSF_MONO, sns::SampleRate, 0);
		}

		panic();

		DrumMachine::setValues(DrumMachine::defaultParameters());
		m->do_log = true;
	}

	void DrumMachine::panic() {
		BaseInstrument::panic();

		// ensure alias mapping is built
		alias(0);

		if (m->tsf == nullptr)
			return;

		tsf_reset(m->tsf);
	}

	DrumMachine::~DrumMachine() {
		if (m->tsf) {
			tsf_close(m->tsf);
			m->tsf = nullptr;
		}
	}

	void DrumMachine::onMidi(MidiMessage const& message) {
		if (message.parameter == ParameterPitchBend) {

			if (m->tsf) {
				int bend = int(message.parameter_value * 8192.0f + 8192.0f);

				Log::d(TAG, sfmt("ParameterPitchBend %d [%.3f]", bend, message.parameter_value));

				tsf_channel_set_pitchwheel(m->tsf, 0, bend);
			}

		}
		else {

			BaseInstrument::onMidi(message);
		}
	}

	void DrumMachine::setNote(int note, float velocity) {
		if (m->tsf == nullptr)
			return;

		if (velocity > 0.0f) {
			tsf_note_on(m->tsf, 0, note, velocity);
		}
		else {
			tsf_note_off(m->tsf, 0, note);
		}
	}

	ParametersValues DrumMachine::defaultParameters() {
		ParametersValues values;

		values[ParameterVolume] = 1.0f;

		return values;
	}

	void DrumMachine::setValues(ParametersValues const& values) {
		if (m->tsf == nullptr)
			return;

		for (auto const& [parameter, value] : values) {
			if (parameter == ParameterVolume) {
				tsf_set_volume(m->tsf, value);
			}
		}

		BaseInstrument::setValues(values);
	}

	float DrumMachine::next() {

		if (m->tsf) {
			if (m->produced_index == SAMPLE_PACKET) {
				tsf_render_float(m->tsf, m->produced.data(), SAMPLE_PACKET, 0);
				m->produced_index = 0;
			}

			if (m->produced_index != SAMPLE_PACKET) {
				float value = m->produced[m->produced_index];
				m->produced[m->produced_index] = 0.0f;
				m->produced_index++;
				return value;
			}
		}

		return 0.0f;
	}

}