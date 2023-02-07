#include "Platform.hpp"
#include "../engine/core/Log.hpp"

#include <codecvt>
#include <Windows.h>
#include <Shlobj.h>

namespace sns {

	std::string platformLocalFolder(std::string const& app) {
		char path[MAX_PATH];
		SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path);
		std::string base_folder(path);
		return mergePaths(base_folder, app);
	}

	void platformShellOpen(std::string const& cmd) {
		ShellExecute(0, NULL, cmd.c_str(), NULL, NULL, SW_SHOWDEFAULT);
	}

	static std::string extensionName(std::string const& ext) {
		if (ext == "tar")
			return "Tarball (*.tar)";

		if (ext == "zip")
			return "Zip (*.zip)";

		if (ext == "wav")
			return "Wav file (*.wav)";

		return ext;
	}

	static void fileAction(bool save,
		std::string const& std_title, std::string const& std_message, std::string const& std_filename, std::string const& std_ext,
		std::function<void(std::string const&)> callback) {

		char initial_path[MAX_PATH];
		{
			SHGetFolderPath(nullptr, CSIDL_DESKTOP, nullptr, SHGFP_TYPE_CURRENT, initial_path);
		}

		char buffer[MAX_PATH];
		{
			memset(buffer, 0, sizeof(buffer));
			if (!std_filename.empty() && std_filename.size() < (MAX_PATH - 1))
				memcpy(buffer, std_filename.c_str(), std_filename.size());
		}

		char filter[MAX_PATH];
		{
			memset(filter, 0, sizeof(filter));
			std::string values = extensionName(std_ext) + "\n*." + std_ext + "\n";
			if (values.size() < MAX_PATH - 2) {
				for (int i = 0; i != values.size(); ++i) {
					if (values[i] == '\n') {
						filter[i] = 0;
					}
					else {
						filter[i] = values[i];
					}
				}
			}
		}

		OPENFILENAME ofn = { 0 };
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = GetForegroundWindow();
		ofn.hInstance = 0;

		ofn.lpstrFilter = filter;
		ofn.lpstrCustomFilter = NULL;
		ofn.nMaxCustFilter = 0;
		ofn.nFilterIndex = 1;

		ofn.lpstrFile = buffer;
		ofn.nMaxFile = MAX_PATH;

		ofn.lpstrInitialDir = initial_path;
		ofn.lpstrTitle = std_title.c_str();

		if (save) {
			ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
		}
		else {
			ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		}

		ofn.nFileOffset = 0;
		ofn.nFileExtension = 0;
		ofn.lpstrDefExt = filter;
		ofn.lCustData = 0L;
		ofn.lpfnHook = NULL;
		ofn.lpTemplateName = NULL;


		std::string path;
		bool ok = false;
		if (save) {
			ok = GetSaveFileName(&ofn);
		}
		else {
			ok = GetOpenFileName(&ofn);
		}

		if (ok) {
			buffer[MAX_PATH - 1] = 0;
			path = buffer;
		}
		callback(path);
	}

	void platformPickLoadFile(std::string const& std_title, std::string const& std_message, std::string const& std_ext, std::function<void(std::string const&)> callback) {
		fileAction(false, std_title, std_message, "", std_ext, callback);
	}

	void platformPickSaveFile(std::string const& std_title, std::string const& std_message, std::string const& std_filename, std::function<void(std::string const&)> callback) {
		std::string ext = getFileExtension(std_filename);
		fileAction(true, std_title, std_message, std_filename, ext, callback);
	}

	bool platformHasFileMenu() { return false; }
	void platformSetupFileMenu(Menu const& menu, MenuCallback callback) { }




	//
	// Dark Mode
	//
	static void enableDarkMode(HWND hWnd) {
		HMODULE dwm = LoadLibraryEx("dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (dwm) {
			//HRESULT(WINAPI * pDwmIsCompositionEnabled)(BOOL*) = (HRESULT(WINAPI*)(BOOL*))GetProcAddress(dwm, "DwmIsCompositionEnabled");
			//HRESULT(WINAPI * pDwmExtendFrameIntoClientArea)(HWND, const MARGINS*) = (HRESULT(WINAPI*)(HWND, const MARGINS*)) GetProcAddress(dwm, "DwmExtendFrameIntoClientArea");
			//HRESULT(WINAPI * pDwmEnableBlurBehindWindow)(HWND, void*) = (HRESULT(WINAPI*)(HWND, void*)) GetProcAddress(dwm, "DwmEnableBlurBehindWindow");
			HRESULT(WINAPI * pDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD) = (HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD)) GetProcAddress(dwm, "DwmSetWindowAttribute");

			// set DWMWA_USE_IMMERSIVE_DARK_MODE; needed for titlebar
			BOOL dark = 1;
			if (S_OK != pDwmSetWindowAttribute(hWnd, 20, &dark, sizeof dark)) {
				// this would be the call before Windows build 18362
				pDwmSetWindowAttribute(hWnd, 19, &dark, sizeof dark);
			}

			FreeLibrary(dwm);
		}
	}


	//https://learn.microsoft.com/en-us/windows/win32/dwm/customframe#enabling-hit-testing-for-the-custom-frame
	//https://github.com/grassator/win32-window-custom-titlebar/blob/main/main.c




	//
    // Singleton
    //
    struct PlatformSingleton {
		std::vector<PlatformEventCallback> callbacks;
        int min_w = 0;
		int min_h = 0;
		WNDPROC original_winproc = 0;
    };

    static PlatformSingleton& platform() {
        static PlatformSingleton singleton;
        return singleton;
    }

	static int win32DpiScale(int value, UINT dpi) {
		return (int)((float)value * (float)dpi / float(96));
	}

	static int getBorder(HWND hWnd) {
		return win32DpiScale(8, GetDpiForWindow(hWnd));
	}

	LRESULT CALLBACK PlatformWin_wndproc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		LRESULT result = 0;
		bool call_default = true;

		PlatformSingleton& p = platform();

		if (message == WM_GETMINMAXINFO) {
			MINMAXINFO* min_max_info = reinterpret_cast<MINMAXINFO*>(lParam);
			min_max_info->ptMinTrackSize.x = win32DpiScale(p.min_w, GetDpiForWindow(hWnd));
			min_max_info->ptMinTrackSize.y = win32DpiScale(p.min_h, GetDpiForWindow(hWnd));
		} 
		
		else if (message == WM_POWERBROADCAST) {
			int32_t event = (int32_t)wParam;
				switch (event) {
				case PBT_APMSUSPEND: {
					// Notifies applications that the computer is about to enter a suspended state.
					// This event is typically broadcast when all applications and installable drivers have returned TRUE to a previous PBT_APMQUERYSUSPEND event.
					//Log::d("Platform", "WM_POWERBROADCAST | PBT_APMSUSPEND");
					for (auto const& callback : p.callbacks)
						callback(PlatformEvent::Sleep);

					break;
				}  
				case PBT_APMRESUMESUSPEND: {
					// Notifies applications that the system has resumed operation after being suspended.
					//Log::d("Platform", "WM_POWERBROADCAST | PBT_APMRESUMESUSPEND");
					for (auto const& callback : p.callbacks)
						callback(PlatformEvent::Wakeup);
				} break;
				case PBT_APMPOWERSTATUSCHANGE: {
					// Notifies applications of a change in the power status of the computer, such as a switch from battery power to A/C. 
					// The system also broadcasts this event when remaining battery power slips below the threshold specified by the user or if the battery power changes by a specified percentage.
					//Log::d("Platform", "WM_POWERBROADCAST | PBT_APMPOWERSTATUSCHANGE");
					break;
				} 
				case PBT_APMRESUMEAUTOMATIC: {
					// Notifies applications that the system is resuming from sleep or hibernation. 
					// This event is delivered every time the system resumes and does not indicate whether a user is present. 
					//Log::d("Platform", "WM_POWERBROADCAST | PBT_APMRESUMEAUTOMATIC");
					break;
				} 
			};
		}


		// Winproc worker for the rest of the application.
		if (call_default)
			result = CallWindowProc(p.original_winproc, hWnd, message, wParam, lParam);

		return result;
	}

	void platformSetupWindow(int min_w, int min_h) {
		PlatformSingleton& p = platform();

		p.min_w = min_w;
		p.min_h = min_h;

		HWND hWnd = GetActiveWindow();
		p.original_winproc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)PlatformWin_wndproc);


		enableDarkMode(hWnd);

		/*
		HWND hWnd = GetActiveWindow();
		g_original_winproc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) PlatformWin_wndproc);

		RECT rect;
		GetWindowRect(hWnd, &rect);
		SetWindowPos(hWnd, NULL, rect.left, rect.top, rect.right - rect.left,  rect.bottom - rect.top,  SWP_FRAMECHANGED);
		*/

	}

	void platformFullscreenChanged(bool on) {

	}

	void platformRegisterCallback(PlatformEventCallback callback) {
		PlatformSingleton& p = platform();
		p.callbacks.push_back(callback);
    }

	void platformClearCallbacks() {
		PlatformSingleton& p = platform();

		p.callbacks.clear();
    }
}
