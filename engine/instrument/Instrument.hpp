#pragma once

#include "../audio/Audio.hpp"

namespace sns {

	using InstrumentId = int;
	using Parameter = int;
	using ParametersValues = std::map<Parameter, float>;


	struct MidiMessage {
		InstrumentId instrument;
		std::vector<unsigned char> bytes;
		double timestamp;

		std::string port;
		Parameter parameter;
		float parameter_value;
	};
	using MidiMessages = std::vector<MidiMessage>;

	class BaseInstrument {
	public:
		BaseInstrument();
		virtual ~BaseInstrument() = default;

		BaseInstrument& operator=(BaseInstrument const&) = delete;
		BaseInstrument(BaseInstrument const&) = delete;

		virtual float next();

		virtual void setNote(int note, float velocity);
		virtual void onMidi(MidiMessage const& message);

		virtual void setValues(ParametersValues const& values);
		ParametersValues getValues() const;

		void takeMidiControllerUpdates(bool& values, bool& notes);
		std::set<int> getNotesPressedOnMidiController() const;

		virtual void panic();
	protected:
		std::string TAG;
		ParametersValues m_values;


		// things that were changed from a midi controller
		bool m_midi_updated_values;

		void internalTrackMidiNotePressed(int note, bool on);
		void internalTrackMidiNotesReset();
		std::set<int> m_midi_note_pressed;
		bool m_midi_updated_note_pressed;
	};

	//
	// Instruments
	//
	constexpr InstrumentId InstrumentIdNone = 0;

	constexpr InstrumentId InstrumentIdSynthMachine = 1;
	constexpr InstrumentId InstrumentIdDrumMachine = 2;
	constexpr InstrumentId InstrumentIdDx7 = 3;
	constexpr InstrumentId InstrumentIdTB303 = 4;

	constexpr InstrumentId InstrumentCount = 5;
	constexpr InstrumentId InstrumentStart = InstrumentIdSynthMachine;

	std::string instrumentToString(InstrumentId id);
	InstrumentId instrumentFromString(std::string const& name);

	//
	// Generic Parameters
	//
	constexpr Parameter ParameterNone = 0;

	constexpr Parameter ParameterGroup = 1;
	constexpr Parameter ParameterBank = 2;
	constexpr Parameter ParameterPatch = 3;
	constexpr Parameter ParameterModulationWheel = 4;
	constexpr Parameter ParameterModulationWheelRange = 5;
	constexpr Parameter ParameterModulationWheelPitch = 6;
	constexpr Parameter ParameterModulationWheelAmp = 7;
	constexpr Parameter ParameterModulationWheelEnv = 8;
	constexpr Parameter ParameterAftertouchRange = 9;
	constexpr Parameter ParameterAftertouchPitch = 10;
	constexpr Parameter ParameterAftertouchAmp = 11;
	constexpr Parameter ParameterAftertouchEnv = 12;
	constexpr Parameter ParameterPitchBend = 13;
	constexpr Parameter ParameterPitchBendStep = 14;
	constexpr Parameter ParameterPitchBendUp = 15;
	constexpr Parameter ParameterPitchBendDown = 16;
	constexpr Parameter ParameterMono = 17;
	constexpr Parameter ParameterPortamento = 18;
	constexpr Parameter ParameterTuning = 19;
	constexpr Parameter ParameterAccent = 21;
	constexpr Parameter ParameterVolume = 22;


	// oscillators
	constexpr Parameter ParameterOscParameters = 100;
	constexpr Parameter ParameterOscCount = 10;

	constexpr Parameter ParameterOscBase = 1000;
	constexpr Parameter ParameterOscKind = 1;
	constexpr Parameter ParameterOscDetune = 2;
	constexpr Parameter ParameterOscVolume = 3;


	// low frequency oscillators
	constexpr Parameter ParameterLfoParameters = 100;
	constexpr Parameter ParameterLfoCount = 10;

	constexpr Parameter ParameterLfoBase = ParameterOscBase + ParameterOscCount * ParameterOscParameters;
	constexpr Parameter ParameterLfoKind = 1;
	constexpr Parameter ParameterLfoFrequency = 2;
	constexpr Parameter ParameterLfoAmplitude = 3;
	constexpr Parameter ParameterLfoPitch = 4;


	// envelopes
	constexpr Parameter ParameterEnvParameters = 100;
	constexpr Parameter ParameterEnvCount = 10;

	constexpr Parameter ParameterEnvBase = ParameterLfoBase + ParameterLfoCount * ParameterLfoParameters;
	constexpr Parameter ParameterEnvAttack = 1;
	constexpr Parameter ParameterEnvDecay = 2;
	constexpr Parameter ParameterEnvSustain = 3;
	constexpr Parameter ParameterEnvRelease = 4;
	constexpr Parameter ParameterEnvMod = 5;


	// filters
	constexpr Parameter ParameterFilterParameters = 100;
	constexpr Parameter ParameterFilterCount = 10;

	constexpr Parameter ParameterFilterBase = ParameterEnvBase + ParameterEnvCount * ParameterEnvParameters;
	constexpr Parameter ParameterFilterKind = 1;
	constexpr Parameter ParameterFilterCutoff = 2;
	constexpr Parameter ParameterFilterResonance = 3;
	constexpr Parameter ParameterFilterDrive = 4;
	constexpr Parameter ParameterFilterTrack = 5;


	constexpr Parameter ParameterCount = ParameterFilterBase + ParameterFilterCount * ParameterFilterParameters;

	std::string parameterToString(Parameter const& parameter);
	Parameter parameterFromString(std::string const& name);

	//
	// Parameter helpers
	//
	bool isParameterOfClass(Parameter const& p, int base, int class_params, int class_count);
	Parameter classBase(int index, int base, int class_params, int class_count);
	void classParameter(Parameter const& p, Parameter& class_parameter, int& class_index, int base, int class_params, int class_count);

#define PARAMETER_CLASS_GETTERS(name) \
	inline bool is##name##Parameter(Parameter const& p) { return isParameterOfClass(p, Parameter##name##Base, Parameter##name##Parameters, Parameter##name##Count); } \
	inline void get##name##Parameter(Parameter const& p, Parameter& parameter, int& index) { return classParameter(p, parameter, index, Parameter##name##Base, Parameter##name##Parameters, Parameter##name##Count); } \
	inline Parameter get##name##Base(int index) { return classBase(index, Parameter##name##Base, Parameter##name##Parameters, Parameter##name##Count); } \

	PARAMETER_CLASS_GETTERS(Osc);
	PARAMETER_CLASS_GETTERS(Lfo);
	PARAMETER_CLASS_GETTERS(Env);
	PARAMETER_CLASS_GETTERS(Filter);
}