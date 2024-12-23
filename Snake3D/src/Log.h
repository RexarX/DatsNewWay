#pragma once

#include <source_location>

#if defined(_WIN32)           
	#define NOGDI             // All GDI defines and routines
	#define NOUSER            // All USER defines and routines
#endif

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Snake {
	class Log {
	public:
		static void Init();

		template <typename... Args>
		static void Message(const std::shared_ptr<spdlog::logger>& logger, spdlog::level::level_enum level,
												const std::source_location& loc, spdlog::fmt_lib::format_string<Args...> fmt, Args&&... args) {
			std::string_view filename(loc.file_name());
			uint64_t lastSlash = filename.find_last_of("/\\");
			if (lastSlash != std::string_view::npos) {
				filename = filename.substr(lastSlash + 1);
			}

			logger->log(spdlog::source_loc{ filename.data(), static_cast<int>(loc.line()), nullptr },
									level, spdlog::fmt_lib::format(fmt, std::forward<Args>(args)...));
		}

		static void Message(const std::shared_ptr<spdlog::logger>& logger, spdlog::level::level_enum level,
												const std::source_location& loc, std::string_view string) {
			std::string_view filename(loc.file_name());
			uint64_t lastSlash = filename.find_last_of("/\\");
			if (lastSlash != std::string_view::npos) {
				filename = filename.substr(lastSlash + 1);
			}

			logger->log(spdlog::source_loc{ filename.data(), static_cast<int>(loc.line()), nullptr }, level, string);
		}

		static inline std::shared_ptr<spdlog::logger>& GetCoreLogger() { return m_CoreLogger; }

	private:
		static inline std::shared_ptr<spdlog::logger> m_CoreLogger = nullptr;
	};
}

#define CORE_TRACE(...) 	 Snake::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define CORE_INFO(...) 		 Snake::Log::GetCoreLogger()->info(__VA_ARGS__)
#define CORE_WARN(...) 		 Snake::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define CORE_ERROR(...) 	 Snake::Log::Message(Snake::Log::GetCoreLogger(), spdlog::level::err, std::source_location::current(), __VA_ARGS__)
#define CORE_CRITICAL(...) Snake::Log::Message(Snake::Log::GetCoreLogger(), spdlog::level::critical, std::source_location::current(), __VA_ARGS__)