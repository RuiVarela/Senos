#include "AboutWindow.hpp"

#include "../engine/core/Log.hpp"
#include "../vendor/imgui/imgui.h"
#include "App.hpp"

constexpr char github[] = "https://github.com/RuiVarela/Senos";

namespace sns {

	AboutWindow::AboutWindow()
	{
		TAG = "AboutWindow";
		m_window_name = "About";
	}

	AboutWindow::~AboutWindow() {
	}


	void AboutWindow::render() {
		beforeRender();

		ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 17.0f, 0.0f));
		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);

		{
			ImGui::TextDisabled("SENOS %s:", app()->versionName().c_str());
			ImGui::Indent();
			ImGui::TextWrapped("A sound exploration tool from a developer point of view.");
			ImGui::Spacing();
			if (ImGui::Button(github)) 
				platformShellOpen(github);
			
			ImGui::Unindent();
			ImGui::Spacing();

			ImGui::TextDisabled("MADE WITH:");
			ImGui::BulletText("Sokol");
			ImGui::BulletText("Dear ImGui");
			ImGui::BulletText("ImGui Knobs");
			ImGui::BulletText("tinyformat.h");
			ImGui::BulletText("nlohmann/json");
			ImGui::BulletText("libremidi");
			ImGui::BulletText("microtar");
			ImGui::BulletText("miniz-cpp");

			ImGui::BulletText("TinySoundFont");
			ImGui::BulletText("Open303");
			ImGui::BulletText("dx7 music-synthesizer-for-android");

			
			
		}

		aboutToFinishRender();
		ImGui::End();
	}
}
