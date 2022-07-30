#pragma once

#include <string>

namespace gdpm::version{
	struct version_context{
		int major;
		int minor;
		int patch;
	};

	std::string to_string(const version_context& context);
	version_context to_version(const std::string& version);
}