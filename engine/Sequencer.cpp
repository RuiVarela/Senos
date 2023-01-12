#include "Sequencer.hpp"
#include "core/Log.hpp"
#include "Engine.hpp"

namespace sns {

	std::string toString(Sequencer::Action action) {
		switch (action)
		{
		case sns::Sequencer::Action::None: return "None";
		case sns::Sequencer::Action::Play: return "Play";
		case sns::Sequencer::Action::PlayOnce: return "PlayOnce";
		case sns::Sequencer::Action::Pause: return "Pause";

		case sns::Sequencer::Action::TogglePlay: return "TogglePlay";
		case sns::Sequencer::Action::Stop: return "Stop";
		default: return sfmt("Invalid Sequencer::Action %d", int(action));
		}
	}

	std::string toString(Sequencer::NoteMode mode) {
		switch (mode)
		{
		case sns::Sequencer::NoteMode::Off: return "Off";
		case sns::Sequencer::NoteMode::Press: return "Press";
		case sns::Sequencer::NoteMode::Accent: return "Accent";
		case sns::Sequencer::NoteMode::Hold: return "Hold";
		default: return sfmt("Invalid Sequencer::NoteMode %d", int(mode));
		}
	}



	Sequencer::NoteMode Sequencer::Configuration::stepState(InstrumentId instrument, int step, int note) {
		auto found = instruments[instrument].steps.find(std::pair<int, int>(step, note));
		if (found != instruments[instrument].steps.end())
			return found->second;
		return Sequencer::NoteMode::Off;
	}

	void Sequencer::Configuration::setStepState(InstrumentId instrument, int step, int note, NoteMode value) {
		auto key = std::pair<int, int>(step, note);

		if (value == Sequencer::NoteMode::Off) {
			instruments[instrument].steps.erase(key);
		}
		else {
			instruments[instrument].steps[key] = value;
		}
	}

	void Sequencer::Configuration::toggleStepState(InstrumentId instrument, int step, int note) {
		NoteMode value = stepState(instrument, step, note);
		value = (value == NoteMode::Off) ? NoteMode::Press : NoteMode::Off;
		setStepState(instrument, step, note, value);
	}

	Sequencer::Sequencer()
		:TAG("Sequencer"),
		m_engine(nullptr)
	{

		m_cfg.tempo = 0;
		apply(Configuration());
		panic();
	}

	void Sequencer::setEngine(Engine* engine) {
		m_engine = engine;
	}

	Sequencer::State Sequencer::state() {
		return m_state;
	}

	void Sequencer::apply(Configuration const& cfg, bool chaining) {

		if (cfg.action != Sequencer::Action::None)
			Log::i(TAG, sfmt("Sequencer::action=%s action_step=%d", toString(cfg.action), cfg.action_step));

		if (cfg.action == Sequencer::Action::Stop) {
			stopAllPlayingNotes();

			m_state.playing = false;
			m_state.active_step = 0;
			if (cfg.action_step >= 0)
				m_state.active_step = cfg.action_step;
		}


		if (cfg.step_count != m_cfg.step_count) {
			m_state.active_step = m_state.active_step % cfg.step_count;
		}

		bool compute_samples = (cfg.tempo != m_cfg.tempo) || !equivalent(cfg.duty, m_cfg.duty);

		if (compute_samples) {
			m_beat_samples = (SampleRate * 60) / cfg.tempo;
			m_duty_samples = int(float(m_beat_samples) * cfg.duty);
		}


		StepNotes steps(cfg.step_count);
		for (InstrumentId instrument = InstrumentStart; instrument != InstrumentCount; ++instrument) {
			auto const& idata = cfg.instruments[instrument];
			if (idata.muted) continue;

			for (auto const& [step_note, note_mode] : idata.steps) {
				auto [step, note] = step_note;
				if (note_mode == NoteMode::Off)
					continue;

				if (step >= steps.size())
					continue;

				steps[step].push_back({ instrument, note , note_mode });
			}
		}
		std::swap(steps, m_steps);

		m_cfg = cfg;
		m_cfg_changed = true;
		m_state.chaining = chaining;
	}

