
#include "package_manager.hpp"
#include "error.hpp"
#include "package.hpp"
#include "utils.hpp"
#include "rest_api.hpp"
#include "remote.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "log.hpp"
#include "http.hpp"
#include "cache.hpp"
#include "types.hpp"

#include <algorithm>
#include <curl/curl.h>
#include <curl/easy.h>

#include <filesystem>
#include <regex>
#include <fmt/printf.h>
#include <rapidjson/document.h>
#include <cxxopts.hpp>
#include "clipp.h"

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <system_error>
#include <future>


/*
 * For cURLpp examples...see the link below:
 *
 * https://github.com/jpbarrette/curlpp/tree/master/examples
 */

namespace gdpm::package_manager{
	CURL 				 		*curl;
	CURLcode 					res;
	config::context 			config;
	remote::repository_map 		remote_sources;
	action_e 					action;

	// opts_t opts;
	bool skip_prompt 	= false;
	bool clean_tmp_dir 	= false;
	int priority 		= -1;


	error initialize(int argc, char **argv){
		// curl_global_init(CURL_GLOBAL_ALL);
		curl 		= curl_easy_init();
		config 		= config::make_context();
		action 		= action_e::none;

		/* Check for config and create if not exists */
		if(!std::filesystem::exists(config.path)){
			config::save(config.path, config);
		}
		error error = config::load(config.path, config);
		if(error.has_occurred()){
			log::error(error);
			return error;
		}

		/* Create the local databases if it doesn't exist already */
		error = cache::create_package_database();
		if(error.has_occurred()){
			log::error(error);
			return error;
		}

		return error;
	}


	error finalize(){
		curl_easy_cleanup(curl);
		error error = config::save(config.path, config);
		return error;
	}


