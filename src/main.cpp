
// Godot Package Manager (GPM)
#include "constants.hpp"
#include "log.hpp"
#include "config.hpp"
#include "package_manager.hpp"
#include "result.hpp"


int main(int argc, char **argv){
	using namespace gdpm;
	result_t <package_manager::exec_args> r_input = package_manager::initialize(argc, argv);
	package_manager::exec_args input = r_input.unwrap_unsafe();
	package_manager::execute(input.args, input.opts);
	package_manager::finalize();	
	return 0;
}