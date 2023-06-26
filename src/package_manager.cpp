
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
	CURL 			*curl;
	CURLcode 		res;
	config::context config;
	action_e 		action;

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
		using namespace clipp;

		/* Replace cxxopts with clipp */
		action_e action = action_e::none;
		package::title_list package_titles;
		package::params params;

		auto doc_format = clipp::doc_formatting{}
			.first_column(7)
			.doc_column(45)
			.last_column(99);
		
		/* Set global options */
		auto debugOpt			= option("-d", "--debug").set(config.verbose, to_int(log::DEBUG)) % "show debug output";
		auto configOpt 			= option("--config-path").set(config.path) % "set config path";
		auto pathOpt				= option("--path").set(params.paths) % "specify a path to use with command";
		auto typeOpt 			= option("--type").set(config.info.type) % "set package type (any|addon|project)";
		auto sortOpt 			= option("--sort").set(config.rest_api_params.sort) % "sort packages in order (rating|cost|name|updated)";
		auto supportOpt 			= option("--support").set(config.rest_api_params.support) % "set the support level for API (all|official|community|testing)";
		auto maxResultsOpt 		= option("--max-results").set(config.rest_api_params.max_results) % "set the request max results";
		auto godotVersionOpt 	= option("--godot-version").set(config.rest_api_params.godot_version) % "set the request Godot version";
		auto packageDirOpt		= option("--package-dir").set(config.packages_dir) % "set the global package location";
		auto tmpDirOpt			= option("--tmp-dir").set(config.tmp_dir) % "set the temporary download location";
		auto timeoutOpt			= option("--timeout").set(config.timeout) % "set the request timeout";
		auto verboseOpt 				= joinable(repeatable(option("-v", "--verbose").call([]{ config.verbose += 1; }))) % "show verbose output";	

		/* Set the options */
		// auto fileOpt 			= repeatable(option("--file", "-f").set(params.input_files)  % "read file as input");
		auto fileOpt 				= repeatable(option("--file", "-f") & values("input", params.input_files)) % "read file as input";
		auto cleanOpt 			= option("--clean").set(config.clean_temporary) % "enable/disable cleaning temps";
		auto parallelOpt 		= option("--jobs").set(config.jobs) % "set number of parallel jobs";
		auto cacheOpt 			= option("--enable-cache").set(config.enable_cache) % "enable/disable local caching";
		auto syncOpt 			= option("--enable-sync").set(config.enable_sync) % "enable/disable remote syncing";
		auto skipOpt 			= option("-y", "--skip-prompt").set(config.skip_prompt, true) % "skip the y/n prompt";
		auto remoteOpt			= option("--remote").set(params.remote_source) % "set remote source to use";
		
		auto packageValues 		= values("packages", package_titles);
		auto requiredPath 		= required("--path", params.args);

		auto installCmd = "install" % (
			command("install").set(action, action_e::install),
			packageValues % "package(s) to install from asset library",
			godotVersionOpt, cleanOpt, parallelOpt, syncOpt, skipOpt, remoteOpt, fileOpt
		);
		auto addCmd = "add" % (
			command("add").set(action, action_e::add),
			packageValues % "package(s) to add to project", 
			parallelOpt, skipOpt, remoteOpt, fileOpt
		);
		auto removeCmd = "remove" % (
			command("remove").set(action, action_e::remove),
			packageValues % "packages(s) to remove from project",
			fileOpt, skipOpt, cleanOpt
		);
		auto updateCmd = "update package(s)" % (
			command("update").set(action, action_e::update),
			packageValues % ""
		);
		auto searchCmd = "search for package(s)" % (
			command("search").set(action, action_e::search),
			packageValues % "",
			godotVersionOpt, fileOpt, remoteOpt, configOpt
		);
		auto exportCmd = "export installed package list to file" % (
			command("export").set(action, action_e::p_export),
			values("paths", params.args) % ""
		);
		auto listCmd = "show installed packages" % (
			command("list").set(action, action_e::list)
		);
		auto linkCmd = "create link from package to project" % (
			command("link").set(action, action_e::link),
			value("package", package_titles) % "",
			value("path", params.args) % ""
		);
		auto cloneCmd = "clone package to project" % (
			command("clone").set(action, action_e::clone),
			value("package", package_titles) % "",
			value("path", params.args) % ""
		);
		auto cleanCmd = "clean temporary download files" % (
			command("clean").set(action, action_e::clean),
			values("packages", package_titles) % ""
		);
		auto configCmd = "manage config properties" % (
			command("config").set(action, action_e::config_get) ,
			(
				( greedy(command("get")).set(action, action_e::config_get),
				  option(repeatable(values("properties", params.args)))
				) % "get config properties"
				| 
				( command("set").set(action, action_e::config_set) , 
				  value("property", params.args[1]).call([]{}),
				  value("value", params.args[2]).call([]{})
				) % "set config properties"
			)
		);
		auto fetchCmd = "fetch asset data from remote" % (
			command("fetch").set(action, action_e::fetch),
			option(values("remote", params.args)) % ""
		);
		auto versionCmd = "show the version and exit" %(
			command("version").set(action, action_e::version)
		);
		auto add_arg = [&params](string arg) { params.args.emplace_back(arg); };
		auto remoteCmd = "manage remote sources" % (
			command("remote").set(action, action_e::remote_list).if_missing(
				[]{ remote::print_repositories(config); }
			),
			( 
				"add" % ( command("add").set(action, action_e::remote_add),
					word("name").call(add_arg) % "", 
					value("url").call(add_arg) % ""
				)
				| 
				"remove a remote source" % ( command("remove").set(action, action_e::remote_remove),
					words("names", params.args) % ""
				)
				|
				"list remote sources" % ( command("list").set(action, action_e::remote_list))
			)
		);
		auto uiCmd = "start with UI" % (
			command("ui").set(action, action_e::ui)
		);
		auto helpCmd = "show this message and exit" % (
			command("help").set(action, action_e::help)
		);

		auto cli = (
			debugOpt, verboseOpt, configOpt,
			(installCmd | addCmd | removeCmd | updateCmd | searchCmd | exportCmd |
			listCmd | linkCmd | cloneCmd | cleanCmd | configCmd | fetchCmd |
			remoteCmd | uiCmd | helpCmd | versionCmd)
		);

		/* Make help output */
		string man_page_format("");
		auto man_page = make_man_page(cli, argv[0], doc_format)
			.prepend_section("DESCRIPTION", "\tManage Godot Game Engine assets from the command-line.")
			.append_section("LICENSE", "\tSee the 'LICENSE.md' file for more details.");
		std::for_each(man_page.begin(), man_page.end(), 
			[&man_page_format](const man_page::section& s){
				man_page_format += s.title() + "\n";
				man_page_format += s.content() + "\n\n";
			}
		);

		// log::level = config.verbose;
		if(clipp::parse(argc, argv, cli)){
			log::level = config.verbose;
			switch(action){
				case action_e::install: 		package::install(config, package_titles, params); break;
				case action_e::add:				package::add(config, package_titles);
				case action_e::remove: 			package::remove(config, package_titles, params); break;
				case action_e::update:			package::update(config, package_titles, params); break;
				case action_e::search: 			package::search(config, package_titles, params); break;
				case action_e::p_export:		package::export_to(params.args); break;
				case action_e::list: 			package::list(config, params); break;
												/* ...opts are the paths here */
				case action_e::link:			package::link(config, package_titles, params); break;
				case action_e::clone:			package::clone(config, package_titles, params); break;
				case action_e::clean:			package::clean_temporary(config, package_titles); break;
				case action_e::config_get:		config::print_properties(config, params.args); break;
				case action_e::config_set:		config::handle_config(config, package_titles, params.opts); break;
				case action_e::fetch:			package::synchronize_database(config, package_titles); break;
				case action_e::sync: 			package::synchronize_database(config, package_titles); break;
				case action_e::remote_list:		remote::print_repositories(config); break;
				case action_e::remote_add: 		remote::add_repository(config, params.args); break;
				case action_e::remote_remove: 	remote::remove_respositories(config, params.args); break;
				case action_e::ui:				log::println("UI not implemented yet"); break;
				case action_e::help: 			log::println("{}", man_page_format); break;
				case action_e::version:			break;
				case action_e::none:			/* ...here to run with no command */ break;
			}
		} else {
			log::println("usage:\n{}", usage_lines(cli, argv[0]).str());
		}
		return error();
	}

} // namespace gdpm::package_manager
