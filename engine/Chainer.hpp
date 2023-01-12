#pragma once

#include "Sequencer.hpp"

namespace sns {


	class Chainer {
	public:

		struct Link {
			std::string name;
			int runs = 0;
			Sequencer::Configuration sequence;
			bool valid = false;
		};

		struct Configuration {
			Sequencer::Action action = Sequencer::Action::None;
			std::vector<Link> chain;
			bool valid = false;
			int action_link = 0;
		};

		struct State {
			int link_index = 0;
			int link_run = 0;
			bool playing = false;
		};

		Chainer();
		Chainer(Chainer const&) = delete;
		Chainer& operator=(Chainer const&) = delete;

		void setEngine(Engine* engine);

		State state();
		bool next();

		void apply(Configuration const& cfg);
		void panic();
	private:
		std::string TAG;
		Engine* m_engine;
		State m_state;
		Configuration m_cfg;

		bool advanceToNext();
	};
}