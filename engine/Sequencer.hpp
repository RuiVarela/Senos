#pragma once

#include "audio/Audio.hpp"
#include "instrument/Instrument.hpp"

namespace sns {

	class Engine;

	class Sequencer {
	public:
		enum class NoteMode {
			Off,
			Press,
			Accent,
			Hold,

			Count
		};

		enum class Action {
			None,
			Play,
			PlayOnce,
			Pause,
			TogglePlay,
			Stop,

			Count
		};

		struct InstrumentConfiguration {
			using StepNote = std::pair<int, int>; // step, note
			using Steps = std::map<StepNote, NoteMode>;

			Steps steps;
			bool muted = false;

			int ui_first_row = 0;
			int ui_first_col = 0;
		};

		struct Configuration {
			float duty = 0.3f;
			int tempo = 90;
			int step_count = 16;

			std::array<InstrumentConfiguration, InstrumentCount> instruments;


			Action action = Action::None;
			int action_step = 0;

			int ui_selected_instrument = InstrumentStart;


			NoteMode stepState(InstrumentId instrument, int step, int note);
			void setStepState(InstrumentId instrument, int step, int note, NoteMode value);
			void toggleStepState(InstrumentId instrument, int step, int note);
		};

		struct State {
			int active_step = 0;
			bool playing = false;
			bool once = false;
			bool chaining = false;
		};

		Sequencer();

		Sequencer(Sequencer const&) = delete;
		Sequencer& operator=(Sequencer const&) = delete;

		void setEngine(Engine* engine);

		State state();
		bool next();

		void apply(Configuration const& cfg, bool chaining = false);
		void panic();
	private:
		std::string TAG;
		Engine* m_engine;

		State m_state;

		Configuration m_cfg;
		bool m_cfg_changed;
		int m_beat_samples;
		int m_duty_samples;
		int m_samples_since_last_beat;


		// lookup data
		using InstrumentNote = std::tuple<InstrumentId, int, NoteMode>; // instrument -> note -> notemode
		using InstrumentNotes = std::vector<InstrumentNote>;
		using StepNotes = std::vector<InstrumentNotes>;
		StepNotes m_steps;
		InstrumentNotes m_playing_notes;

		void stopAllPlayingNotes();
		void stopPlayingPreviousNotes(bool duty_end);
		void startPlayingNewNotes();

		static NoteMode getNote(InstrumentId instrument, int note, InstrumentNotes const& search);
		NoteMode getStepNote(int step, InstrumentId instrument, int note);
		NoteMode getPlayingNote(InstrumentId instrument, int note);

	};

	std::string toString(Sequencer::Action action);
	std::string toString(Sequencer::NoteMode mode);
}