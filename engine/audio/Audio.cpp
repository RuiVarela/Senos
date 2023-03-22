#include "Audio.hpp"
#include "../core/Text.hpp"

namespace sns {

	std::string timecode(uint64_t milliseconds) {
		uint64_t ms = milliseconds % uint64_t(1000);
		uint64_t seconds = (milliseconds / uint64_t(1000)) % uint64_t(60);
		uint64_t minutes = (milliseconds / uint64_t(60 * 1000)) % uint64_t(60);
		uint64_t hours = (milliseconds / uint64_t(60 * 60 * 1000));
		return sfmt("%02d:%02d:%02d.%03d", hours, minutes, seconds, ms);
	}

	constexpr static float MIN_DB = -30.0f;//-96.f;

	float toDb(float linear) {
		return linear > 0.f ? 20.f * std::log10(linear) : MIN_DB;
	}

	float fromDb(float db) {
		return db <= MIN_DB ? 0.f : std::pow(10.f, db / 20.f);
	}

	float toCent(float factor) {
		return std::log(factor) / log(2.f) * 1200.f;
	}

	float fromCent(float cent) {
		return std::pow(2.f, cent / 1200.f);
	}

	float toSemitone(float factor) {
		return std::log(factor) / log(2.f) * 12.f;
	}

	float fromSemitone(float semitone) {
		return std::pow(2.f, semitone / 12.f);
	}

	float linearToLinear(float in, float in_min, float in_max, float out_min, float out_max) {
		float v = (in - in_min) / (in_max - in_min);
		return out_min + v * (out_max - out_min);
	}

	float linearToExponential(float in, float in_min, float in_max, float out_min, float out_max) {
		float v = (in - in_min) / (in_max - in_min);
		return out_min * exp(v * log(out_max / out_min));
	}


	static std::string noteName(int note, bool octave, std::string const names[12]) {
		if (octave) {
			int note_octave = noteOctave(note);
			return sfmt("%s%d", names[note % 12], note_octave);
		}

		return names[note % 12];
	}

	std::string noteName(int note, bool octave, bool alternate) {
		static const std::string n0[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
		static const std::string n1[12] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };

		return noteName(note, octave, alternate ? n1 : n0);
	}

	int noteIndex(int note, int octave) {
		assert(octave >= -1 && octave < (TotalOctaves - 1));
		return note + (octave + 1) * 12;
	}

	int noteOctave(int note) {
		return (note / 12) - 1;
	}

	std::array<float, TotalNotes> noteFrequencies() {
		std::array<float, TotalNotes> frequencies = {};
		for (size_t note = 0; note != TotalNotes; ++note) {

			const int delta = int(note) - 69;
			const double frequency = 440.0 * fromSemitone(float(delta));

			frequencies[note] = float(frequency);
		}
		return frequencies;
	}

	float noteFrequency(int note) {
		assert(note >= 0);
		assert(note < TotalNotes);

		static std::array<float, TotalNotes> frequencies = noteFrequencies();

		return frequencies[note];
	}


	//
	// Clipping
	//

	float SoftLimit(float x) {
		return x * (27.f + x * x) / (27.f + 9.f * x * x);
	}

	float SoftClip(float x) {
		if (x < -3.0f)
			return -1.0f;
		else if (x > 3.0f)
			return 1.0f;
		else
			return SoftLimit(x);
	}

	float HardClip(float x) {
		return maximum(minimum(x, 1.0f), -1.0f);
	}

}