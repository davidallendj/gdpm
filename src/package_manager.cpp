
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
	remote::repository_map remote_sources;
	CURL *curl;
	CURLcode res;
	config::context config;
	rest_api::context api_params;
	action_e action;
	string_list packages;
	// opts_t opts;
	bool skip_prompt = false;
	bool clean_tmp_dir = false;
	int priority = -1;


	result_t<exec_args> initialize(int argc, char **argv){
		// curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();
		config = config::make_context();
		api_params = rest_api::make_context();
		action = action_e::none;

		/* Check for config and create if not exists */
		if(!std::filesystem::exists(config.path)){
			config::save(config.path, config);
		}
		error error = config::load(config.path, config);
		if(error.has_occurred()){
			log::error(error);
		}

		/* Create the local databases if it doesn't exist already */
		error = cache::create_package_database();
		if(error.has_occurred()){
			log::error(error);
		}

		/* Run the rest of the program then exit */
		cxxargs args = _parse_arguments(argc, argv);
		return _handle_arguments(args);
	}


	int execute(const exec_args& in){
		run_command(action, in.args, in.opts);
		if(clean_tmp_dir)
			package::clean_temporary(config, packages);
		return 0;
	}


	void finalize(){
		curl_easy_cleanup(curl);
		error error = config::save(config.path, config);
		if(error()){

		}
	}


	cxxargs _parse_arguments(int argc, char **argv){
		/* Parse command-line arguments using cxxopts */
		cxxopts::Options options(
			argv[0], 
			"Experimental package manager made for managing assets for the Godot game engine through the command-line.\n"
		);
		options.allow_unrecognised_options();
		options.custom_help("[COMMAND] [OPTIONS...]");
		options.add_options("Command")
			("command", "Specify the input parameters", cxxopts::value<string>())
			("positional", "", cxxopts::value<string_list>())
			("install", "Install package or packages.", cxxopts::value<string_list>()->implicit_value(""), "<packages...>")
			("remove", "Remove a package or packages.", cxxopts::value<string_list>()->implicit_value(""), "<packages...>")
			("update", "Update a package or packages. This will update all packages if no argument is provided.", cxxopts::value<string_list>()->implicit_value(""), "<packages...>")
			("search", "Search for a package or packages.", cxxopts::value<string_list>(), "<packages...>")
			("export", "Export list of packages", cxxopts::value<string>()->default_value("./gdpm-packages.txt"))
			("list", "Show list of installed packages.")
			("link", "Create a symlink (or shortcut) to target directory. Must be used with the `--path` argument.", cxxopts::value<string_list>(), "<packages...>")
			("clone", "Clone packages into target directory. Must be used with the `--path` argument.", cxxopts::value<string_list>(), "<packages...>")
			("cache", "Caching operations", cxxopts::value<string_list>())
			("clean", "Clean temporary downloaded files.")
			("fetch", "Fetch asset data from remote sources.")
			("remote", "Set a source repository.", cxxopts::value<string>()->default_value(constants::AssetRepo), "<url>")
			("h,help", "Print this message and exit.")
			("version", "Show the current version and exit.")
		;
		options.parse_positional({"command", "positional"});
		options.positional_help("");
		options.add_options("Other")
			("c,config", "Set the config file path.", cxxopts::value<string>())
			("f,file", "Read file to install or remove packages.", cxxopts::value<string>(), "<path>")
			("path", "Specify a path to use with a command", cxxopts::value<string_list>())
			("type", "Set package type (any|addon|project).", cxxopts::value<string>())
			("sort", "Sort packages in order (rating|cost|name|updated).", cxxopts::value<string>())
			("support", "Set the support level for API (all|official|community|testing).")
			("max-results", "Set the max results to return from search.", cxxopts::value<int>()->default_value("500"), "<int>")
			("godot-version", "Set the Godot version to include in request.", cxxopts::value<string>())
			("package-dir", "Set the local package storage location.", cxxopts::value<string>())
			("tmp-dir", "Set the local temporary storage location.", cxxopts::value<string>())
			("timeout", "Set the amount of time to wait for a response.", cxxopts::value<size_t>())
			("no-sync", "Disable synchronizing with remote.", cxxopts::value<bool>()->implicit_value("true")->default_value("false"))
			("y,no-prompt", "Bypass yes/no prompt for installing or removing packages.")
			("v,verbose", "Show verbose output.", cxxopts::value<int>()->implicit_value("1")->default_value("0"), "0-5")
		;

		auto result = options.parse(argc, argv);
		return {result, options};
	}


	template <typename T>
	void insert(
		var_opts& opts, 
		const cxxopts::ParseResult& result, 
		const string& key
	){
		opts.insert(var_opt(key, result[key].as<T>()));
	};


	result_t<exec_args> _handle_arguments(const cxxargs& args){
		const auto& result = args.result;
		const auto& options = args.options;
		exec_args in;
		
		// auto get_opt =  [](const string& key){
		// 	result[key].as<T>();
		// }
		// auto get_opt = [&result]<typename V>(const string& key){
		// 	return opt(key, result[key].as<V>());
		// };

		/* Set option variables first to be used in functions below. */
		if(result.count("search")){
			in.args = result["search"].as<var_args>();
		}
		if(result.count("help")){
			log::println("{}", options.help());
		}
		if(result.count("config")){
			config.path = result["config"].as<string>();
			insert<string>(in.opts, result, "config");
			config::load(config.path, config);
			log::info("Config: {}", config.path);
		}
		if(result.count("remote")){
			string sub_command = result["remote"].as<string>();
			log::print("sub command: {}", sub_command);
			if(sub_command == "add"){
				string argv = result.arguments_string();
				log::println("argv: {}", argv);
				remote::add_repositories(config, {});
			}
			else if(sub_command == "remove"){
				remote::remove_respositories(config, {});
				log::println("argv: {}");
			}
		}
		if(result.count("file")){
			string path = result["file"].as<string>();
			string contents = utils::readfile(path);
			packages = utils::parse_lines(contents);
			insert<string>(in.opts, result, "file");
		}
		if(result.count("path")){
			insert<string_list>(in.opts, result, "path");
		}
		if(result.count("sort")){
			string r = result["sort"].as<string>();
			rest_api::sort_e 		sort = rest_api::sort_e::none;
			if(r == "none") 		sort = rest_api::sort_e::none;
			else if(r == "rating")	sort = rest_api::sort_e::rating;
			else if(r == "cost")	sort = rest_api::sort_e::cost;
			else if(r == "name")	sort = rest_api::sort_e::name;
			else if(r == "updated") sort = rest_api::sort_e::updated;
			api_params.sort = sort;
			in.opts.insert(var_opt("sort", r));
		}
		if(result.count("type")){
			string r = result["type"].as<string>();
			rest_api::type_e 		type = rest_api::type_e::any;
			if(r == "any")			type = rest_api::type_e::any;
			else if(r == "addon")	type = rest_api::type_e::addon;
			else if(r == "project")	type = rest_api::type_e::project;
			api_params.type = type;
			in.opts.insert(var_opt("type", r));
		}
		if(result.count("support")){
			string r = result["support"].as<string>();
			rest_api::support_e 		support = rest_api::support_e::all;
			if(r == "all")				support = rest_api::support_e::all;
			else if(r == "official") 	support = rest_api::support_e::official;
			else if(r == "community") 	support = rest_api::support_e::community;
			else if(r == "testing") 	support = rest_api::support_e::testing;
			api_params.support = support;
			in.opts.insert(var_opt("support", r));
		}
		if(result.count("max-results")){
			api_params.max_results = result["max-results"].as<int>();
			insert<int>(in.opts, result, "max-results");
		}
		if(result.count("godot-version")){
			config.godot_version = result["godot-version"].as<string>();
			insert<string>(in.opts, result, "godot-version");
		}
		if(result.count("timeout")){
			config.timeout = result["timeout"].as<size_t>();
			insert<size_t>(in.opts, result, "timeout");
		}
		if(result.count("no-sync")){
			config.enable_sync = false;
			in.opts.insert(var_opt("sync", "disabled"));
		}
		if(result.count("package-dir")){
			config.packages_dir = result["package-dir"].as<string>();
			insert<string>(in.opts, result, "package-dir");
		}
		if(result.count("tmp-dir")){
			config.tmp_dir = result["tmp-dir"].as<string>();
			insert<string>(in.opts, result, "tmp-dir");
		}
		if(result.count("yes")){
			skip_prompt = true;
			in.opts.insert(opt("skip-prompt", true));
		}
		if(result.count("link")){
			packages = result["link"].as<string_list>();
			insert<string_list>(in.opts, result, "link");
		}
		if(result.count("clone")){
			packages = result["clone"].as<string_list>();
			insert<string_list>(in.opts, result, "clone");
		}
		if(result.count("clean")){
			in.opts.insert(opt("clean", true));
			clean_tmp_dir = true;
		}
		config.verbose = 0;
		config.verbose += result["verbose"].as<int>();
		insert<int>(in.opts, result, "verbose");

		string json = to_json(config);
		if(config.verbose > 0){
			log::println("Verbose set to level {}", config.verbose);
			log::println("{}", json);
		}

		if(!result.count("command")){
			log::error("Command required. See \"help\" for more information.");
			return result_t(in, error());
		}

		string sub_command = result["command"].as<string>();
		if(result.count("positional")){
			string_list _argv = result["positional"].as<string_list>();
			args_t argv{_argv.begin(), _argv.end()};
			if(!argv.empty()){
				for(const auto& arg : argv){
					in.args.emplace_back(arg);
				}
			}
		}

		/* Catch arguments passed with dashes */
		if(sub_command == "install") 		action = action_e::install;
		else if(sub_command == "add")		action = action_e::add;
		else if(sub_command == "remove") 	action = action_e::remove;
		else if(sub_command == "update") 	action = action_e::update;
		else if(sub_command == "search") 	action = action_e::search;
		else if(sub_command == "export")	action = action_e::p_export;
		else if(sub_command == "list")		action = action_e::list;
		else if(sub_command == "link")		action = action_e::link;
		else if(sub_command == "clone")		action = action_e::clone;
		else if(sub_command == "clean")		action = action_e::clean;
		else if(sub_command == "sync")		action = action_e::sync;
		else if(sub_command == "remote") 	action = action_e::remote;
		else if(sub_command == "help"){		action = action_e::help;
			log::println("{}", options.help());
		}
		else{
			log::error("Unrecognized command. Try 'gdpm help' for more info.");
		}
		return result_t(in, error());
	}


	/* Used to run the command AFTER parsing and setting all command line args. */
	void run_command(action_e c, const var_args& args, const var_opts& opts){
		package::params params = package::make_params(args, opts);
		string_list args_list = unwrap(args);
		opts_t opts_list = unwrap(opts);
		params.skip_prompt = skip_prompt;
		switch(c){
			case action_e::install: 	package::install(config, args_list, params); break;
			case action_e::remove: 		package::remove(config, args_list, params); break;
			case action_e::update:		package::update(config, args_list, params); break;
			case action_e::search: 		package::search(config, args_list, params); break;
			case action_e::p_export:	package::export_to(args_list); break;
			case action_e::list: 		package::list(config, args_list, opts_list); break;
										/* ...opts are the paths here */
			case action_e::link:		package::link(config, args_list, opts_list); break;
			case action_e::clone:		package::clone(config, args_list, opts_list); break;
			case action_e::clean:		package::clean_temporary(config, args_list); break;
			case action_e::sync: 		package::synchronize_database(config, args_list); break;
			case action_e::remote: 		remote::_handle_remote(config, args_list, opts_list); break;
			case action_e::help: 		/* ...runs in handle_arguments() */ break;
			case action_e::none:		/* ...here to run with no command */ break;
		}
	}

} // namespace gdpm::package_manager


