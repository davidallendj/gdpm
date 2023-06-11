
// Godot Package Manager (GPM)
#include "constants.hpp"
#include "log.hpp"
#include "config.hpp"
#include "package_manager.hpp"
#include "result.hpp"


int main(int argc, char **argv){
	using namespace gdpm;
	using namespace gdpm::package_manager;
	
	error error = initialize(argc, argv);
	parse_arguments(argc, argv);
	finalize();	
	return 0;
}