#pragma once

#include "../engine/Engine.hpp"
#include "json.hpp"

namespace sns {
	class App;


	class Window {
	public:
		enum class Anchor {
			Start,
			Center,
			End,
		};

		Window();
		virtual ~Window();

		int subtype() const;
		std::string const& name();

		void setApp(App* app);
		App* app();


		virtual void show(bool value);
		bool showing() const;
		int zOrder() const;
		void setPosition(int x, int y);
		void anchorTo(Anchor x = Anchor::Center, Anchor y = Anchor::Center);

		bool isInstrumentWindow() const;

		virtual void initialize();
		virtual void shutdown();
		virtual void render();

		void saveTo(nlohmann::json& json);
		void loadFrom(nlohmann::json& json);

		void setValues(ParametersValues values);


		static void uiNamePicker(bool trigger,
			std::string const& title, std::string const& item, std::string const& hint,
			std::function<void(std::string)> callback);

		//NonCopyable
		Window(Window const&) = delete;
		Window(Window const&&) = delete;
		Window& operator=(Window const&) = delete;
		Window& operator=(Window const&&) = delete;
	protected:
		std::string TAG;
		App* m_app;
		bool m_showing;
		int m_position_x;
		int m_position_y;
		bool m_position_changed;
		Anchor m_anchor_x;
		Anchor m_anchor_y;
		bool m_anchor_changed;
		int m_z_order;
		bool m_collapsed;
		std::string m_window_name;
		int m_subtype;
		bool m_is_instrument;
		bool m_has_presets;

		void beforeRender();
		void aboutToFinishRender();

		//
		// values and presets
		//
		ParametersValues m_values;
		std::string m_last_preset_selected;
		std::vector<std::string> m_preset_names;
		void savePreset(std::string const& name);
		void loadPreset(std::string const& name);
		void deletePreset(std::string const& name);

		virtual void onSavePreset(std::string const& name);
		virtual void onLoadPreset(std::string const& name);
		virtual void onDeletePreset(std::string const& name);
		virtual void onRefreshPresets();

		void applyInstrumentValues();

		void renderPresets(float height, std::string const& title = "PRESETS", std::string const& item_singular = "preset");

		static void renderSaveList(float height,
			std::vector<std::string> const& names, std::string const& last_selected,
			std::string const& title, std::string const& item_singular,
			std::function<void(std::string const&)> load_callback,
			std::function<void(std::string const&)> save_callback,
			std::function<void(std::string const&)> delete_callback);

		//
		// render helpers
		//
		void pKnob(std::string const& name, Parameter param, float min = 0.0f, float max = 100.0f);
		void pCombo(std::string const& name, Parameter param, std::vector<std::string> const& values);
		bool pCombo(std::string const& name, std::vector<std::string> const& values, int& index);

		bool pBool(Parameter param, std::string name);

		void pTableCheck(Parameter param, float spacing = 0.0f);
		void pTableDragInt(Parameter param, int min = 0, int max = 99);

		void pHelpMarker(std::string const& name);
	};
}