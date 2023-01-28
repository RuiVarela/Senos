#include "Window.hpp"
#include "App.hpp"
#include "../engine/core/Log.hpp"

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_internal.h"
#include "../vendor/imgui-knobs/imgui-knobs.h"

namespace sns {

	Window::Window()
		:TAG("[NOT_SET]"),
		m_app(nullptr),
		m_showing(false),
		m_position_x(-1),
		m_position_y(-1),
		m_position_changed(false),
		m_anchor_x(Anchor::Center),
		m_anchor_y(Anchor::Center),
		m_anchor_changed(false),
		m_z_order(-1),
		m_collapsed(false),
		m_window_name("NAME_NOT_SET"),
		m_subtype(0),
		m_is_instrument(false),
		m_has_presets(false)
	{

	}

	Window::~Window() {

	}

	int Window::subtype() const {
		return m_subtype;
	}

	std::string const& Window::name() {
		return m_window_name;
	}

	void Window::setApp(App* app) {
		m_app = app;
	}

	App* Window::app() {
		return m_app;
	}

	void Window::show(bool value) {

		// bring to front
		if (m_showing && value) {
			ImGui::SetWindowFocus(m_window_name.c_str());
		}

		m_showing = value;
	}

	int Window::zOrder() const {
		if (m_showing)
			return m_z_order;

		return -1;
	}

	bool Window::showing() const {
		return m_showing;
	}

	bool Window::isInstrumentWindow() const {
		return m_is_instrument;
	}

	void Window::initialize() {
		if (m_has_presets) {
			loadPreset(LastSessionName);
		}
	}

	void Window::shutdown() {
		if (m_has_presets) {
			savePreset(LastSessionName);
		}
	}

	void Window::setPosition(int x, int y) {
		if (x < 0 || y < 0)
			return;

		m_position_x = x;
		m_position_y = y;
		m_position_changed = true;
	}

	void Window::anchorTo(Anchor x, Anchor y) {
		m_anchor_x = x;
		m_anchor_y = y;
		m_anchor_changed = true;
	}

	void Window::render() {

	}

