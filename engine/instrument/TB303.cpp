#include "TB303.hpp"
#include "../core/Log.hpp"

#include "tb303/rosic_Open303.h"

using namespace rosic;

namespace sns {

	constexpr float PITCH_BEND_OCTAVES = 2.0f;

	struct TB303::PrivateImplementation {
		bool do_log;
		float mod_wheel;
		Open303 device;
	};

	TB303::TB303()
		:m(std::make_unique<PrivateImplementation>())
	{
		TAG = "TB303";
		m->do_log = false;

		m->device.setSampleRate(SampleRate);

		panic();

		TB303::setValues(TB303::defaultParameters());

		m->do_log = true;
	}

	void TB303::panic() {
		BaseInstrument::panic();

		m->mod_wheel = 0.0f;
		m->device.allNotesOff();
	}

	TB303::~TB303() {

	}

	void TB303::onMidi(MidiMessage const& message) {
		if (message.parameter == ParameterPitchBend) {

			float value = message.parameter_value * PITCH_BEND_OCTAVES;
			//Log::d(TAG, sfmt("ParameterPitchBend [%.3f]",  value));

			m->device.setPitchBend(value);

		}
		else if (message.parameter == ParameterModulationWheel) {
			m->mod_wheel = message.parameter_value;
			//Log::d(TAG, sfmt("ParameterModulationWheel [%.3f]", m->mod_wheel));
			updateEnvelopModulation();

		}
		else {

			BaseInstrument::onMidi(message);
		}
	}

	void TB303::setNote(int note, float velocity) {
		if (velocity > 0.0f) {
			const int ivelocity = int(velocity * 127.0f);
			m->device.noteOn(note, ivelocity);
		}
		else {
			m->device.noteOn(note, 0);
		}
	}

	ParametersValues TB303::defaultParameters() {
		ParametersValues values;

		values[ParameterTuning] = 0.5f;
		values[ParameterFilterBase + ParameterFilterCutoff] = 0.5f;
		values[ParameterFilterBase + ParameterFilterResonance] = 0.5f;
		values[ParameterEnvBase + ParameterEnvMod] = 0.5f;
		values[ParameterEnvBase + ParameterEnvDecay] = 0.5f;
		values[ParameterAccent] = 0.5f;
		values[ParameterOscBase + ParameterOscKind] = 0.0f;
		values[ParameterVolume] = 0.5f;

		return values;
	}

	void TB303::setValues(ParametersValues const& values) {

		bool needs_to_update_modulation = false;

		for (auto const& [parameter, value] : values) {
			bool found = false;
			float previous = 0.0f;

			for (auto const& p : m_values) {
				if (p.first == parameter) {
					found = true;
					previous = p.second;
				}
			}

			bool changed = !found || (found && !equivalent(previous, value));

			if (!changed)
				continue;

			if (m->do_log)
				Log::d(TAG, sfmt("Updating value [%s] [%.3f] -> [%.3f]", parameterToString(parameter), previous, value));


			if (parameter == ParameterTuning) {
				float tuning = linearToLinear(value, 0.0f, 1.0f, 400.0f, 480.0f);
				m->device.setTuning(tuning);
			}
			else if (parameter == (ParameterFilterBase + ParameterFilterCutoff)) {
				float cutoff = linearToExponential(value, 0.0f, 1.0f, 314.0f, 2394.0f);
				m->device.setCutoff(cutoff);
			}
			else if (parameter == (ParameterFilterBase + ParameterFilterResonance)) {
				float resonance = linearToLinear(value, 0.0f, 1.0f, 0.0f, 100.0f);
				m->device.setResonance(resonance);
			}
			else if (parameter == (ParameterEnvBase + ParameterEnvMod)) {
				needs_to_update_modulation = true;
			}
			else if (parameter == (ParameterEnvBase + ParameterEnvDecay)) {
				float decay = linearToExponential(value, 0.0f, 1.0f, 200.0f, 2000.0f);
				m->device.setDecay(decay);
			}
			else if (parameter == ParameterAccent) {
				float accent = linearToLinear(value, 0.0f, 1.0f, 0.0f, 100.0f);
				m->device.setAccent(accent);
			}
			else if (parameter == ParameterVolume) {
				float volume = linearToLinear(value, 0.0f, 1.0f, -60.0f, 0.0f);
				m->device.setVolume(volume);
			}
			else if (parameter == (ParameterOscBase + ParameterOscKind)) {
				double waveform = double(clampTo(value, 0.0f, 1.0f));
				m->device.setWaveform(waveform);
			}
		}

		BaseInstrument::setValues(values);

		if (needs_to_update_modulation) {
			updateEnvelopModulation();
		}
	}

	float TB303::next() {
		double sample = m->device.getSample();

		return float(sample);
	}

	void TB303::updateEnvelopModulation() {
		float value = m_values[ParameterEnvBase + ParameterEnvMod] + m->mod_wheel;
		value = clampTo(value, 0.0f, 1.0f);

		float env_mod = linearToLinear(value, 0.0f, 1.0f, 0.0f, 100.0f);
		m->device.setEnvMod(env_mod);
	}

}