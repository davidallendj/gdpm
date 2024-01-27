#include "config.hpp"
#include "error.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "error.hpp"
#include "types.hpp"

// RapidJSON
#include <any>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <tabulate/table.hpp>


// fmt
#include <fmt/format.h>
#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <ostream>
#include <fstream>
#include <ios>
#include <memory>
#include <set>
#include <unordered_map>


namespace gdpm::config{
	context config;

	string from_style(const print::style& style){
		if(style == print::style::list)			return "list";
		else if(style == print::style::table) 	return "table";
		return "list";
	}

	print::style to_style(const string& s){
		if(s == "list") 		return print::style::list;
		else if(s == "table") 	return print::style::table;
		else					return print::style::list;
	}
	
	string to_json(
		const context& config, 
		bool pretty_print
	){
		/* Build a JSON string to pass to document */
		string prefix = (pretty_print) ? "\n\t" : "";
		string spaces = (pretty_print) ? "    " : "";
		string json{
			"{" + prefix + "\"username\":" + spaces + "\"" + config.username + "\","
			+ prefix + "\"password\":" + spaces + "\"" + config.password + "\","
			+ prefix + "\"path\":"+ spaces + "\"" + config.path + "\","
			+ prefix + "\"token\":" + spaces + "\"" + config.token + "\","
			+ prefix + "\"godot_version\":" + spaces + "\"" + config.info.godot_version + "\","
			+ prefix + "\"packages_dir\":" + spaces + "\"" + config.packages_dir + "\","
			+ prefix + "\"tmp_dir\":" + spaces + "\"" + config.tmp_dir + "\","
			+ prefix + "\"remote_sources\":" + spaces + utils::json::from_object(config.remote_sources, prefix, spaces) + ","
			+ prefix + "\"threads\":" + spaces + fmt::to_string(config.jobs) + ","
			+ prefix + "\"timeout\":" + spaces + fmt::to_string(config.timeout) + ","
			+ prefix + "\"enable_sync\":" + spaces + fmt::to_string(config.enable_sync) + ","
			+ prefix + "\"enable_file_logging\":" + spaces + fmt::to_string(config.enable_file_logging)
			+ "\n}"
		};
		return json;
	}


	error load(
		std::filesystem::path path, 
		context& config
	){
		std::fstream file;
		file.open(path, std::ios::in);
		if(!file){
			if(config.verbose)
				log::warn("No configuration file found. Creating a new one.");
			config = make_context();
			save(config.path, config);
			return error();
		}
		else if(file.is_open()){
			/* 
			 * See RapidJson docs:
			 *
			 * https://rapidjson.org/md_doc_tutorial.html
			 */
			using namespace rapidjson;
			
			/* Read JSON from config, parse, and check document. Must make sure that program does not crash here and use default config instead! */
			string contents, line;
			while(std::getline(file, line))
				contents += line + "\n";

			if(config.verbose > 0)
				log::info("Loading configuration file...\n{}", contents.c_str());
			
			Document doc;
			ParseErrorCode status = doc.Parse(contents.c_str()).GetParseError();

			if(!doc.IsObject()){
				return log::error_rc(
					ec::FILE_NOT_FOUND,
					"Could not load config file."
				);
			}

			error error = validate(doc);
			if(error.has_occurred()){
				return log::error_rc(error);
			}

			/* Make sure contents were read correctly. */
			// if(!status){
			// 	log::error("config::load: Could not parse contents of file (Error: {}/{}).", GetParseError_En(status), doc.GetErrorOffset());

			// 	return context();
			// }

			/* Must check if keys exists first, then populate `_config_params`. */
			if(doc.HasMember("remote_sources")){
				if(doc["remote_sources"].IsObject()){
					const Value& srcs = doc["remote_sources"];
					for(auto& src : srcs.GetObject()){
						// config.remote_sources.push_back(src.GetString());
						config.remote_sources.insert(
							string_pair(src.name.GetString(), src.value.GetString())
						);
					}
				} else {
					return log::error_rc(
						ec::INVALID_KEY,
						"Could not read key `remote_sources`."
					);
				}
			}
			auto _get_value_string = [](Document& doc, const char *property){
				if(doc.HasMember(property))
					if(doc[property].IsString())
						return doc[property].GetString();
				return "";
			};
			auto _get_value_int = [](Document& doc, const char *property){
				if(doc.HasMember(property))
					if(doc[property].IsInt())
						return doc[property].GetInt();
				return 0;
			};

			config.username 			= _get_value_string(doc, "username");
			config.password 			= _get_value_string(doc, "password");
			config.path 				= _get_value_string(doc, "path");
			config.token 				= _get_value_string(doc, "token");
			config.info.godot_version 	= _get_value_string(doc, "godot_version");
			config.packages_dir 		= _get_value_string(doc, "packages_dir");
			config.tmp_dir 				= _get_value_string(doc, "tmp_dir");
			config.jobs 				= _get_value_int(doc, "threads");
			config.enable_sync 			= _get_value_int(doc, "enable_sync");
			config.enable_file_logging 	= _get_value_int(doc, "enable_file_logging");
		}
		return error();
	}


