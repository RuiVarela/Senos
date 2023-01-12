#pragma once
#include "Lang.hpp"
#include "Text.hpp"
#include "Worker.hpp"

#include <chrono>
#include <functional>
#include <map>
#include <mutex>

namespace sns
{
	enum class LogLevel {
		Debug,
		Info,
		Warning,
		Error
	};


	class Log : public Worker {
	public:
		using Handler = std::function<void(std::string const&)>;

		static void d(std::string const& tag, std::string const& message) { logger().log(LogLevel::Debug, tag, message); }
		static void i(std::string const& tag, std::string const& message) { logger().log(LogLevel::Debug, tag, message); }
		static void w(std::string const& tag, std::string const& message) { logger().log(LogLevel::Debug, tag, message); }
		static void e(std::string const& tag, std::string const& message) { logger().log(LogLevel::Debug, tag, message); }

		static Log& logger();

		void log(LogLevel level, std::string const& tag, std::string const& message);

		void setWindowHandler(Handler handler);
		void setFilename(std::string filename);

	protected:
		void workStep() override;
		void preWork() override;
		void postWork() override;

	private:
		Log();
		~Log() override;

		Handler m_window_handler;
		std::string m_filename;
		uint64_t m_max_file_size;
		uint64_t m_message_count;


		struct Message {
			LogLevel level;
			std::string tag;
			std::string message;
			std::chrono::time_point<std::chrono::system_clock> ts;
		};
		std::list<Message> m_messages;
		std::mutex m_messages_mutex;

		void flush(int flush_size);
		void sendMessage(Message const& message);
		void addFirstMessage();
		void addLastMessage();
	};

}