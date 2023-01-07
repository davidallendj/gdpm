
#include <string>


namespace gdpm{
	class error{
	public:
		error(int code = 0, const std::string& message = ""):
			m_code(code), m_message(message)
		{}

		void set_code(int code) { m_code = code; }
		void set_message(const std::string& message) { m_message = message; }

		int get_code() const { return m_code; }
		std::string get_message() const { return m_message; }
	
	private:
		int m_code;
		std::string m_message;
	};
}