#include "App.hpp"
#include "AboutWindow.hpp"
#include "SettingsWindow.hpp"

#include "tools/DebugWindow.hpp"
#include "tools/SequencerWindow.hpp"
#include "tools/ChainerWindow.hpp"
#include "tools/ScopeWindow.hpp"

#include "instrument/SynthMachineWindow.hpp"
#include "instrument/DrumMachineWindow.hpp"
#include "instrument/Dx7Window.hpp"
#include "instrument/TB303Window.hpp"

#include "../engine/core/Log.hpp"
#include "../vendor/imgui/imgui.h"


#ifndef VERSION_MAJOR
#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_BUILD 0
#endif // VERSION

extern "C" {
	void sapp_quit(void);
	bool sapp_is_fullscreen(void);
	void sapp_toggle_fullscreen(void);

	void sapp_set_window_title(const char* str);

	int sapp_width(void);
	int sapp_height(void);
	double sapp_frame_duration(void);
	float sapp_dpi_scale(void);
}

constexpr int window_min_width = 890;
constexpr int window_min_height = 720;

namespace sns {

	App::App()
		:TAG("App"),
		m_fullscreen(false),
		m_frame(0),
		m_frame_ts(0),
		m_render_fps(120, 1.0f),
		m_active_instrument(0)
	{
		setActiveInstrument(InstrumentIdSynthMachine);
	}

	App::~App() {
	}

	int App::versionCode() {
		return (VERSION_MAJOR * 1000000) + (VERSION_MINOR * 10000) + VERSION_BUILD;
	}