	bool Sequencer::next() {
		bool start_notes = false;
		bool report_state = m_cfg_changed;

		if (m_cfg.action != Sequencer::Action::None) {
			if (m_cfg.action == Sequencer::Action::TogglePlay) {
				m_cfg.action = (m_state.playing) ? Sequencer::Action::Pause : Sequencer::Action::Play;
			}

			if (m_cfg.action == Sequencer::Action::Pause) {

				stopAllPlayingNotes();

				m_state.playing = false;

			}
			else if (m_cfg.action == Sequencer::Action::Play || m_cfg.action == Sequencer::Action::PlayOnce) {

				m_state.playing = true;
				m_state.once = m_cfg.action == Sequencer::Action::PlayOnce;

				if (m_cfg.action_step >= 0)
					m_state.active_step = m_cfg.action_step;

				m_samples_since_last_beat = 0;
				start_notes = true;
				//Log::d(TAG, sfmt("Sequencer::Action::Play %d %d", m_state.active_step, m_samples_since_last_beat));
			}

			m_state.active_step = m_state.active_step % m_cfg.step_count;
			report_state = true;
			m_cfg.action = Sequencer::Action::None;
		}


		if (m_state.playing) {
			bool do_next_step = (m_samples_since_last_beat >= m_beat_samples);
			bool do_duty_end = (m_samples_since_last_beat == m_duty_samples);


			// track steps
			if (do_next_step) {
				//Log::d(TAG, sfmt("Step Change from %d %d", m_state.active_step, m_samples_since_last_beat));
				m_state.active_step = (m_state.active_step + 1) % m_cfg.step_count;
				m_samples_since_last_beat = 0;

				// stop when we reached the end
				if (m_state.once && m_state.active_step == 0) {
					stopAllPlayingNotes();
					m_state.playing = false;
					m_state.active_step = 0;
				}
				else {
					//Log::d(TAG, sfmt("Step Change to %d %d", m_state.active_step, m_samples_since_last_beat));
					start_notes = true;
				}

				report_state = true;
			}
			else if (do_duty_end) {
				stopPlayingPreviousNotes(true);
			}

			if (start_notes) {
				stopPlayingPreviousNotes(false);
				startPlayingNewNotes();
			}

			//execute beat
			m_samples_since_last_beat++;
		}

		m_cfg_changed = false;
		return report_state;
	}

	void Sequencer::stopAllPlayingNotes() {
		for (auto const& [step_instrument, step_note, step_note_mode] : m_playing_notes) {
			m_engine->setInstrumentNote(step_instrument, step_note, 0.0f);
		}
		m_playing_notes.clear();
	}

	void Sequencer::stopPlayingPreviousNotes(bool duty_end) {
		auto iterator = m_playing_notes.begin();
		while (iterator != m_playing_notes.end()) {
			bool erase = false;

			auto const& [playing_instrument, playing_note, playing_note_mode] = *iterator;
			NoteMode current_mode = getStepNote(m_state.active_step, playing_instrument, playing_note);

			bool stopped = false;

			// nothing to play now
			if (current_mode == NoteMode::Off)
				stopped = true;
			// it was just a press
			else if (playing_note_mode == NoteMode::Press || playing_note_mode == NoteMode::Accent)
				stopped = true;
			// it was a hold but now we have a accent
			else if (playing_note_mode == NoteMode::Hold && current_mode == NoteMode::Accent)
				stopped = true;


			if (stopped) {
				//Log::d(TAG, sfmt("[%02d] Stop playing %s %s", m_state.active_step, instrumentToString(playing_instrument), noteName(playing_note)));
				m_engine->setInstrumentNote(playing_instrument, playing_note, 0.0f);
				erase = true;
			}

			if (erase) {
				iterator = m_playing_notes.erase(iterator);
			}
			else {
				++iterator;
			}
		}
	}

	void Sequencer::startPlayingNewNotes() {
		for (auto const& current : m_steps[m_state.active_step]) {
			auto const [step_instrument, step_note, step_note_mode] = current;

			auto state = getPlayingNote(step_instrument, step_note);


			if (state == NoteMode::Off) {
				//Log::d(TAG, sfmt("[%02d] Start playing %s %s", m_state.active_step, instrumentToString(step_instrument), noteName(step_note)));

				float velocity = (step_note_mode == NoteMode::Accent) ? AccentPressVelocity : DefaultPressVelocity;

				m_engine->setInstrumentNote(step_instrument, step_note, velocity);
				m_playing_notes.push_back(current);
			}
			else {

				// update playing step
				for (size_t i = 0; i != m_playing_notes.size(); ++i)
					if (step_instrument == std::get<0>(m_playing_notes[i]) && step_note == std::get<1>(m_playing_notes[i]))
						m_playing_notes[i] = current;

			}
		}
	}

	Sequencer::NoteMode Sequencer::getNote(InstrumentId instrument, int note, InstrumentNotes const& search) {
		for (auto const& [step_instrument, step_note, step_note_mode] : search)
			if (step_instrument == instrument && step_note == note)
				return step_note_mode;

		return Sequencer::NoteMode::Off;
	}

	Sequencer::NoteMode Sequencer::getStepNote(int step, InstrumentId instrument, int note) {
		return getNote(instrument, note, m_steps[step]);
	}

	Sequencer::NoteMode Sequencer::getPlayingNote(InstrumentId instrument, int note) {
		return getNote(instrument, note, m_playing_notes);
	}

	void Sequencer::panic() {
		m_state = State();
		m_samples_since_last_beat = 0;
		m_cfg_changed = true;
		stopAllPlayingNotes();
	}
}