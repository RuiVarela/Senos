#include "SynthMachine.hpp"
#include "../core/Log.hpp"

namespace sns {
	constexpr int MAX_EMITERS = 8;

	constexpr float DETUNE_OCTAVES = 4.0f;
	constexpr float PITCH_BEND_OCTAVES = 2.0f;
	constexpr float LFO_PITCH_SEMITONES = 2.0f;

	constexpr float MAX_LFO_FREQUENCY = 20.0f;

	constexpr float MAX_PORTAMENTO_TIME = 3000.0f;

	constexpr float VOLUME_RAMP_INCREMENT = 10.0f / float(SampleRate);
	constexpr float FILTER_RAMP_INCREMENT = 3.0f / float(SampleRate);
	constexpr float LFO_AMP_RAMP_INCREMENT = 10.0f / float(SampleRate);
	constexpr float PITCH_BEND_RAMP_INCREMENT = 15.0f / float(SampleRate);

	constexpr float MAX_VOLUME = 1.0f / float(SynthMachineOscCount);

	SynthMachine::SynthMachine()
	{
		TAG = "SynthMachine";
		m_do_log = false;

		for (int i = 0; i != SynthMachineOscCount; ++i) {
			m_osc_volume[i] = MAX_VOLUME;
			m_osc_detune[i] = 1.0f;

			m_lfo_amp[i] = 0.0f;
			m_lfo_pitch[i] = 0.0f;
		}

		m_filter_cutoff = 1.0f;
		m_filter_resonance = 0.0f;

		m_emitter_counter = 0;

		m_mono = false;
		m_portamento_time = 0;
		m_volume = 1.0f;

		panic();

		SynthMachine::setValues(SynthMachine::defaultParameters());
		m_do_log = true;
	}

	SynthMachine::~SynthMachine() {

	}

	ParametersValues SynthMachine::defaultParameters() {
		ParametersValues values;

		for (int i = 0; i != SynthMachineOscCount; ++i) {
			int base = getOscBase(i);
			values[base + ParameterOscKind] = int(Oscillator::Kind::Sine);
			values[base + ParameterOscDetune] = 0.0f;
			values[base + ParameterOscVolume] = 1.0f;

			if (i > 0)
				values[base + ParameterOscKind] = int(Oscillator::Kind::Off);


			base = getLfoBase(i);
			values[base + ParameterLfoKind] = int(Oscillator::Kind::Off);
			values[base + ParameterLfoFrequency] = 0.0f;
			values[base + ParameterLfoAmplitude] = 0.0f;
			values[base + ParameterLfoPitch] = 0.0f;
		}

		{
			int base = getEnvBase(0);
			values[base + ParameterEnvAttack] = 0.0f;
			values[base + ParameterEnvDecay] = 0.0f;
			values[base + ParameterEnvSustain] = 1.0;
			values[base + ParameterEnvRelease] = 0.0f;
		}


		{
			int base = getFilterBase(0);
			values[base + ParameterFilterKind] = int(Filter::Kind::Off);
			values[base + ParameterFilterCutoff] = 1.0f;
			values[base + ParameterFilterResonance] = 0.0f;
		}


		values[ParameterMono] = 0.0f;
		values[ParameterPortamento] = 0.0f;
		values[ParameterVolume] = 1.0f;

		return values;
	}

	void SynthMachine::setValues(ParametersValues const& values) {
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

			if (m_do_log)
				Log::i(TAG, sfmt("Updating value [%s] [%.3f] -> [%.3f]", parameterToString(parameter), previous, value));

			Parameter class_parameter;
			int class_index = 0;


			if (parameter == ParameterMono)
			{
				m_mono = bool(int(value));
				panic();
			}
			else if (parameter == ParameterPortamento)
			{
				m_portamento_time = uint64_t(value * MAX_PORTAMENTO_TIME);
			}
			else if (parameter == ParameterVolume)
			{
				m_volume.changeWithIncrement(value, VOLUME_RAMP_INCREMENT);
			}
			else if (isOscParameter(parameter))
			{
				getOscParameter(parameter, class_parameter, class_index);
				switch (class_parameter)
				{
				case ParameterOscDetune:
					m_osc_detune[class_index].changeWithIncrement(pow(2.0f, value * DETUNE_OCTAVES), PITCH_BEND_RAMP_INCREMENT);
					break;
				case ParameterOscVolume:
					m_osc_volume[class_index].changeWithIncrement(value * MAX_VOLUME, VOLUME_RAMP_INCREMENT);
					break;
				}
			}
			else if (isFilterParameter(parameter))
			{
				getFilterParameter(parameter, class_parameter, class_index);

				switch (class_parameter)
				{
				case ParameterFilterCutoff:
					m_filter_cutoff.changeWithIncrement(value, FILTER_RAMP_INCREMENT);
					break;
				case ParameterFilterResonance:
					m_filter_resonance.changeWithIncrement(value, FILTER_RAMP_INCREMENT);
					break;
				case ParameterFilterKind:
					m_filter.setKind(Filter::Kind(int(value)));
					break;
				}
			}
			else if (isLfoParameter(parameter))
			{
				getLfoParameter(parameter, class_parameter, class_index);

				switch (class_parameter)
				{
				case ParameterLfoAmplitude:
					m_lfo_amp[class_index].changeWithIncrement(value, LFO_AMP_RAMP_INCREMENT);
					break;
				case ParameterLfoPitch:
					m_lfo_pitch[class_index] = value;
					break;
				}

			}

			updateEmittersParameter(parameter, value);
		}