	error save(
		std::filesystem::path path, 
		const context& config
	){
		using namespace rapidjson;

		/* Build a JSON string to pass to document */
		string json = to_json(config);
		if(config.verbose > 0)
			log::info("Saving configuration file...\n{}", json.c_str());
		
		/* Dump JSON config to file */
		Document doc;
		doc.Parse(json.c_str());
		std::ofstream ofs(path);
		OStreamWrapper osw(ofs);

		PrettyWriter<OStreamWrapper> writer(osw);
		doc.Accept(writer);

		return gdpm::error();
	}


	error handle_config(
		config::context& config,
		const args_t& args,
		const var_opts& opts
	){
		std::for_each(opts.begin(), opts.end(), [](const var_opt& p){
			log::println("opt: {}", p.first);

			if (p.second.index() == STRING_LIST){
				string_list p_list = get<string_list>(p.second);
				std::for_each(p_list.begin(), p_list.end(), [](const string& o){
					log::println("\t{}", o);
				});
			}
		});

		log::println("opts count: {}", opts.size());
		if(opts.contains("--username")){
			string_list v = get<string_list>(opts.at("--username"));
			log::println("username: {}", v[0]);
			if(v.empty()){
				log::println("username: {}", v[0]);
			}
			else
				config.username = v[0];
		}
		if(opts.contains("godot_version")){
			string_list v = get<string_list>(opts.at("godot-version"));
			if(v.empty()){
				// print godot-version
			}
			else
				config.info.godot_version = get<string_list>(opts.at("godot-version"))[0];
		}
		if(opts.contains("threads")){
			string_list v = get<string_list>(opts.at("threads"));
			if(v.empty()){
				
			}
		}

		save(config.path, config);

		return error();
	}


	error set_property(
		config::context& config,
		const string& property,
		const string& value
	){
		if(property == "username")					config.username 		= value;
		else if(property == "password") 			config.password 		= value;
		else if(property == "path")					config.path				= value;
		else if(property == "token")				config.token			= value;
		else if(property == "packages-dir")			config.packages_dir		= value;
		else if(property == "tmp-dir")				config.tmp_dir			= value;
		else if(property == "remote-sources")		log::println("use 'gpdm remote' to manage remotes");
		else if(property == "jobs")					config.jobs				= std::stoi(value);
		else if(property == "timeout")				config.timeout			= std::stoi(value);
		else if(property == "enable-sync")			config.enable_sync		= utils::to_bool(value);
		else if(property == "enable-cache")			config.enable_cache		= utils::to_bool(value);
		else if(property == "skip-prompt")			config.skip_prompt		= utils::to_bool(value);
		else if(property == "enable-file-logging")	config.enable_file_logging	= utils::to_bool(value);
		else if(property == "clean-temporary")		config.clean_temporary	= utils::to_bool(value);
		else if(property == "verbosity")			config.verbose			= std::stoi(value);
		else if(property == "style")				config.style			= to_style(value);
		else{
			return log::error_rc(error(ec::INVALID_CONFIG,
				"Could not find property"
			));
		}
		return error();
	}

