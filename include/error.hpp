
#include <string>


namespace gdpm{
	class error{
	public:
		error(int code = 0, const std::string& message = ""):
			m_code(code), m_message(message)
		{}
	
	private:
		int m_code;
		std::string m_message;
	};
}