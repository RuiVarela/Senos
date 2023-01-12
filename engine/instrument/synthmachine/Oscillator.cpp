#include "Oscillator.hpp"
#include "../../core/Log.hpp"

namespace sns {

    static float Polyblep(float phase_inc, float t)
    {
        float dt = phase_inc * TWO_PI_RECIPROCAL;
        if (t < dt)  {
            t /= dt;
            return t + t - t * t - 1.0f;
        }  else if (t > 1.0f - dt) {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        } else {
            return 0.0f;
        }
    }

    std::string toString(Oscillator::Kind kind)
    {
        switch (kind)
        {
        case Oscillator::Kind::Off:
            return "Off";
        case Oscillator::Kind::Sine:
            return "Sine";
        case Oscillator::Kind::Square:
            return "Square";
        case Oscillator::Kind::Triangle:
            return "Triangle";
        case Oscillator::Kind::Saw:
            return "Saw";
        case Oscillator::Kind::Ramp:
            return "Ramp";
        case Oscillator::Kind::PolyblepSquare:
            return "PlbSquare";
        case Oscillator::Kind::PolyblepTriangle:
            return "PlbTriangle";
        case Oscillator::Kind::PolyblepSaw:
            return "PlbSaw";
        case Oscillator::Kind::SmoothNoise:
            return "SmoothNoise";
        case Oscillator::Kind::WhiteNoise:
            return "WhiteNoise";
        case Oscillator::Kind::PinkNoise:
            return "PinkNoise";
        case Oscillator::Kind::Count:
            return "Count";
        default:
            return "NOT SET";
        }
    }

    Oscillator::Oscillator(float frequency, Kind kind) 
        :m_frequency(frequency), m_kind(Kind::Off)
    {
        m_phase = 0.0f;
        m_end_of_cycle = true;
        m_end_of_rise = false;
        m_last_out = 0.0f;
        m_last_keypoint = 0.0f;
        m_last_interval = 0.0f;
        m_sample_count = 0;
        setKind(kind);
        setFrequency(frequency);
    }

    std::string Oscillator::toString() const {
        return sfmt("%s@%.2fhz", sns::toString(m_kind), m_frequency);
    }

    void Oscillator::setKind(Kind kind) {
        if (m_kind == kind) return;

        m_kind = kind;

        if (m_kind == Kind::PinkNoise)
            setupPinkNoise();
    }

    bool Oscillator::isOff() const {
        return m_kind == Kind::Off;
    }
    
    Oscillator::Kind Oscillator::kind() const {
        return m_kind;
    }

    std::vector<std::string> Oscillator::kindNames() {
        return enumNames<Kind>();
    }

    bool Oscillator::hasDiscontinuities(Kind kind) {
        return ((kind == Kind::Square) || 
                (kind == Kind::Saw) || 
                (kind == Kind::Ramp) || 
                (kind == Kind::PolyblepSquare) || 
                (kind == Kind::PolyblepSaw) || 
                (kind == Kind::WhiteNoise) || 
                (kind == Kind::PinkNoise));
        return false;
    }


    void Oscillator::setFrequency(float frequency) {
        m_frequency = frequency;
        m_phase_increment = (TWO_PI * m_frequency) / float(sns::SampleRate);
    }
    
    float Oscillator::frequency() const {
        return m_frequency;
    }

    bool Oscillator::endOfCycle() const { 
        return m_end_of_cycle;  
    }

    bool Oscillator::endOfRise() const {
        return m_end_of_rise;
    }

    bool Oscillator::isRising() const {
        return m_phase < PI;
    }

    bool Oscillator::isFalling() const {
        return m_phase >= PI;
    }

    float Oscillator::phase() const {
        return m_phase;
    }

    uint64_t Oscillator::sampleCount() const {
        return m_sample_count;
    }

    float Oscillator::whiteNoise() {
        constexpr int q = 15;
        constexpr float c1 = (1 << q) - 1;
        constexpr float c2 = ((int)(c1 / 3)) + 1;
        constexpr float c3 = 1.f / c1;

        float random = uniformRandom();
        return (2.f * ((random * c2) + (random * c2) + (random * c2)) - 3.f * (c2 - 1.f)) * c3;
    }

