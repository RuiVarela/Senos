#pragma once

#include "Instrument.hpp"

namespace sns {

	class DrumMachine : public BaseInstrument {
	public:
		DrumMachine();
		~DrumMachine() override;

		struct DMKey {
			int key;
			std::string name;
			std::string alias;
		};

		static std::vector<DMKey> const& tr808();
		static std::vector<DMKey> const& tr909();
		static std::string alias(int note);

		static ParametersValues defaultParameters();
		void setValues(ParametersValues const& values) override;

		void onMidi(MidiMessage const& message) override;
		void setNote(int note, float velocity) override;

		float next() override;
		void panic() override;
	private:
		struct PrivateImplementation;
		std::unique_ptr<PrivateImplementation> m;
	};
}