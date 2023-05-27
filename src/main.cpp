
// Godot Package Manager (GPM)
#include "constants.hpp"
#include "log.hpp"
#include "config.hpp"
#include "package_manager.hpp"
#include "result.hpp"


int main(int argc, char **argv){
	using namespace gdpm;
	using namespace gdpm::package_manager;
	
	result_t <exec_args> r_input = initialize(argc, argv);
	exec_args input = r_input.unwrap_unsafe();
	execute(input);
	finalize();	
	return 0;
}