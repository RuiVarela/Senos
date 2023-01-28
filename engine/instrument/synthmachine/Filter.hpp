#pragma once

#include "../../audio/Audio.hpp"

namespace sns {

	class Filter {
	public:

        enum class Kind {
            Off,
            Lowpass,
            Bandpass,
            Highpass,
            Notch,
            Peak,
            Count
        };

		Filter(Kind kind = Kind::Lowpass);

        std::string toString() const;

        static std::vector<std::string> kindNames();

        void setKind(Kind kind);
        Kind kind() const;

        void reset();

        //hz
        void setCutoff(float value);
        float cutoff() const;

        void setResonance(float value);
        float resonance();

        void setDrive(float value);
        float drive();

        float next(float input);
    private:
        struct PrivateImplementation;
        std::shared_ptr<PrivateImplementation> m;

        Kind m_kind;
        float m_cutoff;
        float m_resonance;
        float m_drive;
        bool m_force_update;
	};

    std::string toString(Filter::Kind kind);

}