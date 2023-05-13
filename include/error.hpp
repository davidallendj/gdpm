
#include <string>
#include "log.hpp"

namespace gdpm::error_codes{
	enum {
		UNKNOWN		= 0,
		NOT_FOUND	= 1,

	};

	inline std::string to_string(int error_code) {
		return "";
	}
}; 

namespace gdpm{
	class error{
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
}
