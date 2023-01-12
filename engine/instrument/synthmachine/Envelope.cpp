#include "Envelope.hpp"

#include "../../core/Log.hpp"

namespace sns {
	constexpr float HYSTERESIS = 0.0001f; // -80dB
	constexpr float MIN_TIME_MS = 10.0f;

	////////////////////////////////////////////////////////////////////////////
	// fast exp Taylor series approximations (from http://www.musicdsp.org/)
	////////////////////////////////////////////////////////////////////////////
	constexpr float fast_exp3(float x) {
		return (6 + x * (6 + x * (3 + x))) * 0.16666666f;
	}

	constexpr float calculateRate(float factor, float max_time) {
		const float ms = maximum(factor * max_time, MIN_TIME_MS);
		const float samples = (ms / 1000.0f) * float(SampleRate);
		return fast_exp3(-2.0f / samples);
	}


	Envelope::Envelope()
		:m_stage(Stage::NotStarted),
		m_current(0.0f),
		m_attack_rate(0.0f),
		m_decay_rate(0.0f),
		m_sustain(1.0f),
		m_release_rate(0.0f),
		m_off_level(0.0f),
		m_level(0.0f),
		m_kill_rate(calculateRate(0, MIN_TIME_MS))
	{
	}

	void Envelope::setAttack(float a) { m_attack_rate = calculateRate(a, 2000.0f); }
	void Envelope::setDecay(float d) { m_decay_rate = calculateRate(d, 2000.0f); }
	void Envelope::setSustain(float s) { m_sustain = s;  }
	void Envelope::setRelease(float r) { m_release_rate = calculateRate(r, 2000.0f); }

	void Envelope::trigger(float level) {
		if (m_current <= level) {
			m_level = level;
			m_stage = Stage::Attack;
		}
	}

	void Envelope::release() {
		if (m_stage == Stage::NotStarted) 
			m_stage = Stage::Off;
		 else if (m_stage < Stage::Release) 
			m_stage = Stage::Release;
	}

	void Envelope::kill() {
		if ((m_stage == Stage::NotStarted) || (m_stage == Stage::Off))
			m_stage = Stage::Off;
		else
			m_stage = Stage::Kill;
	}

	bool Envelope::completed() {
		return (m_stage == Stage::Off);
	}

	void Envelope::updateAttack() {
		m_current = 1.6f + m_attack_rate * (m_current - 1.6f);
		if (m_current > m_level) {
			m_current = m_level;

			m_stage = Stage::Decay;
			if (equivalent(1.0f, m_sustain))
				m_stage = Stage::Sustain;
		}
	}

	void Envelope::updateDecay() {
		auto level = m_level * m_sustain;
		m_current = level + m_decay_rate * (m_current - level);
		if (m_current < (level + HYSTERESIS)) {
			m_current = level;
			m_stage = Stage::Sustain;
		}
	}

	void Envelope::updateRelease(float rate) {
		m_current = m_off_level + rate * (m_current - m_off_level);
		if (m_current < (m_off_level + HYSTERESIS)) {
			m_current = m_off_level;
			m_stage = Stage::Off;
		}
	}

	float Envelope::next() {

		switch (m_stage) {
			case Stage::Attack: updateAttack(); break;
			case Stage::Decay: updateDecay(); break;
			case Stage::Release: updateRelease(m_release_rate); break;
			case Stage::Kill: updateRelease(m_kill_rate); break;
			default: break;
		}

		return m_current;
	}
}