	void Window::beforeRender() {

		if (m_anchor_changed) {
			auto& io = ImGui::GetIO();
			ImVec2 factor(0.0f, 0.0f);
			float y_offset = 0;

			if (m_anchor_x == Anchor::Start) {
				factor.x = 0.0f;
			}
			else if (m_anchor_x == Anchor::Center) {
				factor.x = 0.5f;
			}
			else if (m_anchor_x == Anchor::End) {
				factor.x = 1.0f;
			}

			if (m_anchor_y == Anchor::Start) {
				factor.y = 0.0f;

				if (app()->shouldRenderMenu())
					y_offset = ImGui::GetFrameHeight();

			}
			else if (m_anchor_y == Anchor::Center) {
				factor.y = 0.5f;
			}
			else if (m_anchor_y == Anchor::End) {
				factor.y = 1.0f;
			}

			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * factor.x, io.DisplaySize.y * factor.y + y_offset), ImGuiCond_None, factor);
			m_anchor_changed = false;
			m_position_changed = false;
		}
		else if (m_position_changed) {

			ImGui::SetNextWindowPos(ImVec2(float(m_position_x), float(m_position_y)));
			m_anchor_changed = false;
			m_position_changed = false;
		}

		ImGui::SetNextWindowCollapsed(m_collapsed, ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always);
	}

	void Window::aboutToFinishRender() {
		m_position_x = int(ImGui::GetWindowPos().x);
		m_position_y = int(ImGui::GetWindowPos().y);
		m_collapsed = ImGui::IsWindowCollapsed();

		ImGuiID id = ImHashStr(name().c_str());


		ImGuiContext* context = ImGui::GetCurrentContext();
		for (int i = 0; i != context->Windows.size(); ++i) {
			auto& current = context->Windows[i];
			if (current->ID == id)
				m_z_order = i;
		}
	}

	void Window::saveTo(nlohmann::json& json) {
		json["showing"] = showing();
		json["position_x"] = m_position_x;
		json["position_y"] = m_position_y;
		json["collapsed"] = m_collapsed;
		json["z_order"] = zOrder();
	}

	void Window::loadFrom(nlohmann::json& json) {
		if (json.contains("showing"))
			show(json["showing"].get<bool>());

		if (json.contains("collapsed"))
			m_collapsed = json["collapsed"].get<bool>();

		int x = -1;
		if (json.contains("position_x"))
			x = json["position_x"].get<int>();

		int y = -1;
		if (json.contains("position_y"))
			y = json["position_y"].get<int>();

		if (json.contains("z_order"))
			m_z_order = json["z_order"].get<int>();

		setPosition(x, y);
	}


	//
	// values and presets
	//
	void Window::setValues(ParametersValues values) {
		m_values = values;
	}

	void Window::savePreset(std::string const& name) {
		Log::d(TAG, sfmt("savePreset %s", name));
		onSavePreset(name);
		onRefreshPresets();
		m_last_preset_selected = name;

	}

	void Window::loadPreset(std::string const& name) {
		Log::d(TAG, sfmt("loadPreset %s", name));
		onRefreshPresets();
		onLoadPreset(name);
		m_last_preset_selected = name;
	}

	void Window::deletePreset(std::string const& name) {
		Log::d(TAG, sfmt("deletePreset %s", name));
		onDeletePreset(name);
		onRefreshPresets();
	}

	void Window::onSavePreset(std::string const& name) { sns::saveInstrumentPreset(app()->configuration(), m_subtype, name, m_values); }
	void Window::onDeletePreset(std::string const& name) { sns::deleteInstrumentPreset(app()->configuration(), m_subtype, name); }
	void Window::onRefreshPresets() { m_preset_names = sns::instrumentPresetsNames(app()->configuration(), m_subtype); }
	void Window::onLoadPreset(std::string const& name) {
		m_values = sns::instrumentPreset(app()->configuration(), m_subtype, name);
		applyInstrumentValues();
	}

	void Window::applyInstrumentValues() {
		m_app->engine().setInstrumentParams(m_subtype, m_values);
	}

	void Window::renderPresets(float height, std::string const& title, std::string const& item_singular) {
		const float width = ImGui::GetFontSize() * 8.3f;
		static char save_name[128];

		ImGui::BeginChild("save_list_block", ImVec2(width, height), true);

		ImGui::TextDisabled("%s", title.c_str());
		ImVec2 title_size = ImGui::CalcTextSize(title.c_str());

		ImGui::SameLine(0.0f, (width - title_size.x - ImGui::GetFontSize() * 3.15f));

		if (ImGui::Button("Save")) {
			memset(save_name, 0, IM_ARRAYSIZE(save_name));
			if (!m_last_preset_selected.empty() &&
				(m_last_preset_selected.size() < (IM_ARRAYSIZE(save_name) - 1)) &&
				(m_last_preset_selected != LastSessionName) &&
				(m_last_preset_selected != DefaultSessionName)) {

				memcpy(save_name, m_last_preset_selected.c_str(), m_last_preset_selected.size());
			}
			else {

				memcpy(save_name, "new ", 4);
				if (item_singular.size() < IM_ARRAYSIZE(save_name) - 10)
					memcpy(save_name + 4, item_singular.c_str(), item_singular.size());
			}

			ImGui::OpenPopup(sfmt("Save %s", item_singular).c_str());
		}


		std::string load_picked;
		std::string delete_picked;

		ImVec2 size(-FLT_MIN, height - 2.0f * ImGui::GetTextLineHeightWithSpacing());
		if (ImGui::BeginListBox("##list", size)) {
			for (size_t i = 0; i != m_preset_names.size(); i++) {
				bool is_selected = (m_preset_names[i] == m_last_preset_selected);
				if (ImGui::Selectable(m_preset_names[i].c_str(), &is_selected) && is_selected) {
					load_picked = m_preset_names[i];
				}

				if (ImGui::BeginPopupContextItem(sfmt("delete_popup_%d", i).c_str())) {
					if (ImGui::Selectable("Delete")) delete_picked = m_preset_names[i];
					ImGui::EndPopup();
				}
			}
			ImGui::EndListBox();
		}

		if (!load_picked.empty())
			loadPreset(load_picked);


		if (!delete_picked.empty())
			deletePreset(delete_picked);

		// Always center this window when appearing
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (ImGui::BeginPopupModal(sfmt("Save %s", item_singular).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			float w = ImGui::GetFontSize() * 5.0f;

			ImGui::Text("Name:");
			ImGui::PushItemWidth(w * 2.1f);
			ImGui::InputText("##filename", save_name, IM_ARRAYSIZE(save_name));
			ImGui::PopItemWidth();

			ImGui::Spacing();

			if (ImGui::Button("OK", ImVec2(w, 0))) {
				savePreset(save_name);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(w, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::EndChild();
	}


	//
	// render helpers
	//
	void Window::pKnob(std::string const& name, Parameter param, float min, float max) {
		float knob_size = 3.0f * ImGui::GetFontSize();
		float v = m_values[param] * 100.0f;
		if (ImGuiKnobs::Knob(name.c_str(), &v, min, max, 0.05f, "%.2f", ImGuiKnobVariant_Tick, knob_size)) {
			m_values[param] = v / 100.0f;
			applyInstrumentValues();
		}
	}

	bool Window::pCombo(std::string const& name, std::vector<std::string> const& values, int& index) {
		std::vector<const char*> c_values;
		for (auto& current : values)
			c_values.push_back(current.c_str());
		int c_values_size = int(c_values.size());

		if (ImGui::Combo(name.c_str(), &index, c_values.data(), c_values_size)) {
			return true;
		}

		return false;
	}

	void Window::pCombo(std::string const& name, Parameter param, std::vector<std::string> const& values) {
		int item = int(m_values[param]);
		if (pCombo(name, values, item)) {
			m_values[param] = float(item);
			applyInstrumentValues();
		}
	}

	void Window::pBool(Parameter param, std::string name) {
		bool value = int(m_values[param]) == 1;
		if (ImGui::Checkbox(name.c_str(), &value)) {
			m_values[param] = float(value);
			applyInstrumentValues();
		}
	}

	void Window::pTableCheck(Parameter param, float spacing) {
		bool value = int(m_values[param]) == 1;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spacing);
		if (ImGui::Checkbox(sfmt("##check_for_param_%d", param).c_str(), &value)) {
			m_values[param] = float(int(value));
			applyInstrumentValues();
		}
	}

	void Window::pTableDragInt(Parameter param, int min, int max) {
		int value = int(m_values[param]);
		ImGui::PushItemWidth(-FLT_MIN);
		if (ImGui::DragInt(sfmt("##dragint_for_param_%d", param).c_str(), &value, 1, min, max)) {
			m_values[param] = float(clampTo(value, min, max));
			applyInstrumentValues();
		}
		ImGui::PopItemWidth();
	}

	void Window::pHelpMarker(std::string const& name)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(name.c_str());
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}


	void Window::uiNamePicker(bool trigger,
		std::string const& title, std::string const& item, std::string const& hint,
		std::function<void(std::string)> callback) {
		static char name[128];

		if (trigger) {
			ImGui::OpenPopup(title.c_str());
			memset(name, 0, IM_ARRAYSIZE(name));
			if (!hint.empty() && (hint.size() < (IM_ARRAYSIZE(name) - 1)))
				memcpy(name, hint.c_str(), hint.size());
		}

		if (ImGui::BeginPopupModal(title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			float w = ImGui::GetFontSize() * 5.0f;

			if (trigger)
				ImGui::SetKeyboardFocusHere(0);

			ImGui::Text("%s", item.c_str());
			ImGui::PushItemWidth(w * 2.1f);
			ImGui::InputText("##name", name, IM_ARRAYSIZE(name));
			ImGui::PopItemWidth();

			ImGui::Spacing();

			if (ImGui::Button("OK", ImVec2(w, 0))) {
				callback(name);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(w, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

	}


}