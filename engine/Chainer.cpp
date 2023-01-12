#include "Chainer.hpp"
#include "core/Log.hpp"
#include "Engine.hpp"

namespace sns {

	Chainer::Chainer()
		:TAG("Chainer"),
		m_engine(nullptr)
	{
		apply(Configuration());
		panic();
	}

	void Chainer::setEngine(Engine* engine) {
		m_engine = engine;
	}

	Chainer::State Chainer::state() {
		return m_state;
	}

	bool Chainer::advanceToNext() {
		if (!m_cfg.valid)
			return false;

		if (m_state.link_run < (m_cfg.chain[m_state.link_index].runs - 1)) {
			m_state.link_run++;
			return false;
		}

		// move to next link
		int current = m_state.link_index;

		m_state.link_index = (m_state.link_index + 1) % m_cfg.chain.size();
		m_state.link_run = 0;

		while (!m_cfg.chain[m_state.link_index].valid)
			m_state.link_index = (m_state.link_index + 1) % m_cfg.chain.size();

		bool looped = m_state.link_index < current;

		return looped;
	}

	void Chainer::apply(Configuration const& cfg) {
		if (cfg.action != Sequencer::Action::None)
			Log::i(TAG, sfmt("Chainer::action=%s", toString(cfg.action)));

		if (cfg.action == Sequencer::Action::Stop || cfg.action == Sequencer::Action::Pause) {
			Sequencer::Configuration sequence;
			sequence.action = Sequencer::Action::Stop;
			m_engine->sequencer().apply(sequence, true);
		}

		m_cfg = cfg;
		m_cfg.valid = false;
		m_state.playing = false;

		for (auto& chain : m_cfg.chain) {
			chain.valid = false;

			if (chain.name.empty() || chain.runs <= 0)
				continue;

			for (InstrumentId instrument = InstrumentStart; instrument != InstrumentCount; ++instrument) {
				auto const& idata = chain.sequence.instruments[instrument];
				if (idata.muted) continue;

				for (auto const& [step_note, note_mode] : idata.steps) {
					auto [step, note] = step_note;
					if (note_mode == Sequencer::NoteMode::Off)
						continue;
					if (step >= chain.sequence.step_count)
						continue;

					chain.valid = true;
					m_cfg.valid = true;
					break;
				}

				if (chain.valid)
					break;
			}
		}
	}

	bool Chainer::next() {
		bool report_state = (m_cfg.action != Sequencer::Action::None);

		bool play_link = false;

		if (m_state.playing) {
			if (!m_engine->sequencer().state().playing) {
				Log::i(TAG, sfmt("%d/%d done", m_state.link_index, m_state.link_run));

				bool looped = advanceToNext();

				//if (looped) {
				//	m_state.playing = false;
				//} 
				//else 
				{
					play_link = true;
				}
				report_state = true;
			}
		}

		if (m_cfg.action != Sequencer::Action::None) {

			if (m_cfg.action == Sequencer::Action::Stop || m_cfg.action == Sequencer::Action::Pause) {
				m_state.playing = false;

				m_state.link_index = 0;
				m_state.link_run = 0;

				if (m_cfg.action_link >= 0)
					m_state.link_index = m_cfg.action_link;

			}
			else if (m_cfg.valid && (m_cfg.action == Sequencer::Action::Play || m_cfg.action == Sequencer::Action::PlayOnce)) {
				m_state.playing = true;

				m_state.link_index = 0;
				m_state.link_run = 0;

				if (m_cfg.action_link >= 0)
					m_state.link_index = m_cfg.action_link;

				if (!m_cfg.chain[m_state.link_index].valid)
					advanceToNext();

				play_link = true;
			}
			m_cfg.action = Sequencer::Action::None;
		}


		if (play_link) {
			Sequencer::Configuration sequence = m_cfg.chain[m_state.link_index].sequence;
			sequence.action = Sequencer::Action::PlayOnce;
			m_engine->sequencer().apply(sequence, true);
		}

		return report_state;
	}

	void Chainer::panic() {
		m_state = State();
	}

}