
// Godot Package Manager (GPM)

#include "constants.hpp"
#include "log.hpp"
#include "config.hpp"
#include "package_manager.hpp"

int main(int argc, char **argv){
	gdpm::package_manager::initialize(argc, argv);
	gdpm::package_manager::execute();
	gdpm::package_manager::finalize();	
	return 0;
}