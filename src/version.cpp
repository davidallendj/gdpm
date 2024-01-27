

#include "version.hpp"

#include <regex>


namespace gdpm::version{

	void print(){

	}
	
	string to_string(const context& c){
		return utils::format("{}.{}.{}", c.major, c.minor, c.patch);
	}

	result_t<context> to_version(const string& version){
		context v;

		// check if string is valid
		if(!is_valid_version_string(version)){
			return result_t(context(), error(ec::IGNORE, "invalid version string"));
		}

		// convert string to version context


		return result_t(v, error());
	}

	bool is_valid_version_string(const string &version){
		return std::regex_match(version, std::regex("^(\\d+\\.)?(\\d+\\.)?(\\*|\\d+)$"));
	}
}