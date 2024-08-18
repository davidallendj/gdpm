
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
#include "argparse/argparse.hpp"

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <stdexcept>
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

	error initialize(int argc, char **argv){
		curl 		= curl_easy_init();
		config 		= config::make_context();
		action 		= action_e::none;

		/* Check for config and create if not exists */
		if(!std::filesystem::exists(config.path)){
			config::save(config.path, config);
		}
		error error = config::load(config.path, config);
		if(error.has_occurred()){
			return log::error_rc(error);
		}

		/* Create the local databases if it doesn't exist already */
		error = cache::create_package_database();
		if(error.has_occurred()){
			return log::error_rc(error);
		}

		return error;
	}


	error finalize(){
		curl_easy_cleanup(curl);
		error error = config::save(config.path, config);
		return error;
	}


	template <typename T, typename String = string>
	auto set_if_used(
		const argparse::ArgumentParser& cmd,
		T& value,
		const String& arg
	){
		using namespace argparse;
		if(cmd.is_used(arg)){
			value = cmd.get<T>(arg);
		}
	};

	template <typename T, typename String = string>
	auto set_if_used_limit(
		const argparse::ArgumentParser& cmd,
		T& value,
		const String& arg
	){

	}

	string_list get_values_from_parser(
		const argparse::ArgumentParser& cmd,
		const string& arg = "packages"
	){
		if(cmd.is_used(arg))
			return cmd.get<string_list>(arg);
		return string_list();
	}


	error parse_arguments(int argc, char **argv){
		using namespace argparse;

		/* Replace cxxopts with clipp */
		action_e action = action_e::none;
		package::title_list package_titles;
		package::params params;

		ArgumentParser program(argv[0], "0.0.1", argparse::default_arguments::help);
		ArgumentParser install_command("install");
		ArgumentParser get_command("get");
		ArgumentParser remove_command("remove");
		ArgumentParser update_command("update");
		ArgumentParser search_command("search");
		ArgumentParser export_command("export");
		ArgumentParser purge_command("purge");
		ArgumentParser list_command("list");
		ArgumentParser link_command("link");
		ArgumentParser clone_command("clone");
		ArgumentParser clean_command("clean");
		ArgumentParser config_command("config");
		ArgumentParser fetch_command("fetch");
		ArgumentParser version_command("version");
		ArgumentParser remote_command("remote");
		ArgumentParser ui_command("ui");
		ArgumentParser help_command("help");

		ArgumentParser config_get("get");
		ArgumentParser config_set("set");

		ArgumentParser remote_add("add");
		ArgumentParser remote_remove("remove");
		ArgumentParser remote_list("list");

		program.add_description("Manage Godot engine assets from CLI");
		program.add_argument("-v", "--verbose")
			.action([&](const auto&){ config.verbose += 1; })
			.append()
			.default_value(false)
			.implicit_value(true)
			.help("set verbosity level")		
			.nargs(nargs_pattern::optional);

		program.add_argument("--ignore-validation")
			.action([&](const auto&){ config.ignore_validation = true; })
			.default_value(false)
			.implicit_value(true)
			.help("ignore checking if current directory is a Godot project")
			.nargs(0);

		install_command.add_description("install package(s)");
		install_command.add_argument("packages")
			.required()
			.nargs(nargs_pattern::any)
			.help("packages to install");
		install_command.add_argument("--godot-version")
			.help("set Godot version for request");
		install_command.add_argument("--clean")
			.help("clean temporary files")
			.implicit_value(true)
			.default_value(false)
			.nargs(0);
		install_command.add_argument("--sync")
			.help("enable syncing with remote before installing")
			.implicit_value(true)
			.default_value(true)
			.nargs(nargs_pattern::any);
		install_command.add_argument("--cache")
			.help("disable caching asset data")
			.implicit_value(true)
			.default_value(false)
			.nargs(nargs_pattern::any);
		install_command.add_argument("--remote")
			.help("set the remote to use")
			.nargs(1);
		install_command.add_argument("-j", "--jobs")
			.help("set the number of parallel downloads")
			.default_value(1)
			.nargs(1)
			.scan<'i', int>();
		install_command.add_argument("-y", "--skip-prompt")
			.help("skip the yes/no prompt")
			.implicit_value(true)
			.default_value(false)
			.nargs(0);
		install_command.add_argument("-f", "--file")
			.help("set the file(s) to read as input")
			.append()
			.nargs(1);
		install_command.add_argument("-t", "--timeout")
			.help("set the request timeout")
			.default_value(30)
			.nargs(0);

		get_command.add_description("add package to project");
		get_command.add_argument("packages").nargs(nargs_pattern::at_least_one);
		get_command.add_argument("--remote");
		get_command.add_argument("-j", "--jobs")
			.help("")
			.nargs(1)
			.default_value(1)
			.nargs(1)
			.scan<'i', int>();
		get_command.add_argument("-y", "--skip-prompt");
		get_command.add_argument("-f", "--file")
			.help("set the file(s) to read as input")
			.append()
			.nargs(nargs_pattern::at_least_one);

		remove_command.add_description("remove package(s)");
		remove_command.add_argument("packages")
			.nargs(nargs_pattern::any);
		remove_command.add_argument("--clean")
			.help("clean temporary files")
			.implicit_value(true)
			.default_value(false)
			.nargs(0);
		remove_command.add_argument("-y", "--skip-prompt");
		remove_command.add_argument("-f", "--file")
			.help("set the file(s) to read as input")
			.append()
			.nargs(1);
		
		update_command.add_description("update package(s)");
		update_command.add_argument("packages")
			.nargs(nargs_pattern::any);
		update_command.add_argument("--clean")
			.help("clean temporary files")
			.implicit_value(true)
			.default_value(false)
			.nargs(0);
		update_command.add_argument("--remote");
		update_command.add_argument("-f", "--file")
			.help("set the file(s) to read as input")
			.append()
			.nargs(nargs_pattern::at_least_one);

		search_command.add_description("search for package(s)");
		search_command.add_argument("packages")
			.nargs(nargs_pattern::any);
		search_command.add_argument("--godot-version")
			.help("set Godot version")
			.nargs(1);
		search_command.add_argument("--remote")
			.help("set remote to use")
			.nargs(1);
		search_command.add_argument("--sort")
			.help("sort the results")
			.nargs(1);
		search_command.add_argument("--type")
			.help("set the asset type")
			.nargs(1);
		search_command.add_argument("--max-results")
			.help("limit the results")
			.nargs(1);
		search_command.add_argument("--author")
			.help("set the asset author")
			.nargs(1);
		search_command.add_argument("--support")
			.help("set the support level")
			.nargs(1);
		search_command.add_argument("-f", "--file")
			.help("set the file(s) to read as input")
			.append()
			.nargs(nargs_pattern::at_least_one);
		search_command.add_argument("--style")
			.help("set how to print output")
			.nargs(1)
			.default_value("list");
		
		ui_command.add_description("show user interface (WIP)");
		version_command.add_description("show version and exit");
		help_command.add_description("show help message and exit");
		
		export_command.add_description("export install package(s) list");
		export_command.add_argument("paths")
			.help("export list of installed packages")
			.required()
			.nargs(nargs_pattern::at_least_one);
		
		list_command.add_description("show install package(s) and remotes");
		list_command.add_argument("show")
			.help("show installed packages or remote")
			.nargs(nargs_pattern::any)
			.default_value("packages");
		list_command.add_argument("--style")
			.help("set how to print output")
			.nargs(1)
			.default_value("list");
		
		link_command.add_description("link package(s) to path");
		link_command.add_argument("packages")
			.help("package(s) to link")
			.nargs(1);
		link_command.add_argument("path")
			.help("path to link")
			.nargs(1);
		link_command.add_argument("-f", "--file")
			.help("files to link")
			.nargs(nargs_pattern::at_least_one);
		link_command.add_argument("-p", "--path")
			.help("set path to link")
			.nargs(1);
		
		clone_command.add_description("clone package(s) to path");
		clone_command.add_argument("packages")
			.help("package(s) to clone")
			.nargs(1);;
		clone_command.add_argument("path")
			.help("path to clone")
			.nargs(1);
		clone_command.add_argument("-f", "--file")
			.help("file")
			.nargs(nargs_pattern::at_least_one);
		clone_command.add_argument("-p", "--path")
			.help("set path to clone")
			.nargs(1);
		
		clean_command.add_description("clean package(s) temporary files");
		clean_command.add_argument("packages")
			.help("package(s) to clean")
			.nargs(nargs_pattern::any);
		clean_command.add_argument("-y", "--skip-prompt")
			.help("skip the yes/no prompt")
			.implicit_value(true)
			.default_value(false)
			.nargs(0);
		
		purge_command.add_description("purge cache database");
		purge_command.add_argument("-y", "--skip-prompt")
			.help("skip the yes/no prompt")
			.implicit_value(true)
			.default_value(false)
			.nargs(0);

		fetch_command.add_description("fetch and sync asset data");
		fetch_command.add_argument("remote")
			.help("remote to fetch")
			.nargs(nargs_pattern::any);

		config_get.add_description("get config properties");
		config_get.add_argument("properties")
			.help("get config properties")
			.nargs(nargs_pattern::any);
		config_get.add_argument("--style")
			.help("set how to print output")
				.nargs(1)
				.default_value("list");
		
		config_set.add_description("set config property");
		config_set.add_argument("property")
			.help("property name")
			.required()
			.nargs(1);
		config_set.add_argument("value")
			.help("property value")
			.required()
			.nargs(1);

		config_command.add_description("manage config properties");
		config_command.add_subparser(config_get);
		config_command.add_subparser(config_set);
		config_command.add_argument("--style")
			.help("set how to print output")
			.nargs(1)
			.default_value("list");

		remote_add.add_argument("name")
			.help("remote name")
			.nargs(1);
		remote_add.add_argument("url")
			.help("remote url")
			.nargs(1);
		remote_remove.add_argument("names")
			.help("remote name")
			.nargs(nargs_pattern::at_least_one);
		remote_list.add_argument("--style")
			.help("set print style")
			.nargs(1);

		remote_command.add_description("manage remote(s)");
		remote_command.add_subparser(remote_add);
		remote_command.add_subparser(remote_remove);
		remote_command.add_subparser(remote_list);

		// version_command.add_argument(Targs f_args...)

		program.add_subparser(install_command);
		program.add_subparser(get_command);
		program.add_subparser(remove_command);
		program.add_subparser(update_command);
		program.add_subparser(search_command);
		program.add_subparser(export_command);
		program.add_subparser(purge_command);
		program.add_subparser(list_command);
		program.add_subparser(link_command);
		program.add_subparser(clone_command);
		program.add_subparser(clean_command);
		program.add_subparser(config_command);
		program.add_subparser(fetch_command);
		program.add_subparser(remote_command);
		program.add_subparser(version_command);
		program.add_subparser(ui_command);
		program.add_subparser(help_command);

		try{
			program.parse_args(argc, argv);
			// program.parse_known_args(argc, argv);
		} catch(const std::runtime_error& e){
			return log::error_rc(ec::ARGPARSE_ERROR, e.what());
		}

		/* Check if we're running in a directory with 'project.godot' file */
		if (!config.ignore_validation) {
			if(!std::filesystem::exists("project.godot")){
				return error(ec::FILE_NOT_FOUND, "no 'project.godot' file found in current directory");
			}
		}

		if(program.is_subcommand_used(install_command)){
			action = action_e::install;
			// if(install_command.is_used("packages"))
			// 	package_titles = install_command.get<string_list>("packages");
			package_titles = get_values_from_parser(install_command);
			set_if_used(install_command, config.rest_api_params.godot_version, "godot-version");
			set_if_used(install_command, config.clean_temporary, "clean");
			// set_if_used(install_command, config.enable_sync, "disable-sync");
			// set_if_used(install_command, config.enable_cache, "disable-cache");
			set_if_used(install_command, params.remote_source, "remote");
			set_if_used(install_command, config.jobs, "jobs");
			if(install_command.is_used("jobs"))
				config.jobs = std::clamp(install_command.get<int>("jobs"), GDPM_MIN_JOBS, GDPM_MAX_JOBS);
			set_if_used(install_command, config.skip_prompt, "skip-prompt");
			set_if_used(install_command, params.input_files, "file");
			set_if_used(install_command, config.timeout, "timeout");
			if(install_command.is_used("sync")){
				string sync = install_command.get<string>("sync");
				if(!sync.compare("enable") || !sync.compare("true") || sync.empty()){
					config.enable_sync = true;
				}
				else if(!sync.compare("disable") || !sync.compare("false")){
					config.enable_sync = false;
				}
			}
			if(install_command.is_used("cache")){
				string cache = install_command.get<string>("sync");
				if(!cache.compare("enable") || !cache.compare("true") || cache.empty()){
					config.enable_sync = true;
				}
				else if(!cache.compare("disable") || !cache.compare("false")){
					config.enable_sync = false;
				}
			}
		}
		else if(program.is_subcommand_used(get_command)){
			action = action_e::get;
			package_titles = get_values_from_parser(get_command);
			set_if_used(get_command, params.remote_source, "remote");
			set_if_used(get_command, config.jobs, "jobs");
			set_if_used(get_command, config.skip_prompt, "skip-prompt");
			set_if_used(get_command, params.input_files, "file");
		}
		else if(program.is_subcommand_used(remove_command)){
			action = action_e::remove;
			package_titles = get_values_from_parser(remove_command);
			set_if_used(remove_command, config.clean_temporary, "clean");
			set_if_used(remove_command, config.skip_prompt, "skip-prompt");
			set_if_used(remove_command, params.input_files, "file");
		}
		else if(program.is_subcommand_used(update_command)){
			action = action_e::update;
			package_titles = get_values_from_parser(update_command);
			set_if_used(update_command, config.clean_temporary, "clean");
			set_if_used(update_command, params.remote_source, "remote");
			set_if_used(update_command, params.input_files, "file");
		}
		else if(program.is_subcommand_used(search_command)){
			action = action_e::search;
			package_titles = get_values_from_parser(search_command);
			set_if_used(search_command, config.rest_api_params.godot_version, "godot-version");
			set_if_used(search_command, params.remote_source, "remote");
			set_if_used(search_command, params.input_files, "file");
			if(search_command.is_used("style")){
				string style = search_command.get<string>("style");
				if(!style.compare("list"))
					config.style = print::style::list;
				else if(!style.compare("table"))
					config.style = print::style::table;
			}
		}
		else if(program.is_subcommand_used(export_command)){
			action = action_e::p_export;
			params.paths = export_command.get<string_list>("paths");
		}
		else if(program.is_subcommand_used(clean_command)){
			action = action_e::clean;
			package_titles = get_values_from_parser(clean_command);
			set_if_used(clean_command, config.skip_prompt, "skip-prompt");
		}
		else if(program.is_subcommand_used(purge_command)){
			action = action_e::purge;
			set_if_used(purge_command, config.skip_prompt, "skip-prompt");
		}
		else if(program.is_subcommand_used(list_command)){
			action = action_e::list;
			if(list_command.is_used("show"))
				params.args = list_command.get<string_list>("show");
			if(list_command.is_used("style")){
				string style = list_command.get<string>("style");
				if(!style.compare("list"))
					config.style = print::style::list;
				else if(!style.compare("table"))
					config.style = print::style::table;
			}
		}
		else if(program.is_subcommand_used(link_command)){
			action = action_e::link;
			package_titles = get_values_from_parser(link_command);
			set_if_used(link_command, params.paths, "path");
			if(link_command.is_used("file")){
				params.input_files = link_command.get<string_list>("file");
			}
			if(link_command.is_used("path")){
				params.paths = link_command.get<string_list>("path");
			}
		}
		else if(program.is_subcommand_used(clone_command)){
			action = action_e::clone;
			package_titles = get_values_from_parser(clone_command);
			set_if_used(clone_command, params.paths, "path");
			if(clone_command.is_used("file")){
				params.input_files = clone_command.get<string_list>("file");
			}
			if(clone_command.is_used("path")){
				params.paths = clone_command.get<string_list>("path");
			}
		}
		else if(program.is_subcommand_used(config_command)){
			if(config_command.is_used("style")){
				string style = config_command.get<string>("style");
				if(!style.compare("list"))
					config.style = print::style::list;
				else if(!style.compare("table"))
					config.style = print::style::table;
			}
			if(config_command.is_subcommand_used(config_get)){
				action = action_e::config_get;
				if(config_get.is_used("properties"))
					params.args = config_get.get<string_list>("properties");
				if(config_get.is_used("style")){
					string style = config_get.get<string>("style");
					if(!style.compare("list"))
						config.style = print::style::list;
					else if(!style.compare("table"))
						config.style = print::style::table;
				}
			}
			else if(config_command.is_subcommand_used(config_set)){
				action = action_e::config_set;
				if(config_set.is_used("property"))
					params.args.emplace_back(config_set.get<string>("property"));
				if(config_set.is_used("value"))
					params.args.emplace_back(config_set.get<string>("value"));
			}
		}
		else if(program.is_subcommand_used(fetch_command)){
			action = action_e::fetch;
			if(fetch_command.is_used("remote"))
				params.remote_source = fetch_command.get("remote");
		}
		else if(program.is_subcommand_used(version_command)){
			action = action_e::version;
		}
		else if(program.is_subcommand_used(remote_command)){
			if(remote_command.is_subcommand_used(remote_add)){
				action = action_e::remote_add;
				if(remote_add.is_used("name"))
					params.args.emplace_back(remote_add.get<string>("name"));
				if(remote_add.is_used("url"))	
					params.args.emplace_back(remote_add.get<string>("url"));
				for(const auto& arg: params.args){
					log::println("{}: {}", params.args[0], params.args[1]);
				}
			}
			if(remote_command.is_subcommand_used(remote_remove)){
				action = action_e::remote_remove;
				if(remote_remove.is_used("names"))
					params.args = remote_remove.get<string_list>("names");
			}
			if(remote_command.is_subcommand_used(remote_list)){
				action = action_e::remote_list;
				string style = remote_list.get<string>("style");
				if(!style.compare("list"))
					config.style = print::style::list;
				else if(!style.compare("table"))
					config.style = print::style::table;
			}
		}
		else if(program.is_subcommand_used("ui")){
			action = action_e::ui;
		}
		else if(program.is_subcommand_used("help")){
			action = action_e::help;
		}

		switch(action){
			case action_e::install: 		package::install(config, package_titles, params); break;
			case action_e::get:				package::get(config, package_titles, params); break;
			case action_e::remove: 			package::remove(config, package_titles, params); break;
			case action_e::update:			package::update(config, package_titles, params); break;
			case action_e::search: 			package::search(config, package_titles, params); break;
			case action_e::p_export:		package::export_to(params.paths); break;
			case action_e::purge:			package::purge(config); break;
			case action_e::list: 			package::list(config, params); break;
											/* ...opts are the paths here */
			case action_e::link:			package::link(config, package_titles, params); break;
			case action_e::clone:			package::clone(config, package_titles, params); break;
			case action_e::clean:			package::clean(config, package_titles); break;
			case action_e::config_get:		config::print_properties(config, params.args); break;
			case action_e::config_set:		config::set_property(config, params.args[0], params.args[1]); break;
			case action_e::fetch:			package::fetch(config, package_titles); break;
			case action_e::sync: 			package::fetch(config, package_titles); break;
			case action_e::remote_list:		remote::print_repositories(config); break;
			case action_e::remote_add: 		remote::add_repository(config, params.args); break;
			case action_e::remote_remove: 	remote::remove_respositories(config, params.args); break;
			case action_e::ui:				log::println("UI not implemented"); break;
			case action_e::help: 			program.print_help(); break;
			case action_e::version:			break;
			case action_e::none:			program.usage(); break;/* ...here to run with no command */ break;
		}
		return error();
	}

} // namespace gdpm::package_manager
