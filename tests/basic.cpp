#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "log.hpp"
#include "cache.hpp"
#include "config.hpp"

#include <doctest.h>



TEST_CASE("Confirm doctest unit testing"){
	CHECK(true);
	gdpm::log::print("Testing doctest");
}


TEST_CASE("Test cache database functions"){
	gdpm::cache::create_package_database();
}


TEST_CASE("Test configuration functions"){
	gdpm::config::context config;
	config.path = gdpm::constants::TestPath + "/";

	std::string json = gdpm::config::to_json(config);
	gdpm::error error_save = gdpm::config::save(config.path, config);
	gdpm::error error_load = gdpm::config::load(config.path, config);
}