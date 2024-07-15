#pragma once

#include "error.hpp"
#include "result.hpp"

#include <string>


namespace gdpm::version{
	struct context{
		int major = 0;
		int minor = 0;
		int patch = 0;
		string description = "";
	};

	std::string to_string(const context& context);
	result_t<context> to_version(const std::string& version);
	bool is_valid_version_string(const std::string& version);
}