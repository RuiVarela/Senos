#include "Configuration.hpp"

#include "../engine/core/Log.hpp"
#include "App.hpp"

#include "../vendor/miniz/miniz.hpp"

constexpr auto TAG = "Configuration";

//zip -d startup.zip "__MACOSX*"
//zip -d startup.zip "*/.DS_Store"

// std::vector<uint8_t> buffer;
// if (sns::readRawBinary("/Users/ruivarela/projects/Senos/app/assets/startup.zip", buffer)) {
// 	sns::saveCppBinary(buffer.data(), buffer.size(), "/Users/ruivarela/projects/Senos/app/assets/startup.cpp");
// }
const size_t startup_data_size = 20450;
extern unsigned char startup_data[startup_data_size];

namespace sns {


	//
	// data migrations
	//
	void unpackAssetsData(Configuration cfg) {
		std::vector<uint8_t> source(startup_data_size);
		memcpy(source.data(), startup_data, startup_data_size);

		zip_file zip(source);
		std::vector<uint8_t> data;
		for (auto &member : zip.infolist()) {
			std::string filename(member.filename);
			if (member.file_size == 0)
				continue;

			zip.read(member, data);

			std::string destination = mergePaths(rootFolder(), filename);
			makeDirectoryForFile(destination);
			writeRawBinary(destination, data);
			Log::d(TAG, sfmt("Wrote File [%s] -> [%s]", filename, destination));
		}
	}

	Configuration migrateApp(App* app, Configuration cfg, int from, int to) {
		if (app == nullptr)
			return cfg;

		if (from == 0) {
			unpackAssetsData(cfg);
			app->windowLayoutPlay();
		}
			

		return cfg;
	}
}