    void Oscillator::setupPinkNoise() {
        constexpr static float f[7] = {8227.219f, 8227.219f, 6388.570f, 3302.754f, 479.412f, 151.070f, 54.264f};

        for (unsigned int i = 0; i < 7; ++i) {
            m_pink_k[i] = exp(-2.0f * PI * f[i] / float(SampleRate));
            m_pink_b[i] = 0.0f;
        }
        m_pink = 0.0f;
    }

    float Oscillator::next() {
        float output = 0.0f;

        if (m_kind == Kind::Sine) {

            output = sin(m_phase);

        }  else if (m_kind == Kind::Square) {

            output = (m_phase < PI) ? 1.0f : -1.0f;

        } else if (m_kind == Kind::Triangle) {

            float t = -1.0f + (2.0f * m_phase * TWO_PI_RECIPROCAL);
            output = 2.0f * (fabsf(t) - 0.5f);

        } else if (m_kind == Kind::Saw) {

            float t = (2.0f * m_phase * TWO_PI_RECIPROCAL);
            output = -1.0f * (t - 1.0f);

        } else if (m_kind == Kind::Ramp) {

            float t = (2.0f * m_phase * TWO_PI_RECIPROCAL);
            output = t - 1.0f;

        } else if (m_kind == Kind::PolyblepSquare) {

            float t = m_phase * TWO_PI_RECIPROCAL;
            output = m_phase < PI ? 1.0f : -1.0f;
            output += Polyblep(m_phase_increment, t);
            output -= Polyblep(m_phase_increment, fmodf(t + 0.5f, 1.0f));
            output *= 0.707f;

        }  else if (m_kind == Kind::PolyblepSaw) {

            float t = m_phase * TWO_PI_RECIPROCAL;
            output = (2.0f * t) - 1.0f;
            output -= Polyblep(m_phase_increment, t);
            output *= -1.0f;

        } else if (m_kind == Kind::PolyblepTriangle) {

            float t = m_phase * TWO_PI_RECIPROCAL;
            output = m_phase < PI ? 1.0f : -1.0f;
            output += Polyblep(m_phase_increment, t);
            output -= Polyblep(m_phase_increment, fmodf(t + 0.5f, 1.0f));
            // Leaky Integrator:
            // y[n] = A + x[n] + (1 - A) * y[n-1]
            output = m_phase_increment * output + (1.0f - m_phase_increment) * m_last_out;
            m_last_out = output;

        } else if (m_kind == Kind::SmoothNoise) {

            float t = m_phase * TWO_PI_RECIPROCAL;
            if (m_end_of_cycle) {
                m_last_keypoint += m_last_interval;
                float random = uniformRandom();
                m_last_interval = (random * 2.0f - 1.0f) - m_last_keypoint;
            }

            float factor = t * t * (3.0f - 2.0f * t);
            output = m_last_keypoint + factor * m_last_interval;

        } else if (m_kind == Kind::WhiteNoise) {

            output = whiteNoise();

        } else if (m_kind == Kind::PinkNoise) {

            float white = whiteNoise();
            for (unsigned int i = 0; i < 7; ++i) 
                m_pink_b[i] = m_pink_k[i] * (white + m_pink_b[i]);

            m_pink = 0.05f * (m_pink_b[0] + m_pink_b[1] + m_pink_b[2] + m_pink_b[3] + m_pink_b[4] + m_pink_b[5] + white - m_pink_b[6]);

            output = m_pink;

        } else if (m_kind == Kind::Off) {

            output = 0.0f;

        } else {
            Log::i("Oscillator", sfmt("Kind Not Implemented"));
        }

        bool was_rising = isRising();
        m_phase += m_phase_increment;

        if (m_phase > TWO_PI) {
            m_phase -= TWO_PI;
            m_end_of_cycle = true;
        } else {
            m_end_of_cycle = false;
        }

        bool is_falling = isFalling();

        m_end_of_rise = was_rising && is_falling;
        m_sample_count++;


        return output;
    }

}