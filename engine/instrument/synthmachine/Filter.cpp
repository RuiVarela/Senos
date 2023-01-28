#include "Filter.hpp"
#include "../../core/Log.hpp"

namespace sns {

    // from daisydsp
    //https://www.musicdsp.org/en/latest/Filters/92-state-variable-filter-double-sampled-stable.html
    struct StateVariableFilter {

        StateVariableFilter() {
            Reset();
        }

        void Reset() {
            sr_ = float(SampleRate);
            fc_ = 200.0f;
            res_ = 0.5f;
            drive_ = 0.5f;
            pre_drive_ = 0.5f;
            freq_ = 0.25f;
            damp_ = 0.0f;
            notch_ = 0.0f;
            low_ = 0.0f;
            high_ = 0.0f;
            band_ = 0.0f;
            peak_ = 0.0f;
            input_ = 0.0f;
            out_notch_ = 0.0f;
            out_low_ = 0.0f;
            out_high_ = 0.0f;
            out_peak_ = 0.0f;
            out_band_ = 0.0f;
            fc_max_ = sr_ / 3.f;
        }

        void Process(float in)
        {
            input_ = in;
            // first pass
            notch_ = input_ - damp_ * band_;
            low_ = low_ + freq_ * band_;
            high_ = notch_ - low_;
            band_ = freq_ * high_ + band_ - drive_ * band_ * band_ * band_;
            // take first sample of output
            out_low_ = 0.5f * low_;
            out_high_ = 0.5f * high_;
            out_band_ = 0.5f * band_;
            out_peak_ = 0.5f * (low_ - high_);
            out_notch_ = 0.5f * notch_;
            // second pass
            notch_ = input_ - damp_ * band_;
            low_ = low_ + freq_ * band_;
            high_ = notch_ - low_;
            band_ = freq_ * high_ + band_ - drive_ * band_ * band_ * band_;
            // average second pass outputs
            out_low_ += 0.5f * low_;
            out_high_ += 0.5f * high_;
            out_band_ += 0.5f * band_;
            out_peak_ += 0.5f * (low_ - high_);
            out_notch_ += 0.5f * notch_;
        }

		// sets the frequency of the cutoff frequency.
		// must be between 0.0 and sample_rate / 3
		void SetFreq(float f) {
			fc_ = clampTo(f, 1.0e-6f, fc_max_);
			// Set Internal Frequency for fc_
			freq_ = 2.0f * sinf(PI * minimum(0.25f, fc_ / (sr_ * 2.0f))); // fs*2 because double sampled
            // recalculate damp
			damp_ = minimum(2.0f * (1.0f - powf(res_, 0.25f)), minimum(2.0f, 2.0f / freq_ - freq_ * 0.5f));
		}

        //sets the resonance of the filter.
        //Must be between 0.0 and 1.0 to ensure stability.
        void SetRes(float r) {
            float res = clampTo(r, 0.f, 1.f);
            res_ = res;
            // recalculate damp
            damp_ = minimum(2.0f * (1.0f - powf(res_, 0.25f)), minimum(2.0f, 2.0f / freq_ - freq_ * 0.5f));
            drive_ = pre_drive_ * res_;
        }

        // sets the drive of the filter 
        // affects the response of the resonance of the filter
        void SetDrive(float d) {
            float drv = clampTo(d * 0.1f, 0.f, 1.f);
            pre_drive_ = drv;
            drive_ = pre_drive_ * res_;
        }

        float out_low_, out_high_, out_band_, out_peak_, out_notch_;

        float sr_, fc_, res_, drive_, freq_, damp_;
        float notch_, low_, high_, band_, peak_;
        float input_;
        float pre_drive_, fc_max_;
    };
    

    //
    // Filter Class
    //
    struct Filter::PrivateImplementation {
        StateVariableFilter svf;
    };

    std::string toString(Filter::Kind kind) {
        switch (kind)
        {
        case Filter::Kind::Off:
            return "Off";
        case Filter::Kind::Lowpass:
            return "Lowpass";
        case Filter::Kind::Bandpass:
            return "Bandpass";
        case Filter::Kind::Highpass:
            return "Highpass";
        case Filter::Kind::Notch:
            return "Notch";
        case Filter::Kind::Peak:
            return "Peak";
        case Filter::Kind::Count:
            return "Count";
        default:
            return "NOT SET";
        }
    }

    Filter::Filter(Kind kind)
        :m(std::make_shared<PrivateImplementation>()), m_kind(Kind::Off), m_cutoff(1.0f), m_resonance(0.0f), m_drive(0.0f)
    {
        setKind(kind);
    }

    std::string Filter::toString() const {
        return sfmt("%s@%.2f|%.2f", sns::toString(m_kind), m_cutoff, m_resonance);
    }

    std::vector<std::string> Filter::kindNames() { return enumNames<Kind>(); }

    Filter::Kind Filter::kind() const { return m_kind; }
    float Filter::cutoff() const { return m_cutoff; }
    float Filter::resonance() { return m_resonance; }
    float Filter::drive() { return m_drive; }

    void Filter::setKind(Kind kind) {
        if (m_kind == kind) return;

        m_force_update = true;
        m_kind = kind;

        if (m_kind == Kind::Off) {

        } else {
            m->svf.Reset();
        }

        setCutoff(m_cutoff);
        setResonance(m_resonance);
        setDrive(m_drive);
        m_force_update = false;
    }

    void Filter::reset() {
        Kind kind = m_kind;
        setKind(Kind::Off);
        setKind(kind);
    }

    void Filter::setCutoff(float value) {
        if (equivalent(m_cutoff, value) && !m_force_update) return;
        m_cutoff = value;

        if (m_kind == Kind::Off) {
            return;
        } else {
            m->svf.SetFreq(value);
        }
    }

    void Filter::setResonance(float value) {
        if (equivalent(m_resonance, value) && !m_force_update) return;
        m_resonance = value;

        if (m_kind == Kind::Off) {
            return;
        } else {

            m->svf.SetRes(clampTo(value, 0.0f, 0.999f));
        }
    }

    void Filter::setDrive(float value) {
        if (equivalent(m_drive, value) && !m_force_update) return;
        m_drive = value;

        if (m_kind == Kind::Off) {
            return;
        } else {
            m->svf.SetDrive(value);
        }
    }

    float Filter::next(float input) {
        if (m_kind == Kind::Off) {
            return input;
        } else {
            m->svf.Process(input);

            switch (m_kind) {
                case Kind::Lowpass: return m->svf.out_low_;
                case Kind::Bandpass: return m->svf.out_band_;
                case Kind::Highpass: return m->svf.out_high_;
                case Kind::Notch: return m->svf.out_notch_;
                case Kind::Peak: return m->svf.out_peak_;
                default: return input;
            }
        }

        
    }

}