#pragma once

#include "log.hpp"
#include "types.hpp"
#include <fmt/core.h>
#include <new>
#include <string>
#include <functional>

namespace gdpm::constants::error{

	enum {
		NONE = 0,
		UNKNOWN,
		UNKNOWN_COMMAND,
		NOT_FOUND,
		NOT_DEFINED,
		NOT_IMPLEMENTED,
		NO_PACKAGE_FOUND,
		PATH_NOT_DEFINED,
		FILE_EXISTS,
		FILE_NOT_FOUND,
		DIRECTORY_EXISTS,
		DIRECTORY_NOT_FOULD,
		HOST_UNREACHABLE,
		REMOTE_NOT_FOUND,
		EMPTY_RESPONSE,
		INVALID_ARGS,
		INVALID_ARG_COUNT,
		INVALID_CONFIG,
		INVALID_KEY,
		HTTP_RESPONSE_ERROR,
		STD_ERROR
	};

	const string_list messages {
		"",
		"An unknown error has occurred.",
		"Unknown command.",
		"Resource not found.",
		"Function not defined.",
		"Function not implemented.",
		"No package found.",
		"Path is not well-defined",
		"File found.",
		"File does not exist.",
		"Directory exists.",
		"Directory not found.",
		"No response from host. Host is unreacheable",
		"Empty response from host.",
		"Invalid arguments.",
		"Invalid configuration.",
		"Invalid key.",
		"An HTTP response error has occurred.",
		"An error has occurred."
	};


	inline string get_message(int error_code) {
		string message{};
		try{ message = messages[error_code]; }
		catch(const std::bad_alloc& e){
			log::error("No default message for error code.");
		}
		return message;
	}
}; 

namespace gdpm{
	class error {
	public:
		constexpr explicit error(int code = 0, const string& message = "{code}"):
			m_code(code), m_message(message == "{code}" ? constants::error::get_message(code): message)
		{}

		void set_code(int code) { m_code = code; }
		void set_message(const string& message) { m_message = message; }

		int get_code() const { return m_code; }
		string get_message() const { return m_message; }
		bool has_occurred() const { return m_code != 0; }

		bool operator()(){ return has_occurred(); }
	
	private:
		int m_code;
		string m_message;
	};

	// Add logging function that can handle error objects
	namespace log {
		static constexpr void error(const gdpm::error& e){
#if GDPM_LOG_LEVEL > ERROR
			vlog(
				fmt::format(GDPM_COLOR_LOG_ERROR "[ERROR {}] {}\n" GDPM_COLOR_LOG_RESET, utils::timestamp(), e.get_message()),
				fmt::make_format_args("" /*e.get_message()*/)
			);
#endif
		}

		template <typename S, typename...Args>
		static constexpr void error(const S& prefix, const gdpm::error& e){
			vlog(
				fmt::format(GDPM_COLOR_LOG_ERROR + prefix + GDPM_COLOR_LOG_RESET, e.get_message())
			);
		}
	}
}
