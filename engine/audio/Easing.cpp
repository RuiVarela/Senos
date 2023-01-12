#include "Easing.hpp"

namespace sns {

	//
	// Ease
	//
	std::string toString(EasingKind kind) {
		switch (kind) {
		case EasingKind::Linear: return "Linear";
		case EasingKind::Sine: return "Sine";
		case EasingKind::Quad: return "Quad";
		case EasingKind::Cubic: return "Cubic";
		case EasingKind::Quart: return "Quart";
		case EasingKind::Quint: return "Quint";
		case EasingKind::Expo: return "Expo";
		default: break;
		}
		return "[EasingKind NOT_SET]";
	}

	std::string toString(EasingMethod method) {
		switch (method) {
		case EasingMethod::Linear: return "Linear";

		case EasingMethod::InSine: return "InSine";
		case EasingMethod::OutSine: return "OutSine";

		case EasingMethod::InQuad: return "InQuad";
		case EasingMethod::OutQuad: return "OutQuad";

		case EasingMethod::InCubic: return "InCubic";
		case EasingMethod::OutCubic: return "OutCubic";

		case EasingMethod::InQuart: return "InQuart";
		case EasingMethod::OutQuart: return "OutQuart";

		case EasingMethod::InQuint: return "InQuint";
		case EasingMethod::OutQuint: return "OutQuint";

		case EasingMethod::InExpo: return "InExpo";
		case EasingMethod::OutExpo: return "OutExpo";
		default: break;
		}
		return "[EasingMethod NOT_SET]";
	}

	float easing(EasingMethod method, float x) {
		switch (method) {
		case EasingMethod::InSine: return easeInSine(x);
		case EasingMethod::OutSine: return easeOutSine(x);

		case EasingMethod::InQuad: return easeInQuad(x);
		case EasingMethod::OutQuad: return easeOutQuad(x);

		case EasingMethod::InCubic: return easeInCubic(x);
		case EasingMethod::OutCubic: return easeOutCubic(x);

		case EasingMethod::InQuart: return easeInQuart(x);
		case EasingMethod::OutQuart: return easeOutQuart(x);

		case EasingMethod::InQuint: return easeInQuint(x);
		case EasingMethod::OutQuint: return easeOutQuint(x);

		case EasingMethod::InExpo: return easeInExpo(x);
		case EasingMethod::OutExpo: return easeOutExpo(x);
		default: break;
		}
		return x;
	}

	std::vector<std::string> easingNames() {
		return enumNames<EasingMethod>();
	}

	std::vector<std::string> easingKindNames() {
		return enumNames<EasingKind>();
	}

	std::pair<EasingMethod, EasingMethod> easingPair(EasingKind kind) {
		switch (kind) {
		case EasingKind::Sine: return std::make_pair<>(EasingMethod::InSine, EasingMethod::OutSine);
		case EasingKind::Quad: return std::make_pair<>(EasingMethod::InQuad, EasingMethod::OutQuad);
		case EasingKind::Cubic: return std::make_pair<>(EasingMethod::InCubic, EasingMethod::OutCubic);
		case EasingKind::Quart: return std::make_pair<>(EasingMethod::InQuart, EasingMethod::OutQuart);
		case EasingKind::Quint: return std::make_pair<>(EasingMethod::InQuint, EasingMethod::OutQuint);
		case EasingKind::Expo: return std::make_pair<>(EasingMethod::InExpo, EasingMethod::OutExpo);
		default: break;
		}
		return std::make_pair<>(EasingMethod::Linear, EasingMethod::Linear);
	}
}
