#pragma once

#include "Menu.hpp"

namespace sns {
	std::string platformLocalFolder(std::string const& app);
	void platformShellOpen(std::string const& cmd);

	void platformPickLoadFile(std::string const& title, std::string const& message, std::string const& ext, std::function<void(std::string const&)> callback);
	void platformPickSaveFile(std::string const& title, std::string const& message, std::string const& filename, std::function<void(std::string const&)> callback);

	bool platformHasFileMenu();
	void platformSetupFileMenu(Menu const& menu, MenuCallback callback);

	void platformSetupWindow(int min_w, int min_h);
	void platformFullscreenChanged(bool on);


	enum class PlatformEvent {
		Sleep,
		Wakeup
	};
	using PlatformEventCallback = std::function<void(PlatformEvent)>;
	void platformRegisterCallback(PlatformEventCallback callback);
	void platformClearCallbacks();
}
