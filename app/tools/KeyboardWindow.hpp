#pragma once

#include "../Window.hpp"

namespace sns {
	class App;

	class KeyboardWindow : public Window {
	public:

		enum class KeyState {
			Off,
			OnByMouse,
			OnByKeyboard,
			OnByMidi
		};
		static std::string KeyStateName(KeyState state);

		KeyboardWindow();
		~KeyboardWindow();

		void render() override;

		bool isPressed(int key);
		KeyState state(int key);
		void changePressed(int key, KeyState state, float velocity = 0.0f);

		void updateMidiController(std::set<int> const& pressed);
	private:
		struct PrivateImplementation;
		std::shared_ptr<PrivateImplementation> m;

		void changedOctave();
		void panic();
		int m_octave;
	};
}