		BaseInstrument::setValues(values);
	}

	void SynthMachine::releaseNote(SynthMachineEmitter& emitter) {
		emitter.on = false;
		emitter.env.release();
	}

	void SynthMachine::setupEmitter(SynthMachineEmitter& emitter, int note) {
		emitter.id = ++m_emitter_counter;
		emitter.note = note;
		emitter.note_frequency = noteFrequency(note);
		emitter.on = true;
		emitter.killed = false;
		emitter.produced = 0;

		for (auto const& [parameter, value] : m_values)
			updateEmittersParameter(parameter, value, true, &emitter);

		emitter.env.trigger();
	}

	void SynthMachine::updateEmittersParameter(Parameter parameter, float value, bool setup, SynthMachineEmitter* filter) {

		for (auto& emitter : m_emiters) {
			Parameter class_parameter;
			int class_index = 0;

			if ((filter != nullptr) && (&emitter != filter)) {
				continue;
			}

			if (isOscParameter(parameter))
			{
				getOscParameter(parameter, class_parameter, class_index);

				switch (class_parameter)
				{
				case ParameterOscKind: {
					if (setup) {
						emitter.osc[class_index].setKind(Oscillator::Kind(int(value)));
					}
					break;
				}
				}
			}
			else if (isEnvParameter(parameter) && setup)
			{
				getEnvParameter(parameter, class_parameter, class_index);

				switch (class_parameter)
				{
				case ParameterEnvAttack:
					emitter.env.setAttack(value);
					break;
				case ParameterEnvDecay:
					emitter.env.setDecay(value);
					break;
				case ParameterEnvSustain:
					emitter.env.setSustain(value);
					break;
				case ParameterEnvRelease:
					emitter.env.setRelease(value);
					break;
				}
			}
			else if (isLfoParameter(parameter))
			{
				getLfoParameter(parameter, class_parameter, class_index);

				switch (class_parameter)
				{
				case ParameterLfoKind:
					if (setup) {
						emitter.lfo[class_index].setKind(Oscillator::Kind(int(value)));
					}
					break;
				case ParameterLfoFrequency:
					emitter.lfo[class_index].setFrequency(value * MAX_LFO_FREQUENCY);
					break;
				}

			}
		}
	}


	float SynthMachine::next() {
		prune();

		for (int i = 0; i != SynthMachineOscCount; ++i) {
			m_osc_detune[i].next();
			m_osc_volume[i].next();
			m_lfo_amp[i].next();
		}

		m_pitch_bend.next();

		m_filter.setCutoff(m_filter_cutoff.next());
		m_filter.setResonance(m_filter_resonance.next());


		float machine_sample = 0.0;
		for (auto& emitter : m_emiters) {

			if (m_mono && m_portamento.changing()) {
				emitter.note_frequency = m_portamento.next();
			}

			float sample = 0.0;

			for (int i = 0; i != SynthMachineOscCount; ++i) {
				if (emitter.osc[i].isOff())
					continue;

				float amplitude = m_osc_volume[i].v();
				float note_frequency = emitter.note_frequency;

				// modulate amplitude and pitch with lfo
				if (!emitter.lfo[i].isOff()) {
					const float lfo_value = emitter.lfo[i].next();

					if (!Oscillator::hasDiscontinuities(emitter.lfo[i].kind())) {
						const float lfo_amp = lfo_value * m_lfo_amp[i].v();
						amplitude -= lfo_amp * amplitude;
					}

					const float lfo_pitch = lfo_value * m_lfo_pitch[i] * LFO_PITCH_SEMITONES;
					note_frequency *= fromSemitone(lfo_pitch);
				}

				float osc_frequency = note_frequency * m_pitch_bend.v() * m_osc_detune[i].v();
				if (osc_frequency != emitter.osc[i].frequency())
					emitter.osc[i].setFrequency(osc_frequency);

				sample += emitter.osc[i].next() * amplitude;
			}


			sample *= emitter.env.next();

			machine_sample += sample;

			emitter.produced++;
		}

		machine_sample = SoftClip(machine_sample);
		machine_sample = m_filter.next(machine_sample);
		machine_sample *= m_volume.next();

		return machine_sample;
	}

	void SynthMachine::onMidi(MidiMessage const& message) {
		if (message.parameter == ParameterPitchBend) {

			float value = pow(2.0f, message.parameter_value * PITCH_BEND_OCTAVES);
			m_pitch_bend.changeWithIncrement(value, PITCH_BEND_RAMP_INCREMENT);

		}
		else {
			MidiMessage copy = message;

			if (message.parameter != ParameterNone) {
				Log::i(TAG, sfmt("onMidi [%s] [%.3f] ", parameterToString(message.parameter), message.parameter_value));

				if (isOscParameter(message.parameter)) {
					Parameter class_parameter;
					int class_index = 0;
					getOscParameter(message.parameter, class_parameter, class_index);
					if (class_parameter == ParameterOscDetune) {
						copy.parameter_value = clampTo(copy.parameter_value * 2.0f - 1.0f, -1.0f, 1.0f);
					}
				}
			}

			BaseInstrument::onMidi(copy);
		}

	}

	void SynthMachine::setNote(int note, float velocity) {
		bool on = velocity > 0.0f;

		if (on && m_mono) {
			constexpr uint64_t not_set = std::numeric_limits<uint64_t>::max();

			uint64_t portamento_id = not_set;

			if (m_portamento_time > 0) {
				for (auto& current : m_emiters) {

					if (!current.killed && current.on) {
						portamento_id = current.id;

						m_portamento.set(current.note_frequency);
						m_portamento.changeWithTime(noteFrequency(note), m_portamento_time);

						current.note = note;
						break;
					}
				}
			}

			// kill all other active ones
			for (auto& current : m_emiters) {
				if (!current.killed && current.id != portamento_id) {
					current.env.kill();
					current.killed = true;
					//Log::d(TAG, sfmt("MonoKilled %d - Total %d", current.id, m_emiters.size()));
				}
			}

			if (portamento_id != not_set)
				return;


			m_portamento.set(0);
		}



		bool found = false;

		for (auto& current : m_emiters) {
			if (current.note != note)
				continue;

			if (current.on && !on) {
				releaseNote(current);
				found = true;
			}
		}

		if (!found && on) {
			m_emiters.emplace_back();
			setupEmitter(m_emiters.back(), note);
			//Log::d(TAG, sfmt("Added Emitter %d - Total %d", m_emiters.back().id, m_emiters.size()));
		}

		//Log::d(TAG, sfmt("Key %d changed %d", note, on));
	}

	void SynthMachine::panic() {
		BaseInstrument::panic();

		for (auto& current : m_emiters) {
			current.env.kill();
			current.killed = true;
		}

		m_portamento = 0.0f;
		m_pitch_bend = 1.0f;

		m_filter.reset();

		internalTrackMidiNotesReset();
	}

	void SynthMachine::prune() {
		uint64_t max = 0;
		size_t already_killed = 0;


		// remove emitter that finished playing the note
		{
			auto current = m_emiters.begin();
			while (current != m_emiters.end()) {
				if (current->env.completed()) {
					//Log::d(TAG, sfmt("Removing Emitter %d - Total %d", current->id, m_emiters.size()));
					current = m_emiters.erase(current);
				}
				else {

					if (current->killed)
						already_killed++;

					if (!current->on && !current->killed && current->produced > max)
						max = current->produced;

					++current;
				}
			}
		}

		// if we have too many emitters start killing them
		if ((m_emiters.size() - already_killed) > MAX_EMITERS)
		{
			for (auto& current : m_emiters) {
				if (!current.on && !current.killed && current.produced == max) {
					current.env.kill();
					current.killed = true;

					//Log::d(TAG, sfmt("Killing Emitter %d - Samples %d", current.id, current.produced));
				}
			}
		}
	}
}