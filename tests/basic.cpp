#include "package_manager.hpp"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "log.hpp"
#include "cache.hpp"
#include "config.hpp"

#include <doctest.h>


TEST_SUITE("Test database functions"){

	TEST_CASE("Test cache database functions"){
		gdpm::cache::create_package_database();
	}
}


TEST_SUITE("Package manager function"){
	using namespace gdpm;

	std::vector<std::string> packages{"ResolutionManagerPlugin","godot-hmac", "Godot"};
	config::context config = config::make_context();

	auto check_error = [](const error& error){
		if(error.has_error()){
			log::error(error.get_message());
		}

		CHECK(!error.has_error());
	};


	TEST_CASE("Test install packages"){
		error error = package_manager::install_packages(packages, true);
		check_error(error);
	}


	TEST_CASE("Test searching packages"){
		error error = package_manager::search_for_packages(packages, true);
		check_error(error);
	}


	TEST_CASE("Test remove packages"){
		error error = package_manager::remove_packages(packages, true);
		check_error(error);
	}

}


TEST_CASE("Test configuration functions"){
	using namespace gdpm;

	config::context config = config::make_context();
	config.path = constants::TestPath + "/";

	std::string json = config::to_json(config);
	error error_save = config::save(config.path, config);
	CHECK(error_save.get_code() == 0);

	error error_load = config::load(config.path, config);
	CHECK(error_load.get_code() == 0);
}