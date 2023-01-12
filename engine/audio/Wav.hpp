#pragma once

#include "../audio/Audio.hpp"

namespace sns {

	//
	// wav writer
	//
	class Wav {
	public:
		Wav();
		~Wav();

		bool open(std::string const& filename);
		void close();
		bool isOk();
		int write(float* data, int frames);

		static std::vector<float> load(std::string const& filename);
	private:
		struct PrivateImplementation;
		std::shared_ptr<PrivateImplementation> m;

		std::string m_filename;
		bool m_is_ok;
	};

}