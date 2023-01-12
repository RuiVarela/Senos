#include "Lang.hpp"
#include "Text.hpp"

#include <filesystem>
#include <fstream>
#include <chrono>
#include <mutex>
#include <ctime>

namespace sns {

    static std::once_flag g_initialization_flag;

    static std::chrono::high_resolution_clock::time_point g_clock_start;

    static void initialize() {
        g_clock_start = std::chrono::high_resolution_clock::now();
    }

    static void checkInitialization() {
        std::call_once(g_initialization_flag, initialize);
    }

    int64_t getCurrentMilliseconds()  {
        checkInitialization();

        return getCurrentMicroseconds() / 1000;
    }

    int64_t getCurrentMicroseconds() {
        checkInitialization();

        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - g_clock_start).count();
    }

    std::string datetimeMarker() 
    {
        time_t rawtime;
        struct tm* timeinfo;
        char buffer[80];

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer, 80, "%d-%m-%Y_%H.%M.%S", timeinfo);
        return std::string(buffer);
    }

    // random number between 0.0 and 1.0
	float uniformRandom() {
        const float max_float = static_cast<float>(RAND_MAX);
        const float rand_float = static_cast<float>(rand());
        return rand_float / max_float;
    }


    //
    // Elapsed Timer
    //
    ElapsedTimer::ElapsedTimer() {
        invalidate();
    }

    void ElapsedTimer::start() {
        m_start = getCurrentMilliseconds();
    }

    void ElapsedTimer::invalidate() {
        m_start = -1;
    }

    bool ElapsedTimer::isValid() const {
        return m_start >= 0;
    }

    int64_t ElapsedTimer::elapsed() const {
        return getCurrentMilliseconds() - m_start;
    }

    bool ElapsedTimer::hasExpired(int64_t timeout) const {
        // if timeout is -1, uint64_t(timeout) is LLINT_MAX, so this will be considered as never expired
        return uint64_t(elapsed()) > uint64_t(timeout);
    }

    //
	// File
	//
	std::string sanitizeName(std::string input, bool allow_spaces) {
        lowercase(input);
        std::string out;
        for(size_t i = 0; i < input.size(); ++i) {
            if ((allow_spaces && (input[i] == ' ')) || 
                (input[i] >= 'a' && input[i] <= 'z') || (input[i] >= '0' && input[i] <= '9')) {
                out += input[i];
            }
        }

        trim(out);
        return out;
    }

    std::string getFirstFolder(std::string const& filename) {
        std::string::size_type slash = filename.find_first_of('/');
        if (slash == std::string::npos)
            slash = filename.find_first_of('\\');

        if (slash == std::string::npos)
            return filename;

        return filename.substr(0, slash);
    }

    std::string getFilePath(std::string const& filename) {
        std::string::size_type slash1 = filename.find_last_of('/');
        std::string::size_type slash2 = filename.find_last_of('\\');

        if (slash1 == std::string::npos) {
            if (slash2 == std::string::npos)
                return std::string();
            return std::string(filename, 0, slash2);
        }

        if (slash2 == std::string::npos)
            return std::string(filename, 0, slash1);
        
        return std::string(filename, 0, slash1 > slash2 ? slash1 : slash2);
    }

    std::string getSimpleFileName(std::string const& filename) {
		std::string::size_type slash1 = filename.find_last_of('/');
		std::string::size_type slash2 = filename.find_last_of('\\');

		if (slash1 == std::string::npos){
			if (slash2 == std::string::npos)
				return filename;
			return std::string(filename.begin() + slash2 + 1, filename.end());
		}

		if (slash2 == std::string::npos)
			return std::string(filename.begin() + slash1 + 1, filename.end());

		return std::string(filename.begin() + (slash1 > slash2 ? slash1 : slash2) + 1, filename.end());
    }

    std::string getFileExtension(std::string const& filename) {
        std::string::size_type dot = filename.find_last_of('.');

        if (dot == std::string::npos)
            return std::string("");
        
        return std::string(filename.begin() + dot + 1, filename.end());
    }

    std::string getNameLessExtension(std::string const& filename) {
        std::string::size_type dot = filename.find_last_of('.');

        if (dot == std::string::npos)
            return filename;
        
        return std::string(filename.begin(), filename.begin() + dot);
    }

    std::string getStrippedName(std::string const& filename) {
        std::string simpleName = getSimpleFileName(filename);
        return getNameLessExtension(simpleName);
    }

    FileType fileType(std::string const& filename) {
        auto status = std::filesystem::status(filename);

        if (std::filesystem::is_directory(status)) {
            return FileType::FileDirectory;
        }  else if (std::filesystem::is_regular_file(status)) {
            return FileType::FileRegular;
        }
        return FileType::FileNotFound;
    }

    bool fileSize(std::string const& filename) {
        uint64_t size = 0;
        std::filesystem::path path(filename);
        size = std::filesystem::file_size(path);
        return size;
    }

    bool fileExists(std::string const& filename) {
        auto status = std::filesystem::status(filename);
        return std::filesystem::exists(status);
    }

    bool deleteFile(std::string const& filename) {
        std::error_code ec;
        return std::filesystem::remove_all(filename, ec) > 0;
        //return std::filesystem::remove(filename, ec) 
    }

	bool copyFolder(std::string const& src, std::string const& dst) {
		std::error_code ec;

		std::filesystem::path src_path(src);
		std::filesystem::path dst_path(dst);

		std::filesystem::copy(src_path, dst_path,
			std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive,
			ec);

        return ec.operator bool();
	}

    DirectoryContents getDirectoryContents(std::string const& directory_name, DirectorySorting sorting, bool reverse) {
        DirectoryContents contents;

        using SorterPair = std::pair<uint64_t, std::string>;
        std::vector<SorterPair> sorted_elements;

        if (fileType(directory_name) != FileType::FileDirectory) 
            return contents;
    
        for (const auto &entry : std::filesystem::directory_iterator(directory_name)) {
            std::string name = getSimpleFileName(entry.path().string());

             if ((sorting == DirectorySorting::DirectorySortingNone) || (sorting == DirectorySorting::DirectorySortingAlphabetical) || !entry.is_regular_file()) {

                 contents.push_back(name);

             } else {

                uint64_t sorter = 0;

                if (sorting == DirectorySorting::DirectorySortingTime) {
                    auto write_time = entry.last_write_time();
                    sorter = std::chrono::duration_cast<std::chrono::seconds>(write_time.time_since_epoch()).count();
                } else if (sorting == DirectorySorting::DirectorySortingSize) {
                    sorter = entry.file_size();
                }

                sorted_elements.push_back(std::make_pair(sorter, name));

            }
        }

        if (sorting == DirectorySorting::DirectorySortingAlphabetical) {

            std::sort(contents.begin(), contents.end());

        } else if (!sorted_elements.empty()) {

             std::sort(sorted_elements.begin(), sorted_elements.end(), [](SorterPair const& l, SorterPair const& r) {
                 return l.first < r.first;
             });

             for (auto const &[sorter, name] : sorted_elements)
                 contents.push_back(name);
        }


        if (reverse)
            std::reverse(contents.begin(), contents.end());

        return contents;
    }


    bool makeDirectory(std::string const& path)
    {
        if (path.empty())
            return false;

        if (fileType(path) == FileType::FileDirectory)
            return false;

        std::filesystem::path fs_path(path);
        return std::filesystem::create_directories(fs_path);
    }


    bool makeDirectoryForFile(std::string const& path) {
        return makeDirectory(getFilePath(path));
    }

    std::string convertFileNameToWindowsStyle(std::string const& filename) {
        std::string new_filename(filename);
        std::string::size_type slash = 0;
        while ((slash = new_filename.find_first_of('/', slash)) != std::string::npos) 
            new_filename[slash] = '\\';
       
        return new_filename;
    }

    std::string convertFileNameToUnixStyle(std::string const& filename) {
        std::string new_filename(filename);
        std::string::size_type slash = 0;

        while ((slash = new_filename.find_first_of('\\', slash)) != std::string::npos)
            new_filename[slash] = '/';

        return new_filename;
    }

    std::string convertFileNameToNativeStyle(std::string const& filename) {
#if defined(_WIN32)
        return convertFileNameToWindowsStyle(filename);
#else 
        return convertFileNameToUnixStyle(filename);
#endif
    }

    std::string mergePaths(std::string const& a, std::string const& b) {
        if (a.empty()) 
            return convertFileNameToNativeStyle(b);
        
        if (b.empty()) 
            return convertFileNameToNativeStyle(a);
        
        if (endsWith(a, "\\") || startsWith(b, "\\") || 
            endsWith(a, "/")  || startsWith(b, "/")) 
            return convertFileNameToNativeStyle(a + b);
       
        return convertFileNameToNativeStyle(a + "/" + b);
    }

    std::string mergePaths(std::string const& a, std::string const& b, std::string const& c) {
        return mergePaths(mergePaths(a, b), c);
    }

    std::string mergePaths(std::string const& a, std::string const& b, std::string const& c, std::string const& d) {
        return mergePaths(mergePaths(a, b, c), d);
    }

    bool readRawText(std::string const& filename, std::string& text) {
        bool is_good = true;

        std::ifstream in(filename.c_str(), std::ios::in | std::ios::ate | std::ios::binary);
        is_good = in.good();

        if (is_good)
        {
            auto end_position = in.tellg();
            char* script_c_str = new char[size_t(end_position) + 1];

            in.seekg(0, std::ios::beg);
            in.read(script_c_str, end_position);
            script_c_str[end_position] = '\0';
            is_good = in.good();
            in.close();

            text = script_c_str;
            delete[](script_c_str);
        }

        return is_good;
    }
    
    bool readRawBinary(std::string const& filename, std::vector<uint8_t>& buffer) {
        std::ifstream file_stream(filename, std::fstream::binary);
        if (!file_stream)
            return false;

        file_stream.seekg(0, file_stream.end);
        auto length = file_stream.tellg();
        file_stream.seekg(0, file_stream.beg);

        buffer.resize(length);
        file_stream.read((char*)buffer.data(), length);

        file_stream.close();
        return true;
    }

    bool writeRawText(std::string const& filename, std::string const& text) {
        std::ofstream out(filename.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);

        if (out.good())
            out.write(text.c_str(), text.size());

        return out.good();
    }

    bool writeRawBinary(std::string const& filename, std::vector<uint8_t> const& buffer) {
        std::ofstream out(filename.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);

        if (out.good())
            out.write((const char*)buffer.data(), buffer.size());
         
        return out.good();
    }

    bool saveCppBinary(uint8_t* data, size_t size, std::string filename) {
        std::ofstream out(filename.c_str(), std::ios::out | std::ios::trunc);

        constexpr size_t line_size = 25;

        if (out.good()) {
            out << "const uint8_t data[" << size << "] = { ";

            for (size_t i = 0; i != size; ++i) {
                if (i % line_size == 0)
                    out << std::endl << "\t";

                out << sfmt("0x%02X", data[i]);

                if (i != size - 1)
                    out << ", ";
            }

            out << std::endl << "};" << std::endl;
        }

        return out.good();
    }
}