	error parse_arguments(int argc, char **argv){
		/* Replace cxxopts with clipp */
		action_e action = action_e::none;
		package::title_list package_titles;
		string_list input;
		package::params params;
		args_t args;
		var_opts opts;
		
		/* Set global options */
		auto configOpt 			= clipp::option("--config-path").set(config.path)% "set config path";
		auto fileOpt 			= clipp::option("--file", "-f").set(input)  % "read file as input";
		auto pathOpt				= clipp::option("--path").set(params.paths) % "specify a path to use with command";
		auto typeOpt 			= clipp::option("--type").set(config.info.type) % "set package type (any|addon|project)";
		auto sortOpt 			= clipp::option("--sort").set(config.api_params.sort) % "sort packages in order (rating|cost|name|updated)";
		auto supportOpt 			= clipp::option("--support").set(config.api_params.support) % "set the support level for API (all|official|community|testing)";
		auto maxResultsOpt 		= clipp::option("--max-results").set(config.api_params.max_results) % "set the request max results";
		auto godotVersionOpt 	= clipp::option("--godot-version").set(config.api_params.godot_version) % "set the request Godot version";
		auto packageDirOpt		= clipp::option("--package-dir").set(config.packages_dir) % "set the global package location";
		auto tmpDirOpt			= clipp::option("--tmp-dir").set(config.tmp_dir) % "set the temporary download location";
		auto timeoutOpt			= clipp::option("--timeout").set(config.timeout) % "set the request timeout";
		auto verboseOpt 			= clipp::option("--verbose", "-v").set(config.verbose) % "show verbose output";	
		
		/* Set the options */
		auto cleanOpt 			= clipp::option("--clean").set(config.clean_temporary) % "enable/disable cleaning temps";
		auto parallelOpt 		= clipp::option("--jobs").set(config.jobs) % "set number of parallel jobs";
		auto cacheOpt 			= clipp::option("--enable-cache").set(config.enable_cache) % "enable/disable local caching";
		auto syncOpt 			= clipp::option("--enable-sync").set(config.enable_sync) % "enable/disable remote syncing";
		auto skipOpt 			= clipp::option("--skip-prompt").set(config.skip_prompt) % "skip the y/n prompt";
		auto remoteOpt			= clipp::option("--remote").set(params.remote_source) % "set remote source to use";
		
		auto packageValues 		= clipp::values("packages", package_titles);
		auto requiredPath 		= clipp::required("--path", input);

		auto installCmd = (
			clipp::command("install")
				.set(action, action_e::install)
				.doc("Install packages from asset library"),
			clipp::values("packages", package_titles),
			clipp::option("--godot-version") & clipp::value("version", config.info.godot_version),
			cleanOpt, parallelOpt, syncOpt, skipOpt, remoteOpt

		);
		auto addCmd = (
			clipp::command("add").set(action, action_e::add)
				.doc("Add a package to a local project"),
			packageValues
		);
		auto removeCmd = (
			clipp::command("remove")
				.set(action, action_e::remove)
				.doc("Remove a package from local project"),
			packageValues
		);
		auto updateCmd = (
			clipp::command("update")
				.set(action, action_e::update)
				.doc("Update package(s)"),
			packageValues
		);
		auto searchCmd = (
			clipp::command("search")
				.set(action, action_e::search)
				.doc("Search for package(s)"),
			packageValues
		);
		auto exportCmd = (
			clipp::command("export")
				.set(action, action_e::p_export)
				.doc("Export package list"),
			clipp::values("path", input)
		);
		auto listCmd = (
			clipp::command("list")
				.set(action, action_e::list)
				.doc("Show installed packages")
		);
		auto linkCmd = (
			clipp::command("link")
				.set(action, action_e::link)
				.doc("Create symlink packages to project"),
			packageValues,
			requiredPath
		);
		auto cloneCmd = (
			clipp::command("clone")
				.set(action, action_e::clone)
				.doc("Clone packages to project"),
			packageValues,
			requiredPath
		);
		auto cleanCmd = (
			clipp::command("clean")
				.set(action, action_e::clean)
				.doc("Clean temporary files"),
			packageValues
		);
		auto configCmd = (
			clipp::command("config")
				.set(action, action_e::config)
				.doc("Set/get config properties")
		);
		auto fetchCmd = (
			clipp::command("fetch")
				.set(action, action_e::fetch)
				.doc("Fetch asset metadata from remote source")
		);
		auto remoteCmd = (
			clipp::command("remote")
				.set(action, action_e::remote)
				.doc("Manage remote sources")
				.required("subcommand")
		);
		auto uiCmd = (
			clipp::command("ui")
				.set(action, action_e::ui)
				.doc("Show the UI")
		);
		auto helpCmd = (
			clipp::command("help")
				.set(action, action_e::help)
		);

		auto cli = (
			(installCmd | addCmd | removeCmd | updateCmd | searchCmd | exportCmd |
			listCmd | linkCmd | cloneCmd | cleanCmd | configCmd | fetchCmd |
			remoteCmd | uiCmd | helpCmd)
		);

		/* Make help output */
		string map_page_format("");
		auto man_page = clipp::make_man_page(cli);
		std::for_each(man_page.begin(), man_page.end(), 
			[&map_page_format](const clipp::man_page::section& s){
				map_page_format += s.title() + "\n";
				map_page_format += s.content() + "\n";
			}
		);
		if(clipp::parse(argc, argv, cli)){
			switch(action){
				case action_e::install: 	package::install(config, package_titles, params); break;
				case action_e::add:			break;
				case action_e::remove: 		package::remove(config, package_titles, params); break;
				case action_e::update:		package::update(config, package_titles, params); break;
				case action_e::search: 		package::search(config, package_titles, params); break;
				case action_e::p_export:	package::export_to(input); break;
				case action_e::list: 		package::list(config, params); break;
											/* ...opts are the paths here */
				case action_e::link:		package::link(config, package_titles, params); break;
				case action_e::clone:		package::clone(config, package_titles, params); break;
				case action_e::clean:		package::clean_temporary(config, package_titles); break;
				case action_e::config:		config::handle_config(config, package_titles, opts); break;
				case action_e::fetch:		package::synchronize_database(config, package_titles); break;
				case action_e::sync: 		package::synchronize_database(config, package_titles); break;
				case action_e::remote: 		remote::handle_remote(config, args, opts); break;
				case action_e::ui:			log::info("ui not implemented yet"); break;
				case action_e::help: 		log::println("{}", map_page_format); break;
				case action_e::none:		/* ...here to run with no command */ break;
			}
		} else {
			log::println("{}", map_page_format);
		}
		return error();
	}

} // namespace gdpm::package_manager
