#include "Log.hpp"

#include <iomanip>
#include <fstream>
#include <filesystem>

namespace sns
{
	static std::string format(std::chrono::system_clock::time_point const& ts, bool date, bool time)
	{
		// get number of milliseconds for the current second, (remainder after division into seconds)
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(ts.time_since_epoch()) % 1000;

		// convert to std::time_t in order to convert to std::tm (broken time)
		auto timer = std::chrono::system_clock::to_time_t(ts);

		// convert to broken time
		std::tm bt = *std::localtime(&timer);

		std::string pattern = "";
		if (date) {
			if (!pattern.empty())
				pattern += " ";
			pattern = "%Y-%m-%d";
		}

		if (time) {
			if (!pattern.empty())
				pattern += " ";
			pattern += "%H:%M:%S";
		}

		//return sfmt("%s.%03d", std::put_time(&bt, "%Y-%m-%d %H:%M:%S"), ms.count());
		return sfmt("%s.%03d", std::put_time(&bt, pattern.c_str()), ms.count());
	}

	Log& Log::logger() {
		static Log logger;
		return logger;
	}


	Log::Log()
		:m_filename("app.log"),
		m_max_file_size(150 * 1024 * 1024),
		m_message_count(0)
	{
		setSleepMs(15);
	}

	Log::~Log() {

	}

	void Log::setWindowHandler(Handler window_handler) {
		m_window_handler = window_handler;
	}

	void Log::setFilename(std::string filename) {
		m_filename = filename;
	}

	void Log::log(LogLevel level, std::string const &tag, std::string const &message) {
		Message m;
		m.level = level;
		m.tag = tag;
		m.message = message;
		m.ts = std::chrono::system_clock::now();

		{
			std::unique_lock<std::mutex> lock(m_messages_mutex);
			if (m_message_count == 0) {
				++m_message_count;
				addFirstMessage();
			}

			++m_message_count;
			m_messages.push_back(m);
		}
	}

	void Log::flush(int flush_size) {

		if (m_messages.empty())
			return;

		std::list<Message> messages;

		{
			std::unique_lock<std::mutex> lock(m_messages_mutex);

			if (m_messages.empty())
				return;

			if (flush_size <= 0) {
				std::swap(messages, m_messages);
			}
			else {

				int count = minimum(flush_size, int(m_messages.size()));

				assert(count > 0);

				auto start = m_messages.begin();
				auto end = m_messages.begin();
				std::advance(end, count);
				messages.splice(messages.begin(), m_messages, start, end);
			}
		}

		for (auto& current : messages) 
			sendMessage(current);
	}

	void Log::addFirstMessage() {
		Message m;
		m.level = LogLevel::Info;
		m.tag = "Log";
		m.message = sfmt("--------- Log Started %s ---------", format(std::chrono::system_clock::now(), true, true));
		m.ts = std::chrono::system_clock::now();
		m_messages.push_back(m);
	}

	void Log::addLastMessage() {
		Log::i("Log", sfmt("--------- Log Stopped %d ---------", m_message_count + 1));
	}

	void Log::sendMessage(Message const& message) {
		std::string level_name;
		if (message.level == LogLevel::Debug)
			level_name = "DBG";
		else if (message.level == LogLevel::Info)
			level_name = "INF";
		else if (message.level == LogLevel::Warning)
			level_name = "WRN";
		else if (message.level == LogLevel::Error)
			level_name = "ERR";

		std::string produced = sfmt("[%s %s %s] %s", level_name, format(message.ts, false, true), message.tag, message.message);

		if (m_window_handler) {
			m_window_handler(produced);
		}

		{
			std::ofstream output(m_filename.c_str(), std::ios_base::app);
			if (output) {
				output << produced << std::endl;
				output.close();
			}
		}
	}

	void Log::workStep() {
		flush(20);
	}

	void Log::preWork() {
		std::filesystem::path path(m_filename);
		std::error_code code;
		auto size = std::filesystem::file_size(path, code);

		if (!code && (size > m_max_file_size)) {
			std::filesystem::remove(m_filename, code);
		}
	}

	void Log::postWork() {
		addLastMessage();
		flush(-1);
	}

}