	template <typename T>
	T& get_property(
		const config::context& config,
		const string& property
	){
		log::println("config::get_property() called");
		if(property == "username")			return config.username;
		else if(property == "password")		return config.password;
		else if(property == "path")			return config.path;
		else if(property == "token")		return config.token;
		else if(property == "package-dir") 	return config.packages_dir;
		else if(property == "tmp-dir")		return config.tmp_dir;
		else if(property == "remote-sources") return utils::join(config.remote_sources);
		else if(property == "jobs")			return config.jobs;
		else if(property == "timeout")		return config.timeout;
		else if(property == "sync")			return config.enable_sync;
		else if(property == "cache")		return config.enable_cache;
		else if(property == "skip-prompt")	return config.skip_prompt;
		else if(property == "file-logging") return config.enable_file_logging;
		else if(property == "clean-temporary") return config.clean_temporary;
		else if(property == "verbosity")	return config.verbose;
		else if(property == "style")		return from_style(config.style);
	}

	context make_context(
		const string& username, 
		const string& password, 
		const string& path, 
		const string& token, 
		const string& godot_version, 
		const string& packages_dir, 
		const string& tmp_dir, 
		const string_map& remote_sources, 
		int jobs, 
		int timeout, 
		bool enable_sync, 
		bool enable_file_logging, 
		int verbose
	){
		context config {
			.username 				= username,
			.password 				= password,
			.path 					= path,
			.token 					= token,
			.packages_dir 			= (packages_dir.empty()) ? string(getenv("HOME")) + ".gdpm" : packages_dir,
			.tmp_dir 				= tmp_dir,
			.remote_sources 		= remote_sources,
			.jobs 					= jobs,
			.timeout 				= timeout,
			.enable_sync 			= enable_sync,
			.enable_file_logging 	= enable_file_logging,
			.verbose 				= verbose,
			.info 					= {
				.godot_version = godot_version,
			},
		};
		return config;
	}


	error validate(const rapidjson::Document& doc){
		error error(ec::INVALID_CONFIG, "");
		if(!doc.IsObject()){
			error.set_message("Document is not a JSON object.");
			return error;
		}
		if(!doc.HasMember("remote_sources")){
			error.set_message("Could not find `remote_sources` in config.");
			return error;
		}
		if(!doc["remote_sources"].IsObject()){
			error.set_message("Key `remote_sources` is not a JSON object.");
			return error;
		}
		error.set_code(ec::IGNORE);
		return error;
	}

	void print_json(const context& config){
		log::println("{}", to_json(config, true));
	}

	void _print_property(
		const context& config,
		const string& property
	){
		using namespace tabulate;

		if(property.empty())					return;
		else if(property == "username") 		log::println("username: {}", config.username);
		else if(property == "password") 		log::println("password: {}", config.password);
		else if(property == "path") 			log::println("path: {}", config.path);
		else if(property == "token") 			log::println("token: {}", config.token);
		else if(property == "packages-dir") 	log::println("package directory: {}", config.packages_dir);
		else if(property == "tmp-dir") 			log::println("temporary directory: {}", config.tmp_dir);
		else if(property == "remote-sources") 	log::println("remote sources: \n{}", utils::join(config.remote_sources, "\t", "\n"));
		else if(property == "jobs") 			log::println("parallel jobs: {}", config.jobs);
		else if(property == "timeout") 			log::println("timeout: {}", config.timeout);
		else if(property == "sync") 			log::println("enable sync: {}", config.enable_sync);
		else if(property == "cache") 			log::println("enable cache: {}", config.enable_cache);
		else if(property == "skip-prompt") 		log::println("skip prompt: {}", config.skip_prompt);
		else if(property == "logging") 			log::println("enable file logging: {}", config.enable_file_logging);
		else if(property == "clean") 			log::println("clean temporary files: {}", config.clean_temporary);
		else if(property == "verbose") 			log::println("verbose: {}", config.verbose);
		else if(property == "style") 			log::println("style: {}", from_style(config.style));
	}


