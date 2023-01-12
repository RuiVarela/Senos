#pragma once

#include "../../audio/Audio.hpp"

namespace sns {

	class Oscillator {
	public:

        enum class Kind {
            Off,
            Sine,
            Square,
            Triangle,
            Saw,
            Ramp,
            PolyblepSquare,
            PolyblepTriangle,
            PolyblepSaw,
            SmoothNoise,
            WhiteNoise,
            PinkNoise,

            Count
        };

		Oscillator(float frequency = 1.0, Kind kind = Kind::Sine);

        std::string toString() const;

        static std::vector<std::string> kindNames();
        static bool hasDiscontinuities(Kind kind);

        void setKind(Kind kind);
        Kind kind() const;

        bool isOff() const;

        void setFrequency(float frequency);
        float frequency() const;

        bool endOfCycle() const;
        bool endOfRise() const;

        bool isRising() const;
        bool isFalling() const;

        float phase() const;

        uint64_t sampleCount() const;

        float next();
    private:
        float m_phase;
        float m_phase_increment;
        bool m_end_of_cycle;
        bool m_end_of_rise;

        float m_frequency;
        Kind m_kind;

        float m_last_out;

        float m_last_keypoint;
        float m_last_interval;

        uint64_t m_sample_count;

        // pink
        float m_pink_k[7];
        float m_pink_b[7];
        float m_pink;
        void setupPinkNoise();
        float whiteNoise();
	};

    std::string toString(Oscillator::Kind kind);
}