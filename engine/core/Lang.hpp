#pragma once

#include <memory>
#include <string>
#include <array>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <algorithm>

#include <cmath>
#include <cstdint>
#include <cassert>

namespace sns {
	//
	// Time 
	//
	int64_t getCurrentMilliseconds();
	int64_t getCurrentMicroseconds();

	std::string datetimeMarker();

	class ElapsedTimer 
	{
		public:
            ElapsedTimer();

            void start();

            void invalidate();
            bool isValid() const;

            int64_t elapsed() const;  // milliseconds
            bool hasExpired(int64_t timeout) const;
		private:
            int64_t m_start;
	};


	//
	// Math
	//
	constexpr float PI = float(3.14159265358979323846);
	constexpr float TWO_PI = 2.0f * PI;
	constexpr float TWO_PI_RECIPROCAL = 1.0f / TWO_PI;
	constexpr float HALF_PI = PI / 2.0f;
	constexpr float QUARTER_PI = PI / 4.0f;


	constexpr float epsilon = (float)1e-6;

	template<typename T> constexpr inline T minimum(T const left, T const right) { return ((left < right) ? left: right); }
	template<typename T> constexpr inline T maximum(T const left, T const right) { return ((left > right) ? left : right); }
	
	template<typename T> inline T absolute(T const value) { return ((value < T(0.0)) ? -value : value); }
	template<typename T> inline bool equivalent(T const left, T const right, T const epsilon = 1e-6) { T delta = right - left; return (delta < T(0.0)) ? (delta >= -epsilon) : (delta <= epsilon); }

    template<typename T> inline T clampNear(T const value, T const target, T const epsilon = 1e-6) { return (equivalent(value, target, epsilon) ? target : value); }
	template<typename T> inline T clampTo(T const value, T const minimum, T const maximum) { return ((value < minimum) ? minimum : ((value > maximum) ? maximum : value) ); }
	template<typename T> inline T clampAbove(T const value, T const minimum) { return ((value < minimum) ? minimum : value); }
	template<typename T> inline T clampBelow(T const value, T const maximum) { return ((value > maximum) ? maximum : value); }

	// random number between 0.0 and 1.0
	float uniformRandom();


	//
	// containers
	//
	template<class A, class B>
	inline void addAll(A& dst, B const& src) { for (auto const& s: src) dst.push_back(s); }

	template<class A, typename B>
	inline bool contains(A const& container, B const& value) { return std::find(container.begin(), container.end(), value) != container.end(); }

	template<class A, typename B>
	inline void remove(A& container, B const& value) { container.erase(std::remove(container.begin(), container.end(), value), container.end()); }

	template<class A, typename B>
	inline int findIndex(A const& container, B const& value, int defaultIndex = -1) { 
		for (size_t i = 0; i != container.size(); ++i)
			if (container[i] == value) return int(i);
		return defaultIndex;
	}


	//
	// File
	//
	enum class FileType
	{
		FileNotFound,
		FileRegular,
		FileDirectory
	};

	enum class  DirectorySorting
	{
		DirectorySortingNone,
		DirectorySortingAlphabetical,   // from A to Z
		DirectorySortingTime,           // from older to newer
		DirectorySortingSize,
	};

	typedef std::vector<std::string> DirectoryContents;
	DirectoryContents getDirectoryContents(std::string const& directory_name, DirectorySorting sorting = DirectorySorting::DirectorySortingNone, bool reverse = false);
	bool makeDirectory(std::string const& path);
	bool makeDirectoryForFile(std::string const& path);

	FileType fileType(std::string const& filename);
	bool fileSize(std::string const& filename);
	bool fileExists(std::string const& filename);
	bool deleteFile(std::string const& filename);
	bool copyFolder(std::string const& src, std::string const& dst);

	std::string getFirstFolder(std::string const& filename);
	std::string getFilePath(std::string const& filename);
	std::string getFileExtension(std::string const& filename);
	std::string getSimpleFileName(std::string const& fileName);
	std::string getNameLessExtension(std::string const& fileName);
	std::string getStrippedName(std::string const& fileName);
	
	std::string sanitizeName(std::string input, bool allow_spaces = false);
	std::string convertFileNameToWindowsStyle(std::string const& filename);
	std::string convertFileNameToUnixStyle(std::string const& filename);
	std::string convertFileNameToNativeStyle(std::string const& filename);
	std::string mergePaths(std::string const& a, std::string const& b);
	std::string mergePaths(std::string const& a, std::string const& b, std::string const& c);
	std::string mergePaths(std::string const& a, std::string const& b, std::string const& c, std::string const& d);

	bool readRawText(std::string const& filename, std::string& text);
	bool readRawBinary(std::string const& filename, std::vector<uint8_t>& buffer);
	bool writeRawText(std::string const& filename, std::string const& text);
	bool writeRawBinary(std::string const& filename, std::vector<uint8_t> const& buffer);

	bool saveCppBinary(uint8_t* data, size_t size, std::string filename);


	//
	// Enum
	//
	template<typename T>
	inline std::vector<std::string> enumNames() {
		std::vector<std::string> names;
		for (int i = 0; i != int(T::Count); ++i)
			names.push_back(toString(T(i)));
		return names;
	}
}