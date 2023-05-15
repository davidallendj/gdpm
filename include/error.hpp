
#include <fmt/core.h>
#include <string>
#include "log.hpp"

namespace gdpm::error_codes{
	enum {
		NONE				= 0,
		UNKNOWN				= 1,
		NOT_FOUND			= 2,
		FILE_EXISTS 		= 3,
		HOST_UNREACHABLE 	= 4,
	};

	inline std::string to_string(int error_code) {
		return "";
	}
}; 

namespace gdpm{
	class error {
	public:
		error(int code = 0, const std::string& message = "", bool print_message = false):
			m_code(code), m_message(message)
		{
			if(print_message)
				print();
		}

		void set_code(int code) { m_code = code; }
		void set_message(const std::string& message) { m_message = message; }

		int get_code() const { return m_code; }
		std::string get_message() const { return m_message; }
		bool has_error() const { return m_code != 0; }
		void print(){ log::println(GDPM_COLOR_LOG_ERROR "ERROR: {}" GDPM_COLOR_LOG_RESET, m_message); }
	
	private:
		int m_code;
		std::string m_message;
	};

	// Add logging function that can handle error objects
	namespace log {
		template <typename S, typename...Args>
		static constexpr void error(const gdpm::error& e){
#if GDPM_LOG_LEVEL > 1
			vlog(
				fmt::format(GDPM_COLOR_LOG_ERROR "[ERROR {}" GDPM_COLOR_LOG_RESET, e.get_message()),
				fmt::make_format_args("" /*e.get_message()*/)
			);
#endif
		}
	}
}
