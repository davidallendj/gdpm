#include "package_manager.hpp"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "log.hpp"
#include "cache.hpp"
#include "config.hpp"
#include "package.hpp"

#include <doctest.h>


TEST_SUITE("Caching functions"){

	TEST_CASE("Test cache database functions"){
		gdpm::cache::create_package_database();
	}
}


TEST_SUITE("Command functions"){
	using namespace gdpm;
	using namespace gdpm::package_manager;

	package::params params = package::params{
		.remote_source	= "test",
	};
	config::context config = config::context{
		.username 		= "",
		.password 		= "",
		.path			= "tests/gdpm/config.json",
		.packages_dir	= "tests/gdpm/packages",
		.tmp_dir		= "tests/gdpm/.tmp",
		.remote_sources	= {
			{"test", "http://godotengine.org/asset-library/api"}
		},
		.skip_prompt	= true,
		.info {
			.godot_version	= "latest",
		},
	};

	package::title_list package_titles{"ResolutionManagerPlugin","godot-hmac", "Godot"};

	/* Set the default parameters to use. */
	auto check_error = [](const error& error){
		if(error.has_occurred()){
			log::error(error);
		}

		CHECK(!error.has_occurred());
	};


	TEST_CASE("Test install packages"){
		check_error(package::install(config, package_titles, params));
	}


	TEST_CASE("Test searching packages"){
		check_error(package::search(config, package_titles, params));
	}


	TEST_CASE("Test remove packages"){
		check_error(package::remove(config, package_titles, params));
	}

	TEST_CASE("Test exporting installed package list"){
		check_error(package::export_to({"tests/gdpm/.tmp/packages.txt"}));
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