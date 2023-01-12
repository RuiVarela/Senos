#pragma once

#include "Audio.hpp"
#include "../core/Worker.hpp"
#include "../instrument/Instrument.hpp"

namespace sns {

	class Midi : public Worker {
	public:

		struct InstrummentMapping {
			std::string port;			// MIDI physical port
			int channel;				// Midi protocol channel

			std::map<int, Parameter> cc; // map of midi cc's to instrument parameter
		};

		struct Mapping {
			Mapping();

			std::array<InstrummentMapping, InstrumentCount> instruments;
			bool filter_active_instrument;
		};

		Midi();
		~Midi() override;


		Midi& operator=(Midi const&) = delete;
		Midi(Midi const&) = delete;

		void setMapping(Mapping const& mapping);
		void setActiveInstrument(InstrumentId instrument);

		std::vector<std::string> ports();
		MidiMessages take();

	protected:
		void preWork() override;
		void workStep() override;
		void postWork() override;

	private:
		struct PrivateImplementation;
		std::unique_ptr<PrivateImplementation> m;

		std::string TAG;

		void midiSetup(std::vector<std::string> const& ports);
		void midiTeardown();

		void midiEnumerate();
		void midiDumpInfo();
		void midiReceived(MidiMessage& message);
	};

}