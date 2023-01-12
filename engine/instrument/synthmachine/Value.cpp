#include "Value.hpp"
#include "../../core/Log.hpp"

namespace sns
{
    Value::Value(float value) 
        :m_easing(EasingMethod::Linear)
    {
        set(value);
    }

    float Value::v() { 
        return m_current; 
    }

    void Value::setEasing(EasingMethod method) {
        m_easing = method;
    }

	EasingMethod Value::easingMethod() {
        return m_easing;
    }

    bool Value::changing() const {
        return m_mode != Mode::Off;
    }

    void Value::operator = (float value) { 
        set(value); 
    }

    void Value::changeWithTime(float value, uint64_t ms) { 
        changeWithSamples(value, samplesFromMilliseconds(ms)); 
    }

    void Value::changeWithSamples(float value, uint64_t samples) {
        if (samples <= 0) {
            set(value);
            return;
        }

       m_start = m_current;
       m_range = value - m_start;
       m_samples = (int)samples;
       m_current_sample = 1;
       m_mode = Mode::Eased;
    }

    void Value::set(float value) {
        m_mode = Mode::Off;
        m_current = value;
        m_start = 0.0f;
		m_range = 0.0f;
		m_samples = 0;
		m_current_sample = 0;
        m_increment = 0.0f;
    }

    void Value::changeWithIncrement(float value, float increment) {
        if (equivalent(m_current, value)) {
            set(value);
            return;
        }

        m_mode = Mode::Constant;

        m_start = m_current;
        m_range = value;

        float delta = value - m_current;
        float direction = (delta > 0.0f) ? 1.0f : -1.0f;
        m_increment = direction * absolute(increment);

        //Log::d("Value", sfmt("Start Value changed to %.3f inc %.3f", m_range, m_increment));
    }
    
    float Value::next() {
        if (m_mode == Mode::Eased) {
            float progress = clampTo(float(m_current_sample) / float(m_samples), 0.0f, 1.0f);

            m_current = m_start + m_range * easing(m_easing, progress);

            if (m_current_sample >= m_samples) 
                m_mode = Mode::Off;
            
            m_current_sample++;
        } else if (m_mode == Mode::Constant) {

            // m_range = target value
            float delta = m_range - m_current;
            if (absolute(delta) < absolute(m_increment)) {
                m_current = m_range;
                m_mode = Mode::Off;
                
                //Log::d("Value", sfmt("Done Value changed Done to %.3f inc %.3f", m_current, m_increment));
            } else {
                m_current += m_increment;
            }

        }

        return m_current;
    }

}