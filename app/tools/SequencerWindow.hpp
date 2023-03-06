#pragma once

#include "../Window.hpp"
#include "../engine/Sequencer.hpp"

namespace sns {

	class SequencerWindow : public Window {
	public:
		SequencerWindow();
		~SequencerWindow() override;

		void render() override;
		void stateChanged(Sequencer::State const& state);

		bool playing();
		void stop();
	private:
		void renderTopBar();
		void renderTable(float height);
		void renderPopup(int step_index, int note);

		void onSavePreset(std::string const& name) override;
		void onLoadPreset(std::string const& name) override;
		void onDeletePreset(std::string const& name) override;
		void onRefreshPresets() override;

		void chainerStop();
		bool chainerPlaying();

		Sequencer::Configuration m_cfg;
		bool m_cfg_updated;
		bool m_cfg_loaded;
		bool m_needs_scroll;
		bool m_rendered_table;
		Sequencer::State m_state;

		char const* m_note_icons[int(Sequencer::NoteMode::Count)];
		int m_note_keys[int(Sequencer::NoteMode::Count)];

		enum class MoveKind {
			Up,
			Down,
			Left,
			Right,

			Count
		};
		char const* m_move_names[int(MoveKind::Count)];
		void handleMove(MoveKind kind, int step_index, int note);
	};
}
