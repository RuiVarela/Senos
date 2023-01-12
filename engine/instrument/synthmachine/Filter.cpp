#include "Filter.hpp"
#include "../../core/Log.hpp"

namespace sns {

    //
    // Moog ladder filter module
    // Ported from soundpipe
    // Original author(s) : Victor Lazzarini, John ffitch (fast tanh), Bob Moog
    //
    struct Moog {
        Moog() {
            Reset();
        }

        void Reset() {
            sample_rate_ = float(SampleRate);
            istor_ = 0.0f;
            res_ = 0.4f;
            freq_ = 1000.0f;

            for (int i = 0; i < 6; i++) {
                delay_[i] = 0.0;
                tanhstg_[i % 3] = 0.0;
            }

            old_freq_ = 0.0f;
            old_res_ = -1.0f;
            old_acr_ = 0.0f;
            old_tune_ = 0.0f;
        }

        float process(float in) {
            float  freq = freq_;
            float  res  = res_;
            float  res4;
            float* delay   = delay_;
            float* tanhstg = tanhstg_;
            float  stg[4];
            float  acr, tune;

            float THERMAL = 0.000025f;

            if(res < 0) {
                res = 0;
            }

            if(old_freq_ != freq || old_res_ != res)
            {
                float f, fc, fc2, fc3, fcr;
                old_freq_ = freq;
                fc        = (freq / sample_rate_);
                f         = 0.5f * fc;
                fc2       = fc * fc;
                fc3       = fc2 * fc2;

                fcr  = 1.8730f * fc3 + 0.4955f * fc2 - 0.6490f * fc + 0.9988f;
                acr  = -3.9364f * fc2 + 1.8409f * fc + 0.9968f;
                tune = (1.0f - expf(-((2 * PI) * f * fcr))) / THERMAL;

                old_res_  = res;
                old_acr_  = acr;
                old_tune_ = tune;
            }
            else
            {
                res  = old_res_;
                acr  = old_acr_;
                tune = old_tune_;
            }

            res4 = 4.0f * res * acr;

            for(int j = 0; j < 2; j++)
            {
                in -= res4 * delay[5];
                delay[0] = stg[0]
                    = delay[0] + tune * (tanh(in * THERMAL) - tanhstg[0]);
                for(int k = 1; k < 4; k++)
                {
                    in     = stg[k - 1];
                    stg[k] = delay[k]
                            + tune
                                * ((tanhstg[k - 1] = tanh(in * THERMAL))
                                    - (k != 3 ? tanhstg[k]
                                                : tanh(delay[k] * THERMAL)));
                    delay[k] = stg[k];
                }
                delay[5] = (stg[3] + delay[4]) * 0.5f;
                delay[4] = stg[3];
            }
            return delay[5];
        }


        float istor_, res_, freq_, delay_[6], tanhstg_[3], old_freq_, old_res_, sample_rate_, old_acr_, old_tune_;
    };

    // from daisydsp
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
        Moog moog;
        StateVariableFilter svf;
    };

    std::string toString(Filter::Kind kind) {
        switch (kind)
        {
        case Filter::Kind::Off:
            return "Off";
        case Filter::Kind::Moog:
            return "Moog";
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
        :m(std::make_shared<PrivateImplementation>()), m_kind(Kind::Off), m_cutoff(1.0f), m_resonance(0.0f)
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

    void Filter::setKind(Kind kind) {
        if (m_kind == kind) return;

        m_force_update = true;
        m_kind = kind;

        if (m_kind == Kind::Off) {

        } else if (m_kind == Kind::Moog) {
            m->moog.Reset();
        }
        else {
            m->svf.Reset();
        }

        setCutoff(m_cutoff);
        setResonance(m_resonance);
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
        } else if (m_kind == Kind::Moog) {
            constexpr float max_frequency = 7000.0f;
            float frequency = easeInQuad(value) * max_frequency;
            m->moog.freq_ = frequency;
        }  else {
            constexpr float max = float(SampleRate) / 3.0f;
            float frequency = easeInQuad(value) * max;
            m->svf.SetFreq(frequency);
        }
    }

    void Filter::setResonance(float value) {
        if (equivalent(m_resonance, value) && !m_force_update) return;
        m_resonance = value;

        if (m_kind == Kind::Off) {
            return;
        } else if (m_kind == Kind::Moog) {
            m->moog.res_ = value * 0.95f; // 0.95 is to avoid audio explosions
        } else {
            m->svf.SetDrive(10.0f);
            m->svf.SetRes(clampTo(value, 0.01f, 0.99f));
        }
    }

    float Filter::next(float input) {
        if (m_kind == Kind::Off) {
            return input;
        } else if (m_kind == Kind::Moog) {
            return m->moog.process(input);
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