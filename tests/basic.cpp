#include "package_manager.hpp"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "log.hpp"
#include "cache.hpp"
#include "config.hpp"

#include <doctest.h>


TEST_SUITE("Cache functions"){

	TEST_CASE("Test cache database functions"){
		gdpm::cache::create_package_database();
	}
}


TEST_SUITE("Command functions"){
	using namespace gdpm;
	using namespace gdpm::package_manager;

	config::context config = config::make_context();
	std::vector<std::string> packages{"ResolutionManagerPlugin","godot-hmac", "Godot"};

	auto check_error = [](const error& error){
		if(error.has_error()){
			log::error(error.get_message());
		}

		CHECK(!error.has_error());
	};


	TEST_CASE("Test install packages"){
		check_error(install_packages(packages, true));
	}


	TEST_CASE("Test searching packages"){
		check_error(search_for_packages(packages, true));
	}


	TEST_CASE("Test remove packages"){
		check_error(remove_packages(packages, true));
	}

	TEST_CASE("Test exporting installed package list"){
		check_error(export_packages({"tests/gdpm/.tmp/packages.txt"}));
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