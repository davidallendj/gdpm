#pragma once
#include "types.hpp"
#include <string>
#include <filesystem>

namespace gdpm::plugin{
	struct info{
		string name;
		string description;
		string version;
	};
	extern error initialize(int argc, char **argv);
	extern error set_name(const char *name);
	extern error set_description(const char *description);
	extern error set_version(const char *version);
	extern error finalize();

	error load(std::filesystem::path path);
}