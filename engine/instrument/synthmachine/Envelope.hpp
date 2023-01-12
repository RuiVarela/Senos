#pragma once

#include "../../audio/Audio.hpp"

namespace sns {

	class Envelope {
	public:

		enum class Stage {
			NotStarted,
			Attack,
			Decay,
			Sustain,
			Release,
			Kill,
			Off
		};

		Envelope();

		void setAttack(float a);
		void setDecay(float d);
		void setSustain(float s);
		void setRelease(float r);

		void trigger(float level = 1.0f);

		void release();  
		void kill(); // signal that we need o kill this as soon as possible

		bool completed();
		float next();
	private:
		Stage m_stage;

		float m_attack_rate;
		float m_decay_rate;
		float m_sustain;
		float m_release_rate;
		float m_kill_rate;

		float m_current;
		float m_level;
		float m_off_level;

		void updateAttack();
		void updateDecay();
		void updateRelease(float rate);
	};
}