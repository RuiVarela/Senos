#pragma once

#include "Instrument.hpp"

namespace sns
{
	class Dx7 : public BaseInstrument
	{
	public:
		Dx7();
		~Dx7() override;

		static ParametersValues defaultParameters();
		void setValues(ParametersValues const& values) override;

		void setNote(int note, float velocity) override;
		void onMidi(MidiMessage const& message) override;

		float next() override;
		void panic() override;
	private:
		struct PrivateImplementation;
		std::unique_ptr<PrivateImplementation> m;

		void onMidi(uint8_t const* data, int data_size);

		// zero-based
		void programChange(int p);

		void setController(int controller, int value);
		bool setSysex(const uint8_t* data, uint32_t size);
		void setPatch(const uint8_t* patch, uint32_t size);
		void setParam(uint32_t id, char value);

		// Choose a note for a new key-down, returns note number, or -1 if none available.
		void resetVoice(int v);
		void midiNotePressed(int midinote, int velocity);
		void midiNoteReleased(int midinote);
	};

}