#pragma once

#include "log.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <fmt/core.h>
#include <new>
#include <string>
#include <functional>

namespace gdpm::constants::error{

	enum {
		NONE = 0,
		UNKNOWN,
		UNKNOWN_COMMAND,
		UNKNOWN_ARGUMENT,
		ARGPARSE_ERROR,
		ASSERTION_FAILED,
		PRECONDITION_FAILED,
		POSTCONDITION_FAILED,
		NOT_FOUND,
		NOT_DEFINED,
		NOT_IMPLEMENTED,
		NO_PACKAGE_FOUND,
		MALFORMED_PATH,
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
		HTTP_RESPONSE_ERR,
		SQLITE_ERR,
		LIBZIP_ERR,
		LIBCURL_ERR,
		JSON_ERR,
		STD_ERR
	};

	const string_list messages {
		"",
		"An unknown error has occurred.",
		"Unknown command.",
		"Unknown argument.",
		"Could not parse argument.",
		"Assertion condition failed.",
		"Pre-condition failed.",
		"Post-condition failed.",
		"Resource not found.",
		"Resource not defined.",
		"Resource not implemented.",
		"No package found.",
		"Path is malformed.",
		"File already exists",
		"File does not exist.",
		"Directory exists.",
		"Directory not found.",
		"No response from host. Host is unreacheable",
		"Empty response from host.",
		"Invalid arguments.",
		"Invalid configuration.",
		"Invalid key.",
		"An HTTP response error has occurred.",
		"A SQLite error has occurred.",
		"A libzip error has occurred.",
		"A libcurl error has occurred.",
		"A JSON error has occurred.",
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
	namespace ec = constants::error;
	class error {
	public:
		constexpr explicit error(int code = 0, const string& message = "{default}"):
			m_code(code), 
			m_message(utils::replace_all(message, "{default}", ec::get_message(code)))
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
			set_prefix_if(std::format("[ERROR {}] ", utils::timestamp()), true);
			set_suffix_if("\n");
			vlog(
				fmt::format(GDPM_COLOR_LOG_ERROR "{}{}\n" GDPM_COLOR_LOG_RESET, prefix.contents, e.get_message()),
				fmt::make_format_args(prefix.contents, e.get_message())
			);
#endif
		}

		// static constexpr void error(int code, const string& message = "{default}"){
		// 	log::error(gdpm::error(code, message));
		// }

		static constexpr gdpm::error error_rc(const gdpm::error& e){
			log::error(e);
			return e;
		}

		static constexpr gdpm::error error_rc(int code, const string& message = "{default}"){
			return error_rc(gdpm::error(code, message));
		}
	}

	namespace concepts{
		template <typename T>concept error_t = requires{ std::is_same<error, T>::value; };
	}
}
