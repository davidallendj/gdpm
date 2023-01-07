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
	std::vector<std::string> packages{"ResolutionManagerPlugin","godot-hmac", "Godot"};
	gdpm::config::context config = gdpm::config::make_context();

	TEST_CASE("Test install packages"){
		gdpm::package_manager::install_packages(packages, true);
	}


	TEST_CASE("Test searching packages"){
		gdpm::package_manager::search_for_packages(packages, true);
	}


	TEST_CASE("Test remove packages"){
		gdpm::package_manager::remove_packages(packages, true);
	}

}


TEST_CASE("Test configuration functions"){
	gdpm::config::context config = gdpm::config::make_context();
	config.path = gdpm::constants::TestPath + "/";

	std::string json = gdpm::config::to_json(config);
	gdpm::error error_save = gdpm::config::save(config.path, config);
	gdpm::error error_load = gdpm::config::load(config.path, config);
}