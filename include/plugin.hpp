

#include <string>

namespace towk::plugin{
	struct info{
		std::string name;
		std::string description;
		std::string version;
	};
	extern int init(int argc, char **argv);
	extern int set_name(const char *name);
	extern int set_description(const char *description);
	extern int set_version(const char *version);
	extern int finalize();
}