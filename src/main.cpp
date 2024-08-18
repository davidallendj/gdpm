
// Godot Package Manager (GPM)

#include "package_manager.hpp"
#include <cstdlib>


int main(int argc, char **argv){
	using namespace gdpm;
	using namespace gdpm::package_manager;
	
	error error = initialize(argc, argv);
	if(error.has_occurred()) {
		log::error(error);
	}
	error = parse_arguments(argc, argv);
	if(error.has_occurred()) {
		log::error(error);
	}
	finalize();
	
	return EXIT_SUCCESS;
}