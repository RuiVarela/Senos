#pragma once

#include "Instrument.hpp"

#include "synthmachine/Oscillator.hpp"
#include "synthmachine/Envelope.hpp"
#include "synthmachine/Value.hpp"
#include "synthmachine/Filter.hpp"

namespace sns {
	constexpr int SynthMachineOscCount = 3;

	struct SynthMachineEmitter {
		uint64_t id = 0;
		int note = 0;
		float note_frequency = 0.0f;
		bool on = false;
		bool killed = false;
		uint64_t produced = 0;

		Oscillator osc[SynthMachineOscCount];
		Oscillator lfo[SynthMachineOscCount];
		Envelope env;
	};

	class SynthMachine : public BaseInstrument {
	public:
		SynthMachine();
		~SynthMachine() override;

		void setValues(ParametersValues const& values) override;

		float next() override;

		void onMidi(MidiMessage const& message) override;
		void setNote(int note, float velocity) override;

		static ParametersValues defaultParameters();

		void panic() override;
	private:
		std::vector<SynthMachineEmitter> m_emiters;
		void prune();

		bool m_do_log;

		uint64_t m_emitter_counter;
		Value m_osc_volume[SynthMachineOscCount];
		Value m_osc_detune[SynthMachineOscCount];
		float m_lfo_pitch[SynthMachineOscCount];
		Value m_lfo_amp[SynthMachineOscCount];

		Filter m_filter;
		Value m_filter_cutoff;
		Value m_filter_resonance;
		Value m_filter_drive;

		bool m_mono;
		Value m_portamento;
		uint64_t m_portamento_time;

		Value m_pitch_bend;
		Value m_volume;

		void setupEmitter(SynthMachineEmitter& emitter, int note);
		void updateEmittersParameter(Parameter parameter, float value, bool setup = false, SynthMachineEmitter* filter = nullptr);


		void releaseNote(SynthMachineEmitter& emitter);
	};

}