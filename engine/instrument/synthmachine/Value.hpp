#pragma once

#include "../../audio/Audio.hpp"

namespace sns
{

    class Value
    {
    public:
        explicit Value(float value = 0.0f);

        void setEasing(EasingMethod method);
		EasingMethod easingMethod();


        void changeWithSamples(float value, uint64_t samples);
        void changeWithTime(float value, uint64_t ms);
        void changeWithIncrement(float value, float increment);

        float v();
        float next();

        void set(float value);
        void operator = (float value);

        bool changing() const;

    private:
        enum class Mode {
            Off,
            Eased,
            Constant
        };

        Mode m_mode;
        float m_current;

        float m_increment;

        EasingMethod m_easing;
        float m_start;
		float m_range;
		int m_samples;
		int m_current_sample;

    };

}