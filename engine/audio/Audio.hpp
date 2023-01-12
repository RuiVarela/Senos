#pragma once

#include "../core/Lang.hpp"
#include "Easing.hpp"

namespace sns {
    
	//
	// we use a predefine audio configuration
	// Mono, 44100, float (32bit) [-1.0f, 1.0f]
	//
	constexpr uint64_t SampleRate = 44100;	// audio sample rate

	constexpr uint64_t Microseconds = 1000000; // microseconds in a seconds
	constexpr uint64_t Milliseconds = 1000; // milliseconds in a seconds

	constexpr float DefaultPressVelocity = 0.65f;
	constexpr float AccentPressVelocity = 1.0f;


	uint64_t audioMicroseconds(uint64_t samples);
	uint64_t audioMilliseconds(uint64_t samples);
	uint64_t samplesFromMicroseconds(uint64_t microseconds);
	uint64_t samplesFromMilliseconds(uint64_t milliseconds);

	std::string timecode(uint64_t milliseconds);


	float toDb(float linear);
	float fromDb(float db);

	float toCent(float factor);
	float fromCent(float cent);

	float toSemitone(float factor);
	float fromSemitone(float semitone);


	float linearToLinear(float in, float in_min, float in_max, float out_min, float out_max);
	float linearToExponential(float in, float in_min, float in_max, float out_min, float out_max);



	//
	// Octaves and Notes
	//
	static constexpr int TotalOctaves = 11;
	static constexpr int TotalNotes = TotalOctaves * 12;

	std::string noteName(int note, bool octave = false, bool alternate = false);
	int noteIndex(int note, int octave); // note [0 <-> 11], octave [-1 <-> TotalOctaves - 2 ]
	int noteOctave(int note); // note [0 <-> TotalOctaves * 12]


	std::array<float, TotalNotes> noteFrequencies();
	float noteFrequency(int note);

	//
	// Audio Helper
	//
	float SoftLimit(float x); // Soft Limiting function ported extracted from pichenettes/stmlib 
	float SoftClip(float x); // Soft Clipping function extracted from pichenettes/stmlib 
	float HardClip(float x);
}