	std::string App::versionName() {
		return sfmt("%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);
	}

	std::vector<std::shared_ptr<Window>> App::windows() {
		return m_windows;
	}

	void App::setActiveInstrument(InstrumentId instrument) {
		if (m_active_instrument == instrument) return;

		m_active_instrument = instrument;
		Log::d(TAG, sfmt("Active instrument changed to [%s]", instrumentToString(m_active_instrument)));

		engine().midi().setActiveInstrument(instrument);
	}

	InstrumentId App::activeInstrument() {
		return m_active_instrument;
	}

	Configuration& App::configuration() {
		return m_configuration;
	}

	Engine& App::engine() {
		return m_engine;
	}

	bool App::isFullscreen() const {
		return m_fullscreen;
		//return sapp_is_fullscreen();
	}

	int App::width() const {
		return sapp_width();
	}

	int App::height() const {
		return sapp_height();
	}

	float App::dpiScale() const {
		return sapp_dpi_scale();
	}

	uint64_t App::frameNumber() const {
		return m_frame;
	}

	float App::filteredFps() const {
		return m_render_fps.average();
	}

	bool App::shouldRenderMenu() {
		bool fullscreen = isFullscreen();
		bool use_menu = !platformHasFileMenu() || fullscreen;
		return use_menu;
	}

	void App::onInstrumentParamsFeedback(InstrumentId instrument_id, ParametersValues const& values) {
		assert(instrument_id > 0);
		assert(instrument_id < InstrumentCount);

		std::unique_lock<std::mutex> lock(m_actions_mutex);
		auto action = [this, instrument_id, values] {
			m_instruments[instrument_id]->setValues(values);
		};
		m_actions.push_back(action);
	}

	void App::onMidiNotesFeedback(InstrumentId instrument_id, std::set<int> const& notes_pressed) {
		assert(instrument_id > 0);
		assert(instrument_id < InstrumentCount);

		std::unique_lock<std::mutex> lock(m_actions_mutex);
		auto action = [this, instrument_id, notes_pressed] {
			if (instrument_id == activeInstrument())
				getWindow<KeyboardWindow>()->updateMidiController(notes_pressed);
		};
		m_actions.push_back(action);
	}

	void App::onSequencerState(Sequencer::State state) {
		std::unique_lock<std::mutex> lock(m_actions_mutex);
		auto action = [this, state] {
			getWindow<SequencerWindow>()->stateChanged(state);
		};
		m_actions.push_back(action);
	}

	void App::onChainerState(Chainer::State state) {
		std::unique_lock<std::mutex> lock(m_actions_mutex);
		auto action = [this, state] {
			getWindow<ChainerWindow>()->stateChanged(state);
		};
		m_actions.push_back(action);
	}

	void App::initialize() {
		platformSetupWindow(window_min_width, window_min_height);

		engine().setFeedbackCallback(this);

		//
		// Setup imgui
		//
		{
			auto& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		}

		//
		// Setup windows
		//
		m_windows.push_back(std::make_shared<SettingsWindow>());
		m_windows.push_back(std::make_shared<DebugWindow>());
		m_windows.push_back(std::make_shared<AboutWindow>());


		KeyboardWindow* keyboard_window = new KeyboardWindow();
		m_windows.push_back(std::shared_ptr<KeyboardWindow>(keyboard_window));
		m_tools.push_back(keyboard_window);

		SequencerWindow* sequencer_window = new SequencerWindow();
		m_windows.push_back(std::shared_ptr<SequencerWindow>(sequencer_window));
		m_tools.push_back(sequencer_window);

		ChainerWindow* chainer_window = new ChainerWindow();
		m_windows.push_back(std::shared_ptr<ChainerWindow>(chainer_window));
		m_tools.push_back(chainer_window);

		ScopeWindow* scope_window = new ScopeWindow();
		m_windows.push_back(std::shared_ptr<ScopeWindow>(scope_window));
		m_tools.push_back(scope_window);



		m_instruments.resize(InstrumentCount);
		SynthMachineWindow* synth_machine_window = new SynthMachineWindow();
		m_windows.push_back(std::shared_ptr<SynthMachineWindow>(synth_machine_window));
		m_instruments[InstrumentIdSynthMachine] = synth_machine_window;

		DrumMachineWindow* drum_machine_window = new DrumMachineWindow();
		m_windows.push_back(std::shared_ptr<DrumMachineWindow>(drum_machine_window));
		m_instruments[InstrumentIdDrumMachine] = drum_machine_window;

		Dx7Window* dx7_window = new Dx7Window();
		m_windows.push_back(std::shared_ptr<Dx7Window>(dx7_window));
		m_instruments[InstrumentIdDx7] = dx7_window;

		TB303Window* tb303_window = new TB303Window();
		m_windows.push_back(std::shared_ptr<TB303Window>(tb303_window));
		m_instruments[InstrumentIdTB303] = tb303_window;

		loadConfiguration();
		m_fullscreen = configuration().window_fullscreen;
		platformFullscreenChanged(m_fullscreen);

		Log::logger().setFilename(mergePaths(rootFolder(), "app.log"));
		Log::logger().startWorking();

		m_configuration.runs += 1;

		buildMenu();
		MenuCallback callback = std::bind(&App::triggerMenuCommand, this, std::placeholders::_1);
		platformSetupFileMenu(m_menu, callback);

		Log::i(TAG, sfmt("Initialized app VersionCode [%d] - VersionName [%s]", versionCode(), versionName()));
	}

	void App::loadConfiguration() {

		if (!m_load_project.empty()) {
			for (auto& current : m_windows)
				current->shutdown();

			if (!m_import_project_file.empty()) {
				auto projects = importableProjects(m_import_project_file);
				if (projects.empty())
					m_import_project_file = "";
				else
					m_load_project = projects[0];
			}

			m_configuration.project = m_load_project;
			saveConfiguration(this, m_configuration);
		}

		if (!m_clone_project.empty()) {
			Log::i(TAG, sfmt("Cloning project... %s", m_clone_project));
			sns::cloneProject(m_configuration, m_clone_project);
		}

		if (!m_import_project_file.empty()) {
			Log::i(TAG, sfmt("Importing project [%s]... %s", m_load_project, m_import_project_file));
			sns::importProject(m_load_project, m_import_project_file);
		}

		if (!m_load_project.empty()) {
			Log::i(TAG, sfmt("Loading configuration... %s", m_configuration.project));
		}

		m_configuration = sns::loadConfiguration(this);

		for (auto& current : m_windows) {
			current->setApp(this);
			current->initialize();
		}

		sapp_set_window_title(sfmt("Senos - %s", m_configuration.project).c_str());

		// sort windows, so they match the last run
		std::sort(m_windows.begin(), m_windows.end(), [](std::shared_ptr<Window> const& a, std::shared_ptr<Window> const& b) {
			return a->zOrder() < b->zOrder();
			});

		m_load_project = "";
		m_clone_project = "";
		m_import_project_file = "";
	}

	void App::cleanup() {
		engine().setFeedbackCallback(nullptr);

		Log::i(TAG, "Cleanup...");

		for (auto& current : m_windows) {
			current->shutdown();
			current->setApp(nullptr);
		}

		saveConfiguration(this, m_configuration);

		Log::i(TAG, "Bye Bye!");

		Log::logger().stopWorking();
	}

	void App::render() {
		//Log::d(TAG, sfmt("Rendering frame %d", m_frame));

		if (!m_load_project.empty())
			loadConfiguration();

		// dispatch actions
		{
			std::vector<std::function<void()>> actions;

			{
				std::unique_lock<std::mutex> lock(m_actions_mutex);
				std::swap(m_actions, actions);
			}

			for (auto& current : actions)
				current();
		}

		renderMainMenu();

		for (auto& current : m_windows) {
			if (current->showing())
				current->render();
		}

		//
		// Render indicators
		//
		if (engine().recorder().isRecording()) {
			uint64_t ms = engine().recorder().recordedMilliseconds();
			std::string const text = timecode(ms);
			float end = width() / dpiScale();
			ImVec2 pos(end - ImGui::GetFontSize() * 5.7f, ImGui::GetFontSize() * 0.7f);
			if ((getCurrentMilliseconds() / 500) % 2 == 0)
				ImGui::GetForegroundDrawList()->AddCircleFilled(pos, ImGui::GetFontSize() * 0.4f, IM_COL32(255, 0, 0, 200));

			pos.y = ImGui::GetFontSize() * 0.15f;
			pos.x += ImGui::GetFontSize() * 0.6f;
			ImGui::GetForegroundDrawList()->AddText(pos, IM_COL32(255, 255, 255, 200), text.c_str());
		}


		//
		// compute active instrument
		//
		{
			Window* higher = nullptr;
			int higher_index = -100000;
			for (auto& instrument : m_instruments) {
				if (instrument && instrument->zOrder() > higher_index) {
					higher_index = instrument->zOrder();
					higher = instrument;
				}
			}

			if (higher) {
				setActiveInstrument(higher->subtype());
			}
		}

		int64_t ts = getCurrentMilliseconds();
		if (m_frame > 0) {
			int64_t fps = 1000LL / maximum(1LL, ts - m_frame_ts);
			m_render_fps.add(float(fps));
		}

		m_frame++;
		m_frame_ts = ts;
	}


	void App::buildMenu() {
		m_menu.clear();

#ifdef __APPLE__
		ImGuiKey super_mod = ImGuiKey::ImGuiKey_ModSuper;
		ImGuiKey quit_key = ImGuiKey::ImGuiKey_Q;
#else 
		ImGuiKey const super_mod = ImGuiKey::ImGuiKey_LeftAlt;
		ImGuiKey const quit_key = ImGuiKey::ImGuiKey_F4;
#endif 

		ImGuiKey const ctr_key = ImGuiKey::ImGuiKey_LeftCtrl;

		size_t column = m_menu.size();
		m_menu.push_back(Menu::value_type());
		m_menu[column].name = "File";
		m_menu[column].add("New");
		m_menu[column].add("Open");
		m_menu[column].add("Clone To...");
		m_menu[column].add("-");
		m_menu[column].add("Import");
		m_menu[column].add("Export");
		m_menu[column].add("-");
		m_menu[column].add("Settings");
		m_menu[column].add("-");
		m_menu[column].add("Quit", quit_key, super_mod);


		column = m_menu.size();
		m_menu.push_back(Menu::value_type());
		m_menu[column].name = "Instruments";
		int count = 0;
		for (auto current : m_instruments) {
			if (current) {
				m_menu[column].add(current->name(), ImGuiKey_0 + count, ctr_key);
			}
			count++;
		}

		column = m_menu.size();
		m_menu.push_back(Menu::value_type());
		m_menu[column].name = "Tools";
		for (auto current : m_tools) {
			m_menu[column].add(current->name(), ImGuiKey_0 + count, ctr_key);
			count++;
		}
		m_menu[column].add("-");
		m_menu[column].add("Toggle Recording", ImGuiKey_R, super_mod);


		column = m_menu.size();
		m_menu.push_back(Menu::value_type());
		m_menu[column].name = "Window";
		m_menu[column].add("Toggle Fullscreen", ImGuiKey_F, super_mod);
		m_menu[column].add("-");
		m_menu[column].add("Close All", ImGuiKey_0, ctr_key);
		m_menu[column].add("Redistribute");
		m_menu[column].add("Cascade");
		m_menu[column].add("-");
		m_menu[column].add("Play");


		column = m_menu.size();
		m_menu.push_back(Menu::value_type());
		m_menu[column].name = "Help";
		m_menu[column].add("Source");
		m_menu[column].add("Debug");
		m_menu[column].add("-");
		m_menu[column].add("About");


		// process shortcuts
		for (auto& column : m_menu) {
			for (auto& item : column.items) {
				if (item.shortcut_key != ImGuiKey_None) {

					std::string name = ImGui::GetKeyName(ImGuiKey(item.shortcut_key));
					if (item.shortcut_mod != ImGuiKey_None) {
						std::string mod = ImGui::GetKeyName(ImGuiKey(item.shortcut_mod));
						replace(mod, "Mod", "");
						replace(mod, "Left", "");
						replace(mod, "Right", "");

#ifdef __APPLE__
						replace(mod, "Super", "Cmd");
						replace(mod, "Alt", "Opt");
#endif //__APPLE__

						name = mod + "+" + name;
					}

					item.shortcut = name;
					m_shortcuts.push_back(item);
				}
			}
		}
	}


	void App::renderMainMenu() {

		if (shouldRenderMenu() && ImGui::BeginMainMenuBar()) {
			for (auto const& column : m_menu) {
				if (ImGui::BeginMenu(column.name.c_str())) {
					for (auto const& item : column.items) {
						if (item.name == "-") {
							ImGui::Separator();
						}
						else {
							char const* shortcut = item.shortcut.empty() ? nullptr : item.shortcut.c_str();
							if (ImGui::MenuItem(item.name.c_str(), shortcut))
								triggerMenuCommand(item.command);
						}
					}
					ImGui::EndMenu();
				}
			}
			ImGui::EndMainMenuBar();
		}

		//
		// process shortcuts
		//
		if (!platformHasFileMenu()) {
			for (auto const& current : m_shortcuts) {
				if (current.shortcut_key != 0) {
					if (current.shortcut_mod != 0) {
						ImGuiIO& io = ImGui::GetIO();

						bool mod_pressed = false;
						ImGuiKey mod_key = ImGuiKey(current.shortcut_mod);
						if ((mod_key == ImGuiKey::ImGuiKey_LeftAlt || mod_key == ImGuiKey::ImGuiKey_RightAlt) && !io.KeyAlt)
							continue;

						if ((mod_key == ImGuiKey::ImGuiKey_LeftCtrl || mod_key == ImGuiKey::ImGuiKey_RightCtrl) && !io.KeyCtrl)
							continue;

						if ((mod_key == ImGuiKey::ImGuiKey_LeftShift || mod_key == ImGuiKey::ImGuiKey_RightCtrl) && !io.KeyShift)
							continue;
					}


					if (ImGui::IsKeyPressed(ImGuiKey(current.shortcut_key), false))
						triggerMenuCommand(current.command);
				}
			}
		}


		dispatchMenuCommands();
	}

	void App::triggerMenuCommand(std::string const& command) {
		m_menu_commands.push_back(command);
	}

	void App::dispatchMenuCommands() {

		bool popup_new_project = false;
		bool popup_open_project = false;
		bool popup_clone_project = false;

		for (auto const& current : m_menu_commands) {
			if (current == "File|New") {
				popup_new_project = true;
			}
			else if (current == "File|Open") {
				popup_open_project = true;
			}
			else if (current == "File|Clone To...") {
				popup_clone_project = true;
			}
			else if (current == "File|Export") {
				projectExport();
			}
			else if (current == "File|Import") {
				projectImport();
			}
			else if (current == "File|Quit") {
				sapp_quit();
			}
			else if (current == "Tools|Record") {
				recordStart();
			}
			else if (current == "Tools|Stop Recording") {
				recordStop();
			}
			else if (current == "Tools|Toggle Recording") {

				if (engine().recorder().isRecording())
					recordStop();
				else
					recordStart();

			}
			else if (current == "Window|Toggle Fullscreen") {
				m_fullscreen = !m_fullscreen;
				sapp_toggle_fullscreen();
				platformFullscreenChanged(m_fullscreen);
			}
			else if (current == "Window|Close All") {
				windowCloseAll();
			}
			else if (current == "Window|Play") {
				windowLayoutPlay();
			}
			else if (current == "Window|Redistribute") {
				windowLayoutRedistribute();
			}
			else if (current == "Window|Cascade") {
				windowLayoutCascade();
			}
			else if (current == "Help|Source") {
				platformShellOpen("https://github.com/RuiVarela/Senos");
			}
			else if (current == "File|Settings" ||
				startsWith(current, "Help|") ||
				startsWith(current, "Instruments|") ||
				startsWith(current, "Tools|")) {

				std::string name = current;
				name = replaceString(name, "Instruments|", "");
				name = replaceString(name, "Tools|", "");
				name = replaceString(name, "File|", "");
				name = replaceString(name, "Help|", "");

				for (auto window : m_windows)
					if (window->name() == name)
						window->show(true);

			}
			else {
				Log::d(TAG, sfmt("Unhandled menu command [%s]", current));
			}
		}
		m_menu_commands.clear();


		projectNew(popup_new_project);
		projectOpen(popup_open_project);
		projectClone(popup_clone_project);
	}

	//
	// Windows
	//
	void App::windowCloseAll() {
		for (auto& current : m_windows)
			current->show(false);
	}

	void App::windowLayoutRedistribute() {
		int counter = 0;

		for (auto& current : m_windows) {
			if (!current->showing()) continue;

			int index = counter % 4;

			switch (index)
			{
			case 0: current->anchorTo(Window::Anchor::Start, Window::Anchor::Start);  break;
			case 1: current->anchorTo(Window::Anchor::End, Window::Anchor::End);  break;
			case 2: current->anchorTo(Window::Anchor::Start, Window::Anchor::End);  break;
			case 3: current->anchorTo(Window::Anchor::End, Window::Anchor::Start);  break;
			default:  break;
			}

			++counter;
		}
	}

	void App::windowLayoutCascade() {
		float counter = 0.0f;

		float y_offset = 0.0f;
		if (shouldRenderMenu())
			y_offset += ImGui::GetFrameHeight();

		float spacing = ImGui::GetFrameHeight() * 1.5f;

		auto windows = m_windows;

		std::sort(windows.begin(), windows.end(), [](std::shared_ptr<Window> const& a, std::shared_ptr<Window> const& b) {
			return a->zOrder() < b->zOrder();
			});

		for (auto& current : windows) {
			if (!current->showing()) continue;

			current->setPosition(int(counter * spacing), int(counter * spacing + y_offset));
			counter += 1.0f;
		}
	}

	void App::windowLayoutPlay() {

		for (auto& current : m_windows) {
			bool show = (current->isInstrumentWindow() && (current->subtype() == InstrumentIdSynthMachine));
			show |= (current.get() == getWindow<KeyboardWindow>().get());
			current->show(show);
		}

		windowLayoutRedistribute();
	}

	//
	// Projects
	//
	void App::projectNew(bool trigger) {
		std::function<void(std::string)> callback = [this](std::string const& name) {
			m_load_project = sanitizeName(name, true);
		};
		Window::uiNamePicker(trigger, "New project", "Name:", "", callback);
	}

	void App::projectOpen(bool trigger) {
		static std::vector<std::string> names;

		if (trigger) {
			ImGui::OpenPopup("Open Project");
			names = projectNames();
		}

		bool p_open = true;

		std::string load_picked;
		std::string delete_picked;

		if (ImGui::BeginPopupModal("Open Project", &p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
			float s = ImGui::GetFontSize() * 10.0f;
			ImVec2 size(s, s);
			if (ImGui::BeginListBox("##list", size)) {
				for (size_t i = 0; i != names.size(); i++)
				{
					bool is_selected = false;
					if (ImGui::Selectable(names[i].c_str(), &is_selected) && is_selected) {
						load_picked = names[i];
					}

					if (ImGui::BeginPopupContextItem(sfmt("delete_popup_%d", i).c_str())) {
						if (ImGui::Selectable("Delete"))
							delete_picked = names[i];
						ImGui::EndPopup();
					}
				}
				ImGui::EndListBox();
			}

			if (!load_picked.empty()) {
				m_load_project = sanitizeName(load_picked, true);
				ImGui::CloseCurrentPopup();
			}

			if (!delete_picked.empty()) {
				if (!delete_picked.empty() &&
					(delete_picked != configuration().project) &&
					(delete_picked != DefaultSessionName)) {

					Log::d(TAG, sfmt("Delete %s", delete_picked));
					deleteProject(delete_picked);
					names = projectNames();
				}

			}

			ImGui::EndPopup();
		}
	}

	void App::recordStart() {
		auto callback = [this](std::string const& filename) {
			if (!filename.empty())
				engine().recorder().startRecording(filename);
		};

		std::string filename = sfmt("%s.wav", datetimeMarker());
		platformPickSaveFile("Audio Recording", "Please select a path where to record", filename, callback);
	}

	void App::recordStop() {
		engine().recorder().stopRecording();
	}


	void App::projectClone(bool trigger) {
		std::function<void(std::string)> callback = [this](std::string const& name) {
			m_clone_project = sanitizeName(name, true);
			m_load_project = configuration().project;
		};
		Window::uiNamePicker(trigger, "Clone project", "Name:", "", callback);
	}

	void App::projectExport() {
		auto callback = [this](std::string const& filename) {
			if (!filename.empty())
				exportProject(configuration(), filename);
		};
		std::string filename = sfmt("%s_-_%s.%s", configuration().project, datetimeMarker(), PackExtension);
		platformPickSaveFile("Export project", "Please select a path where to export the project", filename, callback);
	}

	void App::projectImport() {
		auto callback = [this](std::string const& filename) {
			if (!filename.empty()) {
				m_import_project_file = filename;
				m_load_project = configuration().project;
			}
		};
		platformPickLoadFile("Import project", "Please select the project file", PackExtension, callback);
	}


}