	void add_row(
		tabulate::Table& table, 
		const context& config, 
		const string property
	){
		if(property.empty())					return;
		else if(property == "username") 		table.add_row({"Username", config.username});
		else if(property == "password") 		table.add_row({"Password", config.password});
		else if(property == "path") 			table.add_row({"Path", config.path});
		else if(property == "token") 			table.add_row({"Token", config.token});
		else if(property == "packages-dir") 	table.add_row({"Package Directory", config.packages_dir});
		else if(property == "tmp-dir") 			table.add_row({"Temp Directory", config.tmp_dir});
		else if(property == "remote-sources") 	table.add_row({"Remotes", utils::join(config.remote_sources, "\t", "\n")});
		else if(property == "jobs") 			table.add_row({"Threads", std::to_string(config.jobs)});
		else if(property == "timeout") 			table.add_row({"Timeout", std::to_string(config.timeout)});
		else if(property == "sync") 			table.add_row({"Fetch Assets", std::to_string(config.enable_sync)});
		else if(property == "cache") 			table.add_row({"Cache", std::to_string(config.enable_cache)});
		else if(property == "skip-prompt") 		table.add_row({"Skip Prompt", std::to_string(config.skip_prompt)});
		else if(property == "logging") 			table.add_row({"File Logging", std::to_string(config.enable_file_logging)});
		else if(property == "clean") 			table.add_row({"Clean Temporary", std::to_string(config.clean_temporary)});
		else if(property == "verbose") 			table.add_row({"Verbosity", std::to_string(config.verbose)});
		else if(property == "style") 			table.add_row({"Verbosity", from_style(config.style)});
	}

	void print_properties(
		const context& config,
		const string_list& properties
	){
		using namespace tabulate;

		if(config.style == print::style::list){
			if(properties.empty()){
				_print_property(config, "username");
				_print_property(config, "password");
				_print_property(config, "path");
				_print_property(config, "token");
				_print_property(config, "packages-dir");
				_print_property(config, "tmp-dir");
				_print_property(config, "remote-sources");
				_print_property(config, "jobs");
				_print_property(config, "timeout");
				_print_property(config, "sync");
				_print_property(config, "cache");
				_print_property(config, "prompt");
				_print_property(config, "logging");
				_print_property(config, "clean");
				_print_property(config, "verbose");
				_print_property(config, "style");
			}
			else {
				std::for_each(
					properties.begin(),
					properties.end(),
					[&config](const string& property){
						_print_property(config, property);
					}
				);
			}
		}
		else if(config.style == print::style::table){
			Table table;
			table.add_row({"Property", "Value"});
			if(properties.empty()){
				table.add_row({"Username", config.username});
				table.add_row({"Password", config.password});
				table.add_row({"Path", config.path});
				table.add_row({"Token", config.token});
				table.add_row({"Package Directory", config.token});
				table.add_row({"Temp Directory", config.tmp_dir});
				table.add_row({"Remotes", utils::join(config.remote_sources)});
				table.add_row({"Threads", std::to_string(config.jobs)});
				table.add_row({"Timeout", std::to_string(config.timeout)});
				table.add_row({"Fetch Data", std::to_string(config.enable_sync)});
				table.add_row({"Use Cache", std::to_string(config.enable_cache)});
				table.add_row({"Logging", std::to_string(config.enable_file_logging)});
				table.add_row({"Clean", std::to_string(config.clean_temporary)});
				table.add_row({"Verbosity", std::to_string(config.verbose)});
				table.add_row({"Style", from_style(config.style)});
			}
			else{
				std::for_each(
					properties.begin(),
					properties.end(),
					[&table, &config](const string& property){
						add_row(table, config, property);
					}
				);
			}
			table[0].format()
				.padding_top(1)
				.padding_bottom(1)
				.font_background_color(Color::red)
				.font_style({FontStyle::bold});
			table.column(1).format()
				.font_color(Color::yellow);
			table[0][1].format()
				.font_background_color(Color::blue);
			table.print(std::cout);
		}
	}

}