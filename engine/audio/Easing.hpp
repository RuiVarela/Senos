#pragma once

#include "../core/Lang.hpp"

namespace sns {

	//
	// https://easings.net/
	//
	enum class EasingKind {
		Linear,
		Sine,
		Quad,
		Cubic,
		Quart,
		Quint,
		Expo,

		Count
	};

	enum class EasingMethod {
		Linear,
		InSine, OutSine,
		InQuad, OutQuad,
		InCubic, OutCubic,
		InQuart, OutQuart,
		InQuint, OutQuint,
		InExpo, OutExpo,

		Count
	};


	template<typename T> inline T easeInSine(T x) { return T(1.0 - cos((x * PI) / 2.0)); }
	template<typename T> inline T easeOutSine(T x) { return T(sin((x * PI) / 2.0)); }

	template<typename T> inline T easeInQuad(T x) { return x * x; }
	template<typename T> inline T easeOutQuad(T x) { return T(1.0 - (1.0 - x) * (1.0 - x)); }

	template<typename T> inline T easeInCubic(T x) { return x * x * x; }
	template<typename T> inline T easeOutCubic(T x) { return T(1.0 - pow(1.0 - x, 3.0)); }

	template<typename T> inline T easeInQuart(T x) { return x * x * x * x; }
	template<typename T> inline T easeOutQuart(T x) { return T(1.0 - pow(1.0 - x, 4.0)); }

	template<typename T> inline T easeInQuint(T x) { return x * x * x * x * x; }
	template<typename T> inline T easeOutQuint(T x) { return T(1.0 - pow(1.0 - x, 5.0)); }

	template<typename T> inline T easeInExpo(T x) { return T(x > 0.0 ? pow(2.0, 10.0 * x - 10.0) : 0.0); }
	template<typename T> inline T easeOutExpo(T x) { return T(x < 1.0 ? 1.0 - pow(2.0, -10.0 * x) : 1.0); }

	std::pair<EasingMethod, EasingMethod> easingPair(EasingKind kind);

	std::string toString(EasingKind kind);
	std::string toString(EasingMethod method);

	std::vector<std::string> easingNames();
	std::vector<std::string> easingKindNames();

	float easing(EasingMethod method, float x);

}