#pragma once
#include "types.hpp"
#include <string>

namespace gdpm::plugin{
	struct info{
		string name;
		string description;
		string version;
	};
	extern int init(int argc, char **argv);
	extern int set_name(const char *name);
	extern int set_description(const char *description);
	extern int set_version(const char *version);
	extern int finalize();
}