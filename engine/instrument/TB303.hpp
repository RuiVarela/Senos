#pragma once

#include "Instrument.hpp"

namespace sns {

	class TB303 : public BaseInstrument {
	public:
		TB303();
		~TB303() override;

		static ParametersValues defaultParameters();
		void setValues(ParametersValues const& values) override;

		void onMidi(MidiMessage const& message) override;
		void setNote(int note, float velocity) override;

		float next() override;
		void panic() override;
	private:
		struct PrivateImplementation;
		std::unique_ptr<PrivateImplementation> m;
		void updateEnvelopModulation();
	};
}