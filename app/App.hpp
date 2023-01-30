#pragma once

#include "../engine/Engine.hpp"

#include "Configuration.hpp"
#include "Window.hpp"
#include "tools/KeyboardWindow.hpp"
#include "Platform.hpp"


namespace sns {

	class App : public FeedbackCallback {
	public:
		std::string TAG;

		App();
		~App();

		void onInstrumentParamsFeedback(InstrumentId instrument_id, ParametersValues const& values) override;
		void onMidiNotesFeedback(InstrumentId instrument_id, std::set<int> const& notes_pressed) override;
		void onSequencerState(Sequencer::State state) override;
		void onChainerState(Chainer::State state) override;

		void initialize();
		void cleanup();
		void render();

		bool isFullscreen() const;
		int width() const;
		int height() const;
		float dpiScale() const;
		uint64_t frameNumber() const;
		float filteredFps() const;

		std::vector<std::shared_ptr<Window>> windows();

		bool shouldRenderMenu();
		InstrumentId activeInstrument();

		Configuration& configuration();
		Engine& engine();

		int versionCode();
		std::string versionName();

		template<typename T>
		std::shared_ptr<T> getWindow() {
			for (auto& current : m_windows) {
				std::shared_ptr<T> casted = std::dynamic_pointer_cast<T>(current);
				if (casted)
					return casted;
			}
			return std::shared_ptr<T>();
		}
	private:
		Configuration m_configuration;
		Engine m_engine;
		std::vector<std::shared_ptr<Window>> m_windows;
		std::vector<Window*> m_tools;
		std::vector<Window*> m_instruments;

		std::mutex m_actions_mutex;
		std::vector<std::function<void()>> m_actions;

		void setActiveInstrument(InstrumentId instrument);
		InstrumentId m_active_instrument;

		uint64_t m_frame;
		int64_t m_frame_ts;
		RunningAverage m_render_fps;
		bool m_fullscreen;

		void buildMenu();
		void renderMainMenu();
		void triggerMenuCommand(std::string const& command);
		void dispatchMenuCommands();
		std::vector<std::string> m_menu_commands;
		Menu m_menu;
		MenuShortcuts m_shortcuts;


		void windowCloseAll();
		void windowLayoutRedistribute();
		void windowLayoutCascade();
		void windowLayoutPlay();

		void loadConfiguration();
		void projectNew(bool trigger);
		void projectOpen(bool trigger);
		void projectClone(bool trigger);
		void projectExport();
		void projectImport();

		void recordStart();
		void recordStop();

		std::string m_load_project;
		std::string m_clone_project;
		std::string m_import_project_file;

		friend 	Configuration migrateApp(App* app, Configuration cfg, int from, int to);
	};
}
