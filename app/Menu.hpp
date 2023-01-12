#pragma once

#include "../engine/core/Lang.hpp"
#include "../engine/core/Text.hpp"

namespace sns {

	struct MenuItem {
		std::string name;
		std::string command;

		std::string shortcut;
		int shortcut_key;
		int shortcut_mod;
	};

	struct MenuColumn {
		std::string name;
		std::vector<MenuItem> items;

		inline void add(std::string const& name,
			int shortcut_key = 0, int shortcut_mod = 0) {
			items.push_back({ name, sfmt("%s|%s", this->name, name), "", shortcut_key, shortcut_mod });
		}
	};

	using Menu = std::vector<MenuColumn>;
	using MenuShortcuts = std::vector<MenuItem>;
	using MenuCallback = std::function<void(std::string const&)>;

}
